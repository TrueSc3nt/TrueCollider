@echo off
REM TrueCollider Windows build script
REM
REM Options to build on Windows:
REM 1. Use WSL (recommended): run `bash build_windows.sh` inside WSL.
REM 2. Use MSYS2/MinGW: open MSYS2 shell and run `make`.
REM 3. Use this batch from a MinGW environment where g++ and make are in PATH.

where g++ >nul 2>&1
if %errorlevel% neq 0 (
    echo [E] g++ not found in PATH.
    echo.
    echo Recommended: Use WSL with Ubuntu and run:
    echo   bash build_windows.sh
    echo.
    echo Alternatively, install MSYS2 from https://www.msys2.org/,
    echo open the MSYS2 MinGW 64-bit shell, and run:
    echo   make
    echo.
    exit /b 1
)

where make >nul 2>&1
if %errorlevel% neq 0 (
    echo [E] make not found in PATH.
    echo Please run this from an MSYS2/MinGW shell, or use WSL:
    echo   bash build_windows.sh
    exit /b 1
)

echo [+] Building with g++...
make
if %errorlevel% neq 0 (
    echo [E] Build failed.
    exit /b 1
)

echo [+] Build complete.
if exist keyhunt.exe (
    echo Output: keyhunt.exe
) else (
    echo Output: keyhunt
)
