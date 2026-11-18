# OnyxOS

**Xbox UWP OS Emulator powered by QEMU**

OnyxOS is a project to explore running x86-64 guest operating systems inside a Windows/UWP application, with an interface designed for Xbox and controller-based use.

> **Project status:** Early development / prototype. The features below describe the project goals and planned architecture; they are not a claim that every feature is implemented or working yet.

## Project goals

- Host x86-64 virtual machines using the QEMU system emulator.
- Provide an Xbox-friendly launcher and individual OS launch experiences.
- Support ISO images and persistent virtual disks.
- Forward keyboard, mouse, and Xbox controller input to the guest.
- Provide guest audio through an XAudio2 integration.
- Present the guest display through a Direct3D-based renderer.

## Planned operating-system profiles

The launcher is intended to offer separate profiles for:

- Windows 10
- Windows 11
- Debian
- Ubuntu
- Arch Linux
- macOS

Guest compatibility depends on the operating system, its firmware requirements, QEMU configuration, and the limitations of the UWP environment. macOS virtualization may have additional licensing and hardware restrictions.

## Planned architecture

| Component | Purpose |
| --- | --- |
| Launcher | Select and manage guest OS profiles |
| QEMU x86-64 host | Run virtual machines using software emulation (TCG) |
| Display | Transfer the guest framebuffer to a Direct3D renderer |
| Input | Map keyboard, pointer, and gamepad input |
| Audio | Route guest audio through XAudio2 |
| Storage | Manage ISO images and virtual disks |
| Tests | Hold verification scripts and test notes |

### Graphics and audio notes

- **HLSL / Direct3D:** intended for rendering the guest framebuffer on the host.
- **GLSL:** requires a compatible OpenGL implementation or translation layer; it is not automatically supported by an HLSL renderer.
- **XAudio2:** requires a working QEMU audio callback/backend integration.
- 3D acceleration inside the guest is not guaranteed. The initial target is reliable framebuffer display.

## ISO and data layout

The intended per-app data layout is:

```text
LocalState/
├── ISO/
├── VHDX/
├── Configurations/
└── Logs/
```

Each UWP application has its own storage area. Sharing ISO files between separate apps must use an appropriate Windows file picker or supported access mechanism; one app should not assume it can read another app's LocalState directly.

## QEMU dependency

The intended x86-64 QEMU host library is:

`qemu-system-x86_64.dll`

The DLL is not a standalone executable. A working package also needs the matching runtime dependencies, firmware/resources, host integration, and correct UWP packaging. The exact DLL build must be checked against the host API expected by this project.

Upstream reference: [QEMU-UWP-Host](https://github.com/rodrigoandrigo/QEMU-UWP-Host)

## Tests

The `Tests/` directory is reserved for validation scripts, test instructions, logs, and compatibility checks.

Start here: [Tests/README.md](Tests/README.md)

## Build environment

The upstream QEMU-UWP host uses a Visual Studio C++/UWP project and MSBuild. CMake may be used for independent components or introduced as part of a future build-system migration, but it does not replace the upstream project configuration automatically.

A Windows machine with the required Visual Studio UWP workload, Windows SDK, and matching QEMU dependencies is needed to build and run the UWP package. Xbox deployment and runtime behavior must be tested on the target device.

## Important limitations

- QEMU TCG is software emulation; performance depends heavily on the host CPU.
- UWP sandboxing and Xbox memory limits constrain available devices, files, and resources.
- VT-x/KVM, kernel drivers, unrestricted hardware access, and GPU passthrough must not be assumed available.
- VM memory settings are targets, not guarantees; test each guest profile on the target hardware.

## License

This repository's license and the licenses of all bundled dependencies must be reviewed before redistribution. QEMU and other third-party components retain their respective licenses.
