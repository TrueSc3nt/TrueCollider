@echo off
REM =============================================================================
REM Solana address search — tests\sol_sample.txt (ed25519)
REM Open-ended scan. Quick guaranteed hit: run_sol_sample.bat (-r 1:8)
REM CUDA: set USE_CUDA=1 (needs keyhunt_cuda.exe; no -e)
REM Hits -> FOUND_SOL.txt
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
if not exist "tests\sol_sample.txt" (
  echo [E] Missing fixture: tests\sol_sample.txt
  exit /b 1
)

if "%USE_CUDA%"=="1" (
  call "%~dp0_check_exe.bat" keyhunt_cuda.exe "examples\build_cuda.bat" || exit /b 1
  echo [+] CUDA: keyhunt_cuda.exe -m address -c sol -f tests\sol_sample.txt -U cuda -M auto -t 1 -q -s 5
  keyhunt_cuda.exe -m address -c sol -f tests\sol_sample.txt -U cuda -M auto -t 1 -q -s 5
  exit /b %ERRORLEVEL%
)

call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
set THREADS=4
echo [+] keyhunt.exe -m address -c sol -f tests\sol_sample.txt -t %THREADS% -q -s 5
keyhunt.exe -m address -c sol -f tests\sol_sample.txt -t %THREADS% -q -s 5
exit /b %ERRORLEVEL%
