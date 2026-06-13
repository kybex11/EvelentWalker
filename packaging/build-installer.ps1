<#
    Builds the EvelentWalker Windows installer with Inno Setup.

    Steps:
      1. (Re)build & assemble the binaries via build-release.ps1.
      2. Locate ISCC.exe (Inno Setup compiler); offer to install it via winget
         if it is missing.
      3. Compile build\installer\EvelentWalker.iss into
         build\installer\output\EvelentWalker-Setup-<ver>.exe

    Usage:
        powershell -ExecutionPolicy Bypass -File build\build-installer.ps1
        powershell -ExecutionPolicy Bypass -File build\build-installer.ps1 -SkipBuild
#>

[CmdletBinding()]
param(
    [switch]$SkipBuild,
    [switch]$AutoInstallInnoSetup
)

$ErrorActionPreference = "Stop"
$IssFile = Join-Path $PSScriptRoot "installer\EvelentWalker.iss"

# 1) Build the binaries unless told to skip.
if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot "build-release.ps1")
    if ($LASTEXITCODE -ne 0) { throw "Release build failed." }
}

# 2) Find ISCC.exe.
function Find-ISCC {
    $cmd = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $candidates = @(
        "${env:ProgramFiles}\Inno Setup 7\ISCC.exe",
        "${env:ProgramFiles(x86)}\Inno Setup 7\ISCC.exe",
        "${env:ProgramFiles}\Inno Setup 6\ISCC.exe",
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
        "${env:LOCALAPPDATA}\Programs\Inno Setup 6\ISCC.exe",
        "${env:LOCALAPPDATA}\Programs\Inno Setup 7\ISCC.exe"
    )
    foreach ($c in $candidates) { if (Test-Path $c) { return $c } }
    return $null
}

$iscc = Find-ISCC

if (-not $iscc) {
    Write-Warning "Inno Setup compiler (ISCC.exe) was not found."
    if ($AutoInstallInnoSetup) {
        Write-Host "Installing Inno Setup via winget ..." -ForegroundColor Cyan
        & winget install --id JRSoftware.InnoSetup -e --accept-source-agreements --accept-package-agreements
        $iscc = Find-ISCC
    }
}

if (-not $iscc) {
    Write-Host ""
    Write-Host "Install Inno Setup 6, then re-run this script, e.g.:" -ForegroundColor Yellow
    Write-Host "    winget install JRSoftware.InnoSetup" -ForegroundColor Yellow
    Write-Host "    powershell -ExecutionPolicy Bypass -File build\build-installer.ps1 -SkipBuild" -ForegroundColor Yellow
    throw "ISCC.exe not available."
}

# 3) Compile the installer.
Write-Host "Using ISCC: $iscc" -ForegroundColor Cyan
& $iscc $IssFile
if ($LASTEXITCODE -ne 0) { throw "Inno Setup compilation failed." }

Write-Host ""
Write-Host "Installer created under: $(Join-Path $PSScriptRoot 'installer\output')" -ForegroundColor Green
