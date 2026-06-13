<#
.SYNOPSIS
    Build script for the EvelentWalker C++ port (PowerShell / MSVC).
.PARAMETER Config
    Build configuration: Debug (default) or Release.
.EXAMPLE
    ./build.ps1 -Config Release
#>
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Config = 'Debug'
)

$ErrorActionPreference = 'Stop'
$scriptDir = $PSScriptRoot
$buildDir = Join-Path $scriptDir 'build'

Write-Host "=== Configuring ($Config) ===" -ForegroundColor Cyan
cmake -S $scriptDir -B $buildDir
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }

Write-Host "=== Building ($Config) ===" -ForegroundColor Cyan
cmake --build $buildDir --config $Config
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }

Write-Host "=== Running tests ($Config) ===" -ForegroundColor Cyan
ctest --test-dir $buildDir -C $Config --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }

Write-Host ''
Write-Host 'BUILD OK' -ForegroundColor Green
