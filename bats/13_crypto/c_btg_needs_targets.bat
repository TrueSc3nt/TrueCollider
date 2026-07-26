@echo off
REM =============================================================================
REM Currency -c btg (edit TARGETS=)
REM No bundled fixture - provide your own target file.
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
set "TARGETS=targets_btg.txt"
if not exist "%TARGETS%" (
  echo Create %TARGETS% with one btg address per line, then re-run.
  exit /b 1
)
keyhunt.exe -m address -c btg -f %TARGETS% -t 4 -q -s 10
exit /b %ERRORLEVEL%

