@echo off
REM =============================================================================
REM RMD160 / hash160 search — tests\66.rmd
REM Faster than address mode (no Base58 encode on hot path).
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\66.rmd" (
  echo [E] Missing fixture: tests\66.rmd
  exit /b 1
)
set THREADS=8
echo [+] keyhunt.exe -m rmd160 -f tests\66.rmd -b 66 -l compress -x sequential -t %THREADS% -q -s 10
keyhunt.exe -m rmd160 -f tests\66.rmd -b 66 -l compress -x sequential -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
