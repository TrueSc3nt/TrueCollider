@echo off
REM =============================================================================
REM Ethereum address search
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -c eth -f tests\_eth1.txt -r 1:20 -t 1 -x sequential -q
exit /b %ERRORLEVEL%

