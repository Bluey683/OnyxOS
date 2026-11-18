# OnyxOS: one APPX, seven app entries

The package manifest declares seven launchable entries inside the same `Bluey683.OnyxOS` package:

1. OnyxOS Launcher
2. OnyxOS - Windows 10
3. OnyxOS - Windows 11
4. OnyxOS - Debian
5. OnyxOS - Ubuntu
6. OnyxOS - Arch Linux
7. OnyxOS - macOS Experimental

There is one installer/package file: `OnyxOS-x64.appx`. The entries are not seven separate APPX/MSIX installers.

## Important implementation status

The CMake project currently builds seven executable host targets from the shared CoreWindow host scaffold and the manifest exposes them as seven package applications. They are entry-point placeholders: the guest-specific QEMU startup, profile activation routing, Launcher UI, ISO picker, virtual disk creation, and QEMU integration are not implemented yet. At this stage, selecting one of the OS-labelled entries does not boot that operating system.

The VM profile metadata is in `config/os-profiles.json`. The intended guest ISO and virtual disk storage is under the app's `LocalState/ISO` and `LocalState/VHDX` folders.

The GitHub Actions workflow creates one unsigned APPX artifact. Installing it on Xbox requires a compatible signing certificate and matching package Publisher identity, plus successful validation on the target Xbox/Windows SDK. The workflow has not yet been confirmed by a successful build in this repository.
