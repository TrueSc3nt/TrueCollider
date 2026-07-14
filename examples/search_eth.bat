@echo off
REM =============================================================================
REM Ethereum address search — tests\_eth_1.txt
REM Uses -c eth (keccak256). Hits go to FOUND_ETH.txt
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
set THREADS=8
echo [+] %EXE% -m address -c eth -f tests\_eth_1.txt -t %THREADS% -q -s 10
"%EXE%" -m address -c eth -f tests\_eth_1.txt -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
