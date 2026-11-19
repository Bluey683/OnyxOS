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
    param([Parameter(Mandatory=$true)][string]$FilePath,[Parameter(Mandatory=$true)][string[]]$Arguments)
    Write-Host "> $FilePath $($Arguments -join ' ')" -ForegroundColor Cyan
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Command failed with exit code $LASTEXITCODE : $FilePath" }
}

$repoRoot = $PSScriptRoot
$buildPath = Join-Path $repoRoot $BuildDirectory
$outputPath = Join-Path $repoRoot $OutputDirectory
$projectPath = Join-Path $repoRoot 'OnyxOS.vcxproj'
$manifestPath = Join-Path $repoRoot 'Package.appxmanifest'
$assetsPath = Join-Path $repoRoot 'Assets'

if ($env:OS -ne 'Windows_NT') { throw 'Build.ps1 must run on Windows with Visual Studio C++ and the Windows SDK installed.' }
if (-not (Test-Path $projectPath)) { throw 'OnyxOS.vcxproj was not found.' }
if (-not (Test-Path $manifestPath)) { throw 'Package.appxmanifest was not found.' }

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path $vswhere)) { throw "Visual Studio Installer vswhere.exe was not found: $vswhere" }
$msbuild = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild/**/Bin/MSBuild.exe' | Select-Object -First 1
if (-not $msbuild -or -not (Test-Path $msbuild)) { throw 'MSBuild was not found. Install Visual Studio 2022 with the Desktop development with C++ workload.' }

if (-not $SkipQemuDownload) {
    $qemuFetchScript = Join-Path $repoRoot 'scripts/Fetch-QemuRuntime.ps1'
    if (-not (Test-Path $qemuFetchScript)) { throw 'scripts/Fetch-QemuRuntime.ps1 was not found.' }
    Write-Host 'Fetching QEMU UWP host DLL and matching runtime dependencies...' -ForegroundColor Yellow
    & $qemuFetchScript -RepositoryRoot $repoRoot
}
if ($Clean) {
    if (Test-Path $buildPath) { Remove-Item $buildPath -Recurse -Force }
    if (Test-Path $outputPath) { Remove-Item $outputPath -Recurse -Force }
}
New-Item -ItemType Directory -Force -Path $buildPath,$outputPath | Out-Null

if (-not $SkipAssets) {
    $assetScript = Join-Path $repoRoot 'scripts/Make-Assets.ps1'
    if (-not (Test-Path $assetScript)) { throw 'scripts/Make-Assets.ps1 was not found.' }
    Write-Host 'Generating APPX image assets...' -ForegroundColor Yellow
    & $assetScript
}
if (-not (Test-Path $assetsPath)) { throw 'Assets directory is missing. Run scripts/Make-Assets.ps1.' }

Write-Host "Building and packaging OnyxOS with native UWP MSBuild ($Architecture / $Configuration)..." -ForegroundColor Yellow
$packageDirectory = Join-Path $outputPath 'MSBuildPackage'
New-Item -ItemType Directory -Force -Path $packageDirectory | Out-Null

Invoke-CheckedCommand -FilePath $msbuild -Arguments @(
    $projectPath,'/m','/t:Rebuild',"/p:Configuration=$Configuration","/p:Platform=$Architecture",
    "/p:WindowsTargetPlatformVersion=$WindowsSdkVersion",
    "/p:OutDir=$(Join-Path $buildPath "$Architecture/$Configuration/")",
    "/p:IntDir=$(Join-Path $buildPath "obj/$Architecture/$Configuration/")",
    '/p:AppxPackage=true',
    '/p:GenerateAppxPackageOnBuild=true',
    '/p:AppxPackageSigningEnabled=false',
    '/p:AppxBundle=Never',
    "/p:AppxPackageDir=$packageDirectory\\"
)

# Use the APPX emitted by the official UWP packaging targets. Do not stage
# files manually: MSBuild must generate the XBF files, PRI index, manifest
# metadata, and package layout as one consistent deployment unit.
$generatedAppx = Get-ChildItem -Path $packageDirectory -File -Recurse |
    Where-Object { $_.Extension -in @('.appx', '.msix') } |
    Where-Object { $_.Name -notmatch 'VCLibs|Dependencies' } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if (-not $generatedAppx) {
    Get-ChildItem -Path $buildPath -File -Recurse | Where-Object { $_.Extension -in @('.appx', '.msix') } | ForEach-Object {
        Write-Host "Found package candidate: $($_.FullName)"
    }
    throw "Native MSBuild UWP packaging did not produce an APPX/MSIX in $packageDirectory."
}
$appxPath = Join-Path $outputPath 'OnyxOS-x64.appx'
Copy-Item $generatedAppx.FullName $appxPath -Force
Write-Host "Native UWP package copied to: $appxPath" -ForegroundColor Green

# Do not publish a launcher-only APPX: verify the QEMU host and every runtime
# dependency are physically present in the final package archive.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($appxPath)
try {
    $entries = @{}
    foreach ($entry in $zip.Entries) { $entries[$entry.FullName.Replace('/', '\\')] = $entry.Length }
    $qemuEntry = $entries.Keys | Where-Object { $_ -eq 'qemu-system-x86_64.dll' -or $_ -match '\\qemu-system-x86_64\.dll

# Prefer the Microsoft VCLibs package installed with the Windows SDK.
# This avoids relying on aka.ms redirects returning an HTML/error page in CI.
$vclibsPath = Join-Path $outputPath 'Microsoft.VCLibs.x64.14.00.appx'
$programFilesX86 = [Environment]::GetFolderPath('ProgramFilesX86')
$vclibsCandidates = @(
    (Join-Path $programFilesX86 'Microsoft SDKs/Windows Kits/10/ExtensionSDKs/Microsoft.VCLibs/14.0/AppX/Retail/x64/Microsoft.VCLibs.x64.14.00.appx'),
    (Join-Path $env:WindowsSdkDir 'ExtensionSDKs/Microsoft.VCLibs/14.0/AppX/Retail/x64/Microsoft.VCLibs.x64.14.00.appx')
) | Where-Object { $_ -and (Test-Path $_) }
$vclibsSource = $vclibsCandidates | Select-Object -First 1

if ($vclibsSource) {
    Copy-Item -LiteralPath $vclibsSource -Destination $vclibsPath -Force
    Write-Host "Using SDK-provided Microsoft VCLibs x64 package: $vclibsSource" -ForegroundColor Green
}
else {
    $vclibsUri = 'https://aka.ms/Microsoft.VCLibs.x64.14.00.appx'
    Write-Host 'SDK VCLibs package not found; downloading Microsoft VCLibs x64 framework...' -ForegroundColor Yellow
    Invoke-WebRequest -Uri $vclibsUri -OutFile $vclibsPath -UseBasicParsing
}
if (-not (Test-Path $vclibsPath) -or (Get-Item $vclibsPath).Length -lt 100000) {
    throw 'Microsoft VCLibs x64 framework package is missing or invalid.'
}
Write-Host "VCLibs dependency: $vclibsPath" -ForegroundColor Green

Write-Host ''
Write-Host 'OnyxOS APPX build completed.' -ForegroundColor Green
Write-Host "Package: $appxPath"
Write-Warning 'The shared UWP launcher scaffold is packaged in all seven entries. Per-OS QEMU boot, guest display, and VHDX creation still require implementation.'
 } | Select-Object -First 1
    if (-not $qemuEntry -or $entries[$qemuEntry] -lt 20000000) {
        throw "QEMU DLL is missing or incomplete inside APPX. Package: $appxPath"
    }
    $runtimeFiles = @(Get-ChildItem -LiteralPath (Join-Path $repoRoot 'Dependencies/Runtime') -Filter '*.dll' -File)
    if ($runtimeFiles.Count -lt 20) { throw "Expected QEMU runtime DLLs were not staged; found $($runtimeFiles.Count)." }
    $missingRuntime = @()
    foreach ($runtimeFile in $runtimeFiles) {
        $expected = $runtimeFile.Name
        $found = $entries.Keys | Where-Object { $_ -eq $expected -or $_ -match ("\\{0}$" -f [regex]::Escape($expected)) } | Select-Object -First 1
        if (-not $found -or $entries[$found] -ne $runtimeFile.Length) { $missingRuntime += $expected }
    }
    if ($missingRuntime.Count -gt 0) {
        throw ("APPX is missing QEMU runtime dependencies: " + ($missingRuntime -join ', '))
    }
    Write-Host "Verified QEMU DLL ($($entries[$qemuEntry]) bytes) and $($runtimeFiles.Count) runtime DLLs inside APPX." -ForegroundColor Green
}
finally { $zip.Dispose() }

# Prefer the Microsoft VCLibs package installed with the Windows SDK.
# This avoids relying on aka.ms redirects returning an HTML/error page in CI.
$vclibsPath = Join-Path $outputPath 'Microsoft.VCLibs.x64.14.00.appx'
$programFilesX86 = [Environment]::GetFolderPath('ProgramFilesX86')
$vclibsCandidates = @(
    (Join-Path $programFilesX86 'Microsoft SDKs/Windows Kits/10/ExtensionSDKs/Microsoft.VCLibs/14.0/AppX/Retail/x64/Microsoft.VCLibs.x64.14.00.appx'),
    (Join-Path $env:WindowsSdkDir 'ExtensionSDKs/Microsoft.VCLibs/14.0/AppX/Retail/x64/Microsoft.VCLibs.x64.14.00.appx')
) | Where-Object { $_ -and (Test-Path $_) }
$vclibsSource = $vclibsCandidates | Select-Object -First 1

if ($vclibsSource) {
    Copy-Item -LiteralPath $vclibsSource -Destination $vclibsPath -Force
    Write-Host "Using SDK-provided Microsoft VCLibs x64 package: $vclibsSource" -ForegroundColor Green
}
else {
    $vclibsUri = 'https://aka.ms/Microsoft.VCLibs.x64.14.00.appx'
    Write-Host 'SDK VCLibs package not found; downloading Microsoft VCLibs x64 framework...' -ForegroundColor Yellow
    Invoke-WebRequest -Uri $vclibsUri -OutFile $vclibsPath -UseBasicParsing
}
if (-not (Test-Path $vclibsPath) -or (Get-Item $vclibsPath).Length -lt 100000) {
    throw 'Microsoft VCLibs x64 framework package is missing or invalid.'
}
Write-Host "VCLibs dependency: $vclibsPath" -ForegroundColor Green

Write-Host ''
Write-Host 'OnyxOS APPX build completed.' -ForegroundColor Green
Write-Host "Package: $appxPath"
Write-Warning 'The shared UWP launcher scaffold is packaged in all seven entries. Per-OS QEMU boot, guest display, and VHDX creation still require implementation.'
