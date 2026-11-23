$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$configPath = Join-Path $root 'config/os-profiles.json'
$config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
$profile = $config.profiles | Where-Object { $_.id -eq 'windows7' } | Select-Object -First 1

if (-not $profile) { throw 'Windows 7 profile is missing from os-profiles.json.' }
if ($config.defaultProfile -ne 'windows7') { throw 'Windows 7 must be the default profile.' }
if ($profile.ramMB -ne 1536) { throw "Windows 7 RAM must be 1536 MB; got $($profile.ramMB)." }
if ($profile.cpuCount -ne 1) { throw "Windows 7 CPU count must be 1; got $($profile.cpuCount)." }
if ($profile.cpuModel -ne 'qemu64') { throw "Windows 7 CPU model must be qemu64; got $($profile.cpuModel)." }
if ($profile.diskFormat -ne 'qcow2') { throw 'Windows 7 disk format must be qcow2.' }
if ($profile.audio -ne 'disabled') { throw 'Windows 7 audio must be disabled.' }
if ($config.defaults.ramMB -ne 1536 -or $config.defaults.cpuCount -ne 1 -or $config.defaults.cpuModel -ne 'qemu64') {
    throw 'Global defaults do not match the conservative Windows 7 Xbox profile.'
}
if ($config.defaults.acceleration.thread -ne 'single' -or $config.defaults.acceleration.translationCacheMB -ne 32) {
    throw 'TCG defaults must use single-thread mode and a 32 MB translation cache.'
}

$vmSource = Get-Content -LiteralPath (Join-Path $root 'src/vm/VirtualMachine.cpp') -Raw
foreach ($required in @('windows7', 'qcow2', '1536', 'qemu64', 'thread=single,tb-size=32', 'enableAudio', 'piix3-usb-uhci')) {
    if (-not $vmSource.Contains($required)) {
        throw "QEMU VM implementation is missing required Windows 7 setting: $required"
    }
}
$renderer = Join-Path $root 'src/graphics/CoreWindowD3DRenderer.cpp'
if (-not (Test-Path -LiteralPath $renderer)) { throw 'CoreWindow Direct3D renderer is missing.' }
$downloader = Get-Content -LiteralPath (Join-Path $root 'src/downloads/OsImageDownloader.cpp') -Raw
if (-not $downloader.Contains('Windows%207.qcow2') -or -not $downloader.Contains("magic[0] == 'Q'")) {
    throw 'Windows 7 QCOW2 download/validation code is missing.'
}
Write-Host 'OnyxOS Windows 7 profile validation passed.' -ForegroundColor Green
