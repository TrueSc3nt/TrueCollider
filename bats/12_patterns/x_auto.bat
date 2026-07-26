@echo off
REM =============================================================================
REM Search pattern -x auto (address key1)
REM Pattern: auto
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_btc1.txt -r 1:80 -l compress -t 1 -x auto -q
exit /b %ERRORLEVEL%

