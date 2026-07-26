@echo off
REM =============================================================================
REM Bitcoin minikeys (S...) against tests\66.txt
REM Optional: -C SRPqx8QiwnW4WNWnTVa2W5 for a fixed 22-char base
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\66.txt" (
  echo [E] Missing fixture: tests\66.txt
  exit /b 1
)
set THREADS=4
echo [+] keyhunt.exe -m minikeys -f tests\66.txt -t %THREADS% -q -s 10
keyhunt.exe -m minikeys -f tests\66.txt -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
