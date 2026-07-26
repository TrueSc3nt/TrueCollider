@echo off
REM =============================================================================
REM Puzzle / walk recipe: rmd20_pocket.bat
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m rmd160 -f tests\_puzzle20.rmd -b 20 -l compress -t 2 -x pocket --pocket-bits 8 -q
exit /b %ERRORLEVEL%
