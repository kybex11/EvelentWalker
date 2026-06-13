Add-Type -AssemblyName System.Drawing

# =============================================================================
#  EvelentWalker icon generator
# -----------------------------------------------------------------------------
#  Renders the branded "E" icon and writes it to *every* .ico file used by the
#  solution (each tool project embeds its own copy via <ApplicationIcon>, plus
#  the runtime copies under EvelentWalker\Resources). Each tool gets a distinct
#  accent colour and a small corner badge so they stay recognisable while
#  sharing one consistent look.
#
#  Run after changing the artwork, then rebuild so the new icon is embedded:
#      powershell -ExecutionPolicy Bypass -File make_icon.ps1
# =============================================================================

function New-EvelentIcon {
    param(
        [Parameter(Mandatory)] [string]   $OutPath,
        [Parameter(Mandatory)] [System.Drawing.Color] $Accent,
        [string] $Tag = $null,
        [int[]]  $Sizes = @(16, 32, 48, 64, 128, 256)
    )

    $deep = [System.Drawing.Color]::FromArgb(255, 28, 36, 46)   # deep slate
    $pngStreams = @()

    foreach ($size in $Sizes) {
        $bmp = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
        $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAlias
        $g.Clear([System.Drawing.Color]::Transparent)

        # Rounded rectangle background with a diagonal accent -> slate gradient.
        $pad = [Math]::Max(1, [int]($size * 0.06))
        $rectW = $size - 2 * $pad
        $rectH = $size - 2 * $pad
        $radius = [int]($size * 0.22)

        $path = New-Object System.Drawing.Drawing2D.GraphicsPath
        $d = $radius * 2
        if ($d -lt 1) { $d = 1 }
        $path.AddArc($pad, $pad, $d, $d, 180, 90)
        $path.AddArc($pad + $rectW - $d, $pad, $d, $d, 270, 90)
        $path.AddArc($pad + $rectW - $d, $pad + $rectH - $d, $d, $d, 0, 90)
        $path.AddArc($pad, $pad + $rectH - $d, $d, $d, 90, 90)
        $path.CloseFigure()

        $rect = New-Object System.Drawing.Rectangle($pad, $pad, $rectW, $rectH)
        $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, $Accent, $deep, 55.0)
        $g.FillPath($brush, $path)

        # Subtle inner border.
        $penW = [Math]::Max(1.0, $size * 0.015)
        $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(90, 255, 255, 255), $penW)
        $g.DrawPath($pen, $path)

        # Main "E" letter.
        $fontSize = $size * 0.62
        $font = New-Object System.Drawing.Font('Segoe UI', $fontSize, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
        $sf = New-Object System.Drawing.StringFormat
        $sf.Alignment = [System.Drawing.StringAlignment]::Center
        $sf.LineAlignment = [System.Drawing.StringAlignment]::Center
        $layout = New-Object System.Drawing.RectangleF(0, [single](-$size * 0.03), [single]$size, [single]$size)

        $shadow = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(110, 0, 0, 0))
        $shOff = [Math]::Max(1.0, $size * 0.02)
        $layoutSh = New-Object System.Drawing.RectangleF([single]$shOff, [single](-$size * 0.03 + $shOff), [single]$size, [single]$size)
        $g.DrawString('E', $font, $shadow, $layoutSh, $sf)

        $textBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 245, 252, 250))
        $g.DrawString('E', $font, $textBrush, $layout, $sf)

        # Per-tool corner badge (only big enough to read at >= 32px).
        if ($Tag -and $size -ge 32) {
            $bd = [single]($size * 0.46)
            $bx = [single]($size - $bd - $pad)
            $by = [single]($size - $bd - $pad)
            $badgeRect = New-Object System.Drawing.RectangleF($bx, $by, $bd, $bd)

            $badgeFill = New-Object System.Drawing.SolidBrush($Accent)
            $g.FillEllipse($badgeFill, $badgeRect)
            $badgePenW = [Math]::Max(1.0, $size * 0.02)
            $badgePen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 250, 252, 255), $badgePenW)
            $g.DrawEllipse($badgePen, $badgeRect)

            $tagFont = New-Object System.Drawing.Font('Segoe UI', [single]($bd * 0.68), [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
            $tagBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 18, 22, 30))
            $g.DrawString($Tag, $tagFont, $tagBrush, $badgeRect, $sf)
            $badgeFill.Dispose(); $badgePen.Dispose(); $tagFont.Dispose(); $tagBrush.Dispose()
        }

        $g.Dispose()

        $ms = New-Object System.IO.MemoryStream
        $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        $bmp.Dispose()
        $pngStreams += , ($ms.ToArray())
        $brush.Dispose(); $pen.Dispose(); $font.Dispose(); $shadow.Dispose(); $textBrush.Dispose()
    }

    # Assemble the .ico (PNG-compressed entries).
    $dir = Split-Path -Parent $OutPath
    if ($dir -and -not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir | Out-Null }

    $fs = New-Object System.IO.FileStream($OutPath, [System.IO.FileMode]::Create)
    $bw = New-Object System.IO.BinaryWriter($fs)

    [void]$bw.Write([UInt16]0)              # reserved
    [void]$bw.Write([UInt16]1)              # type = icon
    [void]$bw.Write([UInt16]$Sizes.Count)

    $offset = 6 + (16 * $Sizes.Count)
    for ($i = 0; $i -lt $Sizes.Count; $i++) {
        $size = $Sizes[$i]
        $data = $pngStreams[$i]
        $bDim = if ($size -ge 256) { 0 } else { $size }
        [void]$bw.Write([Byte]$bDim)        # width
        [void]$bw.Write([Byte]$bDim)        # height
        [void]$bw.Write([Byte]0)            # color count
        [void]$bw.Write([Byte]0)            # reserved
        [void]$bw.Write([UInt16]1)          # color planes
        [void]$bw.Write([UInt16]32)         # bits per pixel
        [void]$bw.Write([UInt32]$data.Length)
        [void]$bw.Write([UInt32]$offset)
        $offset += $data.Length
    }
    foreach ($data in $pngStreams) { [void]$bw.Write($data) }
    $bw.Flush(); $bw.Close(); $fs.Close()

    Write-Host "  wrote $OutPath"
}

# ---- Tool palette (accent colour + corner badge) ---------------------------
$teal   = [System.Drawing.Color]::FromArgb(255, 64, 200, 175)   # main app
$indigo = [System.Drawing.Color]::FromArgb(255, 122, 162, 247)  # RPF Explorer
$amber  = [System.Drawing.Color]::FromArgb(255, 245, 180, 80)   # Vehicles
$green  = [System.Drawing.Color]::FromArgb(255, 120, 210, 130)  # Peds

$root = $PSScriptRoot

# Each entry: relative path, accent, optional badge letter, optional size set.
$targets = @(
    # Main app + tools that share CW.ico (base "E", no badge).
    @{ Path = 'EvelentWalker\CW.ico';                        Accent = $teal },
    @{ Path = 'EvelentWalker.Gen9Converter\CW.ico';          Accent = $teal },
    @{ Path = 'EvelentWalker.ModManager\CW.ico';             Accent = $teal },
    @{ Path = 'EvelentWalker.ErrorReport\CW.ico';            Accent = $teal },
    @{ Path = 'EvelentWalker\Resources\CW.ico';              Accent = $teal },
    @{ Path = 'EvelentWalker\Resources\CW16.ico';            Accent = $teal; Sizes = @(16) },

    # RPF Explorer.
    @{ Path = 'EvelentWalker.RPFExplorer\CWRPFExplorer.ico'; Accent = $indigo; Tag = 'R' },
    @{ Path = 'EvelentWalker\Resources\CWRPFExplorer.ico';   Accent = $indigo; Tag = 'R' },

    # Vehicle Viewer.
    @{ Path = 'EvelentWalker.Vehicles\CWVehicles.ico';       Accent = $amber;  Tag = 'V' },
    @{ Path = 'EvelentWalker\Resources\CWVehicles.ico';      Accent = $amber;  Tag = 'V' },

    # Ped Viewer.
    @{ Path = 'EvelentWalker.Peds\CWPeds.ico';               Accent = $green;  Tag = 'P' },
    @{ Path = 'EvelentWalker\Resources\CWPeds.ico';          Accent = $green;  Tag = 'P' }
)

Write-Host "Generating EvelentWalker icons..."
foreach ($t in $targets) {
    $out = Join-Path $root $t.Path
    $args = @{ OutPath = $out; Accent = $t.Accent }
    if ($t.ContainsKey('Tag'))   { $args.Tag = $t.Tag }
    if ($t.ContainsKey('Sizes')) { $args.Sizes = $t.Sizes }
    New-EvelentIcon @args
}
Write-Host "Done. Rebuild the solution to embed the updated icons."
