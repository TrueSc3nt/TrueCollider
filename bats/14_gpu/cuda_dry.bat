@echo off
REM =============================================================================
REM CUDA dry-run memory plan
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -y
exit /b %ERRORLEVEL%

