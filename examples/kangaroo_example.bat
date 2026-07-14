@echo off
REM =============================================================================
REM Kangaroo (CPU) — tiny range demo against tests\125.txt
REM Ranges <= 2^24 use sequential EC walk; larger use DP kangaroo.
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
echo [+] %EXE% -m kangaroo -f tests\125.txt -r 1:100000
"%EXE%" -m kangaroo -f tests\125.txt -r 1:100000
exit /b %ERRORLEVEL%
