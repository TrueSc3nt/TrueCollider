@echo off
REM =============================================================================
REM BTC address search — puzzle #66 fixture (tests\66.txt)
REM Flags: -m address -b 66 -l compress -A auto
REM Note: -e (endomorphism) does NOT help puzzle bit-ranges — omit it here.
REM For CUDA: examples\gpu_cuda_address.bat
REM Edit THREADS as needed. Ctrl+C to stop.
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\66.txt" (
  echo [E] Missing fixture: tests\66.txt
  exit /b 1
)
set THREADS=8
echo [+] keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -A auto -t %THREADS% -q -s 10
keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -A auto -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
