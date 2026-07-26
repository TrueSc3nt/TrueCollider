@echo off
REM =============================================================================
REM Vanity 1Bg - hits privkey 1 quickly
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m vanity -v 1Bg -r 1:20 -l compress -t 1 -x sequential -q
exit /b %ERRORLEVEL%

