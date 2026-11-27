# OnyxOS

**Xbox UWP OS emulator project powered by QEMU**

OnyxOS is an experimental project exploring x86-64 guest operating systems inside a Windows/UWP application, with an interface intended for Xbox and controller-based use.

> **Status: early development / scaffold.** The repository contains the initial package structure, build workflow, OS profile metadata, and graphics shader examples. The seven app entries currently use the same placeholder host. The Launcher interface, QEMU integration, guest boot process, and device/audio pipelines are not complete. No guest operating system is currently claimed to boot successfully.

## Package design

OnyxOS is designed to ship as **one APPX package containing seven application entries**:

1. OnyxOS Launcher
2. Windows 10
3. Windows 11
4. Debian
5. Ubuntu
6. Arch Linux
7. macOS (experimental)

These are entries inside one package, not seven separate installers. At the current stage, the entries are manifest/build scaffolding and do not yet provide independent working virtual machines.

See [docs/APPX-STRUCTURE.md](docs/APPX-STRUCTURE.md).

## Planned operating-system profiles

The profile metadata is stored in [config/os-profiles.json](config/os-profiles.json). It describes intended architecture, machine settings, memory targets, and media locations for Windows 10/11, Debian, Ubuntu, Arch Linux, and experimental macOS.

Guest compatibility depends on firmware, QEMU configuration, operating-system requirements, and UWP restrictions. Windows 11 may require additional TPM/Secure Boot support. macOS has additional licensing and hardware restrictions and remains experimental.

See [docs/OS-PROFILES.md](docs/OS-PROFILES.md).

## Planned architecture

| Component | Purpose | Status |
| --- | --- | --- |
| Launcher | Select and manage the six guest profiles | Planned |
| QEMU x86-64 host | Run VMs using software emulation (TCG) | Not integrated |
| Display | Present the guest framebuffer | Shader source examples only |
| HLSL / Direct3D | Intended initial Xbox graphics path | Not connected to QEMU |
| GLSL | Alternative OpenGL shader path | Not connected to QEMU |
| Input | Keyboard, mouse, and Xbox controller | Planned |
| Audio | Guest audio through XAudio2 | Planned |
| Storage | ISO images and persistent virtual disks | Layout documented |
| Networking | Guest network connectivity | Planned |

### Graphics notes

Shader examples are in [graphics/shaders/](graphics/shaders/):

- `onyxos.hlsl` — HLSL vertex and pixel shader source.
- `onyxos.vert` and `onyxos.frag` — GLSL vertex and fragment shader source.

The shaders are not yet compiled or connected to a QEMU framebuffer. GLSL requires a compatible OpenGL implementation or translation layer; it is not automatically supported by an HLSL renderer. Guest 3D acceleration and GPU passthrough are not guaranteed.

## ISO and virtual disk storage

The intended app-private data layout is:

```text
LocalState/
├── ISO/
├── VHDX/
├── Configurations/
└── Logs/
```

These folders are intended for installation images, virtual disks, VM configuration files, and logs. The current scaffold does not yet include the complete file-picker and VM boot workflow. Each UWP application has its own private storage area; an app must not assume it can directly access another app's LocalState.

## QEMU dependency

On the `feature/windows11-corewindow` branch, the x86-64 runtime is sourced from the upstream [QEMU-UWP-Host](https://github.com/rodrigoandrigo/QEMU-UWP-Host) project at pinned commit `53d6af6314f535c4c71c4be269777a710063327b`. `scripts/Fetch-QemuRuntime.ps1` stages its `qemu-system-x86_64.dll`, matching runtime DLLs, BIOS/UEFI firmware and supporting QEMU resources. `Build.ps1` runs this fetch automatically and validates the staged files before packaging. See [QEMU-UWP-Host integration](docs/QEMU-UWP-HOST-INTEGRATION.md) and the retained upstream Apache-2.0 license. The runtime fetch is not proof of successful VM boot; the CoreWindow frame bridge and Xbox runtime still require end-to-end testing.

## Build and installation

- [BUILDING.md](BUILDING.md) — build prerequisites, GitHub Actions, local build, packaging, and signing notes.
- [INSTALLING.md](INSTALLING.md) — install guidance for Windows and Xbox Developer Mode.

The GitHub Actions workflow is intended to build the seven application targets and package them into one x64 APPX artifact. The workflow and package still need a successful build and device deployment test; do not treat the artifact as validated until those tests pass.

## Tests

The `Tests/` directory is reserved for validation scripts, test instructions, logs, and compatibility checks.

Start here: [Tests/README.md](Tests/README.md).

## Important limitations

- QEMU TCG is software emulation; performance depends heavily on the host CPU.
- UWP sandboxing and Xbox memory limits constrain available devices, files, and resources.
- VT-x/KVM, kernel drivers, unrestricted hardware access, and GPU passthrough must not be assumed available.
- VM memory settings are targets, not guarantees; test each guest profile on the target hardware.
- Keyboard/mouse/controller input, XAudio2 audio, Wi-Fi networking, and full VM lifecycle management are not implemented yet.
- Package signing requires a development certificate matching the Publisher declared in `Package.appxmanifest` (`CN=Bluey683`).

## Repository structure

```text
.
├── .github/workflows/   # GitHub Actions build workflow
├── config/              # Guest OS profile metadata
├── docs/                # Architecture and implementation notes
├── graphics/shaders/    # HLSL and GLSL shader examples
├── scripts/             # Asset generation scripts
├── src/                 # UWP host scaffold
├── Tests/               # Test documentation and checks
├── BUILDING.md
├── INSTALLING.md
├── CMakeLists.txt
└── Package.appxmanifest
```

## License

This repository's license and the licenses of all bundled dependencies must be reviewed before redistribution. QEMU and other third-party components retain their respective licenses.


## VM Engine (initial implementation)

The repository now includes a shared C++ VM configuration layer in
`src/vm/VirtualMachine.h/.cpp`. It validates guest settings and generates
starter QEMU arguments for the configured ISO and VHDX paths. All seven app
entry targets compile this module.

See [VM Engine documentation](docs/VM-ENGINE.md). This is not yet a bootable
virtual machine: the QEMU UWP host DLL/API, UEFI firmware, framebuffer, audio,
input, networking, disk creation, and VM lifecycle controls still need to be
integrated. No successful build or guest boot has been verified yet.

## VM devices

- [Audio, keyboard, mouse, Xbox controller and network integration](docs/VM-DEVICES.md)

## Automatic OS image downloads and VHDX

- [Official ISO download catalog, TCG and VHDX provisioning status](docs/AUTO-OS-DOWNLOAD.md)

## Xbox display layout

The guest interface uses a 1920 × 1080 (16:9) design canvas for installation
and normal use. XAML scales the same layout to the active Xbox/TV mode, with
slightly larger controls and text for TV viewing. The app does not change the
console's system display setting. OnyxOS is Xbox-only; there is no Android/Moto
version.

## XAML guest display

- [Shared Xbox XAML QEMU display for the six OS apps](docs/GUEST-DISPLAY.md)
