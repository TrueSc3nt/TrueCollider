@echo off
REM Build CUDA keyhunt_cuda.exe (VS2022 + CUDA Toolkit)
setlocal
if not exist "%~dp0build_cuda_vs2022.bat" (echo [E] missing build_cuda_vs2022.bat & exit /b 1)
call "%~dp0build_cuda_vs2022.bat"
exit /b %ERRORLEVEL%
