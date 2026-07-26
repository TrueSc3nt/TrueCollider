@echo off
REM Build CPU keyhunt.exe
setlocal
cd /d "%~dp0..\.."
if not exist keyhunt.cpp (echo [E] incomplete tree & exit /b 1)
call build_mingw_native.bat
exit /b %ERRORLEVEL%
