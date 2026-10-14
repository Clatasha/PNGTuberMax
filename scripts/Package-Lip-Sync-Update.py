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
output=root/'dist/updates/PNGTuberMax-Expanded-Mouths-Update.zip'
output.parent.mkdir(parents=True,exist_ok=True)
readme="""PNGTuber Max by Clatasha - 0.1.18 Expanded Mouths Update

INSTALL
Close PNGTuber Max. Extract this ZIP into your current app folder.
Replace BOTH PNGTuberMax.exe and tracker/tracker.py; they are a matched pair.
Keep your bundled Python runtime, resources, camera files and existing settings.
No camera repair or runtime reinstall is needed.

IMPORT YOUR MOUTH PACK
Extract your mouth ZIP to a permanent folder first.
On a dedicated mouth layer, open More Talk settings and Phoneme Detection.
Click Import mouth folder, then select ANY mouth PNG in that extracted folder.
The app maps A, B/P, C, E, F, G, L, N, OO, Q, S, T, U plus Idle automatically.
Names such as mouth_A.png and Mouth_T.png are recognised regardless of case.
Import enables the expanded set, canvas-size matching and camera-assisted mode.
Your PNG files stay unchanged. Keep them in place, or use Portable when exporting.

USE AND CALIBRATE
Select Combined lip sync and start the physical webcam.
The Mouth shape dropdown now offers all 13 slots. Each keeps its own image,
animation, offset, tint, edge feather and trim settings.
Match mouth canvas sizes aligns mixed 2048/1254 canvases by scale and centre.
Use the four basic calibration poses if needed. For a closer match, choose a
mouth slot, make that expression, and Capture camera pose for this mouth.
Hold it for 0.6 seconds. Opening, width and optional teeth/tongue colour cues
are averaged from fresh camera frames. These cues depend on lighting.
Shape matching settings lets you tune targets, audio hints, or disable a shape.
Preview mouth for 3 seconds lets you inspect and position a particular image.
Save your project to keep mode, calibration, images, and individual edge settings.

COMPATIBILITY
Existing five-mouth projects keep their original selection by default.
The expanded set is opt-in and never selects an empty image slot.
Audio uses five broad sound families plus opening levels, not exact recognition
of 13 letters. Camera/Combined choose among the full set of visible mouth shapes.
Tracking loss falls back to audio. Optional scream mouths have 13 slots too,
and missing scream variants reuse their normal mouth image.
Plush surface geometry, source PNGs, gyro cutouts and virtual camera are unchanged.

Source: https://github.com/Clatasha/PNGTuberMax
"""
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    z.writestr('PNGTuberMax.exe',exe)
    z.writestr('tracker/tracker.py',helper)
    z.writestr('README.txt',readme)
with zipfile.ZipFile(output) as z:
    assert z.testzip() is None
    assert z.namelist()==['PNGTuberMax.exe','tracker/tracker.py','README.txt']
print('PASS: paired app/helper expanded mouth update ZIP',hashlib.sha256(output.read_bytes()).hexdigest())
