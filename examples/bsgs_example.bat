@echo off
REM =============================================================================
REM BSGS example — puzzle-style pubkey tests\125.txt
REM First prints a dry-run (-y) plan, then starts a moderated search.
REM Prefer -k powers of 2; use -S to cache blooms on disk.
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\125.txt" (
  echo [E] Missing fixture: tests\125.txt
  exit /b 1
)
echo [+] Dry-run: resolve -k auto / memory tips
keyhunt.exe -m bsgs -f tests\125.txt -b 125 -k auto -y
if errorlevel 1 exit /b %ERRORLEVEL%
echo.
echo [+] Starting BSGS (Ctrl+C to stop). Adjust -k / -t for your RAM.
keyhunt.exe -m bsgs -f tests\125.txt -b 125 -R -k 512 -t 4 -S -q -s 10
exit /b %ERRORLEVEL%
