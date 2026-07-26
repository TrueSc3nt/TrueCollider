@echo off
REM =============================================================================
REM pubkey2addr ETH
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m pubkey2addr -c eth -f tests\_eth1.txt -t 4 -q -s 10
exit /b %ERRORLEVEL%

