@echo off
REM Thin wrapper: CUDA build lives under bats\00_build
call "%~dp0bats\00_build\build_cuda_vs2022.bat"
exit /b %ERRORLEVEL%
