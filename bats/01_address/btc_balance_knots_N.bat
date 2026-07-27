@echo off
REM =============================================================================
REM BTC + local Knots RPC balance (-N). Prefer D:\Bitcoin\daemon\bitcoind.exe
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1

set "BTC_DIR=D:\Bitcoin"
set "RPC_USER="
set "RPC_PASSWORD="
set "RPC_HOST=127.0.0.1"
set "RPC_PORT=8332"

if exist "%BTC_DIR%\gbt_credentials.txt" (
  for /f "usebackq tokens=1,* delims==" %%A in ("%BTC_DIR%\gbt_credentials.txt") do (
    if /I "%%A"=="RPC_USER" set "RPC_USER=%%B"
    if /I "%%A"=="RPC_PASSWORD" set "RPC_PASSWORD=%%B"
    if /I "%%A"=="RPC_HOST" set "RPC_HOST=%%B"
    if /I "%%A"=="RPC_PORT" set "RPC_PORT=%%B"
  )
)
if not defined RPC_PASSWORD if exist "%BTC_DIR%\.cookie" (
  for /f "usebackq tokens=1,* delims=:" %%A in ("%BTC_DIR%\.cookie") do (
    set "RPC_USER=%%A"
    set "RPC_PASSWORD=%%B"
  )
)
if not defined RPC_USER (
  echo [E] No RPC auth. Start: %BTC_DIR%\start_bitcoind.bat
  exit /b 1
)

"%BTC_DIR%\daemon\bitcoin-cli.exe" -datadir="%BTC_DIR%" -rpcuser=!RPC_USER! -rpcpassword=!RPC_PASSWORD! -rpcport=!RPC_PORT! getblockchaininfo >nul 2>&1
if errorlevel 1 (
  echo [E] RPC down. Run %BTC_DIR%\start_bitcoind.bat
  exit /b 1
)

echo [i] Knots daemon RPC http://!RPC_HOST!:!RPC_PORT!/ — smoke keys 1..2
echo [i] scantxoutset can take several minutes; use start_bitcoind.bat not Qt
keyhunt.exe -m address -f tests\_btc_1to2.txt -r 1:2 -l compress -Nhttp://!RPC_USER!:!RPC_PASSWORD!@!RPC_HOST!:!RPC_PORT! -t 1
exit /b %ERRORLEVEL%
