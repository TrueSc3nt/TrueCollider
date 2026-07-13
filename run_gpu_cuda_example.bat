@echo off
REM TrueCollider / KeyCollider — sample CUDA address hunt (puzzle 66 file).
REM Requires: keyhunt_cuda.exe from build_cuda_vs2022.bat + NVIDIA GPU + CUDA driver.

set EXE=keyhunt_cuda.exe
if not exist "%EXE%" set EXE=keyhunt.exe
if not exist "%EXE%" (
  echo [E] Build first: build_cuda_vs2022.bat  or  build_mingw_native.bat
  exit /b 1
)

echo [+] Running CUDA EC path: %EXE%
"%EXE%" -m address -f tests\66.txt -b 66 -l compress -U cuda -G 128 -t 1 -q -s 5
echo.
echo Hits (if any) are in KEYFOUNDKEYFOUND.txt
pause
