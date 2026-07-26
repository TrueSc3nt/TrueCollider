@echo off
REM Usage (from repo root): call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat"
REM Exits 1 if missing or 0-byte.
setlocal EnableExtensions EnableDelayedExpansion
set "EXE=%~1"
set "HINT=%~2"
if "%EXE%"=="" (
  echo [E] _check_exe.bat: missing exe name
  exit /b 1
)
if "%HINT%"=="" set "HINT=build.bat"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Build with %HINT%
  exit /b 1
)
for %%F in ("%EXE%") do set "SZ=%%~zF"
if "!SZ!"=="0" (
  echo [E] %EXE% is 0 bytes ^(corrupt^). Rebuild with %HINT%
  exit /b 1
)
exit /b 0
