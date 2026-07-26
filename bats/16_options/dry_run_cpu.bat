@echo off
REM =============================================================================
REM Dry-run -y (CPU plan)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -A auto -y
exit /b %ERRORLEVEL%

