@echo off
REM =============================================================================
REM CUDA GPU address search — requires keyhunt_cuda.exe + NVIDIA GPU
REM Host hash/bloom; GPU EC. Prefer -t 1 and -M auto.
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt_cuda.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cuda.bat first.
  exit /b 1
)
echo [+] %EXE% -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -t 1 -q -s 5
"%EXE%" -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -t 1 -q -s 5
exit /b %ERRORLEVEL%
