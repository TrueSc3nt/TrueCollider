@echo off
REM =============================================================================
REM BTC address - puzzle #20 known hit
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x sequential -q
exit /b %ERRORLEVEL%

