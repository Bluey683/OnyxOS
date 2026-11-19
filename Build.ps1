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

    [switch]$SkipAssets
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
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE : $FilePath"
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
    & $assetScript
    if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
        throw 'Asset generation failed.'
    }
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

Write-Host 'Building seven app-entry host executables...' -ForegroundColor Yellow
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

# Stage any runtime DLLs emitted by the build. QEMU itself is not bundled yet.
Get-ChildItem -Path $buildPath -Recurse -Filter '*.dll' -File |
    Where-Object { $_.FullName -match "[\\/]$Configuration[\\/]" } |
    ForEach-Object {
        Copy-Item $_.FullName (Join-Path $stagePath $_.Name) -Force
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
Write-Warning 'The current targets still use the placeholder CoreApplication host. LauncherPage.xaml is not wired into the executable or compiled by this build script; QEMU guest boot is not implemented.'
