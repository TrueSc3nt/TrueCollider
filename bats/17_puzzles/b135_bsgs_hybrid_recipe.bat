@echo off
REM =============================================================================
REM Puzzle / walk recipe: b135_bsgs_hybrid_recipe.bat
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
echo [i] Hybrid warmup + random giants; honesty: BSGS not kangaroo sqrtN`nkeyhunt.exe -m bsgs -f tests\_pubkey_g.txt -r 1:2 -n 1048576 -B hybrid -t 2 -q
exit /b %ERRORLEVEL%
