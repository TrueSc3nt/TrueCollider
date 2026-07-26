@echo off
REM Build CPU keyhunt.exe (MinGW)
setlocal
if not exist "%~dp0build_mingw_native.bat" (echo [E] missing build_mingw_native.bat & exit /b 1)
call "%~dp0build_mingw_native.bat"
exit /b %ERRORLEVEL%
