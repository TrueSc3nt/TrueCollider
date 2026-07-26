@echo off
REM =============================================================================
REM Puzzle 66 address
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -A auto -t 8 -q -s 10
exit /b %ERRORLEVEL%

