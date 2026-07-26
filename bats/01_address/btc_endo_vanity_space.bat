@echo off
REM =============================================================================
REM BTC + endomorphism (large space / vanity style)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
echo [W] -e helps full-space/vanity; not small puzzle bit-ranges
keyhunt.exe -m address -f tests\66.txt -l compress -e -A auto -t 8 -q -s 10
exit /b %ERRORLEVEL%

