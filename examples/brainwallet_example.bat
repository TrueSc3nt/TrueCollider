@echo off
REM =============================================================================
REM Brainwallet — 3-word passphrases + mutations (SHA256, leet, caps, ...)
REM Wordlist: tests\brainwalletwords.txt
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\66.txt" (
  echo [E] Missing fixture: tests\66.txt
  exit /b 1
)
if not exist "tests\brainwalletwords.txt" (
  echo [E] Missing wordlist: tests\brainwalletwords.txt
  exit /b 1
)
set THREADS=4
echo [+] keyhunt.exe -m brainwallet -w 3 -f tests\66.txt -t %THREADS% -q -s 10
keyhunt.exe -m brainwallet -w 3 -f tests\66.txt -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
