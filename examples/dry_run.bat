@echo off
REM =============================================================================
REM Dry-run (-y) — print resolved config / memory plan and exit (no search)
REM Runs CPU plan always; CUDA plan if non-zero keyhunt_cuda.exe exists.
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\66.txt" (
  echo [E] Missing fixture: tests\66.txt
  exit /b 1
)

echo ========== CPU dry-run ==========
keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -e -A auto -y
set "RC=%ERRORLEVEL%"
echo.

if exist "keyhunt_cuda.exe" (
  for %%F in (keyhunt_cuda.exe) do set "SZ=%%~zF"
  if "!SZ!"=="0" (
    echo [E] keyhunt_cuda.exe is 0 bytes ^(corrupt^). Rebuild with examples\build_cuda.bat
    exit /b 1
  )
  echo ========== CUDA dry-run ==========
  keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -y
  if errorlevel 1 set "RC=%ERRORLEVEL%"
) else (
  echo [i] keyhunt_cuda.exe not found — skip CUDA dry-run. Use examples\build_cuda.bat
)
exit /b %RC%
