@echo off
REM =============================================================================
REM BSGS dry-run (-k auto -y)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m bsgs -f tests\125.txt -b 125 -k auto -y
exit /b %ERRORLEVEL%

