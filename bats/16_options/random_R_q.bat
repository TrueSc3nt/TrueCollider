@echo off
REM =============================================================================
REM Legacy -R -q (must NOT warn about submode)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_btc1.txt -r 1:50 -l compress -R -q -t 1 -x sequential
exit /b %ERRORLEVEL%

