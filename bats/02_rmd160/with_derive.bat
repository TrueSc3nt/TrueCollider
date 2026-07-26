@echo off
REM =============================================================================
REM RMD160 + BIP-44 derivation
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m rmd160 -f tests\66.rmd -p "m/44'/0'/0'/0" -D 5 -l compress -t 4 -q -s 10
exit /b %ERRORLEVEL%

