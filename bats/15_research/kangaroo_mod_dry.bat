@echo off
REM =============================================================================
REM kangaroo-mod dry-run
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m kangaroo-mod -f tests\_pubkey_g.txt -b 40 --mod-step 8 --mod-rem 0 -y
exit /b %ERRORLEVEL%

