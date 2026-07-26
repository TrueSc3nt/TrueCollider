@echo off
REM =============================================================================
REM pubkey2addr BTC -x auto
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m pubkey2addr -f tests\66.txt -x auto -t 4 -q -s 10
exit /b %ERRORLEVEL%

