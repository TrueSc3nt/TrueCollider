@echo off
REM =============================================================================
REM TrueCollider ? build CPU (MinGW native) wrapper
REM Requires: MSYS2 MinGW64 or the toolchain expected by bats\00_build\build_mingw_native.bat
REM Output: keyhunt.exe in repo root
REM =============================================================================
setlocal EnableExtensions
cd /d "%~dp0.."
if not exist "%CD%\keyhunt.cpp" (
  echo [E] keyhunt.cpp missing - run from a complete TrueCollider checkout.
  exit /b 1
)
echo [+] Building CPU keyhunt via bats\00_build\build_mingw_native.bat ...
call "%CD%\bats\00_build\build_mingw_native.bat"
exit /b %ERRORLEVEL%
