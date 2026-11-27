#requires -Version 5.1
[CmdletBinding()]
param([string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot))
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# Pin the upstream implementation to an immutable commit. Do not silently
# change the QEMU ABI or its runtime DLL set when the upstream branch advances.
$owner = 'rodrigoandrigo'
$repository = 'QEMU-UWP-Host'
$upstreamCommit = '53d6af6314f535c4c71c4be269777a710063327b'
$archiveUrl = "https://codeload.github.com/$owner/$repository/zip/$upstreamCommit"
$dependenciesRoot = Join-Path $RepositoryRoot 'Dependencies'
$qemuDirectory = Join-Path $dependenciesRoot 'Qemu'
$runtimeDirectory = Join-Path $dependenciesRoot 'Runtime'
$firmwareDirectory = Join-Path $RepositoryRoot 'qemu'
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("OnyxOS-QEMU-" + [guid]::NewGuid().ToString('N'))
$archivePath = Join-Path $tempRoot 'qemu-uwp-host.zip'
$extractPath = Join-Path $tempRoot 'source'

New-Item -ItemType Directory -Force -Path $tempRoot,$qemuDirectory,$runtimeDirectory,$firmwareDirectory | Out-Null
try {
    Write-Host "Downloading pinned QEMU-UWP-Host commit $upstreamCommit ..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $archiveUrl -Headers @{ 'User-Agent' = 'OnyxOS-QEMU-runtime-fetcher' } -OutFile $archivePath -UseBasicParsing
    Expand-Archive -LiteralPath $archivePath -DestinationPath $extractPath -Force
    $sourceRoot = Get-ChildItem -LiteralPath $extractPath -Directory | Select-Object -First 1
    if (-not $sourceRoot) { throw 'Upstream archive has no repository root.' }

    $sourceQemu = Join-Path $sourceRoot.FullName 'Dependencies/Qemu'
    $sourceRuntime = Join-Path $sourceRoot.FullName 'Dependencies/Runtime'
    $sourceFirmware = Join-Path $sourceRoot.FullName 'qemu'
    $mainDll = Join-Path $sourceQemu 'qemu-system-x86_64.dll'
    if (-not (Test-Path -LiteralPath $mainDll -PathType Leaf)) { throw 'Upstream x64 QEMU host DLL is missing.' }
    if ((Get-Item -LiteralPath $mainDll).Length -ne 24338555) { throw 'Upstream QEMU DLL size differs from the pinned build; refusing an unverified binary.' }
    if (-not (Test-Path -LiteralPath $sourceRuntime -PathType Container)) { throw 'Upstream Runtime dependency folder is missing.' }
    if (-not (Test-Path -LiteralPath $sourceFirmware -PathType Container)) { throw 'Upstream QEMU firmware folder is missing.' }

    foreach ($file in Get-ChildItem -LiteralPath $sourceQemu -File) {
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $qemuDirectory $file.Name) -Force
    }
    $runtimeFiles = @(Get-ChildItem -LiteralPath $sourceRuntime -Filter '*.dll' -File)
    if ($runtimeFiles.Count -lt 20) { throw "Upstream runtime dependency set is incomplete: $($runtimeFiles.Count) DLLs." }
    foreach ($file in $runtimeFiles) {
        Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $runtimeDirectory $file.Name) -Force
    }

    # Preserve nested firmware resources (for example keymaps and descriptors).
    Copy-Item -Path (Join-Path $sourceFirmware '*') -Destination $firmwareDirectory -Recurse -Force
    $stagedDll = Join-Path $qemuDirectory 'qemu-system-x86_64.dll'
    if ((Get-Item -LiteralPath $stagedDll).Length -ne 24338555) { throw 'Staged QEMU DLL failed size verification.' }
    if (-not (Test-Path -LiteralPath (Join-Path $firmwareDirectory 'bios-256k.bin'))) { throw 'x86 BIOS was not staged.' }
    if (-not (Test-Path -LiteralPath (Join-Path $firmwareDirectory 'edk2-x86_64-code.fd'))) { throw 'x86_64 UEFI firmware was not staged.' }

    Write-Host "QEMU-UWP-Host $upstreamCommit staged: x64 host DLL, $($runtimeFiles.Count) runtime DLLs and firmware resources." -ForegroundColor Green
    Write-Host "Upstream: https://github.com/$owner/$repository/tree/$upstreamCommit"
}
finally {
    if (Test-Path -LiteralPath $tempRoot) { Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue }
}
