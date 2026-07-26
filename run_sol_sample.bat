@echo off
REM Demo: tiny Solana range (seed 1 is in tests\sol_sample.txt) — guaranteed quick hit.
REM Open-ended Sol scan: examples\search_sol.bat
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

if not exist keyhunt.exe (
  echo [E] keyhunt.exe not found. Build first: build_mingw_native.bat
  pause
  exit /b 1
)
for %%F in (keyhunt.exe) do set "SZ=%%~zF"
if "!SZ!"=="0" (
  echo [E] keyhunt.exe is 0 bytes ^(corrupt^). Rebuild with build_mingw_native.bat
  pause
  exit /b 1
)
if not exist "tests\sol_sample.txt" (
  echo [E] Missing fixture: tests\sol_sample.txt
  pause
  exit /b 1
)

echo [+] keyhunt.exe -m address -c sol -f tests\sol_sample.txt -r 1:8 -t 1 -q
keyhunt.exe -m address -c sol -f tests\sol_sample.txt -r 1:8 -t 1 -q
echo.
echo Done. Check FOUND_SOL.txt / KEYFOUNDKEYFOUND.txt if a hit was printed.
pause
exit /b %ERRORLEVEL%
