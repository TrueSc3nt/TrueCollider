@echo off
REM =============================================================================
REM BTC address - random-sequential (-rs)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_btc_1to2.txt -r 1:1000 -rs -n 0x400 -l compress -t 2 -s 5
exit /b %ERRORLEVEL%

