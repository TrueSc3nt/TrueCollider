@echo off
REM =============================================================================
REM Kangaroo -b 40 demo
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m kangaroo -f tests\125.txt -b 40 -t 4 -q -s 10
exit /b %ERRORLEVEL%

