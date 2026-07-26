@echo off
REM =============================================================================
REM Puzzle / walk recipe: b20_clockwork.bat
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x clockwork -q
exit /b %ERRORLEVEL%
