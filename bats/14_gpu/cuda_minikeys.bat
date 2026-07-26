@echo off
REM =============================================================================
REM CUDA minikeys known hit
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cuda || exit /b 1
keyhunt_cuda.exe -m minikeys -f tests\_minikey_known.txt -C S4b3N3oGqDqR5jNuxEvDwe -U cuda -t 1 -q
exit /b %ERRORLEVEL%

