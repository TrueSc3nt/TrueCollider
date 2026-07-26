@echo off
REM =============================================================================
REM Hybrid CPU+CUDA (-U both) — requires keyhunt_cuda.exe (ENABLE_CUDA build)
REM Even thread IDs → CUDA, odd → CPU. Example: -t 4 → 2 GPU + 2 CPU workers.
REM -t 1 → CUDA only. Do NOT pass -e (forces CPU EC path).
REM Note: -U both is NOT the same as -l both (compress/uncompress).
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt_cuda.exe "examples\build_cuda.bat" || exit /b 1
if not exist "tests\66.txt" (
  echo [E] Missing fixture: tests\66.txt
  exit /b 1
)
set THREADS=4
echo [+] keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U both -M auto -t %THREADS% -q -s 5
keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U both -M auto -t %THREADS% -q -s 5
exit /b %ERRORLEVEL%
