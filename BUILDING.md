# Building OnyxOS

This guide describes the current Windows x64 UWP packaging workflow and its implementation limits.

## What the build produces

The intended output is **one unsigned APPX package**: `OnyxOS-x64.appx`.

The manifest declares seven application entries inside that one package:
1. OnyxOS Launcher
2. OnyxOS - Windows 10
3. OnyxOS - Windows 11
4. OnyxOS - Debian
5. OnyxOS - Ubuntu
6. OnyxOS - Arch Linux
7. OnyxOS - macOS Experimental

These are seven launch entries, not seven separate APPX installers. All seven currently use the shared UWP host scaffold. The OS-specific QEMU boot flow and finished Launcher UI are not implemented yet.

## Build with GitHub Actions

1. Open the [OnyxOS repository](https://github.com/Bluey683/OnyxOS).
2. Select **Actions**.
3. Open **OnyxOS Xbox UWP Build**.
4. Select **Run workflow** on the `main` branch.
5. Wait for the workflow to finish.
6. Open the completed run and download the artifact named `OnyxOS-Xbox-UWP-x64`.
7. Extract the artifact to obtain `OnyxOS-x64.appx`.

Pushes and pull requests targeting `main` also trigger the workflow. A successful workflow run is required before treating the APPX as a usable build; this workflow has not yet been confirmed by a successful run.

## Build with Build.ps1

The root `Build.ps1` script performs asset generation, CMake configuration, compilation, and APPX packaging in one command.

Run in a Developer PowerShell for Visual Studio 2022:

```powershell
.\\Build.ps1 -Mode PC
```

For a clean rebuild:

```powershell
.\\Build.ps1 -Mode PC -Clean
```

GitHub Actions uses the same script with `-Mode GitHubActions`. The mode labels identify the build environment; both currently produce the same unsigned x64 APPX. You can override the SDK version with `-WindowsSdkVersion` if a different compatible SDK is installed.

## Build locally on Windows

### Requirements

- Windows 10/11 x64.
- Visual Studio 2022 with **Desktop development with C++**.
- Windows 10/11 SDK, including the x64 Windows SDK tools.
- CMake 3.24 or newer.
- MSVC x64 build tools.

### Configure and compile

Run from a Developer PowerShell for Visual Studio 2022:

```powershell
.\scripts\Make-Assets.ps1

cmake -S . -B build `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_SYSTEM_NAME=WindowsStore `
  -DCMAKE_SYSTEM_VERSION=10.0 `
  -DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION=10.0.26100.0

cmake --build build --config Release --parallel
```

If SDK 10.0.26100.0 is not installed, install it or update the requested SDK version to one installed on the machine and supported by the project.

### Package the APPX

Run this from PowerShell after the build:

```powershell
$sdkRoot = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
$makeAppx = Get-ChildItem $sdkRoot -Filter MakeAppx.exe -Recurse |
  Where-Object { $_.FullName -match '\\x64\\MakeAppx.exe$' } |
  Sort-Object FullName -Descending |
  Select-Object -First 1

if (-not $makeAppx) {
  throw "MakeAppx.exe was not found. Install the Windows SDK."
}

$stage = Join-Path $env:TEMP "OnyxOS-Package"
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item Package.appxmanifest (Join-Path $stage AppxManifest.xml)

$executables = @(
  "OnyxOS-Launcher.exe",
  "OnyxOS-Windows10.exe",
  "OnyxOS-Windows11.exe",
  "OnyxOS-Debian.exe",
  "OnyxOS-Ubuntu.exe",
  "OnyxOS-ArchLinux.exe",
  "OnyxOS-macOS.exe"
)

foreach ($name in $executables) {
  $exe = Get-ChildItem build -Recurse -Filter $name -File |
    Select-Object -First 1
  if (-not $exe) { throw "Missing build output: $name" }
  Copy-Item $exe.FullName $stage
}

Get-ChildItem build -Recurse -Filter "*.dll" -File |
  ForEach-Object { Copy-Item $_.FullName $stage -Force }

Copy-Item Assets (Join-Path $stage Assets) -Recurse -Force
New-Item -ItemType Directory -Force -Path artifacts | Out-Null

& $makeAppx.FullName pack /d $stage /p "artifacts/OnyxOS-x64.appx" /o
if ($LASTEXITCODE -ne 0) {
  throw "MakeAppx failed with exit code $LASTEXITCODE"
}
```

These commands mirror the repository workflow. Validate the generated package with the target Windows SDK before deployment.

## Signing and Xbox installation

The current workflow creates an **unsigned** APPX. It is not ready for normal installation until it is signed.

Use a certificate you control and trust on the target device. Its subject must match the manifest Publisher value: `CN=Bluey683`. For Xbox, enable Developer Mode and use the supported device portal/deployment process. Package identity and certificate trust must be configured correctly.

## VM storage

The intended writable app storage layout is:

```text
LocalState/
  ISO/
  VHDX/
  Configurations/
  Logs/
```

Guest ISO files are supplied by the user. The project does not bundle Windows, Linux, or macOS installation media.

## Graphics shaders

- HLSL: `graphics/shaders/onyxos.hlsl`
- GLSL vertex: `graphics/shaders/onyxos.vert`
- GLSL fragment: `graphics/shaders/onyxos.frag`

These are starter shader sources; they are not yet compiled or connected to QEMU video output. HLSL/Direct3D is the intended initial Xbox rendering path. GLSL requires a separate supported OpenGL/EGL renderer.

## Current limitations

- The seven entries currently use the shared UWP host scaffold.
- Launcher UI, per-entry QEMU routing, QEMU DLL integration, ISO picker, VHDX creation/attachment, guest boot, and save/resume still need implementation.
- Windows 11 may need virtual TPM and Secure Boot support.
- macOS is an experimental placeholder; compatibility, firmware, and licensing requirements must be resolved.
- QEMU software emulation may be slow. GPU passthrough and hardware virtualization are not promised.
- A successful CMake build and APPX packaging do not prove that any guest OS boots on Xbox.

See [docs/IMPLEMENTATION-STATUS.md](docs/IMPLEMENTATION-STATUS.md), [docs/OS-PROFILES.md](docs/OS-PROFILES.md), and [docs/APPX-STRUCTURE.md](docs/APPX-STRUCTURE.md).
