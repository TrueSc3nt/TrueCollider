@echo off
REM =============================================================================
REM Search pattern -x reverse (address key1)
REM Pattern: reverse
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_btc1.txt -r 1:80 -l compress -t 1 -x reverse -q
exit /b %ERRORLEVEL%

