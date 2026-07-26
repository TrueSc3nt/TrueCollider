@echo off
REM =============================================================================
REM Search pattern -x twinflame (address key1 smoke)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_btc1.txt -r 1:200 -l compress -t 1 -x twinflame -q
exit /b %ERRORLEVEL%
