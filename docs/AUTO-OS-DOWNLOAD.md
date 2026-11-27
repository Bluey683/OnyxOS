# Windows 11 ISO, QCOW2 and TCG/JIT

## Windows 11 installer ISO

OnyxOS accepts the official Windows 11 x64 ISO selected by the user. Microsoft
requires edition/language selection and uses time-limited download links, so
OnyxOS opens the official
[Windows 11 download page](https://www.microsoft.com/software-download/windows11)
instead of pretending that the page itself is an ISO file.

Save any Windows 11 installer ISO with its original filename in:

    LocalState/Windows11/ISO/

OnyxOS scans that folder and selects the first valid `*.iso` file. It does not require a fixed filename.

The storage provisioner scans the ISO folder, checks each candidate for an ISO9660 CD001 volume descriptor, and returns the first valid ISO path to the VM configuration.

## Windows 11 virtual disk

The persistent guest disk path is:

    LocalState/Windows11/Disks/Windows11-22H2.qcow2

QCOW2 images are validated using their QFI\xfb magic header before use.
Automatic QCOW2 creation is not implemented yet; the app must not create an
empty file and misrepresent it as a bootable disk. A UWP-compatible QCOW2
creator or QEMU image-creation path still needs to be integrated and tested.

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
