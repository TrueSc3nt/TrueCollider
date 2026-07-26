@echo off
REM =============================================================================
REM Puzzle / walk recipe: b72_keyhole_recipe.bat
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
echo [i] Puzzle 72-160 address/rmd160: -x keyhole --window-bits 40 (hash160; not kangaroo)`nkeyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x keyhole --window-bits 16 -q
exit /b %ERRORLEVEL%
