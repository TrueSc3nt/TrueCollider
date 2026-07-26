@echo off
REM =============================================================================
REM Vanity prefix 1Cool + endomorphism
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m vanity -v 1Cool -e -t 8 -q -s 10
exit /b %ERRORLEVEL%

