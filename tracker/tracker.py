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
    def publish(status, pose=(0,0,0), crops=None, fields=None):
        nonlocal sequence
        sequence+=1
        header=struct.pack('<IIIIfff',0x52494733,sequence,status,0,*pose).ljust(64,b'\0')
        if kernel.WaitForSingleObject(mutex,100) in (0,128):
            try:
                memory.seek(0);memory.write(header[:28])
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
        while active():publish(1,(12.,7.,-9.),[pixels]*3,[field]*3);time.sleep(1/30)
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
                publish(1,pose,crops,fields)
    finally:
        camera.release();memory.close();kernel.CloseHandle(ctypes.c_void_p(mutex))

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--name');parser.add_argument('--camera',type=int,default=0);parser.add_argument('--self-test',action='store_true');parser.add_argument('--test-stream',action='store_true');args=parser.parse_args()
    if args.self_test:self_test()
    else:run(args)
