@echo off
REM =============================================================================
REM Poetry mode — uses tests\poetry.txt wordlist internally
REM Encoding: poetry words → hex private key → address vs targets
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
set THREADS=4
echo [+] %EXE% -m poetry -f tests\66.txt -t %THREADS% -q -s 10
"%EXE%" -m poetry -f tests\66.txt -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
