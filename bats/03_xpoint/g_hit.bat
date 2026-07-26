@echo off
REM =============================================================================
REM X-point - generator G (privkey 1)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m xpoint -f tests\_xpoint_g.txt -r 1:20 -t 1 -x sequential -q
exit /b %ERRORLEVEL%

