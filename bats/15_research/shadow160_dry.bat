@echo off
REM =============================================================================
REM Mode shadow160 (prefix hash160)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m shadow160 -f tests\_puzzle20.rmd -b 20 -y
exit /b %ERRORLEVEL%

