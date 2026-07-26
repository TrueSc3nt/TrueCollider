@echo off
REM =============================================================================
REM hybrid-dl (HerdHandoff) dry
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m hybrid-dl -f tests\125.txt -b 40 -H 20 -y
exit /b %ERRORLEVEL%

