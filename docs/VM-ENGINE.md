# Windows 11 VM Engine

OnyxOS has one guest profile: **Windows 11 x86-64**. The VM configuration
prepares QEMU arguments; successful boot on Xbox has not yet been verified.

## Profile defaults

- QEMU q35 machine and max CPU model.
- 2 vCPUs and 4096 MB configured guest RAM (subject to Xbox UWP limits).
- QEMU TCG multi-threaded software translation.
- TCG translation-block/JIT cache: 128 MB by default, configurable from 16 to
  256 MB.
- Installer ISO mounted as a read-only CD-ROM ahead of the VHDX boot device.
- VHDX used as the persistent Windows 11 system disk.
- Virtio VGA, USB keyboard/tablet, user-mode networking and XAudio2/HDA
  arguments are prepared, but device integration still needs target testing.

## App-private storage

LocalState contains the following layout:

    LocalState/
    ├── Windows11/
    │   ├── ISO/
    │   │   └── Windows11.iso
    │   ├── Disks/
    │   │   └── Windows11.vhdx
    │   ├── Configurations/
    │   └── State/
    │       └── installation-complete.marker
    └── Logs/
        ├── OnyxOS.log
        └── QEMU.log

The folders are created by the storage provisioner when it is called. The
Windows 11 ISO is selected/downloaded into Windows11/ISO; the VHDX is the
persistent guest disk in Windows11/Disks. The marker is only written after
an explicit installation-complete confirmation.

## JIT / TCG

QEMU TCG is the CPU translator and JIT engine; it is not a separate host
hypervisor. OnyxOS passes the following default argument:

    -accel tcg,thread=multi,tb-size=128

The value is generated from VmConfiguration::tcgJitCacheMB and validated
between 16 and 256 MB. This controls the translation-block cache, not the
amount of RAM assigned to Windows 11. TCG remains software emulation; Xbox
hardware virtualization/KVM is not assumed.

## Implementation boundary

The VM configuration builds the argument vector, and the QEMU adapter loads
the packaged host DLL and exposes lifecycle methods. This documentation does
not claim that the full CoreWindow-to-QEMU start path, guest frame rendering,
audio, networking, VHDX creation, TPM 2.0 or Secure Boot are complete. Those
items still require build and Xbox testing.
