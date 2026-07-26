@echo off
REM =============================================================================
REM Solana - open-ended scan
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -c sol -f tests\sol_sample.txt -t 4 -q -s 5
exit /b %ERRORLEVEL%

