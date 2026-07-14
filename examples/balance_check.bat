@echo off
REM =============================================================================
REM Online balance checking (-N) — STATUS: WIRED
REM
REM On each hit, writekey / writekeyeth / writekeysol call node_check_balance()
REM (curl → blockstream / etherscan / blockcypher, or Bitcoin Core scantxoutset).
REM Requires curl on PATH. Custom -Nhttp://user:pass@host:port needs a live node.
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
echo [+] Balance check is WIRED (called on hits; needs curl + network or live RPC).
echo [+] Public-API form:
echo     %EXE% -m address -c btc -f tests\66.txt -N -t 4
echo [+] Own-node form:
echo     %EXE% -m address -c btc -f tests\66.txt -Nhttp://user:pass@127.0.0.1:8332 -t 4
echo.
echo [+] Smoke: search keys 1..2 for tests\_btc_1to2.txt with -N (public API):
"%EXE%" -m address -f tests\_btc_1to2.txt -r 1:2 -l compress -N -t 1
exit /b %ERRORLEVEL%
