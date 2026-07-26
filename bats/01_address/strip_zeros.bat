@echo off
REM =============================================================================
REM Address - strip leading zero bytes (-Z) with -b
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\66.txt -b 40 -Z 2 -l compress -t 4 -q -s 10
exit /b %ERRORLEVEL%

