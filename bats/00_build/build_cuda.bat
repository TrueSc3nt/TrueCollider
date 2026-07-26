@echo off
REM Build CUDA keyhunt_cuda.exe (VS2022 + CUDA Toolkit)
setlocal
cd /d "%~dp0..\.."
if not exist keyhunt.cpp (echo [E] incomplete tree & exit /b 1)
call build_cuda_vs2022.bat
exit /b %ERRORLEVEL%
