@echo off
REM =============================================================================
REM BSGS -B modfan (tiny pubkey G smoke)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m bsgs -f tests\_pubkey_g.txt -r 1:2 -n 1048576 -B modfan --mod-step 4 -t 1 -q
exit /b %ERRORLEVEL%
