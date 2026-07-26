@echo off
REM =============================================================================
REM Verbose derivation -V
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\66.txt -p "m/84'/0'/0'/0" -D 3 -V -t 2 -q -s 10
exit /b %ERRORLEVEL%

