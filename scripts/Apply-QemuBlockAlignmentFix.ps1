#requires -Version 5.1
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$QemuSourceRoot
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$file = Join-Path $QemuSourceRoot 'block/file-win32.c'
if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
    throw "QEMU source file not found: $file"
}
$content = [IO.File]::ReadAllText($file)
$old = @'
    if (s->drive_path[0]) {
        GetDiskFreeSpace(s->drive_path, &sectorsPerCluster,
                         &bytesPerSector,
                         &freeClusters, &totalClusters);
        bs->bl.request_alignment = bytesPerSector;
        return;
    }
'@
$new = @'
    if (s->drive_path[0]) {
        /*
         * GetDiskFreeSpace can fail in packaged UWP/Xbox processes. The
         * output parameters are then undefined (often zero), and assigning
         * bytesPerSector directly violates QEMU's power-of-two invariant.
         * Keep the conservative 512-byte default unless the query succeeds
         * and returns a valid power-of-two sector size.
         */
        if (GetDiskFreeSpace(s->drive_path, &sectorsPerCluster,
                             &bytesPerSector,
                             &freeClusters, &totalClusters) &&
            bytesPerSector != 0 &&
            (bytesPerSector & (bytesPerSector - 1)) == 0) {
            bs->bl.request_alignment = bytesPerSector;
        } else {
            bs->bl.request_alignment = BDRV_SECTOR_SIZE;
        }
        return;
    }
'@
if ($content.Contains($new)) {
    Write-Host 'QEMU block alignment fix is already applied.' -ForegroundColor Green
    exit 0
}
if (-not $content.Contains($old)) {
    throw 'Expected raw_probe_alignment block was not found. Refusing to patch an unknown QEMU source revision.'
}
$content = $content.Replace($old, $new)
[IO.File]::WriteAllText($file, $content, [Text.UTF8Encoding]::new($false))
Write-Host 'Applied UWP GetDiskFreeSpace alignment fallback to block/file-win32.c.' -ForegroundColor Green
