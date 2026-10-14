"""Bundled, local-only face tracker for PNGTuber Max. No network access."""
import argparse, ctypes, math, mmap, struct, time
import cv2
import mediapipe as mp
import numpy as np
SIZE = 64 + 3 * 256 * 192 * 5
# Ordered outer contours from MediaPipe FaceMesh connections (v0.10.21).
CONTOURS = [[33,7,163,144,145,153,154,155,133,173,157,158,159,160,161,246],
            [263,249,390,373,374,380,381,382,362,398,384,385,386,387,388,466],
            [61,146,91,181,84,17,314,405,321,375,291,409,270,269,267,0,37,39,40,185]]

def distance_field(polygon):
    mask=np.zeros((192,256),np.uint8)
    # Subpixel polygon rasterisation avoids jumps at pixel boundaries.
    cv2.fillPoly(mask,[np.rint(np.asarray(polygon)*16).astype(np.int32)],255,shift=4)
    inside=cv2.distanceTransform(mask,cv2.DIST_L2,cv2.DIST_MASK_PRECISE)
    outside=cv2.distanceTransform(255-mask,cv2.DIST_L2,cv2.DIST_MASK_PRECISE)
    return np.rint((np.clip(inside-outside,-32,32)+32)*255/64).astype(np.uint8).tobytes()

def face_roll(points):
    # Eye centres give one shared roll for all features. Mouth expression must
    # never independently rotate the mouth crop.
    left=(np.asarray(points[33])+np.asarray(points[133]))*.5
    right=(np.asarray(points[263])+np.asarray(points[362]))*.5
    direction=right-left
    return math.atan2(float(direction[1]),float(direction[0])) if np.linalg.norm(direction)>1 else 0.

def mouth_measurements(landmarks, frame_width, frame_height, pose=(0,0,0)):
    # Using 3D distances instead of screen rectangles avoids opening/width
    # changes when the head rolls or turns. Z shares X's image scale.
    points=np.asarray([(p.x*frame_width,p.y*frame_height,p.z*frame_width) for p in landmarks],np.float64)
    if len(points)<292 or not np.isfinite(points).all() or abs(pose[2])>50 or abs(pose[1])>40:return (0.,0.,0.)
    eye_span=np.linalg.norm(points[33]-points[263])
    if eye_span<15:return (0.,0.,0.)
    opening=float(np.linalg.norm(points[13]-points[14])/eye_span)
    width=float(np.linalg.norm(points[61]-points[291])/eye_span)
    good=0<=opening<=1.5 and .1<=width<=2
    return (opening,width,1. if good else 0.)

INNER_LIPS=[78,95,88,178,87,14,317,402,318,324,308,415,310,311,312,13,82,81,80,191]

def mouth_visual_cues(frame, points, opening):
    # Appearance cues, not speech recognition. Limit sampling to the inner lip
    # polygon so surrounding skin and the outer red lip do not dominate.
    if opening<.008:return (0.,0.,0.)
    polygon=np.asarray([points[i] for i in INNER_LIPS],np.float32)
    if not np.isfinite(polygon).all():return (0.,0.,0.)
    x,y,w,h=cv2.boundingRect(polygon);x0=max(0,x);y0=max(0,y);x1=min(frame.shape[1],x+w);y1=min(frame.shape[0],y+h)
    if x1<=x0 or y1<=y0:return (0.,0.,0.)
    mask=np.zeros((y1-y0,x1-x0),np.uint8)
    cv2.fillPoly(mask,[np.rint(polygon-[x0,y0]).astype(np.int32)],255)
    pixels=frame[y0:y1,x0:x1][mask>0].astype(np.float32)
    if len(pixels)<12:return (0.,0.,0.)
    blue,green,red=pixels.T
    light=(pixels.min(axis=1)>115)&(pixels.max(axis=1)-pixels.min(axis=1)<55)
    reddish=(red>70)&(red>green*1.25)&(red>blue*1.2)
    return (float(light.mean()),float(reddish.mean()),1.)

def cutout(frame, points, indices, contour=None, previous=None, roll=0.):
    pts = np.array([points[i] for i in indices], dtype=np.float32)
    # Measure the crop in a level face basis, then map its corners back to the
    # camera. This cancels roll without shrinking width as the head tilts.
    c,s=math.cos(roll),math.sin(roll)
    basis=np.array([[c,-s],[s,c]],np.float32)
    origin=pts.mean(axis=0)
    local=(pts-origin)@basis
    lo,hi=local.min(axis=0),local.max(axis=0)
    center=origin+basis@((lo+hi)*.5)
    w=max(12.,float(hi[0]-lo[0])*3.4)
    # Keep the crop aspect stable so a blink or opening mouth is not resized away.
    h=w*.75
    corners=np.array([[-w/2,-h/2],[w/2,-h/2],[-w/2,h/2]],np.float32)
    src=(corners@basis.T+center).astype(np.float32)
    dst = np.array([[0,0],[256,0],[0,192]], np.float32)
    transform = cv2.getAffineTransform(src,dst)
    rgb = cv2.cvtColor(cv2.warpAffine(frame,transform,(256,192)),cv2.COLOR_BGR2RGB)
    rgba = np.dstack((rgb,np.full((192,256),255,np.uint8)))
    if contour is None:return rgba.tobytes()
    polygon=cv2.transform(np.asarray([points[i] for i in contour],np.float32)[None,:,:],transform)[0]
    if previous is not None:
        old_height=np.ptp(previous[:,1]);new_height=np.ptp(polygon[:,1])
        # Respond immediately to blinks/open mouths; lightly smooth small boundary jitter.
        if .65*old_height<new_height<1.5*old_height:polygon=.85*polygon+.15*previous
    return rgba.tobytes(),distance_field(polygon),polygon

def gyro_test():
    # Rotate a whole synthetic face and its pixels; compare every stabilized
    # feature with the neutral crop, including its tracked boundary.
    frame=np.full((400,400,3),40,np.uint8)
    points=np.full((468,2),200.,np.float32)
    regions=[[33,133,159,145],[263,362,386,374],[61,291,0,17]]
    for i,(cx,cy) in enumerate([(125,160),(275,160),(200,250)]):
        count=len(CONTOURS[i])
        theta=np.linspace(0,2*np.pi,count,endpoint=False)
        polygon=np.column_stack((cx+20*np.cos(theta),cy+(6 if i<2 else 12)*np.sin(theta)))
        points[CONTOURS[i]]=polygon
        for index,offset in zip(regions[i],[(-20,0),(20,0),(0,-6 if i<2 else -12),(0,6 if i<2 else 12)]):
            points[index]=np.array([cx,cy])+offset
        cv2.fillPoly(frame,[np.rint(polygon).astype(np.int32)],(180,210,240))
        cv2.line(frame,(cx-15,cy),(cx+15,cy),(40,80,120),2)
    neutral=[cutout(frame,points,r,CONTOURS[i],roll=face_roll(points)) for i,r in enumerate(regions)]
    for angle in (-45,-30,-15,-5,5,15,30,45):
        rotation=cv2.getRotationMatrix2D((200,200),angle,1.)
        tilted=cv2.warpAffine(frame,rotation,(400,400),borderValue=(40,40,40))
        moved=cv2.transform(points[None,:,:],rotation)[0]
        roll=face_roll(moved)
        for i,r in enumerate(regions):
            image,field,polygon=cutout(tilted,moved,r,CONTOURS[i],roll=roll)
            ref_image,ref_field,ref_polygon=neutral[i]
            assert np.max(np.abs(polygon-ref_polygon))<.001, ('gyro contour',angle,i)
            actual=np.frombuffer(field,np.uint8);expected=np.frombuffer(ref_field,np.uint8)
            assert np.abs(actual.astype(float)-expected.astype(float)).mean()<.1, ('gyro field',angle,i)
            union=np.count_nonzero((actual>128)|(expected>128))
            assert np.count_nonzero((actual>128)&(expected>128))/max(1,union)>.995, ('gyro mask',angle,i)
            error=np.abs(np.frombuffer(image,np.uint8).astype(float)-np.frombuffer(ref_image,np.uint8).astype(float)).mean()
            assert error<3., ('gyro pixels',angle,i,error)
    # Derolling does not suppress blinks or an opening mouth.
    assert np.ptp(neutral[2][2][:,1])>np.ptp(neutral[0][2][:,1])*1.9
    print('PASS: shared gyro stabilization for both eyes and mouth at +/-5,15,30,45 degrees; source pixels and contours agree')

def mouth_test():
    from types import SimpleNamespace
    points=np.zeros((478,3),np.float64)
    points[33]=(-100,0,0);points[263]=(100,0,0)
    points[13]=(0,100,0);points[14]=(0,160,0)
    points[61]=(-70,120,0);points[291]=(70,120,0)
    def measure(coords,pose=(0,0,0)):
        lm=[SimpleNamespace(x=(p[0]+320)/640,y=(p[1]+200)/480,z=p[2]/640) for p in coords]
        return mouth_measurements(lm,640,480,pose)
    baseline=measure(points);assert np.allclose(baseline,(.3,.7,1.))
    for degrees in (-40,-15,15,40):
        t=math.radians(degrees);c,s=math.cos(t),math.sin(t)
        for rot in ([[c,-s,0],[s,c,0],[0,0,1]],[[c,0,s],[0,1,0],[-s,0,c]],[[1,0,0],[0,c,-s],[0,s,c]]):
            assert np.allclose(measure(points@np.asarray(rot).T),baseline)
    assert measure(points,(0,0,55))[2]==0
    assert measure(points*.01)[2]==0
    bad=points.copy();bad[13,0]=float('nan');assert measure(bad)[2]==0
    print('PASS: camera mouth opening/width invariant to synthetic roll, yaw and pitch; occluded/small/invalid face rejection')

def cue_test():
    frame=np.full((160,160,3),(30,30,220),np.uint8);points=np.full((478,2),80.,np.float32)
    theta=np.linspace(0,2*np.pi,len(INNER_LIPS),endpoint=False)
    points[INNER_LIPS]=np.column_stack((80+30*np.cos(theta),80+15*np.sin(theta)))
    frame[60:100,45:80]=(210,210,210)
    teeth,tongue,valid=mouth_visual_cues(frame,points,.15)
    assert valid and teeth>.3 and tongue>.3,(teeth,tongue)
    changed=frame.copy();changed[:50]=(250,250,250);changed[110:]=(250,250,250)
    assert np.allclose(mouth_visual_cues(changed,points,.15),(teeth,tongue,valid))
    assert mouth_visual_cues(frame,points,0)==(0.,0.,0.)
    print('PASS: inner-lip teeth/red appearance cues exclude surrounding skin and closed mouths')

def self_test():
    with mp.solutions.face_mesh.FaceMesh(static_image_mode=True,refine_landmarks=True,max_num_faces=1) as detector:
        assert not detector.process(np.zeros((128,128,3),np.uint8)).multi_face_landmarks
    image=np.full((128,128,3),200,np.uint8); points=[(40,50),(80,70)]
    crop=np.frombuffer(cutout(image,points,[0,1]),np.uint8).reshape(192,256,4)
    assert crop[96,128,3]==255 and crop[0,0,3]==255
    assert np.all(crop[:,:,:3]<=crop[:,:,3,None])
    # A narrow eyelid band and an open mouth interior must remain distinct.
    opened=np.frombuffer(distance_field([(60,96),(90,72),(166,72),(196,96),(166,120),(90,120)]),np.uint8).reshape(192,256)
    closed=np.frombuffer(distance_field([(60,96),(90,94),(166,94),(196,96),(166,98),(90,98)]),np.uint8).reshape(192,256)
    assert opened[96,128]>128 and opened[70,128]<128
    assert closed[96,128]>128 and closed[80,128]<128
    assert np.count_nonzero(closed>128)<np.count_nonzero(opened>128)/4
    gyro_test()
    mouth_test()
    cue_test()
    print('PASS: eyelid/open-mouth contours preserve expression and exclude surrounding skin')
    print('PASS: bundled face model loads; full-resolution source crops for the in-app mask editor')

def run(args):
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.OpenMutexW.argtypes=[ctypes.c_uint32,ctypes.c_int,ctypes.c_wchar_p];kernel.OpenMutexW.restype=ctypes.c_void_p
    kernel.WaitForSingleObject.argtypes=[ctypes.c_void_p,ctypes.c_uint32]
    kernel.ReleaseMutex.argtypes=[ctypes.c_void_p]
    mutex=kernel.OpenMutexW(0x100001,False,args.name+'Lock')
    if not mutex: raise OSError(ctypes.get_last_error(),'Tracker mutex unavailable')
    memory=mmap.mmap(-1,SIZE,tagname=args.name)
    camera=None if args.test_stream else cv2.VideoCapture(args.camera,cv2.CAP_DSHOW)
    if camera is not None:
        camera.set(cv2.CAP_PROP_FRAME_WIDTH,640);camera.set(cv2.CAP_PROP_FRAME_HEIGHT,480)
    sequence=0
    def publish(status, pose=(0,0,0), crops=None, fields=None, mouth=(0,0,0), details=(0,0,0)):
        nonlocal sequence
        sequence+=1
        header=struct.pack('<IIIIfff',0x52494735,sequence,status,0,*pose).ljust(64,b'\0')
        if kernel.WaitForSingleObject(mutex,100) in (0,128):
            try:
                memory.seek(0);memory.write(header[:28])
                memory.seek(32);memory.write(struct.pack('<ffffff',*mouth,*details))
                if crops:
                    memory.seek(64);memory.write(b''.join(crops))
                    if fields:memory.write(b''.join(fields))
            finally: kernel.ReleaseMutex(mutex)
    def active():
        memory.seek(28)
        return memory.read(4)==b'\x01\0\0\0'
    if args.test_stream:
        pixels=bytes((35,70,100,128))*(256*192)
        field=distance_field([(60,96),(90,72),(166,72),(196,96),(166,120),(90,120)])
        while active():
            if sequence%75<45:publish(1,(12.,7.,-9.),[pixels]*3,[field]*3,(.24,.64,1.),(.5,.25,1.))
            else:publish(2)
            time.sleep(1/30)
        memory.close();kernel.CloseHandle(ctypes.c_void_p(mutex));return
    if not camera.isOpened():
        publish(3)
        while active():time.sleep(.1)
        camera.release();memory.close();kernel.CloseHandle(ctypes.c_void_p(mutex));return
    previous=[None,None,None]
    try:
        with mp.solutions.face_mesh.FaceMesh(refine_landmarks=True,max_num_faces=1,min_detection_confidence=.5,min_tracking_confidence=.5) as face:
            while True:
                # The app owns byte 28: closing or stopping ends this helper.
                memory.seek(28)
                if memory.read(4)!=b'\x01\0\0\0':break
                good,frame=camera.read()
                if not good:publish(3);time.sleep(.1);continue
                result=face.process(cv2.cvtColor(frame,cv2.COLOR_BGR2RGB))
                if not result.multi_face_landmarks:previous=[None,None,None];publish(2);continue
                landmarks=result.multi_face_landmarks[0].landmark
                points=[(p.x*frame.shape[1],p.y*frame.shape[0]) for p in landmarks]
                # Six stable face landmarks with a generic face model give head rotation.
                indices=[1,152,33,263,61,291]
                model=np.array([(0,0,0),(0,-330,-65),(-225,170,-135),(225,170,-135),(-150,-150,-125),(150,-150,-125)],np.float64)
                h,w=frame.shape[:2]; intrinsic=np.array([[w,0,w/2],[0,w,h/2],[0,0,1]],np.float64)
                ok,rvec,tvec=cv2.solvePnP(model,np.array([points[i] for i in indices],np.float64),intrinsic,np.zeros((4,1)),flags=cv2.SOLVEPNP_ITERATIVE)
                pose=(0.,0.,0.)
                if ok:
                    rotation=cv2.Rodrigues(rvec)[0]; angles=cv2.RQDecomp3x3(rotation)[0]
                    pitch=((angles[0]+180)%360)-180
                    if abs(pitch)>90:pitch=(180-abs(pitch))*(-1 if pitch<0 else 1)
                    pose=(float(np.clip(-angles[2],-45,45)),float(np.clip(pitch,-45,45)),float(np.clip(angles[1],-60,60)))
                regions=[[33,133,159,145],[263,362,386,374],[61,291,0,17]]
                crops=[];fields=[]
                roll=face_roll(points)
                for i in range(3):
                    image,field,polygon=cutout(frame,points,regions[i],CONTOURS[i],previous[i],roll)
                    crops.append(image);fields.append(field);previous[i]=polygon
                measure=mouth_measurements(landmarks,w,h,pose)
                publish(1,pose,crops,fields,measure,mouth_visual_cues(frame,points,measure[0]) if measure[2] else (0,0,0))
    finally:
        camera.release();memory.close();kernel.CloseHandle(ctypes.c_void_p(mutex))

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--name');parser.add_argument('--camera',type=int,default=0);parser.add_argument('--self-test',action='store_true');parser.add_argument('--test-stream',action='store_true');args=parser.parse_args()
    if args.self_test:self_test()
    else:run(args)
