@echo off
REM TrueCollider run helper for Windows
REM Prefers native keyhunt.exe; if args include -U, prefers keyhunt_cuda.exe.
REM Falls back to WSL ./keyhunt when no native binary is available.

setlocal EnableExtensions EnableDelayedExpansion
title TrueCollider
echo =============================================================
echo  TrueCollider
echo =============================================================
echo.

set "SCRIPT_DIR=%~dp0"
set "WANT_CUDA=0"
echo %*| findstr /I /C:"-U" >nul && set "WANT_CUDA=1"

REM 1. CUDA binary when -U is requested
if "!WANT_CUDA!"=="1" (
  if exist "!SCRIPT_DIR!keyhunt_cuda.exe" (
    for %%F in ("!SCRIPT_DIR!keyhunt_cuda.exe") do set "KH_SIZE=%%~zF"
    if not "!KH_SIZE!"=="0" (
      "!SCRIPT_DIR!keyhunt_cuda.exe" %*
      exit /b %errorlevel%
    )
    echo [E] keyhunt_cuda.exe is 0 bytes ^(corrupt^). Rebuild with build_cuda_vs2022.bat
    exit /b 1
  )
  echo [E] -U requires keyhunt_cuda.exe. Build with build_cuda_vs2022.bat
  exit /b 1
)

REM 2. Native CPU keyhunt.exe
set "KH_SIZE="
if exist "!SCRIPT_DIR!keyhunt.exe" (
  for %%F in ("!SCRIPT_DIR!keyhunt.exe") do set "KH_SIZE=%%~zF"
)
if defined KH_SIZE if not "!KH_SIZE!"=="0" (
  "!SCRIPT_DIR!keyhunt.exe" %*
  exit /b %errorlevel%
)
if exist "!SCRIPT_DIR!keyhunt.exe" if "!KH_SIZE!"=="0" (
  echo [E] keyhunt.exe is 0 bytes ^(corrupt^). Rebuild with build.bat / build_mingw_native.bat
  exit /b 1
)

REM 3. Fall back to WSL keyhunt (drive letter aware — works on C: and D:)
where wsl >nul 2>&1
if errorlevel 1 (
  echo [E] keyhunt.exe not found and WSL is not available.
  echo.
  echo Build a native .exe:
  echo   build.bat
  echo   REM or: build_mingw_native.bat
  echo   REM CUDA: build_cuda_vs2022.bat -^> keyhunt_cuda.exe
  echo.
  exit /b 1
)

set "DRIVE_LETTER=%SCRIPT_DIR:~0,1%"
for /f %%i in ('powershell -NoProfile -Command "'%DRIVE_LETTER%'.ToLower()"') do set "DRIVE_LETTER=%%i"
set "WSL_PATH=%SCRIPT_DIR:~3%"
set "WSL_PATH=%WSL_PATH:\=/%"
set "WSL_DIR=/mnt/%DRIVE_LETTER%/%WSL_PATH%"

if "%~1"=="" (
  wsl -- bash -c "cd '%WSL_DIR%' && ./keyhunt -h"
) else (
  wsl -- bash -c "cd '%WSL_DIR%' && ./keyhunt %*"
)
exit /b %errorlevel%
