@echo off
REM =============================================================================
REM Kangaroo tiny range (key 1)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m kangaroo -f tests\_pubkey_g.txt -r 1:1000 -t 1 -q
exit /b %ERRORLEVEL%

