@echo off
REM =============================================================================
REM BTC - uncompressed only
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
if not exist "tests\_btc_key1_u.txt" (echo [E] missing uncompressed fixture & exit /b 1)
keyhunt.exe -m address -f tests\_btc_key1_u.txt -r 1:50 -l uncompress -t 1 -x sequential -q
exit /b %ERRORLEVEL%

