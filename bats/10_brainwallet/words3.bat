@echo off
REM =============================================================================
REM Brainwallet 3-word + mutations
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
if not exist "tests\brainwalletwords.txt" (echo [E] missing wordlist & exit /b 1)
keyhunt.exe -m brainwallet -w 3 -f tests\66.txt -t 4 -q -s 10
exit /b %ERRORLEVEL%

