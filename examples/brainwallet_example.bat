@echo off
REM =============================================================================
REM Brainwallet — 3-word passphrases + mutations (SHA256, leet, caps, ...)
REM Wordlist: tests\brainwalletwords.txt
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
set THREADS=4
echo [+] %EXE% -m brainwallet -w 3 -f tests\66.txt -t %THREADS% -q -s 10
"%EXE%" -m brainwallet -w 3 -f tests\66.txt -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
