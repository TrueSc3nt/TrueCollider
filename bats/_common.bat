@echo off
REM bats\_common.bat - cd to repo root, require CPU or CUDA exe
REM Usage: call "%~dp0..\_common.bat" cpu
REM    or: call "%~dp0..\_common.bat" cuda
setlocal EnableExtensions EnableDelayedExpansion
set "TC_ROOT=%~dp0.."
cd /d "%TC_ROOT%" || exit /b 1

set "NEED=%~1"
if /I "%NEED%"=="" set "NEED=cpu"

if /I "%NEED%"=="cuda" (
  set "EXE=keyhunt_cuda.exe"
  set "HINT=bats\00_build\build_cuda.bat"
) else (
  set "EXE=keyhunt.exe"
  set "HINT=bats\00_build\build_cpu.bat"
)

if not exist "%EXE%" (
  echo [E] %EXE% not found. Build with %HINT%
  endlocal & exit /b 1
)
for %%F in ("%EXE%") do set "SZ=%%~zF"
if "!SZ!"=="0" (
  echo [E] %EXE% is 0 bytes ^(corrupt^). Rebuild with %HINT%
  endlocal & exit /b 1
)
endlocal & (
  set "TC_EXE=%EXE%"
  cd /d "%TC_ROOT%"
)
exit /b 0
