@echo off
REM TrueCollider run helper for Windows
REM Prefers a native keyhunt.exe, then falls back to WSL.

title TrueCollider Search Modes
color 0A
echo =============================================================
echo  TrueCollider Search Modes
echo =============================================================
echo.

set "SCRIPT_DIR=%~dp0"

REM 1. Use native keyhunt.exe if it exists
if exist "%SCRIPT_DIR%keyhunt.exe" (
    "%SCRIPT_DIR%keyhunt.exe" %*
    exit /b %errorlevel%
)

REM 2. Fall back to WSL keyhunt
where wsl >nul 2>&1
if %errorlevel% neq 0 (
    echo [E] keyhunt.exe not found and WSL is not available.
    echo.
    echo Build a native .exe with WSL:
    echo   bash build_windows.sh
    echo.
    exit /b 1
)

set "WSL_DIR=%SCRIPT_DIR:\=/%"
set "WSL_DIR=/mnt/c/%WSL_DIR:~3%"

if "%~1"=="" (
    wsl -- bash -c "cd '%WSL_DIR%' && ./keyhunt -h"
) else (
    wsl -- bash -c "cd '%WSL_DIR%' && ./keyhunt %*"
)
