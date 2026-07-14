@echo off
REM =============================================================================
REM Online balance checking (-N) — DOCUMENTED STATUS: PARTIAL
REM
REM CLI accepts -N and optional -Nhttp://user:pass@host:port
REM node_check_balance() exists (curl → blockstream / etherscan / RPC)
REM BUT FLAGNODECHECK is not yet read on the hit/write path.
REM This script demonstrates the intended flags and exits after -y dry-run
REM so you do not expect live HTTP checks until wiring is completed.
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
echo [!] Balance check is PARTIAL in this build (flag parses; not called on hits).
echo [+] Intended public-API form:
echo     %EXE% -m address -c btc -f tests\66.txt -N -t 4
echo [+] Intended own-node form:
echo     %EXE% -m address -c btc -f tests\66.txt -Nhttp://user:pass@127.0.0.1:8332 -t 4
echo.
echo [+] Dry-run with -N (will print that node checking is enabled):
"%EXE%" -m address -f tests\66.txt -b 16 -N -y
exit /b %ERRORLEVEL%
