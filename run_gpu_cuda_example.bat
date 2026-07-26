@echo off
REM TrueCollider — sample CUDA address hunt (puzzle 66 fixture).
REM Requires: keyhunt_cuda.exe (build_cuda_vs2022.bat) + NVIDIA GPU + CUDA driver.
REM Do NOT pass -e with -U cuda (endomorphism forces CPU path).
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

set "EXE=keyhunt_cuda.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Build with build_cuda_vs2022.bat ^(or examples\build_cuda.bat^)
  exit /b 1
)
for %%F in ("%EXE%") do set "SZ=%%~zF"
if "!SZ!"=="0" (
  echo [E] %EXE% is 0 bytes ^(corrupt^). Rebuild with build_cuda_vs2022.bat
  exit /b 1
)
if not exist "tests\66.txt" (
  echo [E] Missing fixture: tests\66.txt
  exit /b 1
)

echo [+] %EXE% -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -t 1 -q -s 5
"%EXE%" -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -t 1 -q -s 5
echo.
echo Hits ^(if any^) are in KEYFOUNDKEYFOUND.txt
exit /b %ERRORLEVEL%
