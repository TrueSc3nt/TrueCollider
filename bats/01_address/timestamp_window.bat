@echo off
REM =============================================================================
REM Address search around Unix timestamp (-T)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
echo [+] ~4B key window from timestamp (demo uses tiny -b too if combined elsewhere)
keyhunt.exe -m address -f tests\66.txt -T 1421345234 -b 40 -l compress -t 4 -q -s 10
exit /b %ERRORLEVEL%

