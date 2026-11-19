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
$apiRoot = "https://api.github.com/repos/$owner/$repository/contents"
$rawRoot = "https://raw.githubusercontent.com/$owner/$repository/$Branch"

$qemuDirectory = Join-Path $RepositoryRoot 'Dependencies/Qemu'
$runtimeDirectory = Join-Path $RepositoryRoot 'Dependencies/Runtime'
New-Item -ItemType Directory -Force -Path $qemuDirectory, $runtimeDirectory | Out-Null

$headers = @{
    'User-Agent' = 'OnyxOS-QEMU-runtime-fetcher'
    'Accept' = 'application/vnd.github+json'
}

function Get-UpstreamFiles {
    param([Parameter(Mandatory = $true)][string]$Path)

    $uri = "$apiRoot/$Path" + "?ref=$Branch"
    $items = Invoke-RestMethod -Uri $uri -Headers $headers -Method Get
    return @($items | Where-Object { $_.type -eq 'file' })
}

function Save-UpstreamFile {
    param(
        [Parameter(Mandatory = $true)]$Item,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    $target = Join-Path $Destination $Item.name
    if ((Test-Path $target) -and ((Get-Item $target).Length -eq [long]$Item.size)) {
        Write-Host "Already present: $($Item.name)" -ForegroundColor DarkGray
        return
    }

    # Use GitHub's canonical download URL. The previous URL builder removed
    # the Dependencies/ prefix, producing a path that does not exist (404).
    $downloadUrl = $Item.download_url
    if ([string]::IsNullOrWhiteSpace($downloadUrl)) {
        $downloadUrl = "$rawRoot/$($Item.path)"
    }
    Write-Host "Downloading $($Item.name) ($($Item.size) bytes)..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $downloadUrl -Headers @{ 'User-Agent' = 'OnyxOS-QEMU-runtime-fetcher' } -OutFile $target -UseBasicParsing

    $actualSize = (Get-Item $target).Length
    if ($actualSize -ne [long]$Item.size) {
        Remove-Item $target -Force -ErrorAction SilentlyContinue
        throw "Tamanho incorreto para $($Item.name): esperado $($Item.size), recebido $actualSize."
    }
}

$qemuFiles = Get-UpstreamFiles -Path 'Dependencies/Qemu'
$runtimeFiles = Get-UpstreamFiles -Path 'Dependencies/Runtime'

if (-not ($qemuFiles | Where-Object { $_.name -eq 'qemu-system-x86_64.dll' })) {
    throw 'O repositório upstream não publicou qemu-system-x86_64.dll no caminho esperado.'
}

foreach ($file in $qemuFiles) {
    Save-UpstreamFile -Item $file -Destination $qemuDirectory
}
foreach ($file in $runtimeFiles) {
    Save-UpstreamFile -Item $file -Destination $runtimeDirectory
}

Write-Host "QEMU host + $($runtimeFiles.Count) runtime files staged for OnyxOS." -ForegroundColor Green
Write-Host "Source: https://github.com/$owner/$repository/tree/$Branch/Dependencies"
