#requires -Version 5.1
[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$MSYS2Root = 'C:\msys64',
    [string]$QemuVersion = '11.0.2',
    [string]$QemuOverlayCommit = 'b9d726d9c4e1c06a5666178a267fd635d0e69e52'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$shell = Join-Path $MSYS2Root 'msys2_shell.cmd'
if (-not (Test-Path -LiteralPath $shell)) { throw "MSYS2 shell not found: $shell" }
$work = Join-Path ([IO.Path]::GetTempPath()) ("OnyxOS-QEMU-Build-" + [guid]::NewGuid().ToString('N'))
$archive = Join-Path $work "qemu-$QemuVersion.tar.xz"
$overlayZip = Join-Path $work 'qemu-uwp-overlay.zip'
$extract = Join-Path $work 'src'
$overlayExtract = Join-Path $work 'overlay'
$source = Join-Path $extract "qemu-$QemuVersion"
$qemuDll = Join-Path $source 'build/qemu-system-x86_64.dll'
$destination = Join-Path $RepositoryRoot 'Dependencies/Qemu/qemu-system-x86_64.dll'

function ConvertTo-MsysPath([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path).Replace('\','/')
    if ($full -match '^([A-Za-z]):/(.*)$') { return '/' + $Matches[1].ToLowerInvariant() + '/' + $Matches[2] }
    throw "Cannot convert path to MSYS2 format: $Path"
}

New-Item -ItemType Directory -Force -Path $work,$extract,$overlayExtract | Out-Null
try {
    $qemuUrl = "https://download.qemu.org/qemu-$QemuVersion.tar.xz"
    $overlayUrl = "https://codeload.github.com/rodrigoandrigo/Qemu-Dll-shadps4/zip/$QemuOverlayCommit"
    Write-Host "Downloading QEMU $QemuVersion source..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $qemuUrl -OutFile $archive -UseBasicParsing
    Write-Host "Downloading pinned UWP source overlay $QemuOverlayCommit..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $overlayUrl -OutFile $overlayZip -UseBasicParsing

    & tar.exe -xf $archive -C $extract
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $source -PathType Container)) {
        throw "Could not extract QEMU source archive: $archive"
    }
    Expand-Archive -LiteralPath $overlayZip -DestinationPath $overlayExtract -Force
    $overlayRoot = Get-ChildItem -LiteralPath $overlayExtract -Directory | Select-Object -First 1
    if (-not $overlayRoot) { throw 'UWP overlay archive has no root directory.' }
    Copy-Item -Path (Join-Path $overlayRoot.FullName '*') -Destination $source -Recurse -Force

    $patchScript = Join-Path $RepositoryRoot 'scripts/Apply-QemuBlockAlignmentFix.ps1'
    if (-not (Test-Path -LiteralPath $patchScript)) { throw "QEMU alignment patch script missing: $patchScript" }
    & $patchScript -QemuSourceRoot $source
    if (-not $?) { throw 'Applying the QEMU UWP block alignment fix failed.' }

    $sourceUnix = ConvertTo-MsysPath $source
    $command = "cd '$sourceUnix' && mkdir -p build && cd build && ../configure --target-list=x86_64-softmmu --disable-gtk --disable-werror --disable-docs --disable-libnfs --disable-qga-vss --disable-sdl --disable-sdl-image --disable-dsound --enable-uwp --enable-xaudio2 --audio-drv-list=xaudio2 && make -j2 && strip --strip-unneeded qemu-system-x86_64.dll"
    Write-Host 'Configuring and compiling patched UWP QEMU x64 (TCG)...' -ForegroundColor Cyan
    & $shell -defterm -no-start -ucrt64 -c $command
    if ($LASTEXITCODE -ne 0) { throw "QEMU UWP compilation failed with exit code $LASTEXITCODE." }
    if (-not (Test-Path -LiteralPath $qemuDll -PathType Leaf)) { throw 'QEMU build did not produce qemu-system-x86_64.dll.' }
    $built = Get-Item -LiteralPath $qemuDll
    if ($built.Length -lt 20000000) { throw "Compiled QEMU DLL is unexpectedly small: $($built.Length) bytes." }

    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
    Copy-Item -LiteralPath $qemuDll -Destination $destination -Force
    $hash = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash
    Write-Host "Patched QEMU UWP DLL staged: $destination ($($built.Length) bytes)" -ForegroundColor Green
    Write-Host "SHA256: $hash" -ForegroundColor Green
}
finally {
    if (Test-Path -LiteralPath $work) { Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue }
}
