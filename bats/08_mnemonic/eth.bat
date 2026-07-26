@echo off
REM =============================================================================
REM Mnemonic Ethereum (-W)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m mnemonic -w 12 -L english -W -D 5 -f tests\_eth1.txt -t 4 -q -s 10
exit /b %ERRORLEVEL%

