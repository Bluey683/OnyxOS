$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$assets = Join-Path $PSScriptRoot '..\Assets'
New-Item -ItemType Directory -Force -Path $assets | Out-Null

function New-OnyxLogo([string]$Name, [int]$Width, [int]$Height) {
    $path = Join-Path $assets $Name
    $bmp = [System.Drawing.Bitmap]::new($Width, $Height)
    $graphics = [System.Drawing.Graphics]::FromImage($bmp)
    $graphics.Clear([System.Drawing.Color]::FromArgb(16, 24, 32))
    $font = [System.Drawing.Font]::new('Segoe UI', [Math]::Max(12, [int]($Height * 0.22)), [System.Drawing.FontStyle]::Bold)
    $brush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(70, 190, 255))
    $format = [System.Drawing.StringFormat]::new()
    $format.Alignment = [System.Drawing.StringAlignment]::Center
    $format.LineAlignment = [System.Drawing.StringAlignment]::Center
    $graphics.DrawString('ONYX', $font, $brush, [System.Drawing.RectangleF]::new(0, 0, $Width, $Height), $format)
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $format.Dispose(); $brush.Dispose(); $font.Dispose(); $graphics.Dispose(); $bmp.Dispose()
}
New-OnyxLogo 'StoreLogo.png' 50 50
New-OnyxLogo 'Square150x150Logo.png' 150 150
New-OnyxLogo 'Square44x44Logo.png' 44 44
New-OnyxLogo 'Wide310x150Logo.png' 310 150
New-OnyxLogo 'SplashScreen.png' 620 300
