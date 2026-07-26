@echo off
REM =============================================================================
REM RMD160 puzzle 66
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m rmd160 -f tests\66.rmd -b 66 -l compress -t 8 -x sequential -q -s 10
exit /b %ERRORLEVEL%

