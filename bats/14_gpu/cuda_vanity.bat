@echo off
REM =============================================================================
REM CUDA vanity
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m vanity -v 1Bg -r 1:50 -l compress -U cuda -G 256 -t 1 -x sequential -q
exit /b %ERRORLEVEL%

