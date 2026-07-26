@echo off
REM =============================================================================
REM wif-mask dry-run
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m wif-mask --wif-mask L------------? -f tests\_btc1.txt -y
exit /b %ERRORLEVEL%

