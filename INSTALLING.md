# Installing OnyxOS

This guide explains how to install the current OnyxOS package on a Windows PC or an Xbox in Developer Mode.

> **Current status:** OnyxOS is an early UWP scaffold. The package definition contains seven app entries, but the entries still use the same placeholder host. The Launcher, QEMU integration, guest boot flow, audio, controller support, and shader pipeline are not complete. Installing the package does **not** mean that Windows, Linux, or macOS guests can boot yet.

## 1. Requirements

- A supported Windows 10/11 PC for building, with Visual Studio 2022 (or compatible Visual Studio), the **Desktop development with C++** workload, CMake, and a Windows 10.0.19041.0-or-newer SDK.
- Git and access to this repository.
- For Xbox installation: an Xbox console with **Developer Mode** enabled and access to its Device Portal.
- The package signing certificate must be trusted by the target device. Its subject must match the Publisher value in Package.appxmanifest: `CN=Bluey683`.

## 2. Get the signed APPX package

1. Open the repository's **Actions** tab.
2. Select **OnyxOS Xbox UWP Build**.
3. Run the workflow, or open a completed run for the branch you want.
4. Download the artifact named **OnyxOS-Xbox-UWP-x64-Signed**.
5. Extract it. It contains:
   - `OnyxOS-x64.appx` — signed application package.
   - `OnyxOS-signing.cer` — public certificate needed to trust this package signer.

The workflow creates a temporary self-signed development certificate, signs the APPX, verifies the signature, and publishes the public certificate beside the package. A new certificate is generated on each workflow run, so always use the .cer from the **same artifact** as the APPX. The private PFX is not published.

## 3. Trust the development certificate

Only trust a certificate from a workflow run and repository you control.

On a Windows development PC, import `OnyxOS-signing.cer` into the **Local Computer** certificate store under **Trusted People** (or the trust store required by your managed development environment). Then deploy the signed APPX.

On Xbox, use the certificate installation/trust mechanism provided by the console's Developer Mode and Device Portal. If the console does not expose a certificate trust/import option, do not assume that uploading the .cer as an app installs it; consult the current Xbox Developer Mode deployment instructions for that console version.

The certificate subject is `CN=Bluey683`, matching the package Publisher. Trusting the certificate is separate from installing the APPX.

## 4. Install on Windows

1. Trust the matching `OnyxOS-signing.cer` as described above.
2. Open PowerShell in the folder containing the extracted files.
3. Install the package:

   ```powershell
   Add-AppxPackage -Path ".\\OnyxOS-x64.appx"
   ```

4. If Windows reports missing framework dependencies, install the matching dependencies built for the same architecture and target SDK, then retry.
5. Find OnyxOS in the Start menu.

## 5. Install on Xbox (Developer Mode)

1. Enable Developer Mode on the Xbox and restart into Developer Mode.
2. Open the Xbox Developer Home app and enable the Device Portal.
3. From a PC on the same network, open the Device Portal address shown by the console.
4. Trust/import the matching `OnyxOS-signing.cer` using the certificate mechanism supported by that Xbox Developer Mode version.
5. Open the Device Portal Apps section and upload `OnyxOS-x64.appx`.
6. If the portal requests dependencies, provide the matching dependency packages.
7. After installation, launch OnyxOS from the console's app list.

The exact Device Portal labels and certificate workflow can vary by console system version. Keep the Xbox and PC on the same network during deployment.

## 6. Local signing

For a local build, the APPX produced directly by `Build.ps1` is unsigned. Create or select a development certificate with subject `CN=Bluey683`, sign the package with the Windows SDK SignTool, and trust its public certificate on the target device. Keep the private key secure.

## 7. ISO and virtual disk storage

The planned guest-media layout inside the app's private LocalState storage is:

```text
LocalState/
├── ISO/
├── VHDX/
├── Configurations/
└── Logs/
```

These folders are intended for guest installation media, virtual disks, VM configurations, and logs. The current scaffold does not yet provide a completed in-app file picker or QEMU boot implementation, so placing an ISO there will not start a virtual machine.

## 8. Troubleshooting

| Problem | What to check |
|---|---|
| Package publisher or signature error | Use the .cer from the same Actions artifact as the APPX, and confirm its subject is CN=Bluey683. |
| Certificate is not trusted | Import/trust the public .cer on the target device using its supported development workflow. |
| Missing dependency | Install the required framework packages for the package architecture and target SDK. |
| Package architecture error | Use the x64 package only on a supported x64 target. |
| App installs but does not boot an OS | Expected in this scaffold: guest boot and QEMU integration are not implemented yet. |
| App does not appear after deployment | Check the Device Portal deployment result and the console's installed-app list. |

## 9. Current limitations

- The seven manifest entries are packaged together in one APPX, but currently launch the same placeholder host implementation.
- No guest OS has been validated as booting on Xbox.
- HLSL and GLSL files are source examples; they are not yet compiled or connected to a guest framebuffer.
- Audio, Wi-Fi networking, keyboard/mouse/controller input, and VM lifecycle management remain planned work.

For build instructions, see [BUILDING.md](BUILDING.md). For the package layout, see [docs/APPX-STRUCTURE.md](docs/APPX-STRUCTURE.md).
