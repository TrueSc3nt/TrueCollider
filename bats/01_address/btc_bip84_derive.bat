@echo off
REM =============================================================================
REM BTC address + BIP-84 derivation path
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\66.txt -p "m/84'/0'/0'/0" -D 5 -l compress -t 4 -q -s 10 -V
exit /b %ERRORLEVEL%

