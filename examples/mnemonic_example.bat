@echo off
REM =============================================================================
REM BIP-39 mnemonic grind — English 12-word, BTC BIP-44/49/84 paths
REM -D 5 checks five indices per path (15 address checks per mnemonic).
REM For ETH: add -W and use an ETH target file.
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
set THREADS=4
echo [+] %EXE% -m mnemonic -w 12 -L english -D 5 -f tests\66.txt -t %THREADS% -q -s 10
"%EXE%" -m mnemonic -w 12 -L english -D 5 -f tests\66.txt -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
