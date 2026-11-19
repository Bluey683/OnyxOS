# VM Engine

The VM engine now prepares a more complete QEMU configuration for the six guest
profiles while keeping the seven app entries inside one APPX package.

## Profile defaults

- x86_64 guests on the QEMU `q35` machine.
- QEMU TCG software emulation with two vCPUs.
- 3072 MB guest RAM as a conservative starting point for the Xbox UWP memory budget.
- VHDX virtual disk plus an installer ISO.
- Virtio VGA, USB keyboard/tablet and user-mode networking arguments.
- Per-guest ISO and VHDX paths under the app's LocalState directory.

The current profile list is Windows 10, Windows 11, Debian, Ubuntu, Arch Linux
and an experimental macOS placeholder. macOS remains disabled.

## Storage layout

```
LocalState/
  ISO/
  VHDX/
  Configurations/
  Logs/
```

## Important implementation boundary

This code validates profile values and builds QEMU command-line argument vectors.
It does **not** yet pass those arguments to the QEMU UWP host API or start a guest.
The QemuHostAdapter currently loads the packaged DLL and checks expected exports;
it does not call the host lifecycle functions or run the guest loop.

The generated virtio, USB and user-network arguments are therefore preparation,
not proof that those devices are active. Audio through XAudio2, keyboard/mouse/
controller event forwarding, guest framebuffer presentation, UEFI firmware
discovery and actual networking still require host-API integration and testing.

UEFI arguments must be added only after the exact compatible firmware files are
available. Windows 11 TPM 2.0/Secure Boot support is not configured, so its
installer may reject this profile. macOS compatibility, firmware and licensing
remain unresolved.

See [QEMU-HOST-ADAPTER.md](QEMU-HOST-ADAPTER.md) for DLL integration status.
