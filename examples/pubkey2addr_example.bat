@echo off
REM =============================================================================
REM pubkey2addr — random key → address (defaults to -x random / -x auto)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\66.txt" (
  echo [E] Missing fixture: tests\66.txt
  exit /b 1
)
set THREADS=4
echo [+] keyhunt.exe -m pubkey2addr -f tests\66.txt -x auto -t %THREADS% -q -s 10
keyhunt.exe -m pubkey2addr -f tests\66.txt -x auto -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
