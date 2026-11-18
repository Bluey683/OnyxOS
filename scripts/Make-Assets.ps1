$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# Generate all image assets referenced by Package.appxmanifest.
# This script runs on the Windows GitHub Actions runner (System.Drawing).
if (-not $IsWindows -and $env:OS -ne 'Windows_NT') {
    throw 'Make-Assets.ps1 must run on Windows.'
}

Add-Type -AssemblyName System.Drawing

$assets = Join-Path $PSScriptRoot '..\Assets'
New-Item -ItemType Directory -Force -Path $assets | Out-Null

function New-OnyxLogo {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][int]$Width,
        [Parameter(Mandatory = $true)][int]$Height,
        [string]$Label = 'ONYXOS'
    )

    $path = Join-Path $assets $Name
    $bitmap = [System.Drawing.Bitmap]::new(
        $Width,
        $Height,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb
    )
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $font = $null
    $brush = $null
    $background = $null
    $format = $null

    try {
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
        $graphics.Clear([System.Drawing.Color]::FromArgb(16, 24, 32))

        # A simple cyan accent mark keeps the generated tile legible at small sizes.
        $markSize = [Math]::Max(3, [int]($Height * 0.07))
        $markX = [int](($Width - $markSize) / 2)
        $markY = [int]($Height * 0.20)
        $background = [System.Drawing.SolidBrush]::new(
            [System.Drawing.Color]::FromArgb(70, 190, 255)
        )
        $graphics.FillEllipse($background, $markX, $markY, $markSize, $markSize)

        $fontSize = [Math]::Max(7, [Math]::Min(
            [int]($Height * 0.20),
            [int]($Width / [Math]::Max(4, $Label.Length) * 1.35)
        ))
        $font = [System.Drawing.Font]::new(
            'Segoe UI',
            [single]$fontSize,
            [System.Drawing.FontStyle]::Bold,
            [System.Drawing.GraphicsUnit]::Pixel
        )
        $brush = [System.Drawing.SolidBrush]::new(
            [System.Drawing.Color]::FromArgb(235, 245, 255)
        )
        $format = [System.Drawing.StringFormat]::new()
        $format.Alignment = [System.Drawing.StringAlignment]::Center
        $format.LineAlignment = [System.Drawing.StringAlignment]::Center

        $textArea = [System.Drawing.RectangleF]::new(
            2,
            [single]($Height * 0.28),
            [single]($Width - 4),
            [single]($Height * 0.55)
        )
        $graphics.DrawString($Label, $font, $brush, $textArea, $format)

        $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)

        # Verify the generated PNG can be reopened and has the requested dimensions.
        $check = [System.Drawing.Image]::FromFile($path)
        try {
            if ($check.Width -ne $Width -or $check.Height -ne $Height) {
                throw "Generated asset has unexpected dimensions: $Name"
            }
        }
        finally {
            $check.Dispose()
        }

        Write-Host "Generated $Name ($Width x $Height)"
    }
    finally {
        if ($format) { $format.Dispose() }
        if ($brush) { $brush.Dispose() }
        if ($background) { $background.Dispose() }
        if ($font) { $font.Dispose() }
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

# Required by the current single-package manifest. All seven app entries share
# the package's visual assets; their display names are defined in the manifest.
New-OnyxLogo -Name 'StoreLogo.png' -Width 50 -Height 50 -Label 'O'
New-OnyxLogo -Name 'Square150x150Logo.png' -Width 150 -Height 150 -Label 'ONYXOS'
New-OnyxLogo -Name 'Square44x44Logo.png' -Width 44 -Height 44 -Label 'O'
New-OnyxLogo -Name 'Wide310x150Logo.png' -Width 310 -Height 150 -Label 'ONYXOS'
New-OnyxLogo -Name 'SplashScreen.png' -Width 620 -Height 300 -Label 'ONYXOS'

Write-Host 'OnyxOS package assets generated successfully.'
