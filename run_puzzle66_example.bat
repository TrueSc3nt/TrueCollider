@echo off
REM Example Bitcoin address search using targets.txt (edit TARGETS= below).
REM Name is historical: default demo uses -b 40 (tiny). For puzzle 66 use -b 66
REM and put the puzzle address in targets.txt, or run examples\search_btc_address.bat
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

set "TARGETS=targets.txt"
if not exist "%TARGETS%" (
  echo Create %TARGETS% with one BTC address per line, then re-run this bat.
  echo Example ^(pubkey #1 compressed P2PKH^): 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
  echo Or copy: copy tests\_btc1.txt targets.txt
  pause
  exit /b 1
)

REM -b 40 is a small demo range — change to your puzzle bit length (e.g. 66)
echo [+] keyhunt.exe -m address -f %TARGETS% -b 40 -l compress -R -q -s 10 -t 8 -A auto
keyhunt.exe -m address -f %TARGETS% -b 40 -l compress -R -q -s 10 -t 8 -A auto
pause
exit /b %ERRORLEVEL%
