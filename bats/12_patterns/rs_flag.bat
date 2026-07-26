@echo off
REM =============================================================================
REM Random-sequential via -rs flag
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_btc1.txt -r 1:200 -rs -n 0x400 -l compress -t 1 -q
exit /b %ERRORLEVEL%

