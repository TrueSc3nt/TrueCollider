@echo off
REM =============================================================================
REM pubkey2addr sequential hit in tiny range
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m pubkey2addr -f tests\_btc1.txt -r 1:20 -x sequential -t 1 -q
exit /b %ERRORLEVEL%

