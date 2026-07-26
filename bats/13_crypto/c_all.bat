@echo off
REM =============================================================================
REM Search all currencies (-c all) - needs mixed file
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
set "TARGETS=targets_mixed.txt"
if not exist "%TARGETS%" (
  echo Create targets_mixed.txt with mixed addresses, then re-run.
  exit /b 1
)
keyhunt.exe -m address -c all -f %TARGETS% -t 4 -q -s 10
exit /b %ERRORLEVEL%

