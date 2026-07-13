@echo off
REM Example Bitcoin address search (edit TARGETS= below)
cd /d "%~dp0"
if not exist keyhunt.exe (
  echo keyhunt.exe not found. Build first: build_mingw_native.bat
  pause
  exit /b 1
)
set TARGETS=targets.txt
if not exist %TARGETS% (
  echo Create %TARGETS% with one BTC address per line, then re-run this bat.
  echo Example line: 1BgGZ9tcN4rm9KBzDn7KprQz87DX02FijI
  pause
  exit /b 1
)
REM -b 40 is a small demo range — change to your puzzle bit length (e.g. 66)
keyhunt.exe -m address -f %TARGETS% -b 40 -l compress -R -q -s 10 -t 8 -e -A auto
pause
