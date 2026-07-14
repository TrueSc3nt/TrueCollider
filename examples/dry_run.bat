@echo off
REM =============================================================================
REM Dry-run (-y) — print resolved config / memory plan and exit (no search)
REM Runs CPU plan always; CUDA plan if keyhunt_cuda.exe exists.
REM =============================================================================
setlocal
cd /d "%~dp0.."
if not exist "keyhunt.exe" (
  echo [E] keyhunt.exe not found. Run examples\build_cpu.bat first.
  exit /b 1
)
echo ========== CPU dry-run ==========
keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -e -A auto -y
echo.
if exist "keyhunt_cuda.exe" (
  echo ========== CUDA dry-run ==========
  keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -y
) else (
  echo [i] keyhunt_cuda.exe not found — skip CUDA dry-run. Use examples\build_cuda.bat
)
exit /b 0
