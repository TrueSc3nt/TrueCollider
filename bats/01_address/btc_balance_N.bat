@echo off
REM =============================================================================
REM BTC address + public API balance check (-N)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
echo [i] Needs curl + network. Smoke on keys 1..2:
keyhunt.exe -m address -f tests\_btc_1to2.txt -r 1:2 -l compress -N -t 1
exit /b %ERRORLEVEL%

