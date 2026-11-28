#requires -Version 5.1
[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$MSYS2Root = 'C:\msys64',
    [string]$QemuVersion = '11.0.2',
    [string]$QemuOverlayCommit = 'b9d726d9c4e1c06a5666178a267fd635d0e69e52',
    [string]$QemuRepository = 'https://gitlab.com/qemu-project/qemu.git',
    [string]$OverlayRepository = 'https://github.com/rodrigoandrigo/Qemu-Dll-shadps4.git'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$shell = Join-Path $MSYS2Root 'msys2_shell.cmd'
if (-not (Test-Path -LiteralPath $shell)) { throw "MSYS2 shell not found: $shell" }
$work = Join-Path ([IO.Path]::GetTempPath()) ("OnyxOS-QEMU-Build-" + [guid]::NewGuid().ToString('N'))
$extract = Join-Path $work 'src'
$overlayExtract = Join-Path $work 'overlay'
$source = Join-Path $extract 'qemu-source'
$overlaySource = Join-Path $overlayExtract 'overlay-source'
$qemuDll = Join-Path $source 'build/qemu-system-x86_64.dll'
$destination = Join-Path $RepositoryRoot 'Dependencies/Qemu/qemu-system-x86_64.dll'

function ConvertTo-MsysPath([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path).Replace('\','/')
    if ($full -match '^([A-Za-z]):/(.*)$') { return '/' + $Matches[1].ToLowerInvariant() + '/' + $Matches[2] }
    throw "Cannot convert path to MSYS2 format: $Path"
}

New-Item -ItemType Directory -Force -Path $work,$extract,$overlayExtract | Out-Null
try {
    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) { throw 'Git is required but git.exe was not found on PATH.' }
    $gitExe = $git.Source

    Write-Host "Cloning QEMU repository and checking out v$QemuVersion..." -ForegroundColor Cyan
    & $gitExe clone --filter=blob:none --no-checkout $QemuRepository $source
    if ($LASTEXITCODE -ne 0) { throw 'Git clone of the QEMU repository failed.' }
    & $gitExe -C $source fetch --depth 1 origin "refs/tags/v$QemuVersion"
    if ($LASTEXITCODE -ne 0) { throw "Could not fetch QEMU tag v$QemuVersion." }
    & $gitExe -C $source checkout --detach FETCH_HEAD
    if ($LASTEXITCODE -ne 0) { throw "Could not check out QEMU v$QemuVersion." }

    Write-Host "Cloning UWP overlay repository and checking out $QemuOverlayCommit..." -ForegroundColor Cyan
    & $gitExe clone --filter=blob:none --no-checkout $OverlayRepository $overlaySource
    if ($LASTEXITCODE -ne 0) { throw 'Git clone of the UWP overlay repository failed.' }
    & $gitExe -C $overlaySource fetch --depth 1 origin $QemuOverlayCommit
    if ($LASTEXITCODE -ne 0) { throw "Could not fetch pinned UWP overlay commit $QemuOverlayCommit." }
    & $gitExe -C $overlaySource checkout --detach FETCH_HEAD
    if ($LASTEXITCODE -ne 0) { throw "Could not check out pinned UWP overlay commit $QemuOverlayCommit." }
    Copy-Item -Path (Join-Path $overlaySource '*') -Destination $source -Recurse -Force
    # Repair missing public ABI constants used by the pinned CXBX TCG backend.
    $cxbxHeader = Join-Path $source 'include/qemu/qemu-cxbx-cpu.h'
    if (-not (Test-Path -LiteralPath $cxbxHeader -PathType Leaf)) { throw "CXBX CPU header missing: $cxbxHeader" }
    $cxbxText = [IO.File]::ReadAllText($cxbxHeader)
    if (-not $cxbxText.Contains('QEMU_CXBX_CPU_RUN_TIMESLICE')) {
        $enumOld = '    QEMU_CXBX_CPU_RUN_UNHANDLED_HLE,\n} QemuCxbxCpuRunReason;'
        $enumNew = '    QEMU_CXBX_CPU_RUN_UNHANDLED_HLE,\n    QEMU_CXBX_CPU_RUN_TIMESLICE,\n    QEMU_CXBX_CPU_RUN_INTERRUPT,\n} QemuCxbxCpuRunReason;'
        if (-not $cxbxText.Contains($enumOld)) { throw 'CXBX run-reason enum patch target not found.' }
        $cxbxText = $cxbxText.Replace($enumOld, $enumNew)
    }
    if (-not $cxbxText.Contains('QEMU_CXBX_CPU_INTERRUPT_HARD')) {
        $flagsAnchor = 'typedef struct QemuCxbxCpuRegisters {'
        $flagsDef = 'enum {\n    QEMU_CXBX_CPU_INTERRUPT_HARD = 1u << 0,\n    QEMU_CXBX_CPU_INTERRUPT_NMI = 1u << 1,\n};\n\n'
        if (-not $cxbxText.Contains($flagsAnchor)) { throw 'CXBX interrupt flags patch target not found.' }
        $cxbxText = $cxbxText.Replace($flagsAnchor, $flagsDef + $flagsAnchor)
    }
    [IO.File]::WriteAllText($cxbxHeader, $cxbxText, [Text.UTF8Encoding]::new($false))
    $patchScript = Join-Path $RepositoryRoot 'scripts/Apply-QemuBlockAlignmentFix.ps1'
    if (-not (Test-Path -LiteralPath $patchScript)) { throw "QEMU alignment patch script missing: $patchScript" }
    & $patchScript -QemuSourceRoot $source
    if (-not $?) { throw 'Applying the QEMU UWP block alignment fix failed.' }

    $sourceUnix = ConvertTo-MsysPath $source
    # Meson runs through native Windows Python, so use a Windows pkg-config path.
    $mesonWindowsFile = Join-Path $source 'configs/meson/windows.txt'
    if (-not (Test-Path -LiteralPath $mesonWindowsFile)) { throw "Meson Windows cross file not found: $mesonWindowsFile" }
    $mesonWindowsText = Get-Content -LiteralPath $mesonWindowsFile -Raw
    $pkgConfigWindows = Join-Path $MSYS2Root 'ucrt64/bin/pkg-config.exe'
    $pkgConfigWindows = $pkgConfigWindows.Replace('\','/')
    $mesonWindowsText = [regex]::Replace($mesonWindowsText, '(?m)^\s*pkgconfig\s*=\s*.*$', "pkgconfig = '$pkgConfigWindows'")
    Set-Content -LiteralPath $mesonWindowsFile -Value $mesonWindowsText -Encoding utf8
    # Install and verify build tools in the same UCRT64 shell.
    $command = "set -e; export PATH=/ucrt64/bin:/usr/bin:`$PATH; pacman -S --needed --noconfirm git diffutils mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-glib2; export PATH=/ucrt64/bin:/usr/bin:/bin:`$PATH; command -v git; git --version; command -v diff; diff --version; command -v pkg-config; pkg-config --modversion glib-2.0; command -v ninja; ninja --version; command -v gcc; gcc --version; command -v python; python --version; cd '$sourceUnix'; mkdir -p build; cd build; ../configure --python=/ucrt64/bin/python.exe --target-list=x86_64-softmmu --disable-gtk --disable-werror --disable-docs --disable-libnfs --disable-qga-vss --disable-sdl --disable-sdl-image --disable-dsound --disable-plugins --enable-uwp --enable-xaudio2 --audio-drv-list=xaudio2; ninja -j2; strip --strip-unneeded qemu-system-x86_64.dll"
    Write-Host 'Installing/verifying UCRT64 GCC, Python, Ninja, pkg-config and GLib, then configuring and compiling patched QEMU x64 (TCG)...' -ForegroundColor Cyan
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
