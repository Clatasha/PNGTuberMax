"""Package the paired application/helper camera lip-sync update."""
from pathlib import Path
import hashlib,struct,zipfile
root=Path(__file__).resolve().parents[1]
folder=root/'dist/PNGTuberMax'
exe=(folder/'PNGTuberMax.exe').read_bytes()
helper=(folder/'tracker/tracker.py').read_bytes()
assert helper==(root/'tracker/tracker.py').read_bytes()
assert exe[:2]==b'MZ'
pe=struct.unpack_from('<I',exe,0x3c)[0]
assert exe[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',exe,pe+4)[0]==0x8664
output=root/'dist/updates/PNGTuberMax-Camera-Lip-Sync-Update.zip'
output.parent.mkdir(parents=True,exist_ok=True)
readme="""PNGTuber Max by Clatasha - 0.1.17 Camera Lip Sync Update

INSTALL
Close PNGTuber Max. Extract this ZIP into your existing app folder.
Replace BOTH PNGTuberMax.exe and tracker/tracker.py. They are a matched pair.
Keep the existing bundled Python runtime, resources, camera files and settings.
No camera repair or tracker runtime reinstall is needed.
Requires an existing complete Windows build with the bundled webcam tracker.

USE
On your mouth layer, enable Use Talk Sprite and Phoneme Detection.
Open More Talk settings and choose Lip-sync source: Audio, Camera, or Combined.
Audio keeps the existing microphone selection. Old projects default to Audio.
Camera follows visible mouth movement, including silent expressions.
Combined lets the camera guide shape, uses audio for close E/S choices, and
selects P/B when your lips are closed during speech.
Both camera modes fall back to audio when the face is lost or too far sideways.
Start the physical webcam from the main toolbar or the lip-sync controls.
Use Preferences > Webcam rig to change the selected camera.

CALIBRATION
Expand Mouth calibration. Hold closed lips and capture Closed lips.
Then capture Open tall (A), Wide (E), and Rounded (OO), holding each for 0.6 sec.
Several fresh camera frames are averaged for each capture.
Shape smoothing reduces flicker; lower it for quicker image changes.
Closed lips respond immediately. The live shape label and opening bar help you tune.
Save the project to keep mode, smoothing and per-layer calibration.
Reset mouth calibration restores the default measurements.

Your existing five phoneme images and Idle are reused. Missing mouth images
fall back to Talk or Idle instead of making the avatar disappear.
PNGs, edge blending, cutout controls, attachment and plush geometry are unchanged.
Automated tests cover synthetic shapes, landmark rotation, selection, camera
loss/recovery, saved settings and renderer integration. Try calibration on your
own face and camera to check the visual match with your mouth images.

Source: https://github.com/Clatasha/PNGTuberMax
"""
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    z.writestr('PNGTuberMax.exe',exe)
    z.writestr('tracker/tracker.py',helper)
    z.writestr('README.txt',readme)
with zipfile.ZipFile(output) as z:
    assert z.testzip() is None
    assert z.namelist()==['PNGTuberMax.exe','tracker/tracker.py','README.txt']
print('PASS: paired app/helper camera lip-sync update ZIP',hashlib.sha256(output.read_bytes()).hexdigest())
