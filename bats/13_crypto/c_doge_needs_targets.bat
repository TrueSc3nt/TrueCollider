@echo off
REM =============================================================================
REM Currency -c doge (edit TARGETS=)
REM No bundled fixture - provide your own target file.
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
set "TARGETS=targets_doge.txt"
if not exist "%TARGETS%" (
  echo Create %TARGETS% with one doge address per line, then re-run.
  exit /b 1
)
keyhunt.exe -m address -c doge -f %TARGETS% -t 4 -q -s 10
exit /b %ERRORLEVEL%

