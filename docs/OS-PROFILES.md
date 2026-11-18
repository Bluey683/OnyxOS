# OnyxOS integrated operating-system profiles

OnyxOS is one Xbox UWP application and one package. The Launcher will present six VM profiles; the operating systems are not separate UWP applications.

## Profiles

| Launcher entry | Profile ID | Initial RAM | State |
|---|---|---:|---|
| Windows 10 | windows-10 | 3072 MB | Profile scaffold |
| Windows 11 | windows-11 | 3072 MB | Profile scaffold |
| Debian | debian | 3584 MB | Profile scaffold |
| Ubuntu | ubuntu | 3584 MB | Profile scaffold |
| Arch Linux | arch-linux | 3584 MB | Profile scaffold |
| macOS | macos | 3584 MB | Experimental placeholder; disabled by default |

The values are initial configuration targets, not guarantees that every guest will boot or perform acceptably on Xbox. QEMU TCG software emulation is expected unless a supported acceleration path is proven.

## Per-app LocalState layout

The app will create and use this layout in its own UWP LocalState directory:

```text
LocalState/
  ISO/
    windows-10.iso
    windows-11.iso
    debian.iso
    ubuntu.iso
    archlinux.iso
    macos.iso
  VHDX/
    windows-10.vhdx
    windows-11.vhdx
    debian.vhdx
    ubuntu.vhdx
    archlinux.vhdx
    macos.vhdx
  Configurations/
  Logs/
```

The ISO files are supplied by the user. The profile JSON is packaged at `config/os-profiles.json`; user-created disks and VM state belong in LocalState, not in the read-only package.

## Current implementation boundary

This commit adds profile metadata only. The Launcher UI, file picker, VHDX creation/attachment, QEMU argument builder, QEMU DLL adapter, guest boot flow, and save/resume behavior still need implementation and testing.

Windows 11 may require additional virtual TPM/Secure Boot support. macOS remains disabled until compatibility, firmware and licensing requirements are resolved. No operating-system ISO is bundled.
