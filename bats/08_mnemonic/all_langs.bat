@echo off
REM =============================================================================
REM Mnemonic all BIP-39 languages (-L all)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m mnemonic -w 12 -L all -D 1 -f tests\66.txt -t 4 -q -s 10
exit /b %ERRORLEVEL%

