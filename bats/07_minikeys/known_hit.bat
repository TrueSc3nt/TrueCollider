@echo off
REM =============================================================================
REM Minikeys known hit (S4b3...Dwf)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
keyhunt.exe -m minikeys -f tests\_minikey_known.txt -C S4b3N3oGqDqR5jNuxEvDwe -t 1 -q
exit /b %ERRORLEVEL%

