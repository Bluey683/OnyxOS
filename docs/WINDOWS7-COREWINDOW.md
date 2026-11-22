# Windows 7 x64 / CoreWindow implementation plan

This branch establishes the Windows 7 guest profile and the native CoreWindow graphics path. The following work remains before it can be considered a working Xbox build.

## Implemented in this branch

- Windows 7 x64 VM profile: QEMU `pc` machine, SeaBIOS path, `-cpu max`, 2560 MB RAM, QCOW2 disk, IDE disk device, standard VGA, USB 1.1 controller, no guest audio device.
- QEMU argument builder supports `qcow2`, `vhdx`, and `raw` disk formats and omits all audio devices when the profile disables audio.
- App-side downloader for the supplied Archive.org Windows 7 QCOW2 URL.
- Local QCOW2 import helper; both download and import validate the `QFI\xfb` QCOW2 magic before returning the file.
- Native CoreWindow + DXGI + Direct3D 11 renderer and a full-screen HLSL triangle.
- Full-screen shader UV mapping corrected in the existing renderer to avoid sampling outside the guest texture.

## Still required

1. Replace the XAML-generated app entry point and XAML pages with a native `IFrameworkViewSource` / `IFrameworkView` CoreWindow application.
2. Connect `CoreWindowD3DRenderer::Present` to the QEMU video-frame callback and handle CoreWindow size/DPI changes.
3. Connect the image download and local-file picker flows to persistent `ApplicationData::Current->LocalFolder` storage.
4. Automatically start the Windows 7 VM when a valid QCOW2 disk is present; otherwise expose only the disk-source choice during first setup.
5. Add a controller-driven on-screen keyboard and complete physical keyboard/mouse input forwarding.
6. Validate Xbox memory availability for the 2560 MB guest plus QEMU and D3D allocations.
7. Build, install, and test the APPX on Xbox Series X|S; GitHub Actions compilation alone does not prove guest boot compatibility.

## Guest disk source

The default download URL is the exact user-provided Archive.org URL:

`https://archive.org/download/windows-image-collection/Windows%207.qcow2`

GitHub Actions does not download or publish the Windows image. The app downloads it at runtime or imports a user-selected local QCOW2 file. The app stores the resulting disk in its private persistent storage. Verify image provenance and applicable licensing before use.

## Important CoreWindow distinction

The existing app still contains XAML files and its old XAML application entry point. The new CoreWindow renderer is a separate compiled component; this branch does **not** yet claim that the XAML-free application conversion is complete. The app entry point must be migrated before the final no-XAML requirement is satisfied.
