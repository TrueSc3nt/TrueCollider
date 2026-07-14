@echo off
REM =============================================================================
REM X-coordinate match — tests\_xpoint_g.txt
REM Input: 64-char x-only, 66-char compressed, or 130-char uncompressed pubkeys
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
set THREADS=8
echo [+] %EXE% -m xpoint -f tests\_xpoint_g.txt -t %THREADS% -q -s 10
"%EXE%" -m xpoint -f tests\_xpoint_g.txt -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
