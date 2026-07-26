@echo off
REM Alternate CUDA MSVC build
setlocal
cd /d "%~dp0..\.."
call build_cuda_msvc.bat
exit /b %ERRORLEVEL%
