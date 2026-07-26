@echo off
REM =============================================================================
REM TrueCollider ? build CUDA (VS 2022 + CUDA Toolkit) wrapper
REM Requires: Visual Studio 2022 Build Tools, CUDA 12.x, NVIDIA GPU driver
REM Output: keyhunt_cuda.exe in repo root
REM =============================================================================
setlocal EnableExtensions
cd /d "%~dp0.."
if not exist "%CD%\keyhunt.cpp" (
  echo [E] keyhunt.cpp missing - run from a complete TrueCollider checkout.
  exit /b 1
)
echo [+] Building CUDA keyhunt via bats\00_build\build_cuda_vs2022.bat ...
call "%CD%\bats\00_build\build_cuda_vs2022.bat"
exit /b %ERRORLEVEL%
