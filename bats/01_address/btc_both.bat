@echo off
REM =============================================================================
REM BTC - compress + uncompress (-l both)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_btc1.txt -r 1:50 -l both -t 1 -x sequential -q
exit /b %ERRORLEVEL%

