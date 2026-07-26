@echo off
REM =============================================================================
REM BSGS random-sequential (-B rseq / --walk)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m bsgs -f tests\125.txt -b 125 -B rseq --walk 2M -k 256 -t 4 -q -s 10
exit /b %ERRORLEVEL%

