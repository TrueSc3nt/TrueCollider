@echo off
REM Demo: tiny Solana range (seed 1 is in tests\sol_sample.txt)
cd /d "%~dp0"
if not exist keyhunt.exe (
  echo keyhunt.exe not found. Build first: build_mingw_native.bat
  pause
  exit /b 1
)
keyhunt.exe -m address -c sol -f tests\sol_sample.txt -r 1:8 -t 1 -q
echo.
echo Done. Check KEYFOUNDKEYFOUND.txt if a hit was printed.
pause
