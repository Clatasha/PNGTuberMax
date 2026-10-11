# PNGTuber Max

**by Clatasha**

A standalone PNG avatar app based on RahiTuber, with a familiar sprite manager and a light redesign.

This product includes software developed by Rahisaurus.

## Current status

This repository contains the initial branded source foundation, based on upstream RahiTuber 14.42. It is not a downloadable, validated PNGTuber Max release yet.

The inherited engine includes PNG layers, folders, talking/blinking/screaming states, sprite sheets, voice-driven mouth shapes, inherited motion and physics, masks/blend modes, and shortcuts. User-facing titles and Help branding now identify PNGTuber Max by Clatasha. A neutral blue theme is supplied. Upstream update checks are disabled; release links point to this repository.

The standalone virtual camera, webcam face tracking, plush-style front/back PNG mode, and live eye/mouth cutouts are **planned additions**. They have not been integrated into this source foundation.

## Build on Windows

Install Visual Studio 2022 with Desktop development with C++, a Windows SDK, Git, and CMake (3.24 or newer). Build dependencies download during source setup, not at regular app launch.

```powershell
git clone --recurse-submodules https://github.com/Clatasha/PNGTuberMax.git
cd PNGTuberMax
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target RahiTuber
```

The internal target/directory names remain RahiTuber to limit unnecessary changes. The application executable is named `PNGTuberMax.exe`. Keep its resources, configuration, version file and required runtime libraries together. A complete Windows package still needs build and runtime validation.

## Personal artwork

No DERSPAWN mascot, personal environment art, saved character setups, or user device settings are included in this public repository. Add your own assets locally. Use the inherited Make Portable tool when sharing a character setup, and share only artwork you have permission to distribute.

## Development

See [the roadmap](docs/ROADMAP.md) and [upstream provenance](docs/UPSTREAM.md). Existing XML character files should remain supported; compatibility will be verified during the first Windows build.

## Credits and licensing

PNGTuber Max is a modified derivative of [RahiTuber](https://github.com/ThomasR95/rahituber), created by Rahisaurus (Tom Rule). It is not an official RahiTuber release and does not imply endorsement.

The upstream license and notices are retained in [LICENSE.txt](LICENSE.txt). Third-party code retains its own licenses. The About screen retains author acknowledgements. The repository license text contains BSD-style modification/redistribution terms; the upstream itch.io page labels the project GPLv3, so this discrepancy must be resolved before a public binary release. This source foundation preserves the existing notices and does not claim a different license for upstream code.
