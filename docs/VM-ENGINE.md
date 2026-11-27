# Windows 11 22H2 VM Engine

Only Windows 11 22H2 x64 is configured as a guest.

## Strict Xbox resource limits

- Exactly 1 virtual CPU.
- Maximum 3072 MB (3 GiB) guest RAM.
- QEMU TCG single-threaded software translation.
- 128 MB JIT translation-block cache by default (16–256 MB range).
- q35 machine and UEFI.

**Compatibility warning:** Microsoft's published Windows 11 minimums are
2 CPU cores and 4 GB RAM. This project's 1-vCPU/3-GB configuration is below
those requirements and is unsupported. Windows Setup may refuse installation,
and a bypass would not make the configuration supported or guarantee stability.

## Storage

    LocalState/
    ├── Windows11/
    │   ├── ISO/any-Windows11-installer.iso
    │   ├── Disks/Windows11-22H2.qcow2
    │   ├── Configurations/
    │   └── State/
    └── Logs/

Any valid Windows 11 installer ISO filename is accepted from the ISO folder. The ISO is installation media; the QCOW2 file is persistent guest storage.
Automatic QCOW2 creation and end-to-end install are not implemented yet.

## TCG JIT

QEMU TCG is software CPU translation, not a separate hypervisor. The current
argument uses a bounded translation-block cache:

    -accel tcg,thread=single,tb-size=128

The cache size is separate from the 3072 MB guest RAM cap. The final accepted
option set must be checked against the exact upstream QEMU-UWP-Host build.

## Status

CoreWindow startup, guest boot, frame presentation, TPM 2.0, Secure Boot,
QCOW2 creation, and full device integration still require build and Xbox testing.
