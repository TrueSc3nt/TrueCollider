@echo off
REM =============================================================================
REM BSGS backward
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m bsgs -f tests\125.txt -b 125 -B backward -k 256 -t 4 -q -s 10
exit /b %ERRORLEVEL%

