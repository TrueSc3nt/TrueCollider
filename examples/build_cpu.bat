@echo off
REM =============================================================================
REM TrueCollider — build CPU (MinGW native) wrapper
REM Requires: MSYS2 MinGW64 or the toolchain expected by build_mingw_native.bat
REM Output: keyhunt.exe in repo root
REM =============================================================================
setlocal
cd /d "%~dp0.."
echo [+] Building CPU keyhunt via build_mingw_native.bat ...
call "%~dp0..\build_mingw_native.bat"
exit /b %ERRORLEVEL%
