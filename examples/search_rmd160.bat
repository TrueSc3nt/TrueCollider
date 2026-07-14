@echo off
REM =============================================================================
REM RMD160 / hash160 search — tests\66.rmd
REM Faster than address mode (no Base58 encode on hot path).
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
set THREADS=8
echo [+] %EXE% -m rmd160 -f tests\66.rmd -l compress -e -x gravity -t %THREADS% -q -s 10
"%EXE%" -m rmd160 -f tests\66.rmd -l compress -e -x gravity -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
