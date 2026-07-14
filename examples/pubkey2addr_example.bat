@echo off
REM =============================================================================
REM pubkey2addr — random key → address (defaults to -x random)
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
set THREADS=4
echo [+] %EXE% -m pubkey2addr -f tests\66.txt -x auto -t %THREADS% -q -s 10
"%EXE%" -m pubkey2addr -f tests\66.txt -x auto -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
