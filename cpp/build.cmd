@echo off
REM Build script for the EvelentWalker C++ port (Windows / MSVC).
REM Usage: build.cmd [Debug|Release]   (default: Debug)

setlocal
set CONFIG=%1
if "%CONFIG%"=="" set CONFIG=Debug

set SCRIPT_DIR=%~dp0
set BUILD_DIR=%SCRIPT_DIR%build

echo === Configuring (%CONFIG%) ===
cmake -S "%SCRIPT_DIR%." -B "%BUILD_DIR%"
if errorlevel 1 goto :fail

echo === Building (%CONFIG%) ===
cmake --build "%BUILD_DIR%" --config %CONFIG%
if errorlevel 1 goto :fail

echo === Running tests (%CONFIG%) ===
ctest --test-dir "%BUILD_DIR%" -C %CONFIG% --output-on-failure
if errorlevel 1 goto :fail

echo.
echo BUILD OK
endlocal
exit /b 0

:fail
echo.
echo BUILD FAILED
endlocal
exit /b 1
