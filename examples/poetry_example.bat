@echo off
REM =============================================================================
REM Poetry mode — uses tests\poetry.txt wordlist internally
REM Encoding: poetry words → hex private key → address vs targets
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\66.txt" (
  echo [E] Missing fixture: tests\66.txt
  exit /b 1
)
if not exist "tests\poetry.txt" (
  echo [E] Missing wordlist: tests\poetry.txt
  exit /b 1
)
set THREADS=4
echo [+] keyhunt.exe -m poetry -f tests\66.txt -t %THREADS% -q -s 10
keyhunt.exe -m poetry -f tests\66.txt -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
