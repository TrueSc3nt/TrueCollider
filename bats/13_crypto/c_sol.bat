@echo off
REM =============================================================================
REM Currency -c sol (Solana)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
if not exist "tests\sol_sample.txt" (echo [E] missing tests\sol_sample.txt & exit /b 1)
keyhunt.exe -m address -c sol -f tests\sol_sample.txt -t 2 -q -s 5
exit /b %ERRORLEVEL%

