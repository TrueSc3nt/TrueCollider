@echo off
REM =============================================================================
REM Puzzle / walk recipe: b135_bsgs_modfan_recipe.bat
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
echo [i] Pubkey puzzles: BSGS -B modfan --mod-step M (not address hash160)`nkeyhunt.exe -m bsgs -f tests\_pubkey_g.txt -r 1:2 -n 1048576 -B modfan --mod-step 4 -t 2 -q
exit /b %ERRORLEVEL%
