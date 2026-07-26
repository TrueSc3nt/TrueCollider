@echo off
REM =============================================================================
REM Auto-detect currency from file (-c auto)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -c auto -f tests\_btc1.txt -r 1:20 -t 1 -x sequential -q
exit /b %ERRORLEVEL%

