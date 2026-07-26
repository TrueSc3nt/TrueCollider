@echo off
REM =============================================================================
REM BTC address - random walk full space
REM Bare -R = random (not research). Do not confuse with --submode.
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\66.txt -l compress -R -t 8 -q -s 10
exit /b %ERRORLEVEL%

