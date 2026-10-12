"""Package the tracker-only roll stabilization update after bundled tests."""
from pathlib import Path
import hashlib,zipfile
root=Path(__file__).resolve().parents[1]
helper=root/'dist/PNGTuberMax/tracker/tracker.py'
assert helper.read_bytes()==(root/'tracker/tracker.py').read_bytes()
output=root/'dist/updates/PNGTuberMax-Gyro-Update.zip'
output.parent.mkdir(parents=True,exist_ok=True)
readme="""PNGTuber Max by Clatasha - gyro stabilization update for 0.1.12

Close PNGTuber Max. Extract this ZIP into your current PNGTuber Max folder,
replacing tracker/tracker.py inside the existing tracker folder.
Keep the bundled runtime and your existing app executable.
Open PNGTuber Max and start the webcam again. No new setting is needed.

Both eyes and the mouth now cancel the webcam's head roll together.
The webcam pixels and precise masks use the same leveling transform.
Your source zoom, crop pan, lighting, manual rotation and placements remain.
This levels features within the character; character rig motion still applies.

Requires the Precise Cutouts 0.1.12 app update. No camera repair is needed.
Automated tests rotate a synthetic face both directions at 5, 15, 30 and
45 degrees and compare all three stabilized crops and tracked masks.
Physical webcam tracking still needs checking on your face.

Source: https://github.com/Clatasha/PNGTuberMax
"""
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as z:
    z.writestr('tracker/tracker.py',helper.read_bytes())
    z.writestr('README.txt',readme)
with zipfile.ZipFile(output) as z:
    assert z.testzip() is None
    assert z.namelist()==['tracker/tracker.py','README.txt']
print('PASS: tracker-only gyro update ZIP',hashlib.sha256(output.read_bytes()).hexdigest())
