@echo off
REM =============================================================================
REM Hybrid -U both (CPU+CUDA threads)
REM -U both != -l both
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -U both -M auto -t 4 -x sequential -q
exit /b %ERRORLEVEL%

