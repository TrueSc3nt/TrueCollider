@echo off
REM =============================================================================
REM CUDA rmd160
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m rmd160 -f tests\_puzzle20.rmd -b 20 -l compress -U cuda -M auto -t 1 -x sequential -q
exit /b %ERRORLEVEL%

