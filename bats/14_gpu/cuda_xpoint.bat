@echo off
REM =============================================================================
REM CUDA xpoint
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m xpoint -f tests\_xpoint_g.txt -r 1:50 -U cuda -G 256 -t 1 -x sequential -q
exit /b %ERRORLEVEL%

