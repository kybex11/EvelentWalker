<#
    EvelentWalker - Release build & packaging script
    -------------------------------------------------
    Builds every executable in Release (x64), each packed by Costura.Fody into a
    single self-contained .exe (no loose dependency DLLs), and assembles a clean
    distribution folder under  build\dist  ready for the installer.

    Usage:
        powershell -ExecutionPolicy Bypass -File build\build-release.ps1
#>

[CmdletBinding()]
param(
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

# Repo root = parent of this script's folder.
$RepoRoot = Split-Path -Parent $PSScriptRoot
$DistDir  = Join-Path $PSScriptRoot "dist"

Write-Host "Repo root : $RepoRoot"
Write-Host "Dist dir  : $DistDir"
Write-Host ""

# Executable projects to build & ship.
$ExeProjects = @(
    "EvelentWalker\EvelentWalker.csproj",
    "EvelentWalker.RPFExplorer\EvelentWalker.RPFExplorer.csproj",
    "EvelentWalker.Vehicles\EvelentWalker.Vehicles.csproj",
    "EvelentWalker.Peds\EvelentWalker.Peds.csproj",
    "EvelentWalker.Gen9Converter\EvelentWalker.Gen9Converter.csproj",
    "EvelentWalker.ModManager\EvelentWalker.ModManager.csproj",
    "EvelentWalker.ErrorReport\EvelentWalker.ErrorReport.csproj"
)

# 1) Build each executable project.
foreach ($proj in $ExeProjects) {
    $full = Join-Path $RepoRoot $proj
    Write-Host "==> Building $proj ..." -ForegroundColor Cyan
    & dotnet build $full -c $Configuration -v m
    if ($LASTEXITCODE -ne 0) { throw "Build failed for $proj" }
}

# 2) Reset the dist folder contents (keep the folder itself, so an open
#    Explorer window or shell cwd can't block the build).
if (Test-Path $DistDir) {
    Get-ChildItem -LiteralPath $DistDir -Force | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
} else {
    New-Item -ItemType Directory -Path $DistDir | Out-Null
}

# 3) Collect the packed exe (+ .config) from each project output.
foreach ($proj in $ExeProjects) {
    $projDir = Join-Path $RepoRoot (Split-Path -Parent $proj)
    $outDir  = Join-Path $projDir "bin\$Configuration\net48"
    if (-not (Test-Path $outDir)) { throw "Output not found: $outDir" }

    Get-ChildItem $outDir -Filter *.exe | ForEach-Object {
        Copy-Item $_.FullName -Destination $DistDir -Force
        $cfg = "$($_.FullName).config"
        if (Test-Path $cfg) { Copy-Item $cfg -Destination $DistDir -Force }
        Write-Host "    packed: $($_.Name)" -ForegroundColor Green
    }
}

# 4) Copy shared runtime data once (taken from the main app output).
$mainOut = Join-Path $RepoRoot "EvelentWalker\bin\$Configuration\net48"

foreach ($folder in @("Shaders", "icons")) {
    $src = Join-Path $mainOut $folder
    if (Test-Path $src) {
        Copy-Item $src -Destination (Join-Path $DistDir $folder) -Recurse -Force
        Write-Host "    data folder: $folder" -ForegroundColor Green
    }
}

foreach ($file in @("strings.txt", "ShadersGen9Conversion.xml")) {
    $src = Join-Path $mainOut $file
    if (Test-Path $src) {
        Copy-Item $src -Destination $DistDir -Force
        Write-Host "    data file: $file" -ForegroundColor Green
    }
}

# 5) ModManager ini, if present.
$mmIni = Join-Path $RepoRoot "EvelentWalker.ModManager\bin\$Configuration\net48\EvelentWalker.ModManager.ini"
if (Test-Path $mmIni) { Copy-Item $mmIni -Destination $DistDir -Force }

Write-Host ""
Write-Host "Distribution assembled at: $DistDir" -ForegroundColor Yellow
Get-ChildItem $DistDir | Sort-Object Name | Format-Table Name, Length -AutoSize
