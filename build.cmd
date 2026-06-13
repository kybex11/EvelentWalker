@echo off
REM ============================================================================
REM  EvelentWalker - one-click build
REM ----------------------------------------------------------------------------
REM  Builds every tool as a single packed .exe (Costura) and compiles the
REM  Windows installer (Start Menu shortcuts + uninstaller).
REM
REM  Usage:
REM      build.cmd            Build binaries + installer
REM      build.cmd bin        Build the packed binaries only (no installer)
REM ============================================================================
setlocal
cd /d "%~dp0"

if /I "%~1"=="bin" (
    echo Building packed binaries only...
    powershell -NoProfile -ExecutionPolicy Bypass -File "packaging\build-release.ps1"
    goto :done
)

echo Building packed binaries + installer...
powershell -NoProfile -ExecutionPolicy Bypass -File "packaging\build-installer.ps1" -AutoInstallInnoSetup

:done
if errorlevel 1 (
    echo.
    echo *** BUILD FAILED ***
    exit /b 1
)
echo.
echo *** BUILD COMPLETE ***
echo   Binaries : packaging\dist
echo   Installer: packaging\installer\output
endlocal
