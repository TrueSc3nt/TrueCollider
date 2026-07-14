@echo off
REM =============================================================================
REM BTC vanity prefix search — no -f file; prefix via -v
REM -e enables GLV endomorphism on CPU.
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
set THREADS=8
set PREFIX=1Cool
echo [+] %EXE% -m vanity -v %PREFIX% -e -t %THREADS% -q -s 10
"%EXE%" -m vanity -v %PREFIX% -e -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
