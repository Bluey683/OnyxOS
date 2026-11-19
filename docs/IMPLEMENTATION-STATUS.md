# OnyxOS implementation status

This repository targets one Xbox UWP application and one APPX package. The Launcher and operating-system profiles are modules inside this application; they are not separate app packages.

## Added in the starter scaffold

- CMake project and a single OnyxOS executable target.
- Initial UWP CoreApplication / IFrameworkView entry point.
- One root Package.appxmanifest.
- PowerShell script that generates the PNG assets required by the manifest.
- Network client capabilities in the manifest.

## Not implemented yet

- XAML Launcher page scaffold and profile selection UI source (not yet wired into the app or compiled by the current CMake workflow).
- QEMU DLL, matching headers, dependent DLLs, firmware and license notices.
- QEMU API adapter, VM configuration, ISO picker and persistent disk management.
- HLSL renderer, optional GLSL backend, XAudio2 audio output.
- Keyboard, mouse and Xbox controller mapping.
- Guest OS boot profiles and per-VM RAM limits.

## Important build and deployment notes

The current CMake target is a starter UWP host, not a completed emulator. A successful native compile does not by itself prove that the APPX is correctly packaged, signed, installable or compatible with Xbox. The package publisher and signing certificate must match the certificate trusted by the target Xbox Developer Mode environment.

The GitHub Actions workflow must generate the manifest PNG assets, build the UWP target, stage the executable and assets, and call Windows SDK MakeAppx. QEMU binaries must be added only after their exact version, architecture, exports and redistribution terms are verified.
