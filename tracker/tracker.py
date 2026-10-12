"""Bundled, local-only face tracker for PNGTuber Max. No network access."""
import argparse, ctypes, math, mmap, struct, time
import cv2
import mediapipe as mp
import numpy as np
SIZE = 64 + 3 * 128 * 96 * 4

def cutout(frame, points, indices):
    pts = np.array([points[i] for i in indices], dtype=np.float32)
    lo, hi = pts.min(axis=0), pts.max(axis=0)
    center = (lo + hi) / 2
    w, h = max(12., float(hi[0]-lo[0])*1.7), max(8., float(hi[1]-lo[1])*2.1)
    src = np.array([[center[0]-w/2, center[1]-h/2], [center[0]+w/2, center[1]-h/2], [center[0]-w/2, center[1]+h/2]], np.float32)
    dst = np.array([[0,0],[128,0],[0,96]], np.float32)
    transform = cv2.getAffineTransform(src,dst)
    rgb = cv2.cvtColor(cv2.warpAffine(frame,transform,(128,96)),cv2.COLOR_BGR2RGB)
    mask = np.zeros((96,128), np.uint8)
    cv2.ellipse(mask,(64,48),(58,40),0,0,360,255,-1)
    mask = cv2.GaussianBlur(mask,(15,15),3)
    rgba = np.dstack(((rgb.astype(np.float32)*(mask[:,:,None]/255)).astype(np.uint8),mask))
    return rgba.tobytes()

def self_test():
    with mp.solutions.face_mesh.FaceMesh(static_image_mode=True,refine_landmarks=True,max_num_faces=1) as detector:
        assert not detector.process(np.zeros((128,128,3),np.uint8)).multi_face_landmarks
    image=np.full((128,128,3),200,np.uint8); points=[(40,50),(80,70)]
    crop=np.frombuffer(cutout(image,points,[0,1]),np.uint8).reshape(96,128,4)
    assert crop[48,64,3]==255 and crop[0,0,3]<10
    assert np.all(crop[:,:,:3]<=crop[:,:,3,None])
    print('PASS: bundled face model loads; transparent premultiplied cutouts')

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
    def publish(status, pose=(0,0,0), crops=None):
        nonlocal sequence
        sequence+=1
        header=struct.pack('<IIIIfff',0x52494731,sequence,status,0,*pose).ljust(64,b'\0')
        if kernel.WaitForSingleObject(mutex,100)==0:
            try:
                memory.seek(0);memory.write(header[:28])
                if crops:
                    memory.seek(64);memory.write(b''.join(crops))
            finally: kernel.ReleaseMutex(mutex)
    def active():
        memory.seek(28)
        return memory.read(4)==b'\x01\0\0\0'
    if args.test_stream:
        pixels=bytes((35,70,100,128))*(128*96)
        while active():publish(1,(12.,7.,-9.),[pixels]*3);time.sleep(1/30)
        memory.close();kernel.CloseHandle(ctypes.c_void_p(mutex));return
    if not camera.isOpened():
        publish(3)
        while active():time.sleep(.1)
        camera.release();memory.close();kernel.CloseHandle(ctypes.c_void_p(mutex));return
    try:
        with mp.solutions.face_mesh.FaceMesh(refine_landmarks=True,max_num_faces=1,min_detection_confidence=.5,min_tracking_confidence=.5) as face:
            while True:
                # The app owns byte 28: closing or stopping ends this helper.
                memory.seek(28)
                if memory.read(4)!=b'\x01\0\0\0':break
                good,frame=camera.read()
                if not good:publish(3);time.sleep(.1);continue
                result=face.process(cv2.cvtColor(frame,cv2.COLOR_BGR2RGB))
                if not result.multi_face_landmarks:publish(2);continue
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
                crops=[cutout(frame,points,[33,133,159,145]),cutout(frame,points,[263,362,386,374]),cutout(frame,points,[61,291,0,17])]
                publish(1,pose,crops)
    finally:
        camera.release();memory.close();kernel.CloseHandle(ctypes.c_void_p(mutex))

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--name');parser.add_argument('--camera',type=int,default=0);parser.add_argument('--self-test',action='store_true');parser.add_argument('--test-stream',action='store_true');args=parser.parse_args()
    if args.self_test:self_test()
    else:run(args)
