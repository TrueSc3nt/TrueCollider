@echo off
REM =============================================================================
REM Bloom size multiplier -z
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\66.txt -b 40 -z 4 -l compress -t 4 -q -s 10
exit /b %ERRORLEVEL%

