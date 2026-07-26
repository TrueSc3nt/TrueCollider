@echo off
REM =============================================================================
REM CUDA address puzzle 66
REM Do NOT pass -e with -U cuda
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -t 1 -q -s 5
exit /b %ERRORLEVEL%

