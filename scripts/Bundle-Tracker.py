"""Build-only script: package a private Python runtime, wheels and face models."""
import pathlib, subprocess, sys, urllib.request, zipfile
root=pathlib.Path(__file__).resolve().parents[1]
out=root/'dist/PNGTuberMax/tracker';out.mkdir(parents=True,exist_ok=True)
archive=root/'build/python-3.11.9-embed-amd64.zip'
urllib.request.urlretrieve('https://www.python.org/ftp/python/3.11.9/python-3.11.9-embed-amd64.zip',archive)
with zipfile.ZipFile(archive) as z:z.extractall(out)
subprocess.run([sys.executable,'-m','pip','install','--only-binary=:all:','--target',str(out/'Lib/site-packages'),'mediapipe==0.10.21','numpy==1.26.4','opencv-contrib-python==4.11.0.86','jax==0.4.35','jaxlib==0.4.35'],check=True)
(out/'python311._pth').write_text('python311.zip\n.\nLib/site-packages\nimport site\n')
(out/'tracker.py').write_bytes((root/'tracker/tracker.py').read_bytes())
# Keep wheel metadata/license files with the standalone runtime.
subprocess.run([str(out/'python.exe'),str(out/'tracker.py'),'--self-test'],check=True)
