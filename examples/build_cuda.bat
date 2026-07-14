@echo off
REM =============================================================================
REM TrueCollider — build CUDA (VS 2022 + CUDA Toolkit) wrapper
REM Requires: Visual Studio 2022 Build Tools, CUDA 12.x, NVIDIA GPU driver
REM Output: keyhunt_cuda.exe in repo root
REM =============================================================================
setlocal
cd /d "%~dp0.."
echo [+] Building CUDA keyhunt via build_cuda_vs2022.bat ...
call "%~dp0..\build_cuda_vs2022.bat"
exit /b %ERRORLEVEL%
