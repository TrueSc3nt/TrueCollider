@echo off
REM =============================================================================
REM Currency -c eth (Ethereum)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
if not exist "tests\_eth1.txt" (echo [E] missing tests\_eth1.txt & exit /b 1)
keyhunt.exe -m address -c eth -f tests\_eth1.txt -t 2 -q -s 5
exit /b %ERRORLEVEL%

