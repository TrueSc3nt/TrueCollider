@echo off
setlocal EnableExtensions
cd /d "%~dp0"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS=%%i"
if not defined VS (
  echo [E] Visual Studio with C++ tools not found
  exit /b 1
)

call "%VS%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b 1

set "CMAKE=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "%CMAKE%" set "CMAKE=cmake"

rem Prefer CUDA 13.3 (newer host compiler support); fall back to 12.8 with override.
set "CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.3"
if not exist "%CUDA_PATH%\bin\nvcc.exe" set "CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.8"
if not exist "%CUDA_PATH%\bin\nvcc.exe" set "CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.6"
set "PATH=%CUDA_PATH%\bin;%PATH%"
set "INCLUDE=%CUDA_PATH%\include;%CUDA_PATH%\include\cccl;%INCLUDE%"
set "LIB=%CUDA_PATH%\lib\x64;%LIB%"
set "NVCC_PREPEND_FLAGS=-allow-unsupported-compiler"
set "CUDAFLAGS=-allow-unsupported-compiler"

set "NINJA=%VS%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
set "BDIR=build-cuda-msvc"
if exist "%BDIR%" rd /s /q "%BDIR%"

"%CMAKE%" -S . -B "%BDIR%" -G "Ninja" ^
  -DCMAKE_MAKE_PROGRAM="%NINJA%" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DENABLE_CUDA=ON ^
  -DENABLE_AVX512=ON ^
  -DBUILD_TESTS=OFF ^
  -DCMAKE_CUDA_COMPILER="%CUDA_PATH%\bin\nvcc.exe" ^
  -DCMAKE_CUDA_FLAGS=-allow-unsupported-compiler
if errorlevel 1 exit /b 1

"%CMAKE%" --build "%BDIR%" -j
if errorlevel 1 exit /b 1

if exist "%BDIR%\keyhunt.exe" (
  copy /y "%BDIR%\keyhunt.exe" keyhunt_cuda.exe >nul
) else if exist "%BDIR%\Release\keyhunt.exe" (
  copy /y "%BDIR%\Release\keyhunt.exe" keyhunt_cuda.exe >nul
) else (
  echo [E] keyhunt.exe not found after CUDA build
  exit /b 1
)
echo [+] Built keyhunt_cuda.exe
dir keyhunt_cuda.exe
endlocal
