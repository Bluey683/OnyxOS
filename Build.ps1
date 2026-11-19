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

Write-Host "Building OnyxOS with Visual Studio MSBuild ($Architecture / $Configuration)..." -ForegroundColor Yellow
Invoke-CheckedCommand -FilePath $msbuild -Arguments @(
    $projectPath,'/m','/t:Build',"/p:Configuration=$Configuration","/p:Platform=$Architecture",
    "/p:WindowsTargetPlatformVersion=$WindowsSdkVersion","/p:OutDir=$(Join-Path $buildPath "$Architecture/$Configuration/")",
    "/p:IntDir=$(Join-Path $buildPath "obj/$Architecture/$Configuration/")"
)

$builtExecutable = Join-Path $buildPath "$Architecture/$Configuration/OnyxOS/OnyxOS.exe"
if (-not (Test-Path $builtExecutable)) { throw "MSBuild output not found: $builtExecutable" }

$sdkBinRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/bin'
if (-not (Test-Path $sdkBinRoot)) { throw "Windows SDK tools directory was not found: $sdkBinRoot" }
$makeAppx = Get-ChildItem -Path $sdkBinRoot -Filter 'MakeAppx.exe' -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match '[\\/]x64[\\/]MakeAppx.exe$' } |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $makeAppx) { throw 'MakeAppx.exe was not found. Install the Windows SDK packaging tools.' }

$stagePath = Join-Path $env:TEMP 'OnyxOS-Package'
if (Test-Path $stagePath) { Remove-Item $stagePath -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stagePath | Out-Null
Copy-Item $manifestPath (Join-Path $stagePath 'AppxManifest.xml')
Copy-Item $assetsPath (Join-Path $stagePath 'Assets') -Recurse -Force

# Keep a single registered launcher executable. OS profiles are selected inside the launcher,
# not exposed as separate Xbox Start menu applications.
Copy-Item $builtExecutable (Join-Path $stagePath 'OnyxOS.exe') -Force

Get-ChildItem -Path (Split-Path $builtExecutable) -Filter '*.dll' -File -ErrorAction SilentlyContinue |
    ForEach-Object { Copy-Item $_.FullName (Join-Path $stagePath $_.Name) -Force }

$qemuDll = Join-Path $repoRoot 'Dependencies/Qemu/qemu-system-x86_64.dll'
if (Test-Path $qemuDll) {
    Copy-Item $qemuDll (Join-Path $stagePath 'qemu-system-x86_64.dll') -Force
    Write-Host 'Bundled QEMU x86_64 UWP host DLL.' -ForegroundColor Green
} else { Write-Warning 'QEMU UWP DLL is missing. VM startup will not be available.' }

# Ship the complete dependency tree with the APPX under the predictable Deps folder.
# Keep runtime DLLs at package root as well, because the Windows loader resolves them by module name.
$dependenciesPath = Join-Path $repoRoot 'Dependencies'
$depsStagePath = Join-Path $stagePath 'Deps'
if (Test-Path $dependenciesPath) {
    Copy-Item $dependenciesPath $depsStagePath -Recurse -Force
    Get-ChildItem (Join-Path $dependenciesPath 'Runtime') -Filter '*.dll' -File -ErrorAction SilentlyContinue |
        ForEach-Object { Copy-Item $_.FullName (Join-Path $stagePath $_.Name) -Force }
} else {
    Write-Warning 'Dependencies folder is missing; APPX will not contain Deps.'
}
$firmwarePath = Join-Path $repoRoot 'Dependencies/qemu'
if (Test-Path $firmwarePath) { Copy-Item $firmwarePath (Join-Path $stagePath 'qemu') -Recurse -Force }

$appxPath = Join-Path $outputPath 'OnyxOS-x64.appx'
if (Test-Path $appxPath) { Remove-Item $appxPath -Force }
Write-Host 'Packaging one APPX containing all seven application entries...' -ForegroundColor Yellow
Invoke-CheckedCommand -FilePath $makeAppx.FullName -Arguments @('pack','/d',$stagePath,'/p',$appxPath,'/o')
if (-not (Test-Path $appxPath)) { throw "APPX output was not created: $appxPath" }

Write-Host ''
Write-Host 'OnyxOS APPX build completed.' -ForegroundColor Green
Write-Host "Package: $appxPath"
Write-Warning 'The shared UWP launcher scaffold is packaged in all seven entries. Per-OS QEMU boot, guest display, and VHDX creation still require implementation.'
