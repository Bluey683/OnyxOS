# OnyxOS

**Xbox UWP application for running a Windows 11 x86-64 guest with QEMU**

OnyxOS is an experimental Windows UWP application targeting Xbox Developer
Mode. It uses a native CoreWindow host and QEMU software emulation.

> **Status: experimental.** The repository contains the CoreWindow shell,
> QEMU runtime staging, Windows 11 VM configuration, app-private storage
> layout, and diagnostic logging. A successful Windows 11 guest boot on Xbox
> is not yet verified. VHDX creation, full VM lifecycle, guest display,
> audio, networking, TPM 2.0 and Secure Boot still require implementation
> and target testing.

## Scope

OnyxOS supports **Windows 11 only** as its guest operating system. It does not
provide a multi-OS launcher. Target platform is Xbox Series X|S in UWP
Developer Mode, x64, using QEMU TCG. Hardware virtualization, KVM and VT-x
are not assumed.

## Architecture

- Native C++/WinRT CoreWindow application; no XAML pages.
- Direct3D 11 host swap chain.
- QEMU x86-64 through the upstream QEMU-UWP-Host runtime.
- TCG multi-threaded software CPU translation with a configurable JIT
  translation-block cache (128 MB default; 16–256 MB allowed).
- Windows 11 x64 guest profile, with 2 vCPUs and 4096 MB configured RAM.
- Windows 11 ISO installer and persistent VHDX disk kept in app-private
  LocalState storage.

## Windows 11 files

On first storage initialization, OnyxOS creates the following app-private
folders:

    LocalState/
    ├── Windows11/
    │   ├── ISO/
    │   │   └── Windows11.iso
    │   ├── Disks/
    │   │   └── Windows11.vhdx
    │   ├── Configurations/
    │   └── State/
    └── Logs/
        ├── OnyxOS.log
        └── QEMU.log

The ISO is the Windows 11 installer. The VHDX stores the installed Windows
11 system. Existing ISO/VHDX files are signature-checked before use. Automatic
VHDX creation is not implemented yet, so a valid disk must be provisioned
before booting an installed guest.

## Logs

- LocalState/Logs/OnyxOS.log: application INFO, WARN and ERROR events.
- LocalState/Logs/QEMU.log: QEMU host/runtime diagnostics.
- Messages are also sent to the debugger output stream.

## QEMU runtime

This branch uses upstream
[QEMU-UWP-Host](https://github.com/rodrigoandrigo/QEMU-UWP-Host), pinned to
commit 53d6af6314f535c4c71c4be269777a710063327b. The script
[scripts/Fetch-QemuRuntime.ps1](scripts/Fetch-QemuRuntime.ps1) stages the
matching x64 host DLL, runtime DLLs and firmware. Runtime staging does not
prove that a guest boots.

## Build and installation

- [BUILDING.md](BUILDING.md) — prerequisites and build instructions.
- [INSTALLING.md](INSTALLING.md) — Windows/Xbox Developer Mode installation.
- [GitHub Actions](.github/workflows/build-uwp.yml) — x64 UWP build.

## Current limitations

- The complete CoreWindow-to-QEMU startup and guest framebuffer path are not
  yet verified end to end.
- Automatic VHDX creation is not implemented.
- Windows 11 TPM 2.0 and Secure Boot compatibility must be completed and tested.
- Input, audio and network integration still require Xbox testing.
- TCG is software emulation and may be slow; the JIT cache is not a hardware
  virtualization feature.
- Xbox/UWP memory, storage and executable-memory restrictions apply.

## Documentation

- [VM engine and JIT](docs/VM-ENGINE.md)
- [ISO and VHDX storage](docs/AUTO-OS-DOWNLOAD.md)
- [QEMU host integration](docs/QEMU-UWP-HOST-INTEGRATION.md)
- [QEMU host API](docs/QEMU-UWP-HOST-API.md)

## License

Review this repository's license and all third-party licenses before
redistribution. QEMU and QEMU-UWP-Host retain their respective licenses.
