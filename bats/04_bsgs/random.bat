@echo off
REM =============================================================================
REM BSGS random giant steps
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m bsgs -f tests\125.txt -b 125 -R -k 512 -t 4 -S -q -s 10
exit /b %ERRORLEVEL%

