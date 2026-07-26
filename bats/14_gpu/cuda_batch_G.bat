@echo off
REM =============================================================================
REM CUDA with explicit -G batch
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -U cuda -G 128 -M auto -t 1 -x sequential -q
exit /b %ERRORLEVEL%

