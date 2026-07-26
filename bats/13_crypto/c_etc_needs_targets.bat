@echo off
REM =============================================================================
REM Currency -c etc (edit TARGETS=)
REM No bundled fixture - provide your own target file.
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
set "TARGETS=targets_etc.txt"
if not exist "%TARGETS%" (
  echo Create %TARGETS% with one etc address per line, then re-run.
  exit /b 1
)
keyhunt.exe -m address -c etc -f %TARGETS% -t 4 -q -s 10
exit /b %ERRORLEVEL%

