@echo off
REM =============================================================================
REM Puzzle-style -b 40 demo (small)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\66.txt -b 40 -l compress -R -A auto -t 8 -q -s 10
exit /b %ERRORLEVEL%

