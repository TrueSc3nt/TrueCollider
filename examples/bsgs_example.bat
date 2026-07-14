@echo off
REM =============================================================================
REM BSGS example — puzzle-style pubkey tests\125.txt
REM First prints a dry-run (-y) plan, then starts a moderated search.
REM Prefer -k powers of 2; use -S to cache blooms on disk.
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
echo [+] Dry-run: resolve -k auto / memory tips
"%EXE%" -m bsgs -f tests\125.txt -b 125 -k auto -y
echo.
echo [+] Starting BSGS (Ctrl+C to stop). Adjust -k / -t for your RAM.
"%EXE%" -m bsgs -f tests\125.txt -b 125 -R -k 512 -t 4 -S -q -s 10
exit /b %ERRORLEVEL%
