# OnyxOS

**Xbox UWP OS emulator project powered by QEMU**

OnyxOS is an experimental Windows UWP application designed for one purpose: running a Windows 11 x86-64 guest on Xbox consoles through QEMU software emulation. The project targets Xbox Developer Mode and uses a native C++/WinRT CoreWindow application model.

> **Development status: experimental scaffold.** The repository contains the CoreWindow host shell, package/build configuration, Windows 11 guest configuration, VM configuration code, and scripts for staging a pinned upstream QEMU-UWP-Host runtime. The QEMU runtime is not yet proven to boot a guest from the OnyxOS application. The guest display bridge, input, audio, networking, storage workflow, and full VM lifecycle still require integration and testing. No successful guest boot or fully validated Xbox deployment is claimed.

## Target platform

- **Platform:** Xbox Series X|S, through UWP Developer Mode.
- **Application model:** native CoreWindow; this branch removes the previous XAML page structure.
- **Architecture:** x64.
- **Virtualization:** QEMU x86-64 using software emulation (TCG); hardware virtualization, KVM, and VT-x are not assumed.
- **Graphics direction:** Direct3D 11/CoreWindow swap chain for the host window. Guest framebuffer presentation is still an integration task.

OnyxOS is an Xbox/UWP project. It is not an Android application and does not provide a Moto/phone version.

## Project scope: Windows 11 only

OnyxOS will support **Windows 11 only** as its guest operating system. Windows 10, Debian, Ubuntu, Arch Linux, macOS, and other guest operating systems are outside the project scope. The APPX is the OnyxOS application; Windows 11 is the single target guest, not one item in a multi-OS launcher.

The Windows 11 guest configuration is maintained in [config/os-profiles.json](config/os-profiles.json). Compatibility depends on QEMU, firmware, guest requirements, and UWP restrictions. Windows 11 may require additional TPM 2.0 and Secure Boot support, which must be validated in the target environment.

## Architecture and implementation status

| Component | Intended role | Current status |
| --- | --- | --- |
| CoreWindow host | Native Xbox/UWP application window and event loop | Initial host shell present; device testing required |
| D3D11 presentation | Swap chain and host rendering surface | Host renderer scaffold; guest framebuffer not yet connected end to end |
| QEMU x86-64 | Run the Windows 11 guest using TCG | Pinned upstream runtime staging is configured; VM boot not verified |
| VM configuration | Validate guest settings and prepare QEMU configuration/arguments | Shared configuration layer present |
| Windows 11 configuration | Configure the single supported guest OS | Configuration scaffolding; complete setup workflow pending |
| Input | Keyboard, pointer, and Xbox controller | Not fully integrated |
| Audio | Guest audio output through the Xbox/UWP audio path | Not integrated |
| Storage | ISO images, virtual disks, and VM configuration | Intended layout documented; complete provisioning workflow pending |
| Networking | Guest network connectivity | Not integrated |

## QEMU-UWP-Host runtime

This branch uses the upstream [QEMU-UWP-Host](https://github.com/rodrigoandrigo/QEMU-UWP-Host) project, pinned to commit:

`53d6af6314f535c4c71c4be269777a710063327b`

The script [scripts/Fetch-QemuRuntime.ps1](scripts/Fetch-QemuRuntime.ps1) downloads that exact revision and stages the x86-64 QEMU host DLL, matching runtime DLLs, firmware, and supporting QEMU resources. [Build.ps1](Build.ps1) invokes the fetch script and validates required runtime assets before packaging.

This is dependency staging only. It does not establish that the upstream API is fully connected to the OnyxOS CoreWindow lifecycle, that guest frames are displayed, or that a guest operating system boots successfully.

See:
- [QEMU-UWP-Host integration notes](docs/QEMU-UWP-HOST-INTEGRATION.md)
- [Upstream QEMU-UWP-Host API notes](docs/QEMU-UWP-HOST-API.md)
- [Upstream license](Dependencies/QEMU-UWP-Host-LICENSE)

## Graphics

The host application uses a Direct3D 11 swap chain associated with CoreWindow. Shader examples, where present, are not evidence of a completed guest GPU pipeline or hardware acceleration.

UWP sandboxing and Xbox hardware restrictions mean GPU passthrough and unrestricted device access must not be assumed. The current design relies on CPU-based QEMU TCG emulation for guest execution.

## Guest files and storage

The intended app-private data layout is:

```text
LocalState/
├── ISO/
├── VHDX/
├── Configurations/
└── Logs/
```

These locations are intended for the Windows 11 installation image, its virtual disk, VM configuration, and logs. The complete file-selection, disk-creation, and guest-start workflow is still under development. UWP applications have isolated private storage; the application must not assume access to another app's LocalState.

## Build and installation

- [BUILDING.md](BUILDING.md) — build prerequisites, local build, GitHub Actions, packaging, and signing.
- [INSTALLING.md](INSTALLING.md) — installation guidance for Windows and Xbox Developer Mode.
- [GitHub Actions](.github/workflows/build-uwp.yml) — x64 UWP build workflow.

The workflow is configured to build and package the project. A workflow run or generated APPX is not proof of a successful Xbox deployment. Treat the package as unvalidated until the build completes successfully and the app is installed and tested on target hardware.

Package signing requires a development certificate whose subject matches the Publisher in [Package.appxmanifest](Package.appxmanifest).

## Documentation

- [APPX package structure](docs/APPX-STRUCTURE.md)
- [Windows 11 guest configuration](docs/OS-PROFILES.md)
- [VM engine](docs/VM-ENGINE.md)
- [VM devices](docs/VM-DEVICES.md)
- [Automatic OS downloads and VHDX](docs/AUTO-OS-DOWNLOAD.md)
- [Guest display status](docs/GUEST-DISPLAY.md)
- [QEMU-UWP-Host integration](docs/QEMU-UWP-HOST-INTEGRATION.md)

## Important limitations

- QEMU TCG is software emulation and can be slow, especially on console-class low-power execution environments.
- Xbox/UWP memory budgets, sandboxing, API availability, and package restrictions constrain what the application can do.
- VT-x/KVM, kernel drivers, unrestricted hardware access, and GPU passthrough are not assumed available.
- Configured guest RAM values are targets, not guarantees of available memory.
- Input, audio, networking, persistent disk provisioning, guest display integration, and complete VM lifecycle controls are not yet verified end to end.
- Windows 11 guest compatibility, including TPM 2.0, Secure Boot, and installation, must be verified on target hardware.

## Repository structure

```text
.
├── .github/workflows/       # GitHub Actions workflow
├── config/                  # Windows 11 guest configuration
├── Dependencies/            # Third-party runtime staging locations
├── docs/                    # Architecture and integration notes
├── graphics/                # Host graphics and shader sources
├── qemu/                    # QEMU firmware and supporting resources
├── scripts/                 # Runtime and asset scripts
├── src/                     # Native UWP/CoreWindow and VM source
├── Tests/                   # Test documentation and checks
├── Build.ps1                # Build and packaging entry point
├── BUILDING.md
├── INSTALLING.md
└── Package.appxmanifest
```

## License

Review this repository's license and the licenses of all third-party dependencies before redistribution. QEMU and upstream QEMU-UWP-Host components retain their respective licenses; see the included upstream license file and upstream project for details.
