@echo off
REM =============================================================================
REM BSGS both (top/bottom)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m bsgs -f tests\125.txt -b 125 -B both -k 256 -t 4 -q -s 10
exit /b %ERRORLEVEL%

