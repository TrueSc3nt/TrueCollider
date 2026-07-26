@echo off
REM =============================================================================
REM Collider --mode rseq --walk 2M
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m address -f tests\_btc1.txt -r 1:1000000 --mode rseq --walk 2M -l compress -t 2 -q -s 5
exit /b %ERRORLEVEL%

