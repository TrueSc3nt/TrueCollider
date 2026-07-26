@echo off
REM =============================================================================
REM CUDA Solana
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m address -c sol -f tests\sol_sample.txt -r 1:20 -U cuda -G 256 -t 1 -x sequential -q
exit /b %ERRORLEVEL%

