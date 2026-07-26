@echo off
REM =============================================================================
REM X-point + spiral pattern
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m xpoint -f tests\_xpoint_g.txt -b 40 -x spiral -t 4 -q -s 10
exit /b %ERRORLEVEL%

