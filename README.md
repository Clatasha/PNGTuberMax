# PNGTuber Max

**by Clatasha**

A standalone PNG avatar app based on RahiTuber, with a familiar sprite manager and a light redesign.

This product includes software developed by Rahisaurus.

## Current status

This repository contains the blue-interface and virtual-camera development preview, based on upstream RahiTuber 14.42. Windows preview builds are produced in [Actions](https://github.com/Clatasha/PNGTuberMax/actions). These are development artifacts; receiving-app and visual compatibility still require Windows testing.

The inherited engine includes PNG layers, folders, talking/blinking/screaming states, sprite sheets, voice-driven mouth shapes, inherited motion and physics, masks/blend modes, and shortcuts. User-facing titles and Help branding now identify PNGTuber Max by Clatasha. The Max design uses raised blue controls, soft shadows, edge highlights, Inter typography, a branded header and 30 original action icons. Upstream update checks are disabled; release links point to this repository.

The Windows virtual camera publisher and one-time hidden installer are now integrated. They send the clean avatar scene at 640x480 / 30 FPS, without the editor interface. The DirectShow camera backend remains experimental. Plush-style front/back PNG mode is available. Webcam face tracking and live eye/mouth cutouts are planned additions.

## Build on Windows

Install Visual Studio 2022 with Desktop development with C++, a Windows SDK, Git, and CMake (3.24 or newer). Build dependencies download during source setup, not at regular app launch.

```powershell
git clone --recurse-submodules https://github.com/Clatasha/PNGTuberMax.git
cd PNGTuberMax
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target RahiTuber MaxCameraFrameTests
.\scripts\Package-Windows.ps1
```

The internal target/directory names remain RahiTuber to limit unnecessary changes. The application executable is named `PNGTuberMax.exe`. Keep its resources, configuration, version file and required runtime libraries together. The packaging script bundles the camera components, notices, assets and MSVC runtime. It requires the Windows .NET Framework compiler and Visual Studio redistributable files.

## Personal artwork

No DERSPAWN mascot, personal environment art, saved character setups, or user device settings are included in this public repository. Add your own assets locally. Use the inherited Make Portable tool when sharing a character setup, and share only artwork you have permission to distribute.

## Development

See [the roadmap](docs/ROADMAP.md) and [upstream provenance](docs/UPSTREAM.md). Existing XML character files should remain supported; compatibility will be verified during the first Windows build.

## Credits and licensing

PNGTuber Max is a modified derivative of [RahiTuber](https://github.com/ThomasR95/rahituber), created by Rahisaurus (Tom Rule). It is not an official RahiTuber release and does not imply endorsement.

The upstream license and notices are retained in [LICENSE.txt](LICENSE.txt). Third-party code retains its own licenses. The About screen retains author acknowledgements. The repository license text contains BSD-style modification/redistribution terms; the upstream itch.io page labels the project GPLv3, so this discrepancy must be resolved before a public binary release. This source foundation preserves the existing notices and does not claim a different license for upstream code.

## 0.1.2 workspace update

The editor now separates studio/layers (left), avatar preview (middle), and persistent Preferences (right). File imports and project open/save use Windows native dialogs. The selected microphone has a six-block live raw-input peak meter, independent of the talking threshold. Layer additions/duplicates are deferred to avoid invalidating UI references, and blank-sprite editor guides are guarded.

Camera installation handles downloaded-DLL blocking and logs native registration warnings without treating them as fatal errors. The in-app Camera preferences show setup results and launch a frame-delivery diagnostic. Windows CI additionally installs the camera and verifies actual DirectShow samples against a shared-memory test pattern. This does not prove compatibility with every receiving app or the user's specific camera error.

### 0.1.3 audio recovery and plush PNG preview
Missing microphones use an inline status and default-input action, so editing remains accessible.
Audio streams validate raw device IDs and close safely before device refresh.
A body layer can join its Idle/front sprite to a Back view PNG, with roundness, a full turn
slider and optional mouse turning. The two PNG surfaces share silhouette seams; no 3D rig
is used. Project persistence and portable asset copies include the rear image and settings.
Separate accessories retain their existing motion attachments. Surface attachment is future work.

### 0.1.4 editor fixes
Alt-drag rotates the selected plush body in the preview. Facing-range clipping, continuous
rear UVs and optional GPU mipmaps improve rotated PNG rendering. Sprite inputs stack in the
studio panel with explicit blinking controls and scrolling. Save toolbar labels are clickable;
new projects open Save As, shortcuts work, and successful saves show a receipt in the editor.

### 0.1.5 editor space and continuous plush sampling
Restores the original RahiTuber sprite and phoneme arrangements in a wider studio panel.
Transform fields use the available width rather than a fixed large spacing deduction.
Plush mode now samples the visible front/back surface per pixel, caches the result, removes
source mipmap averaging and bypasses the preview FXAA pass. Its premultiplied colours are
composited once. Sharpness tests cover detailed PNGs at small turn angles.

Camera capability outputs now use caller-owned native buffers. Windows tests probe the unmanaged format API, invalid arguments, twenty device-selection cycles, and repeated receiver start/stop connections. Compatibility with the reporting user’s recorder still needs verification. Close receiving apps and use Install / repair camera once when replacing an older preview.

### 0.1.6 camera installation and discovery checks

Camera DLLs install into separate directories per build, allowing updates while older DLLs are locked. Receiving apps still need a full restart to load the new registration. The checker rejects installed DLLs that do not match the package, and describes frame delivery as an internal test. Native tests now instantiate the camera through its device moniker, covering a discovery step used by OBS. User testing in OBS and Mirillis Action remains necessary.

### 0.1.7 native camera replacement

The camera DLL now uses Microsoft's MIT-licensed DirectShow base classes at commit 434f6002bdf9cf9829406c3ff2b33387982d6168. The native source includes RGB32/RGB24 and separate 32-bit/64-bit DLLs. Install / repair migrates the existing device registration from the CLR loader to the native DLL. The standalone checker uses .NET but does not reference or load a managed camera implementation. Real OBS and Mirillis Action compatibility remains subject to user verification.

### 0.1.8 visible camera controls

The user confirmed camera output worked after pressing Start camera. Both camera controls now use green for Start and red for Stop, with larger buttons and explicit output status. Output-off guidance points to the green control. The native backend remains 0.1.7.
