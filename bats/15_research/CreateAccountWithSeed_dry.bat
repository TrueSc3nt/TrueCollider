@echo off
REM =============================================================================
REM Solana CreateAccountWithSeed dry
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m CreateAccountWithSeed -f tests\sol_sample.txt -y
exit /b %ERRORLEVEL%

