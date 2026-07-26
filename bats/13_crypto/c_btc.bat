@echo off
REM =============================================================================
REM Currency -c btc (Bitcoin)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
if not exist "tests\_btc1.txt" (echo [E] missing tests\_btc1.txt & exit /b 1)
keyhunt.exe -m address -c btc -f tests\_btc1.txt -t 2 -q -s 5
exit /b %ERRORLEVEL%

