@echo off
call "%~dp0..\_common.bat" cuda || exit /b 1
"%TC_EXE%" -h 2>&1 | findstr /I /C:"Custom CUDA" /C:"KeyHunt-class" >nul
if errorlevel 1 (
  echo [E] custom CUDA edition help strings missing
  exit /b 1
)
echo [+] custom CUDA edition help OK
exit /b 0
