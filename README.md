# PNGTuber Max

**by Clatasha**

A standalone PNG avatar app based on RahiTuber, with a familiar sprite manager and a light redesign.

This product includes software developed by Rahisaurus.

## Current status

This repository contains the blue-interface and virtual-camera development preview, based on upstream RahiTuber 14.42. Windows preview builds are produced in [Actions](https://github.com/Clatasha/PNGTuberMax/actions). These are development artifacts; receiving-app and visual compatibility still require Windows testing.

The inherited engine includes PNG layers, folders, talking/blinking/screaming states, sprite sheets, voice-driven mouth shapes, inherited motion and physics, masks/blend modes, and shortcuts. User-facing titles and Help branding now identify PNGTuber Max by Clatasha. The Max design uses raised blue controls, soft shadows, edge highlights, Inter typography, a branded header and 30 original action icons. Upstream update checks are disabled; release links point to this repository.

The Windows virtual camera publisher and one-time hidden installer are now integrated. They send the clean avatar scene at 640x480 / 30 FPS, without the editor interface. The DirectShow camera backend remains experimental. Webcam face tracking, plush-style front/back PNG mode, and live eye/mouth cutouts are **planned additions**.

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
