@echo off
REM =============================================================================
REM BTC address search — puzzle #66 fixture (tests\66.txt)
REM Flags: -m address -b 66 -l compress -e -A auto
REM Edit THREADS / BITS as needed. Ctrl+C to stop.
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
set THREADS=8
echo [+] %EXE% -m address -f tests\66.txt -b 66 -l compress -e -A auto -t %THREADS% -q -s 10
"%EXE%" -m address -f tests\66.txt -b 66 -l compress -e -A auto -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
