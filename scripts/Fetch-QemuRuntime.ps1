#requires -Version 5.1
[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$Branch = 'x86-64'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$owner = 'rodrigoandrigo'
$repository = 'QEMU-UWP-Host'
$archiveUrl = "https://codeload.github.com/$owner/$repository/zip/refs/heads/$Branch"
$dependenciesRoot = Join-Path $RepositoryRoot 'Dependencies'
$qemuDirectory = Join-Path $dependenciesRoot 'Qemu'
$runtimeDirectory = Join-Path $dependenciesRoot 'Runtime'
$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("OnyxOS-QEMU-" + [guid]::NewGuid().ToString('N'))
$archivePath = Join-Path $tempRoot 'qemu-uwp-host.zip'
$extractPath = Join-Path $tempRoot 'source'

New-Item -ItemType Directory -Force -Path $tempRoot, $qemuDirectory, $runtimeDirectory | Out-Null

try {
    Write-Host "Downloading QEMU UWP source archive from $archiveUrl ..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $archiveUrl -Headers @{ 'User-Agent' = 'OnyxOS-QEMU-runtime-fetcher' } -OutFile $archivePath -UseBasicParsing

    Expand-Archive -LiteralPath $archivePath -DestinationPath $extractPath -Force
    $sourceRoot = Get-ChildItem -LiteralPath $extractPath -Directory | Select-Object -First 1
    if (-not $sourceRoot) {
        throw 'O arquivo do repositório QEMU não contém a pasta raiz esperada.'
    }

    $sourceQemu = Join-Path $sourceRoot.FullName 'Dependencies/Qemu'
    $sourceRuntime = Join-Path $sourceRoot.FullName 'Dependencies/Runtime'
    $sourceFirmware = Join-Path $sourceRoot.FullName 'qemu'
    $firmwareDirectory = Join-Path $RepositoryRoot 'qemu'
    $mainDll = Join-Path $sourceQemu 'qemu-system-x86_64.dll'
    if (-not (Test-Path -LiteralPath $mainDll -PathType Leaf)) {
        throw "A DLL qemu-system-x86_64.dll não foi encontrada em $sourceQemu"
    }

    foreach ($sourceDirectory in @(
        @{ Source = $sourceQemu; Destination = $qemuDirectory },
        @{ Source = $sourceRuntime; Destination = $runtimeDirectory },
        @{ Source = $sourceFirmware; Destination = $firmwareDirectory }
    )) {
        if (-not (Test-Path -LiteralPath $sourceDirectory.Source -PathType Container)) {
            if ($sourceDirectory.Source -eq $sourceRuntime) {
                Write-Warning 'A pasta de dependências Runtime não foi encontrada no arquivo upstream.'
                continue
            }
            throw "Diretório upstream ausente: $($sourceDirectory.Source)"
        }

        foreach ($file in Get-ChildItem -LiteralPath $sourceDirectory.Source -File) {
            $target = Join-Path $sourceDirectory.Destination $file.Name
            if ((Test-Path -LiteralPath $target) -and
                (Get-Item -LiteralPath $target).Length -eq $file.Length) {
                Write-Host "Already present: $($file.Name)" -ForegroundColor DarkGray
                continue
            }
            Copy-Item -LiteralPath $file.FullName -Destination $target -Force
            if ((Get-Item -LiteralPath $target).Length -ne $file.Length) {
                Remove-Item -LiteralPath $target -Force -ErrorAction SilentlyContinue
                throw "Falha na validação de tamanho de $($file.Name)."
            }
            Write-Host "Staged $($file.Name) ($($file.Length) bytes)" -ForegroundColor Cyan
        }
    }

    Write-Host 'QEMU host DLL and runtime dependencies staged successfully.' -ForegroundColor Green
    Write-Host "Source: https://github.com/$owner/$repository/tree/$Branch/Dependencies"
}
finally {
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
