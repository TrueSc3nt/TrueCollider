@echo off
REM =============================================================================
REM CUDA puzzle 20 known hit
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -U cuda -M auto -t 1 -x sequential -q
exit /b %ERRORLEVEL%

