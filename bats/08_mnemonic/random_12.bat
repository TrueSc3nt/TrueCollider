@echo off
REM =============================================================================
REM Mnemonic random 12-word English grind
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m mnemonic -w 12 -L english -D 5 -f tests\btc.txt -t 8 -q -s 10
exit /b %ERRORLEVEL%

