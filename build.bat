@echo off
REM TrueCollider Windows build script
REM Works with MSYS2 MinGW, Cygwin, or MSVC

where g++ >nul 2>&1
if %errorlevel%==0 (
    echo Building with g++...
    make
) else (
    echo g++ not found. Install MSYS2 from https://www.msys2.org/
    echo Or run from MSYS2 shell: make
    exit /b 1
)
