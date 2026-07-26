@echo off
REM =============================================================================
REM Quiet -q + stats -s 5
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_btc1.txt -r 1:100 -l compress -t 1 -x sequential -q -s 5
exit /b %ERRORLEVEL%

