@echo off
REM =============================================================================
REM CUDA sequential GRP smoke: known hit at privkey = 1
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m address -f tests\_btc1.txt -r 1:1000 -l compress -U cuda -M auto -t 1 -x sequential -q
exit /b %ERRORLEVEL%
