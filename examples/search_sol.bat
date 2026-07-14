@echo off
REM =============================================================================
REM Solana address search — tests\sol_sample.txt (ed25519)
REM CPU by default. For CUDA: add -U cuda -M auto -t 1 (needs keyhunt_cuda.exe)
REM Hits -> FOUND_SOL.txt
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat first.
  exit /b 1
)
set THREADS=4
echo [+] %EXE% -m address -c sol -f tests\sol_sample.txt -t %THREADS% -q -s 5
"%EXE%" -m address -c sol -f tests\sol_sample.txt -t %THREADS% -q -s 5
exit /b %ERRORLEVEL%
