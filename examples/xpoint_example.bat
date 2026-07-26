@echo off
REM =============================================================================
REM X-coordinate match — tests\_xpoint_g.txt
REM Input: 64-char x-only, 66-char compressed, or 130-char uncompressed pubkeys
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\_xpoint_g.txt" (
  echo [E] Missing fixture: tests\_xpoint_g.txt
  exit /b 1
)
set THREADS=8
echo [+] keyhunt.exe -m xpoint -f tests\_xpoint_g.txt -t %THREADS% -q -s 10
keyhunt.exe -m xpoint -f tests\_xpoint_g.txt -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
