@echo off
REM =============================================================================
REM Minikeys open grind vs puzzle66 addr
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m minikeys -f tests\66.txt -t 4 -q -s 10
exit /b %ERRORLEVEL%

