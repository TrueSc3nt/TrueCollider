@echo off
REM TrueCollider CUDA build (MSVC via vswhere + latest/preferred CUDA)
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" (
  echo [E] vswhere.exe not found. Install Visual Studio Installer.
  exit /b 1
)

REM Prefer VS 2022 for nvcc host compatibility, then latest
set "VS="
for /f "usebackq delims=" %%i in (`"!VSWHERE!" -version "[17.0,18.0)" -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VS=%%i"
if not defined VS (
  for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VS=%%i"
)
if not defined VS (
  echo [E] Visual Studio with C++ tools not found
  exit /b 1
)

echo [+] VS: !VS!
if exist "!VS!\Common7\Tools\VsDevCmd.bat" (
  call "!VS!\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
) else if exist "!VS!\VC\Auxiliary\Build\vcvars64.bat" (
  call "!VS!\VC\Auxiliary\Build\vcvars64.bat"
) else (
  echo [E] Neither VsDevCmd.bat nor vcvars64.bat found under !VS!
  exit /b 1
)
if errorlevel 1 exit /b 1

set "CMAKE=!VS!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "!CMAKE!" set "CMAKE=cmake"
where "!CMAKE!" >nul 2>&1
if errorlevel 1 (
  where cmake >nul 2>&1
  if errorlevel 1 (
    echo [E] cmake not found
    exit /b 1
  )
  set "CMAKE=cmake"
)

set "CUDA_PATH="
REM Prefer complete 12.x; skip incomplete toolkits missing cuda_runtime.h
for %%V in (12.8 12.6 12.4 12.5 13.0 13.1 13.2 13.3) do (
  if not defined CUDA_PATH if exist "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v%%V\include\cuda_runtime.h" (
    if exist "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v%%V\bin\nvcc.exe" (
      set "CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v%%V"
    )
  )
)
if not defined CUDA_PATH (
  for /d %%d in ("C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v*") do (
    if not defined CUDA_PATH if exist "%%d\include\cuda_runtime.h" if exist "%%d\bin\nvcc.exe" set "CUDA_PATH=%%d"
  )
)
if not defined CUDA_PATH (
  echo [E] NVIDIA CUDA Toolkit not found ^(need nvcc.exe + include\cuda_runtime.h^)
  exit /b 1
)

echo [+] CUDA: !CUDA_PATH!
set "PATH=!CUDA_PATH!\bin;!PATH!"
set "INCLUDE=!CUDA_PATH!\include;!CUDA_PATH!\include\cccl;!INCLUDE!"
set "LIB=!CUDA_PATH!\lib\x64;!LIB!"
set "NVCC_PREPEND_FLAGS=-allow-unsupported-compiler"
set "CUDAFLAGS=-allow-unsupported-compiler"

set "NINJA=!VS!\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
if not exist "!NINJA!" (
  where ninja >nul 2>&1
  if errorlevel 1 (
    echo [E] ninja not found
    exit /b 1
  )
  set "NINJA=ninja"
)

set "BDIR=build-cuda-msvc"
if exist "!BDIR!" rd /s /q "!BDIR!"

"!CMAKE!" -S . -B "!BDIR!" -G "Ninja" ^
  -DCMAKE_MAKE_PROGRAM="!NINJA!" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DENABLE_CUDA=ON ^
  -DENABLE_AVX512=ON ^
  -DBUILD_TESTS=OFF ^
  -DCMAKE_C_COMPILER=cl.exe ^
  -DCMAKE_CXX_COMPILER=cl.exe ^
  "-DCMAKE_CUDA_COMPILER=!CUDA_PATH!\bin\nvcc.exe" ^
  -DCMAKE_CUDA_HOST_COMPILER=cl.exe ^
  -DCMAKE_CUDA_FLAGS=-allow-unsupported-compiler ^
  -DCMAKE_CUDA_ARCHITECTURES="75;86;89"
if errorlevel 1 exit /b 1

"!CMAKE!" --build "!BDIR!" -j
if errorlevel 1 exit /b 1

if exist "!BDIR!\keyhunt.exe" (
  copy /y "!BDIR!\keyhunt.exe" keyhunt_cuda.exe >nul
) else if exist "!BDIR!\Release\keyhunt.exe" (
  copy /y "!BDIR!\Release\keyhunt.exe" keyhunt_cuda.exe >nul
) else (
  echo [E] keyhunt.exe not found after CUDA build
  exit /b 1
)
echo [+] Built keyhunt_cuda.exe
dir keyhunt_cuda.exe
echo PASS: CUDA MSVC build
endlocal
exit /b 0
