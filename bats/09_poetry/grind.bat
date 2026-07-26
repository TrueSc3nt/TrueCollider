@echo off
REM =============================================================================
REM Poetry mode grind
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
if not exist "tests\poetry.txt" (echo [E] missing poetry wordlist & exit /b 1)
keyhunt.exe -m poetry -f tests\66.txt -t 4 -q -s 10
exit /b %ERRORLEVEL%

