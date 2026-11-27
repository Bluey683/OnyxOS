# Windows 11 ISO, VHDX and TCG/JIT

## Windows 11 installer ISO

OnyxOS accepts the official Windows 11 x64 ISO selected by the user. Microsoft
requires edition/language selection and uses time-limited download links, so
OnyxOS opens the official
[Windows 11 download page](https://www.microsoft.com/software-download/windows11)
instead of pretending that the page itself is an ISO file.

Save the ISO as:

    LocalState/Windows11/ISO/Windows11.iso

The storage provisioner checks the ISO9660 CD001 volume descriptor before
returning the image path to the VM configuration.

## Windows 11 virtual disk

The persistent guest disk path is:

    LocalState/Windows11/Disks/Windows11.vhdx

An existing VHDX is checked for its VHDX signatures before use. Automatic
VHDX creation is not implemented yet; the app must not create an empty file
and misrepresent it as a bootable disk. A UWP-compatible VHDX creator still
needs to be integrated and tested.

## Logs

Runtime diagnostics are written to:

- LocalState/Logs/OnyxOS.log — application INFO/WARN/ERROR messages.
- LocalState/Logs/QEMU.log — QEMU host and guest startup diagnostics.

Debug output is also sent to the Visual Studio/Win32 debug output stream.

## TCG JIT

QEMU TCG performs software CPU translation using its JIT translation-block
cache. The default is 128 MB, configurable between 16 and 256 MB through the
Windows 11 VM profile. It is not hardware virtualization and does not mean
that Windows 11 has that amount of memory.
