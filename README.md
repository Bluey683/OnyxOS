# OnyxOS

**Xbox UWP host for a Windows 11 22H2 x86-64 virtual machine**

OnyxOS is an experimental Windows UWP application targeting Xbox Developer
Mode. It uses a native CoreWindow host and QEMU TCG software emulation.

> **Experimental:** a complete Windows 11 guest boot on Xbox is not yet
> verified. The VM profile is configured for one virtual CPU and a strict
> maximum of 3072 MB guest RAM. These limits are below Microsoft's published
> Windows 11 minimum requirements (2 cores and 4 GB RAM), so Windows 11 22H2
> setup or runtime may refuse to proceed or be unstable.

## Scope

The only guest target is **Windows 11 version 22H2, x64**. No other guest
operating systems are included. Target host is Xbox Series X|S UWP Developer
Mode. Hardware virtualization, KVM and VT-x are not assumed.

## VM configuration

- One virtual CPU.
- 3072 MB maximum guest RAM (3 GiB).
- QEMU TCG single-threaded software translation.
- 128 MB JIT translation-block cache (configurable 16–256 MB).
- Q35 machine and UEFI firmware.
- Windows 11 22H2 x64 installer ISO and persistent QCOW2 disk.
- Direct3D 11 CoreWindow host; no XAML pages.

## LocalState storage

    LocalState/
    ├── Windows11/
    │   ├── ISO/
    │   │   └── Windows11-22H2-x64.iso
    │   ├── Disks/
    │   │   └── Windows11-22H2.qcow2
    │   ├── Configurations/
    │   └── State/
    └── Logs/
        ├── OnyxOS.log
        └── QEMU.log

The ISO is the installer media; the QCOW2 file stores the installed guest.
Automatic QCOW2 creation and full installation flow are not implemented yet.

## Diagnostics

- LocalState/Logs/OnyxOS.log — application INFO/WARN/ERROR messages.
- LocalState/Logs/QEMU.log — QEMU runtime messages.
- Debugger output is also emitted for development diagnostics.

## QEMU and JIT

This branch uses upstream
[QEMU-UWP-Host](https://github.com/rodrigoandrigo/QEMU-UWP-Host), pinned to
commit `53d6af6314f535c4c71c4be269777a710063327b`. QEMU TCG performs software
CPU translation with a bounded JIT translation-block cache. This is not
hardware virtualization.

## Build

- [BUILDING.md](BUILDING.md)
- [INSTALLING.md](INSTALLING.md)
- [GitHub Actions](.github/workflows/build-uwp.yml)

## Known limitations

- Windows 11 22H2 officially requires at least 2 CPU cores and 4 GB RAM;
  this project's 1-vCPU/3-GB cap is an unsupported configuration.
- TPM 2.0 and Secure Boot emulation still need implementation and validation.
- QCOW2 creation, complete VM lifecycle, guest framebuffer, input, audio and
  networking still need end-to-end Xbox testing.
- TCG software emulation may be slow; Xbox/UWP memory and executable-memory
  restrictions apply.

## Documentation

- [VM engine and JIT](docs/VM-ENGINE.md)
- [ISO and QCOW2 storage](docs/AUTO-OS-DOWNLOAD.md)
- [QEMU host integration](docs/QEMU-UWP-HOST-INTEGRATION.md)
- [QEMU host API](docs/QEMU-UWP-HOST-API.md)
