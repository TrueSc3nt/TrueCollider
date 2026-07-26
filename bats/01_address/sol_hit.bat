@echo off
REM =============================================================================
REM Solana - tiny range guaranteed hit
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -c sol -f tests\sol_sample.txt -r 1:8 -t 1 -q
exit /b %ERRORLEVEL%

