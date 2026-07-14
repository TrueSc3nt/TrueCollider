@echo off
REM =============================================================================
REM Bitcoin minikeys (S...) against tests\66.txt
REM Optional: -C SRPqx8QiwnW4WNWnTVa2W5 for a fixed 22-char base
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
set THREADS=4
echo [+] %EXE% -m minikeys -f tests\66.txt -t %THREADS% -q -s 10
"%EXE%" -m minikeys -f tests\66.txt -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
