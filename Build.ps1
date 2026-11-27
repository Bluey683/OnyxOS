#requires -Version 5.1
[CmdletBinding()]
param(
    [ValidateSet('PC','GitHubActions')][string]$Mode='PC',
    [ValidateSet('Debug','Release')][string]$Configuration='Release',
    [ValidateSet('x64')][string]$Architecture='x64',
    [string]$WindowsSdkVersion='10.0.26100.0',
    [string]$BuildDirectory='build',
    [string]$OutputDirectory='artifacts',
    [switch]$Clean,
    [switch]$SkipAssets,
    [switch]$SkipQemuDownload
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
function Invoke-CheckedCommand {
    param([string]$FilePath,[string[]]$Arguments)
    Write-Host "> $FilePath $($Arguments -join ' ')" -ForegroundColor Cyan
    $global:LASTEXITCODE = 0
    & $FilePath @Arguments
    $commandExitCode = $LASTEXITCODE
    if ($commandExitCode -ne 0) { throw "Command failed with exit code $commandExitCode : $FilePath" }
}
$root=$PSScriptRoot
$buildPath=Join-Path $root $BuildDirectory
$outPath=Join-Path $root $OutputDirectory
$project=Join-Path $root 'OnyxOS.vcxproj'
if ($env:OS -ne 'Windows_NT') { throw 'Build.ps1 requires Windows, Visual Studio C++ and the Windows SDK.' }
if (!(Test-Path $project)) { throw "Project not found: $project" }
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (!(Test-Path $vswhere)) { throw 'vswhere.exe not found. Install Visual Studio 2022.' }
$msbuild=& $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -find 'MSBuild/**/Bin/MSBuild.exe' | Select-Object -First 1
if (!$msbuild -or !(Test-Path $msbuild)) { throw 'MSBuild not found.' }
if (!$SkipQemuDownload) {
    $fetch=Join-Path $root 'scripts/Fetch-QemuRuntime.ps1'
    if (!(Test-Path $fetch)) { throw 'scripts/Fetch-QemuRuntime.ps1 not found.' }
    & $fetch -RepositoryRoot $root
    if (!$?) { throw 'QEMU runtime download/staging failed.' }
}
$qemu=Join-Path $root 'Dependencies/Qemu/qemu-system-x86_64.dll'
$runtime=Join-Path $root 'Dependencies/Runtime'
if (!(Test-Path $qemu) -or (Get-Item $qemu).Length -lt 20000000) { throw 'QEMU host DLL is missing or incomplete.' }
$runtimeFiles=@(Get-ChildItem -LiteralPath $runtime -Filter '*.dll' -File -ErrorAction SilentlyContinue)
if ($runtimeFiles.Count -lt 20) { throw "QEMU runtime dependency set incomplete: found $($runtimeFiles.Count) DLLs." }
if ($Clean) {
    if (Test-Path $buildPath) { Remove-Item $buildPath -Recurse -Force }
    if (Test-Path $outPath) { Remove-Item $outPath -Recurse -Force }
}
New-Item -ItemType Directory -Force -Path $buildPath,$outPath | Out-Null
if (!$SkipAssets) {
    $assetScript=Join-Path $root 'scripts/Make-Assets.ps1'
    if (!(Test-Path $assetScript)) { throw 'scripts/Make-Assets.ps1 not found.' }
    & $assetScript
    if (!$?) { throw 'Asset generation failed.' }
}
$packageDir=Join-Path $outPath 'MSBuildPackage'
New-Item -ItemType Directory -Force -Path $packageDir | Out-Null
Invoke-CheckedCommand $msbuild @(
    $project,'/m','/t:Rebuild',"/p:Configuration=$Configuration","/p:Platform=$Architecture",
    "/p:WindowsTargetPlatformVersion=$WindowsSdkVersion",
    "/p:OutDir=$(Join-Path $buildPath "$Architecture/$Configuration/")",
    "/p:IntDir=$(Join-Path $buildPath "obj/$Architecture/$Configuration/")",
    '/p:AppxPackage=true','/p:GenerateAppxPackageOnBuild=true',
    '/p:AppxPackageSigningEnabled=false','/p:AppxBundle=Never',
    "/p:AppxPackageDir=$packageDir\"
)
$appx=Get-ChildItem -LiteralPath $packageDir -File -Recurse |
    Where-Object { $_.Extension -in '.appx','.msix' -and $_.Name -notmatch 'VCLibs|Dependencies' } |
    Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (!$appx) { throw "MSBuild did not produce an APPX/MSIX in $packageDir." }
$finalAppx=Join-Path $outPath 'OnyxOS-x64.appx'
Copy-Item -LiteralPath $appx.FullName -Destination $finalAppx -Force
Add-Type -AssemblyName System.IO.Compression.FileSystem

# MSBuild can omit wildcard Content items from UWP package layouts. Inject the
# staged QEMU binaries explicitly before validating the final unsigned APPX.
# Rebuild the APPX with MakeAppx after adding QEMU. Editing an APPX as a raw ZIP
# leaves AppxBlockMap.xml stale and SignTool rejects the package format.
$sdkBin=Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/bin'
$makeAppx=Get-ChildItem -Path $sdkBin -Directory -ErrorAction Stop |
    Sort-Object Name -Descending |
    ForEach-Object { Join-Path $_.FullName 'x64/MakeAppx.exe' } |
    Where-Object { Test-Path $_ } |
    Select-Object -First 1
if (!$makeAppx) { throw "MakeAppx.exe was not found under $sdkBin." }
$unpacked=Join-Path $buildPath 'AppxRepack'
if (Test-Path $unpacked) { Remove-Item $unpacked -Recurse -Force }
New-Item -ItemType Directory -Force -Path $unpacked | Out-Null
Invoke-CheckedCommand $makeAppx @('unpack','/p',$finalAppx,'/d',$unpacked,'/o')
foreach ($file in @((Get-Item -LiteralPath $qemu)) + $runtimeFiles) {
    Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $unpacked $file.Name) -Force
}
$diskTemplate = Join-Path $outPath 'Windows11-22H2-template.qcow2'
if (!(Test-Path -LiteralPath $diskTemplate)) {
    throw 'The qemu-img QCOW2 template is missing. Run scripts/New-Qcow2Template.ps1 before packaging.'
}
$diskTemplateInfo = Get-Item -LiteralPath $diskTemplate
if ($diskTemplateInfo.Length -lt 72) { throw 'The generated QCOW2 template is too small.' }
Copy-Item -LiteralPath $diskTemplate -Destination (Join-Path $unpacked 'Windows11-22H2-template.qcow2') -Force

$firmwareSource = Join-Path $root 'qemu'
if (!(Test-Path -LiteralPath (Join-Path $firmwareSource 'bios-256k.bin'))) {
    throw 'QEMU firmware is missing: qemu/bios-256k.bin. The guest cannot boot without the packaged BIOS.'
}
$firmwareDestination = Join-Path $unpacked 'qemu'
New-Item -ItemType Directory -Force -Path $firmwareDestination | Out-Null
Copy-Item -Path (Join-Path $firmwareSource '*') -Destination $firmwareDestination -Recurse -Force

# Include the HLSL source tracked in GitHub in the final APPX.
$shaderSource=Join-Path $root 'graphics/shaders/onyxos.hlsl'
if (!(Test-Path -LiteralPath $shaderSource)) { throw "HLSL shader source missing: $shaderSource" }
$shaderPackageDir=Join-Path $unpacked 'Shaders'
New-Item -ItemType Directory -Force -Path $shaderPackageDir | Out-Null
Copy-Item -LiteralPath $shaderSource -Destination (Join-Path $shaderPackageDir 'onyxos.hlsl') -Force
$rebuiltAppx=Join-Path $outPath 'OnyxOS-x64-repacked.appx'
if (Test-Path $rebuiltAppx) { Remove-Item $rebuiltAppx -Force }
Invoke-CheckedCommand $makeAppx @('pack','/d',$unpacked,'/p',$rebuiltAppx,'/o')
Move-Item -LiteralPath $rebuiltAppx -Destination $finalAppx -Force

$zip = [IO.Compression.ZipFile]::OpenRead($finalAppx)
try {
    # ZIP/AppX entry names always use forward slashes.
    $entries = @{}
    foreach ($entry in $zip.Entries) {
        $entries[$entry.FullName] = $entry.Length
    }

    $qemuEntry = $entries.Keys |
        Where-Object { $_ -match '(^|/)qemu-system-x86_64\.dll$' } |
        Select-Object -First 1

    $diskEntry = 'Windows11-22H2-template.qcow2'
    if (!$entries.ContainsKey($diskEntry) -or $entries[$diskEntry] -lt 72) {
        throw 'The final APPX does not contain the generated QCOW2 disk template.'
    }
    $shaderEntry = 'Shaders/onyxos.hlsl'
    if (!$entries.ContainsKey($shaderEntry) -or $entries[$shaderEntry] -le 0) {
        throw "The final APPX does not contain Shaders/onyxos.hlsl."
    }
    if (!$qemuEntry -or $entries[$qemuEntry] -lt 20000000) {
        throw "The final APPX does not contain the complete QEMU host DLL."
    }
    if (!$entries.ContainsKey('qemu/bios-256k.bin') -or $entries['qemu/bios-256k.bin'] -le 0) {
        throw "The final APPX does not contain qemu/bios-256k.bin; QEMU has no packaged x86 BIOS."
    }
    if (!$entries.ContainsKey('qemu/edk2-x86_64-code.fd') -or $entries['qemu/edk2-x86_64-code.fd'] -le 0) {
        throw "The final APPX does not contain qemu/edk2-x86_64-code.fd; x86_64 UEFI guests cannot boot."
    }

    $missing = @()
    foreach ($file in $runtimeFiles) {
        $found = $entries.Keys |
            Where-Object { $_ -eq $file.Name -or $_ -match ("(^|/){0}$" -f [regex]::Escape($file.Name)) } |
            Select-Object -First 1
        if (!$found -or $entries[$found] -ne $file.Length) {
            $missing += $file.Name
        }
    }
    if ($missing.Count) {
        throw ("APPX missing runtime DLLs: " + ($missing -join ', '))
    }
    Write-Host "Verified HLSL, QEMU host, BIOS, x86_64 UEFI firmware and $($runtimeFiles.Count) runtime DLLs inside APPX." -ForegroundColor Green
} finally {
    $zip.Dispose()
}
$vclibs=Join-Path $outPath 'Microsoft.VCLibs.x64.14.00.appx'
$pf86=[Environment]::GetFolderPath('ProgramFilesX86')
$candidates=@((Join-Path $pf86 'Microsoft SDKs/Windows Kits/10/ExtensionSDKs/Microsoft.VCLibs/14.0/AppX/Retail/x64/Microsoft.VCLibs.x64.14.00.appx'))
if ($env:WindowsSdkDir) { $candidates += Join-Path $env:WindowsSdkDir 'ExtensionSDKs/Microsoft.VCLibs/14.0/AppX/Retail/x64/Microsoft.VCLibs.x64.14.00.appx' }
$candidates=$candidates | Where-Object { $_ -and (Test-Path $_) }
$source=$candidates | Select-Object -First 1
if ($source) { Copy-Item $source $vclibs -Force }
else { Invoke-WebRequest -Uri 'https://aka.ms/Microsoft.VCLibs.x64.14.00.appx' -OutFile $vclibs -UseBasicParsing }
if (!(Test-Path $vclibs) -or (Get-Item $vclibs).Length -lt 100000) { throw 'Microsoft VCLibs x64 package missing or invalid.' }
Write-Host "APPX: $finalAppx" -ForegroundColor Green
Write-Host "VCLibs: $vclibs" -ForegroundColor Green
Write-Warning 'Packaging validation is not a runtime test. QEMU boot, D3D guest-frame rendering, audio, networking and VHDX provisioning must be tested on the Xbox.'
