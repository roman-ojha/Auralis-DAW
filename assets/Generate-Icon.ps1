# Rebuild the original Auralis orbit logo; requires Windows System.Drawing.
Add-Type -AssemblyName System.Drawing
$size = 512
$bitmap = [System.Drawing.Bitmap]::new($size, $size)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$background = [System.Drawing.ColorTranslator]::FromHtml('#101419')
$mint = [System.Drawing.ColorTranslator]::FromHtml('#8ee3c2')
$pen = [System.Drawing.Pen]::new($mint, 24)
$brush = [System.Drawing.SolidBrush]::new($mint)
try {
    $graphics.Clear($background)
    $graphics.DrawEllipse($pen, 102, 102, 308, 308)
    $graphics.DrawLine($pen, 78, 434, 434, 78)
    $graphics.FillEllipse($brush, 208, 208, 96, 96)
    $bitmap.Save((Join-Path $PSScriptRoot 'auralis-icon.png'), [System.Drawing.Imaging.ImageFormat]::Png)
} finally {
    $brush.Dispose()
    $pen.Dispose()
    $graphics.Dispose()
    $bitmap.Dispose()
}
