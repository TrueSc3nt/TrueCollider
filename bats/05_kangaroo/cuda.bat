@echo off
REM =============================================================================
REM Kangaroo CUDA
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m kangaroo -f tests\_pubkey_g.txt -r 1:1000 -U cuda -t 1 -q
exit /b %ERRORLEVEL%

