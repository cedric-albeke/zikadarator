param(
    [string] $OutputDir = "packaging/windows/assets"
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path -Parent $PSScriptRoot
$sourceLogo = Join-Path $repoRoot "assets/images/zikada-cicada-128.png"
$outputPath = Join-Path $repoRoot $OutputDir
$fontPath = Join-Path $repoRoot "assets/fonts/Anta-Regular.ttf"

New-Item -ItemType Directory -Force -Path $outputPath | Out-Null

$logo = [System.Drawing.Image]::FromFile($sourceLogo)
$fontCollection = New-Object System.Drawing.Text.PrivateFontCollection
$fontCollection.AddFontFile($fontPath)
$brandFont = $fontCollection.Families[0]

function New-BrandBitmap {
    param(
        [int] $Width,
        [int] $Height
    )

    $bitmap = New-Object System.Drawing.Bitmap($Width, $Height, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $bitmap.SetResolution(96, 96)
    return $bitmap
}

function New-BrandGraphics {
    param([System.Drawing.Bitmap] $Bitmap)

    $graphics = [System.Drawing.Graphics]::FromImage($Bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    return $graphics
}

function Save-Bitmap {
    param(
        [System.Drawing.Bitmap] $Bitmap,
        [string] $Name
    )

    $target = Join-Path $outputPath $Name
    $Bitmap.Save($target, [System.Drawing.Imaging.ImageFormat]::Bmp)
}

$background = [System.Drawing.ColorTranslator]::FromHtml("#001713")
$surface = [System.Drawing.ColorTranslator]::FromHtml("#02231D")
$green = [System.Drawing.ColorTranslator]::FromHtml("#00F5A0")
$cyan = [System.Drawing.ColorTranslator]::FromHtml("#16D9F4")
$magenta = [System.Drawing.ColorTranslator]::FromHtml("#D63BC6")
$white = [System.Drawing.ColorTranslator]::FromHtml("#F3FFF9")
$muted = [System.Drawing.ColorTranslator]::FromHtml("#7EA89A")

$large = New-BrandBitmap -Width 164 -Height 314
$largeGraphics = New-BrandGraphics -Bitmap $large
$largeGraphics.Clear($background)

$surfaceBrush = New-Object System.Drawing.SolidBrush($surface)
$largeGraphics.FillRectangle($surfaceBrush, 8, 8, 148, 298)

$scanlinePen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(28, $green), 1)
for ($y = 12; $y -lt 306; $y += 6) {
    $largeGraphics.DrawLine($scanlinePen, 9, $y, 155, $y)
}

$largeGraphics.DrawImage($logo, 21, 32, 122, 122)

$greenPen = New-Object System.Drawing.Pen($green, 2)
$cyanPen = New-Object System.Drawing.Pen($cyan, 1)
$magentaPen = New-Object System.Drawing.Pen($magenta, 1)
$largeGraphics.DrawLine($greenPen, 20, 174, 144, 174)
$largeGraphics.DrawLine($cyanPen, 20, 178, 104, 178)
$largeGraphics.DrawLine($magentaPen, 108, 178, 144, 178)

$centered = New-Object System.Drawing.StringFormat
$centered.Alignment = [System.Drawing.StringAlignment]::Center
$centered.LineAlignment = [System.Drawing.StringAlignment]::Center

$titleFont = New-Object System.Drawing.Font($brandFont, 15, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
$sublineFont = New-Object System.Drawing.Font($brandFont, 7, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
$metaFont = New-Object System.Drawing.Font($brandFont, 6, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
$whiteBrush = New-Object System.Drawing.SolidBrush($white)
$greenBrush = New-Object System.Drawing.SolidBrush($green)
$mutedBrush = New-Object System.Drawing.SolidBrush($muted)

$largeGraphics.DrawString("ZIKADARATOR", $titleFont, $whiteBrush, (New-Object System.Drawing.RectangleF(8, 190, 148, 28)), $centered)
$largeGraphics.DrawString("SEQUENCE THE SIGNAL", $sublineFont, $greenBrush, (New-Object System.Drawing.RectangleF(8, 219, 148, 16)), $centered)
$largeGraphics.DrawString("V1  /  WINDOWS X64", $metaFont, $mutedBrush, (New-Object System.Drawing.RectangleF(8, 278, 148, 14)), $centered)

Save-Bitmap -Bitmap $large -Name "wizard-large.bmp"

$small = New-BrandBitmap -Width 55 -Height 55
$smallGraphics = New-BrandGraphics -Bitmap $small
$smallGraphics.Clear($background)
$smallGraphics.FillRectangle($surfaceBrush, 2, 2, 51, 51)
$smallGraphics.DrawImage($logo, 4, 4, 47, 47)
$smallGraphics.DrawRectangle($greenPen, 1, 1, 52, 52)
Save-Bitmap -Bitmap $small -Name "wizard-small.bmp"

function New-ButtonBitmap {
    param(
        [string] $Name,
        [string] $Caption,
        [int] $Width,
        [bool] $Primary
    )

    $height = 36
    $bitmap = New-BrandBitmap -Width $Width -Height $height
    $graphics = New-BrandGraphics -Bitmap $bitmap
    if ($Primary) {
        $graphics.Clear($green)
        $textBrush = New-Object System.Drawing.SolidBrush($background)
        $borderPen = New-Object System.Drawing.Pen($cyan, 1)
    } else {
        $graphics.Clear($surface)
        $textBrush = New-Object System.Drawing.SolidBrush($white)
        $borderPen = New-Object System.Drawing.Pen($muted, 1)
    }

    $graphics.DrawRectangle($borderPen, 0, 0, $Width - 1, $height - 1)
    $font = New-Object System.Drawing.Font($brandFont, 10, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
    $graphics.DrawString($Caption, $font, $textBrush, (New-Object System.Drawing.RectangleF(0, 0, $Width, $height)), $centered)
    Save-Bitmap -Bitmap $bitmap -Name $Name

    $font.Dispose()
    $textBrush.Dispose()
    $borderPen.Dispose()
    $graphics.Dispose()
    $bitmap.Dispose()
}

New-ButtonBitmap -Name "button-back.bmp" -Caption "<  BACK" -Width 92 -Primary $false
New-ButtonBitmap -Name "button-next.bmp" -Caption "NEXT  >" -Width 112 -Primary $true
New-ButtonBitmap -Name "button-install.bmp" -Caption "INSTALL  >" -Width 132 -Primary $true
New-ButtonBitmap -Name "button-finish.bmp" -Caption "FINISH" -Width 112 -Primary $true
New-ButtonBitmap -Name "button-cancel.bmp" -Caption "CANCEL" -Width 92 -Primary $false
New-ButtonBitmap -Name "button-browse.bmp" -Caption "BROWSE" -Width 92 -Primary $false

$iconBitmap = New-Object System.Drawing.Bitmap(256, 256, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$iconGraphics = [System.Drawing.Graphics]::FromImage($iconBitmap)
$iconGraphics.Clear([System.Drawing.Color]::Transparent)
$iconGraphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$iconGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$iconGraphics.DrawImage($logo, 0, 0, 256, 256)

$pngStream = New-Object System.IO.MemoryStream
$iconBitmap.Save($pngStream, [System.Drawing.Imaging.ImageFormat]::Png)
$pngBytes = $pngStream.ToArray()
$iconPath = Join-Path $outputPath "setup-icon.ico"
$iconStream = [System.IO.File]::Create($iconPath)
$writer = New-Object System.IO.BinaryWriter($iconStream)
$writer.Write([uint16] 0)
$writer.Write([uint16] 1)
$writer.Write([uint16] 1)
$writer.Write([byte] 0)
$writer.Write([byte] 0)
$writer.Write([byte] 0)
$writer.Write([byte] 0)
$writer.Write([uint16] 1)
$writer.Write([uint16] 32)
$writer.Write([uint32] $pngBytes.Length)
$writer.Write([uint32] 22)
$writer.Write($pngBytes)
$writer.Dispose()

$iconGraphics.Dispose()
$iconBitmap.Dispose()
$pngStream.Dispose()
$largeGraphics.Dispose()
$large.Dispose()
$smallGraphics.Dispose()
$small.Dispose()
$titleFont.Dispose()
$sublineFont.Dispose()
$metaFont.Dispose()
$surfaceBrush.Dispose()
$whiteBrush.Dispose()
$greenBrush.Dispose()
$mutedBrush.Dispose()
$scanlinePen.Dispose()
$greenPen.Dispose()
$cyanPen.Dispose()
$magentaPen.Dispose()
$centered.Dispose()
$fontCollection.Dispose()
$logo.Dispose()

Write-Host "Windows installer branding generated in $outputPath"
