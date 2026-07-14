@echo off
REM TrueCollider Windows CPU build entrypoint
REM Tries native MinGW first, then asks for WSL / MSYS2 if missing.
setlocal EnableExtensions
cd /d "%~dp0"

if exist "%~dp0build_mingw_native.bat" (
  echo [+] Trying build_mingw_native.bat ...
  call "%~dp0build_mingw_native.bat"
  if not errorlevel 1 exit /b 0
)

where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo [E] g++ not found in PATH and MinGW native build failed.
    echo.
    echo Recommended:
    echo   1. Install MSYS2 from https://www.msys2.org/
    echo      pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
    echo      then run: build_mingw_native.bat
    echo   2. Or use WSL: bash build_windows.sh
    echo.
    exit /b 1
)

where make >nul 2>&1
if %errorlevel% neq 0 (
    where mingw32-make >nul 2>&1
    if %errorlevel% neq 0 (
        echo [E] make / mingw32-make not found in PATH.
        echo Please run build_mingw_native.bat from an MSYS2-equipped machine,
        echo or use WSL: bash build_windows.sh
        exit /b 1
    )
    set MAKE=mingw32-make
) else (
    set MAKE=make
)

echo [+] Building with g++ via %MAKE%...
%MAKE% TARGET=keyhunt.exe OS=MINGW64 ARCH=x86_64
if %errorlevel% neq 0 (
    echo [E] Build failed.
    exit /b 1
)

echo [+] Build complete.
if exist keyhunt.exe (
    echo Output: keyhunt.exe
    echo PASS: CPU build
) else (
    echo Output: keyhunt
)
endlocal
exit /b 0
