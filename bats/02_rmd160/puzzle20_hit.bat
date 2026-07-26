@echo off
REM =============================================================================
REM RMD160 puzzle 20 known hit
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m rmd160 -f tests\_puzzle20.rmd -b 20 -l compress -t 2 -x sequential -q
exit /b %ERRORLEVEL%

