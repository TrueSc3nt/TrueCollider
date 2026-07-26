@echo off
REM =============================================================================
REM hex-mask - last nibble free (hits key 1)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m hex-mask --key-mask 000000000000000000000000000000000000000000000000000000000000000? -f tests\_btc1.txt -t 1 -q
exit /b %ERRORLEVEL%

