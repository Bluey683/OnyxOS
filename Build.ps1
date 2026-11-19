#requires -Version 5.1
[CmdletBinding()]
param(
    [ValidateSet('PC', 'GitHubActions')]
    [string]$Mode = 'PC',

    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    [ValidateSet('x64')]
    [string]$Architecture = 'x64',

    [string]$WindowsSdkVersion = '10.0.26100.0',

    [string]$BuildDirectory = 'build',

    [string]$OutputDirectory = 'artifacts',

    [switch]$Clean,

    [switch]$SkipAssets,

    [switch]$SkipQemuDownload
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-CheckedCommand {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [Parameter(Mandatory = $true)][string[]]$Arguments
    )

    Write-Host "> $FilePath $($Arguments -join ' ')" -ForegroundColor Cyan
    & $FilePath @Arguments
    $commandSucceeded = $?
    if (-not $commandSucceeded) {
        throw "Command failed: $FilePath"
    }
}

$repoRoot = $PSScriptRoot
$buildPath = Join-Path $repoRoot $BuildDirectory
$outputPath = Join-Path $repoRoot $OutputDirectory
$manifestPath = Join-Path $repoRoot 'Package.appxmanifest'
$assetsPath = Join-Path $repoRoot 'Assets'

if ($env:OS -ne 'Windows_NT') {
    throw 'Build.ps1 must run on Windows with Visual Studio C++ and the Windows SDK installed.'
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw 'CMake was not found in PATH. Install CMake 3.24 or newer.'
}

if (-not (Test-Path $manifestPath)) {
    throw 'Package.appxmanifest was not found in the repository root.'
}

if (-not $SkipQemuDownload) {
    $qemuFetchScript = Join-Path $repoRoot 'scripts/Fetch-QemuRuntime.ps1'
    if (-not (Test-Path $qemuFetchScript)) {
        throw 'scripts/Fetch-QemuRuntime.ps1 was not found.'
    }
    Write-Host 'Fetching QEMU UWP host DLL and matching runtime dependencies...' -ForegroundColor Yellow
    # This is another PowerShell script, not a native executable.
    # Its terminating errors propagate because ErrorActionPreference is Stop.
    & $qemuFetchScript -RepositoryRoot $repoRoot
}

if ($Clean -and (Test-Path $buildPath)) {
    Remove-Item -Path $buildPath -Recurse -Force
}
if ($Clean -and (Test-Path $outputPath)) {
    Remove-Item -Path $outputPath -Recurse -Force
}

New-Item -ItemType Directory -Force -Path $buildPath, $outputPath | Out-Null

if (-not $SkipAssets) {
    $assetScript = Join-Path $repoRoot 'scripts/Make-Assets.ps1'
    if (-not (Test-Path $assetScript)) {
        throw 'scripts/Make-Assets.ps1 was not found.'
    }
    Write-Host 'Generating APPX image assets...' -ForegroundColor Yellow
    # This is another PowerShell script, not a native executable.
    # Its terminating errors propagate because ErrorActionPreference is Stop.
    & $assetScript
}

if (-not (Test-Path $assetsPath)) {
    throw 'Assets directory is missing. Run scripts/Make-Assets.ps1.'
}

Write-Host "Configuring OnyxOS ($Mode / $Architecture / $Configuration)..." -ForegroundColor Yellow
$configureArgs = @(
    '-S', $repoRoot,
    '-B', $buildPath,
    '-G', 'Visual Studio 17 2022',
    '-A', $Architecture,
    '-DCMAKE_SYSTEM_NAME=WindowsStore',
    '-DCMAKE_SYSTEM_VERSION=10.0',
    "-DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION=$WindowsSdkVersion"
)
Invoke-CheckedCommand -FilePath 'cmake' -Arguments $configureArgs

Write-Host 'Building seven UWP XAML app-entry executables...' -ForegroundColor Yellow
Invoke-CheckedCommand -FilePath 'cmake' -Arguments @(
    '--build', $buildPath,
    '--config', $Configuration,
    '--parallel'
)

$sdkBinRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/bin'
if (-not (Test-Path $sdkBinRoot)) {
    throw "Windows SDK tools directory was not found: $sdkBinRoot"
}

$makeAppx = Get-ChildItem -Path $sdkBinRoot -Filter 'MakeAppx.exe' -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match '[\\/]x64[\\/]MakeAppx.exe$' } |
    Sort-Object FullName -Descending |
    Select-Object -First 1

if (-not $makeAppx) {
    throw 'MakeAppx.exe was not found. Install the Windows 10/11 SDK with packaging tools.'
}

$stagePath = Join-Path $env:TEMP 'OnyxOS-Package'
if (Test-Path $stagePath) {
    Remove-Item -Path $stagePath -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stagePath | Out-Null

Copy-Item $manifestPath (Join-Path $stagePath 'AppxManifest.xml')
Copy-Item $assetsPath (Join-Path $stagePath 'Assets') -Recurse -Force

$executableNames = @(
    'OnyxOS-Launcher.exe',
    'OnyxOS-Windows10.exe',
    'OnyxOS-Windows11.exe',
    'OnyxOS-Debian.exe',
    'OnyxOS-Ubuntu.exe',
    'OnyxOS-ArchLinux.exe',
    'OnyxOS-macOS.exe'
)

foreach ($executableName in $executableNames) {
    $builtExecutable = Get-ChildItem -Path $buildPath -Recurse -Filter $executableName -File |
        Where-Object { $_.FullName -match "[\\/]$Configuration[\\/]" } |
        Select-Object -First 1

    if (-not $builtExecutable) {
        throw "Build output not found for $executableName ($Configuration)."
    }

    Copy-Item $builtExecutable.FullName (Join-Path $stagePath $executableName) -Force
}

# Stage build runtime DLLs.
Get-ChildItem -Path $buildPath -Recurse -Filter '*.dll' -File |
    Where-Object { $_.FullName -match "[\\/]$Configuration[\\/]" } |
    ForEach-Object {
        Copy-Item $_.FullName (Join-Path $stagePath $_.Name) -Force
    }

# Stage the QEMU UWP host DLL and its matching runtime/firmware payload when supplied.
$qemuDll = Join-Path $repoRoot 'Dependencies/Qemu/qemu-system-x86_64.dll'
if (Test-Path $qemuDll) {
    Copy-Item $qemuDll (Join-Path $stagePath 'qemu-system-x86_64.dll') -Force
    Write-Host 'Bundled QEMU x86_64 UWP host DLL.' -ForegroundColor Green
} else {
    Write-Warning 'Dependencies/Qemu/qemu-system-x86_64.dll is missing. The APPX will build without QEMU; VM startup will not be available.'
}

$runtimePath = Join-Path $repoRoot 'Dependencies/Runtime'
if (Test-Path $runtimePath) {
    Get-ChildItem -Path $runtimePath -Filter '*.dll' -File | ForEach-Object {
        Copy-Item $_.FullName (Join-Path $stagePath $_.Name) -Force
    }
}

$firmwarePath = Join-Path $repoRoot 'Dependencies/qemu'
if (Test-Path $firmwarePath) {
    Copy-Item $firmwarePath (Join-Path $stagePath 'qemu') -Recurse -Force
}

$appxPath = Join-Path $outputPath 'OnyxOS-x64.appx'
if (Test-Path $appxPath) {
    Remove-Item $appxPath -Force
}

Write-Host 'Packaging one APPX containing all seven application entries...' -ForegroundColor Yellow
Invoke-CheckedCommand -FilePath $makeAppx.FullName -Arguments @(
    'pack',
    '/d', $stagePath,
    '/p', $appxPath,
    '/o'
)

if (-not (Test-Path $appxPath)) {
    throw "APPX output was not created: $appxPath"
}

Write-Host ''
Write-Host 'OnyxOS APPX build completed.' -ForegroundColor Green
Write-Host "Package: $appxPath"
Write-Host 'Package is unsigned. Sign it with a trusted development certificate whose subject matches CN=Bluey683.'
Write-Warning 'The launcher interface is created with native UWP XAML controls in src/App.cpp. The standalone LauncherPage.xaml and GuestPage.xaml files are not compiled by this CMake build. QEMU guest boot and the guest display lifecycle are not implemented yet.'
