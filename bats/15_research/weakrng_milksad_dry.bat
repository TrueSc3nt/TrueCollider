@echo off
REM =============================================================================
REM Mode weakrng / milksad dry
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m weakrng --submode milksad -T 1514764800:1514851200 -f tests\_btc1.txt -y
exit /b %ERRORLEVEL%

