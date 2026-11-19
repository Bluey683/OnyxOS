# Installing OnyxOS

This guide explains how to install the current OnyxOS package on a Windows PC or an Xbox in Developer Mode.

> **Current status:** OnyxOS is an early UWP scaffold. The package definition contains seven app entries, but the entries still use the same placeholder host. The Launcher, QEMU integration, guest boot flow, audio, controller support, and shader pipeline are not complete. Installing the package does **not** mean that Windows, Linux, or macOS guests can boot yet.

## 1. Requirements

- A supported Windows 10/11 PC for building, with Visual Studio 2022 (or compatible Visual Studio), the **Desktop development with C++** workload, CMake, and a Windows 10.0.19041.0-or-newer SDK.
- Git and access to this repository.
- For Xbox installation: an Xbox console with **Developer Mode** enabled and access to its Device Portal.
- A package signing certificate trusted by the target device. The certificate's subject must match the Publisher value in Package.appxmanifest (currently CN=Bluey683).

## 2. Get the APPX package

There are two ways to obtain a package:

### Option A — GitHub Actions

1. Open the repository's **Actions** tab.
2. Select the **Build OnyxOS Xbox UWP** workflow.
3. Run the workflow, or open a completed run for the branch you want.
4. Download the artifact named OnyxOS-Xbox-UWP-x64.
5. Extract the downloaded artifact and locate OnyxOS-x64.appx.

The workflow artifact is an unsigned build. It may need to be signed before Windows or Xbox will accept it.

### Option B — Build locally

Follow [BUILDING.md](BUILDING.md) to configure CMake, compile the UWP targets, and package the APPX with the Windows SDK packaging tools.

## 3. Sign the package

The package manifest declares:

- Package identity: Bluey683.OnyxOS
- Publisher: CN=Bluey683

Use a signing certificate whose subject exactly matches that Publisher. Sign the APPX with the Windows SDK SignTool, then install or trust the matching certificate on the target device as required by its deployment flow.

Do not install a certificate from an untrusted source. For development, create and manage your own test certificate and keep its private key secure. A certificate with a different Publisher will not match this package identity.

## 4. Install on Windows

1. Make sure the signing certificate is trusted for development on the PC.
2. Open PowerShell in the folder containing the signed APPX.
3. Install it with:

   ```powershell
   Add-AppxPackage -Path ".\OnyxOS-x64.appx"
   ```

4. If Windows reports missing framework dependencies, install the matching dependencies built for the same architecture and SDK, then retry.
5. Find OnyxOS in the Start menu.

## 5. Install on Xbox (Developer Mode)

1. Enable Developer Mode on the Xbox and restart into Developer Mode.
2. Open the Xbox Developer Home app and enable the Device Portal.
3. From a PC on the same network, open the Device Portal address shown by the console.
4. Open the Apps section and use its app installation option to upload the signed .appx package.
5. If the portal requests dependencies, provide the matching dependency packages. Install or trust the development signing certificate using the supported Developer Mode workflow.
6. After installation, launch OnyxOS from the console's app list.

The exact Device Portal labels can vary by console system version. Keep the Xbox and PC on the same network during deployment.

## 6. ISO and virtual disk storage

The planned guest-media layout inside the app's private LocalState storage is:

```text
LocalState/
├── ISO/
├── VHDX/
├── Configurations/
└── Logs/
```

These folders are intended for guest installation media, virtual disks, VM configurations, and logs. The current scaffold does not yet provide a completed in-app file picker or QEMU boot implementation, so placing an ISO there will not start a virtual machine.

## 7. Troubleshooting

| Problem | What to check |
|---|---|
| Package publisher or signature error | Confirm the signing certificate subject matches CN=Bluey683 and that the certificate is trusted on the device. |
| Missing dependency | Install the required framework packages for the package architecture and target SDK. |
| Package architecture error | Use the x64 package only on a supported x64 target. |
| App installs but does not boot an OS | Expected in this scaffold: guest boot and QEMU integration are not implemented yet. |
| App does not appear after deployment | Check the Device Portal deployment result and the console's installed-app list. |

## 8. Current limitations

- The seven manifest entries are packaged together in one APPX, but currently launch the same placeholder host implementation.
- The QEMU host DLL and its runtime dependencies are not yet integrated.
- No guest OS has been validated as booting on Xbox.
- HLSL and GLSL files are source examples; they are not yet compiled or connected to a guest framebuffer.
- Audio, Wi-Fi networking, keyboard/mouse/controller input, and VM lifecycle management remain planned work.

For build instructions, see [BUILDING.md](BUILDING.md). For the package layout, see [docs/APPX-STRUCTURE.md](docs/APPX-STRUCTURE.md).
