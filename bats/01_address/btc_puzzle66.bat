@echo off
REM =============================================================================
REM BTC address - puzzle #66 range
REM Ctrl+C to stop. Hits -> KEYFOUNDKEYFOUND.txt / FOUND_BTC.txt
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
if not exist "tests\66.txt" (echo [E] missing tests\66.txt & exit /b 1)
echo [+] BTC puzzle 66 (no -e - endomorphism does not help bit-ranges)
keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -A auto -t 8 -q -s 10
exit /b %ERRORLEVEL%

