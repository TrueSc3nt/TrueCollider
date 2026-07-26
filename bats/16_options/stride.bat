@echo off
REM =============================================================================
REM Custom stride -I
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_btc1.txt -r 1:200 -I 2 -l compress -t 1 -x sequential -q
exit /b %ERRORLEVEL%

