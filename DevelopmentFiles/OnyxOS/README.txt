OnyxOS Xbox Dev Mode storage layout

Place the extracted VHDX files in:
C:\DevelopmentFiles\OnyxOS\VHDX\

Expected filenames:
- Windows10.vhdx
- Windows11.vhdx
- Debian.vhdx
- Ubuntu.vhdx
- ArchLinux.vhdx
- MacOS.vhdx

The app copies a manually selected VHDX into this folder when Xbox Dev Mode permits the copy.
Installer ISO images selected in the app are copied to:
C:\DevelopmentFiles\OnyxOS\ISO\

Installation confirmation markers are stored in:
C:\DevelopmentFiles\OnyxOS\State\

The VHDX files are not copied into LocalState. The app checks the VHDX container signatures before passing a disk path to QEMU.
A first-time install boots from the selected ISO. After the guest installer has completed and the VM is shut down, the user must confirm "Instalação concluída". The marker then allows future boots directly from the VHDX without the installer ISO.

Important:
- The ISO must be a readable ISO9660 image.
- A valid VHDX header does not prove that an operating system is installed inside the disk.
- The installer progress bar is indeterminate because the current QEMU host interface does not report real guest installation percentage.
- Xbox Dev Mode must grant the app access to C:\DevelopmentFiles. If the copy or marker write is denied, the app reports the failure instead of claiming the files were moved.
