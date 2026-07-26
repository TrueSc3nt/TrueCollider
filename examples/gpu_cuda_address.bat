@echo off
REM =============================================================================
REM CUDA GPU address search — requires keyhunt_cuda.exe + NVIDIA GPU
REM Host hash/bloom; GPU EC. Prefer -t 1 and -M auto.
REM Do NOT use -e with -U cuda (endomorphism forces CPU).
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt_cuda.exe "examples\build_cuda.bat" || exit /b 1
if not exist "tests\66.txt" (
  echo [E] Missing fixture: tests\66.txt
  exit /b 1
)
echo [+] keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -t 1 -q -s 5
keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -t 1 -q -s 5
exit /b %ERRORLEVEL%
