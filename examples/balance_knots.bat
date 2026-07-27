@echo off
REM =============================================================================
REM BTC balance check via local Bitcoin Knots JSON-RPC (-N)
REM Prefer headless: D:\Bitcoin\daemon\bitcoind.exe  (start_bitcoind.bat)
REM Auth: D:\Bitcoin\gbt_credentials.txt  OR  D:\Bitcoin\.cookie
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\_btc_1to2.txt" (
  echo [E] Missing fixture: tests\_btc_1to2.txt
  exit /b 1
)

set "BTC_DIR=D:\Bitcoin"
set "CLI=%BTC_DIR%\daemon\bitcoin-cli.exe"
if not exist "%CLI%" (
  echo [E] Missing %CLI% — Knots daemon tools not found
  exit /b 1
)

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
  echo [E] No RPC auth. Need %BTC_DIR%\gbt_credentials.txt or .cookie
  echo     Start node: %BTC_DIR%\start_bitcoind.bat
  exit /b 1
)

"%CLI%" -datadir="%BTC_DIR%" -rpcuser=!RPC_USER! -rpcpassword=!RPC_PASSWORD! -rpcport=!RPC_PORT! getblockchaininfo >nul 2>&1
if errorlevel 1 (
  echo [E] RPC not reachable. Start headless daemon:
  echo       %BTC_DIR%\start_bitcoind.bat
  echo     ^(bitcoin-qt not required; daemon\bitcoind.exe is enough^)
  exit /b 1
)

echo [+] Knots RPC OK http://!RPC_HOST!:!RPC_PORT!/ ^(rpcuser set; password redacted^)
echo [+] Smoke: keys 1..2 + -N ^(scantxoutset on hits; often 3-8 minutes per hit^)
echo [!] Prefer daemon: %BTC_DIR%\start_bitcoind.bat  ^(not bitcoin-qt^)
echo.

keyhunt.exe -m address -f tests\_btc_1to2.txt -r 1:2 -l compress -Nhttp://!RPC_USER!:!RPC_PASSWORD!@!RPC_HOST!:!RPC_PORT! -t 1
exit /b %ERRORLEVEL%
