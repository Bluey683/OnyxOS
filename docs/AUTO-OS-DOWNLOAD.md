# Automatic OS downloads, TCG and VHDX

## Installer ISO fallback
When a local ISO is missing, the catalog can download official Debian, Ubuntu
and Arch Linux x86_64 installer images into LocalState/ISO using UWP
BackgroundTransfer. Windows 10/11 open Microsoft's official download pages
because Microsoft generates expiring edition/language-specific links; OnyxOS
does not save a web page as an ISO. macOS remains disabled and has no download.

## VHDX fallback
The intended flow is: reuse an existing VHDX; otherwise download an official
installer ISO and create a new VHDX. The creation step is not implemented yet.
The upstream QEMU UWP host API provides VM lifecycle and device integration,
but no qemu-img or VHDX-creation export. Desktop-only Virtual Disk Service APIs
cannot be assumed to work in the UWP app container.

VhdxProvisioner currently fails explicitly instead of creating a zero-byte fake
disk. A UWP-compatible VHDX creator or a matching packaged qemu-img build must
be added and tested before automatic disk creation can be enabled.

## TCG
QEMU uses `-accel tcg,thread=multi`: multi-threaded software CPU emulation,
not hardware virtualization.
