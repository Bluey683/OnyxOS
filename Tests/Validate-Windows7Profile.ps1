$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$configPath = Join-Path $root 'config/os-profiles.json'
$config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
$profile = $config.profiles | Where-Object { $_.id -eq 'windows7' } | Select-Object -First 1
if (-not $profile) { throw 'Windows 7 profile is missing from os-profiles.json.' }
if ($config.defaultProfile -ne 'windows7') { throw 'Windows 7 must be the default profile.' }
if ($profile.ramMB -ne 2560) { throw "Windows 7 RAM must be 2560 MB; got $($profile.ramMB)." }
if ($profile.cpuModel -ne 'max') { throw 'Windows 7 CPU model must be max.' }
if ($profile.diskFormat -ne 'qcow2') { throw 'Windows 7 disk format must be qcow2.' }
if ($profile.audio -ne 'disabled') { throw 'Windows 7 audio must be disabled.' }

$vmSource = Get-Content -LiteralPath (Join-Path $root 'src/vm/VirtualMachine.cpp') -Raw
foreach ($required in @('windows7', 'qcow2', '2560', 'enableAudio', 'piix3-usb-uhci')) {
    if (-not $vmSource.Contains($required)) { throw "QEMU VM implementation is missing required Windows 7 setting: $required" }
}
$renderer = Join-Path $root 'src/graphics/CoreWindowD3DRenderer.cpp'
if (-not (Test-Path -LiteralPath $renderer)) { throw 'CoreWindow Direct3D renderer is missing.' }
$downloader = Get-Content -LiteralPath (Join-Path $root 'src/downloads/OsImageDownloader.cpp') -Raw
if (-not $downloader.Contains('Windows 7.qcow2') -or -not $downloader.Contains('QFI')) {
    throw 'Windows 7 QCOW2 download/validation code is missing.'
}
Write-Host 'OnyxOS Windows 7 profile validation passed.' -ForegroundColor Green
