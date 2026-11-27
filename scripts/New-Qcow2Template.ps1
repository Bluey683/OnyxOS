#requires -Version 5.1
[CmdletBinding()]
param([string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot), [string]$VirtualSize = '64G')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$artifactDir = Join-Path $RepositoryRoot 'artifacts'
New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null
$qemuImg = Get-Command qemu-img.exe -ErrorAction SilentlyContinue
if (-not $qemuImg) { $qemuImg = Get-Command qemu-img -ErrorAction SilentlyContinue }
if (-not $qemuImg) { throw 'qemu-img was not installed or is not on PATH.' }
$image = Join-Path $artifactDir 'Windows11-22H2-template.qcow2'
if (Test-Path $image) { Remove-Item -LiteralPath $image -Force }
Write-Host "Creating fresh QCOW2 template ($VirtualSize) with qemu-img ..."
& $qemuImg.Source create -f qcow2 -o compat=1.1,lazy_refcounts=on,preallocation=off $image $VirtualSize
if ($LASTEXITCODE -ne 0) { throw "qemu-img create failed with exit code $LASTEXITCODE" }
& $qemuImg.Source info --output=json $image
if ($LASTEXITCODE -ne 0) { throw 'qemu-img could not validate the generated QCOW2 image.' }
$header = [IO.File]::ReadAllBytes($image)
if ($header.Length -lt 72 -or $header[0] -ne 0x51 -or $header[1] -ne 0x46 -or $header[2] -ne 0x49 -or $header[3] -ne 0xfb) {
    throw 'Generated disk does not have a valid QCOW2 header.'
}
Write-Host "QCOW2 template created: $image ($((Get-Item $image).Length) bytes allocated)." -ForegroundColor Green
