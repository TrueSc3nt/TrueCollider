@echo off
REM Quick known-hit + dry-run smoke (CPU; CUDA if present)
setlocal EnableExtensions
cd /d "%~dp0.."
if not exist keyhunt.exe (
  echo [E] Build first: bats\00_build\build_cpu.bat
  exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0_run_smoke.ps1"
exit /b %ERRORLEVEL%
