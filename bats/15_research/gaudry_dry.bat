@echo off
REM =============================================================================
REM gaudry / ResidueHerd dry
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m gaudry -f tests\_pubkey_g.txt -b 40 --mod-step 4 --mod-rem 1 -y
exit /b %ERRORLEVEL%

