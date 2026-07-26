@echo off
REM =============================================================================
REM Smoke: # comments / blanks are skipped (no "Invalid length: #")
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1

set OUT=%TEMP%\tc_load_comments.txt
del /q "%OUT%" 2>nul

echo [i] Starting BSGS load of tests\125.txt (kill after a few seconds)...
start /b "" cmd /c "keyhunt.exe -m bsgs -f tests\125.txt -r 1:1048576 -n 1048576 -k 1 -t 1 -q -s 0 >\"%OUT%\" 2>&1"
timeout /t 6 /nobreak >nul
taskkill /IM keyhunt.exe /F >nul 2>&1

findstr /C:"Invalid length" "%OUT%" >nul 2>&1
if not errorlevel 1 (
  echo [E] Still printing Invalid length for comments
  type "%OUT%"
  exit /b 1
)
findstr /C:"Added 1 points from file" "%OUT%" >nul 2>&1
if errorlevel 1 (
  echo [E] Did not see Added 1 points
  type "%OUT%"
  exit /b 1
)
echo [+] PASS: no Invalid length; Added 1 points from tests\125.txt
exit /b 0
