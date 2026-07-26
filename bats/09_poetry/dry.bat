@echo off
REM =============================================================================
REM Poetry dry-run
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m poetry -f tests\66.txt -y
exit /b %ERRORLEVEL%

