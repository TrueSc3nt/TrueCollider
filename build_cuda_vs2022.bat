@echo off
REM TrueCollider CUDA build via CMake + MSVC (prefers VS 2022 for nvcc host compat)
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

REM ---- Visual Studio / Build Tools (prefer 2022 for CUDA host compiler) ----
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VCVARS="
set "VS="

REM Prefer VS 2022 products first (nvcc often rejects newer MSVC alone)
if exist "!VSWHERE!" (
  for /f "usebackq delims=" %%i in (`"!VSWHERE!" -version "[17.0,18.0)" -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do (
    if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" (
      set "VS=%%i"
      set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
      goto :vs_found
    )
  )
)

REM Hardcoded Build Tools 2022 fallback
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
  set "VS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
  set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
  goto :vs_found
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
  set "VS=C:\Program Files\Microsoft Visual Studio\2022\Community"
  set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
  goto :vs_found
)

REM Any latest VS with C++ tools (18/2025+)
if exist "!VSWHERE!" (
  for /f "usebackq delims=" %%i in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do (
    if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" (
      set "VS=%%i"
      set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
      goto :vs_found
    )
  )
)

echo [E] Visual Studio with C++ tools / vcvars64.bat not found.
echo     Install VS 2022 Build Tools with "Desktop development with C++"
echo     or set VCVARS to the full path of vcvars64.bat.
exit /b 1

:vs_found
echo [+] VS: !VS!
echo [+] Loading vcvars64...
call "!VCVARS!"
if errorlevel 1 (
  echo [E] vcvars64 failed
  exit /b 1
)

where cl >nul 2>&1
if errorlevel 1 (
  echo [E] cl.exe not on PATH after vcvars64
  exit /b 1
)

REM ---- CUDA Toolkit (require complete install: cuda_runtime.h present) ----
set "CUDA_PATH="
REM Prefer complete 12.x toolkits; skip incomplete installs (e.g. partial 13.x)
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
if not defined CUDA_PATH goto :no_cuda
if not exist "!CUDA_PATH!\bin\nvcc.exe" goto :no_cuda
if not exist "!CUDA_PATH!\include\cuda_runtime.h" goto :no_cuda

:cuda_ok
echo [+] CUDA: !CUDA_PATH!
set "PATH=!CUDA_PATH!\bin;!PATH!"
set "INCLUDE=!CUDA_PATH!\include;!CUDA_PATH!\include\cccl;!INCLUDE!"
set "LIB=!CUDA_PATH!\lib\x64;!LIB!"
set "NVCC_PREPEND_FLAGS=-allow-unsupported-compiler"
set "CUDAFLAGS=-allow-unsupported-compiler"

where nvcc >nul 2>&1
if errorlevel 1 (
  echo [E] nvcc.exe not found after setting CUDA_PATH
  exit /b 1
)

REM ---- CMake + Ninja ----
set "CMAKE="
if exist "!VS!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
  set "CMAKE=!VS!\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)
if not defined CMAKE if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
  set "CMAKE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)
if not defined CMAKE (
  where cmake >nul 2>&1 && set "CMAKE=cmake"
)
if not defined CMAKE (
  echo [E] cmake not found. Install CMake or Visual Studio CMake component.
  exit /b 1
)

set "NINJA="
if exist "!VS!\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" (
  set "NINJA=!VS!\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
)
if not defined NINJA if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" (
  set "NINJA=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
)
if not defined NINJA (
  where ninja >nul 2>&1 && set "NINJA=ninja"
)

set "BDIR=build-cuda-vs2022"
if exist "!BDIR!" rd /s /q "!BDIR!"

set "GEN_OK=0"
if defined NINJA (
  echo [+] Configuring ^(Ninja^)...
  "!CMAKE!" -S . -B "!BDIR!" -G Ninja ^
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
  if not errorlevel 1 set "GEN_OK=1"
)

if "!GEN_OK!"=="0" (
  echo [!] Ninja configure failed or unavailable, trying VS generator...
  if exist "!BDIR!" rd /s /q "!BDIR!"
  "!CMAKE!" -S . -B "!BDIR!" -G "Visual Studio 17 2022" -A x64 -T host=x64 ^
    -DENABLE_CUDA=ON -DENABLE_AVX512=ON -DBUILD_TESTS=OFF ^
    "-DCMAKE_CUDA_COMPILER=!CUDA_PATH!\bin\nvcc.exe" ^
    -DCMAKE_CUDA_FLAGS=-allow-unsupported-compiler ^
    -DCMAKE_CUDA_ARCHITECTURES="75;86;89"
  if errorlevel 1 (
    echo [E] CMake configure failed
    exit /b 1
  )
  echo [+] Building Release...
  "!CMAKE!" --build "!BDIR!" --config Release -j
  if errorlevel 1 (
    echo [E] Build failed
    exit /b 1
  )
  if exist "!BDIR!\Release\keyhunt.exe" (
    copy /y "!BDIR!\Release\keyhunt.exe" keyhunt_cuda.exe >nul
  ) else if exist "!BDIR!\keyhunt.exe" (
    copy /y "!BDIR!\keyhunt.exe" keyhunt_cuda.exe >nul
  ) else (
    echo [E] keyhunt.exe missing after VS build
    exit /b 1
  )
) else (
  echo [+] Building...
  "!CMAKE!" --build "!BDIR!" -j
  if errorlevel 1 (
    echo [E] Build failed
    exit /b 1
  )
  if not exist "!BDIR!\keyhunt.exe" (
    echo [E] keyhunt.exe missing after Ninja build
    exit /b 1
  )
  copy /y "!BDIR!\keyhunt.exe" keyhunt_cuda.exe >nul
)

if not exist keyhunt_cuda.exe (
  echo [E] keyhunt_cuda.exe was not produced
  exit /b 1
)

echo [+] OK: %CD%\keyhunt_cuda.exe
dir keyhunt_cuda.exe
keyhunt_cuda.exe -h >nul 2>&1
if errorlevel 1 (
  echo [!] keyhunt_cuda.exe -h returned non-zero ^(binary may still be usable^)
) else (
  echo [+] Smoke: keyhunt_cuda.exe -h PASS
)
echo PASS: CUDA build
endlocal
exit /b 0

:no_cuda
echo [E] NVIDIA CUDA Toolkit not found or incomplete.
echo     Need nvcc.exe and include\cuda_runtime.h under the toolkit tree.
echo     Install CUDA 12.x from https://developer.nvidia.com/cuda-downloads
echo     ^(Partial installs such as toolkit fragments without headers will be skipped.^)
exit /b 1
