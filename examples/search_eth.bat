@echo off
REM =============================================================================
REM Ethereum address search — tests\_eth_1.txt
REM Uses -c eth (keccak256). Hits go to FOUND_ETH.txt
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\_eth_1.txt" (
  echo [E] Missing fixture: tests\_eth_1.txt
  exit /b 1
)
set THREADS=8
echo [+] keyhunt.exe -m address -c eth -f tests\_eth_1.txt -t %THREADS% -q -s 10
keyhunt.exe -m address -c eth -f tests\_eth_1.txt -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
