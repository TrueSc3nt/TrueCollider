@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist "!VCVARS!" (
  echo [E] VS 2022 Build Tools vcvars64.bat not found
  exit /b 1
)

echo [+] Loading VS2022 Build Tools...
call "!VCVARS!"
if errorlevel 1 (
  echo [E] vcvars64 failed
  exit /b 1
)

where cl
where nvcc

set "CMAKE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
set "CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.8"
set "PATH=!CUDA_PATH!\bin;!PATH!"
set "INCLUDE=!CUDA_PATH!\include;!INCLUDE!"
set "LIB=!CUDA_PATH!\lib\x64;!LIB!"

set "BDIR=build-cuda-vs2022"
if exist "!BDIR!" rd /s /q "!BDIR!"

echo [+] Configuring (Ninja)...
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
  -DCMAKE_CUDA_ARCHITECTURES="75;86;89"
if errorlevel 1 (
  echo [!] Ninja configure failed, trying VS 2022 generator...
  if exist "!BDIR!" rd /s /q "!BDIR!"
  "!CMAKE!" -S . -B "!BDIR!" -G "Visual Studio 17 2022" -A x64 -T host=x64 ^
    -DENABLE_CUDA=ON -DENABLE_AVX512=ON -DBUILD_TESTS=OFF ^
    "-DCMAKE_CUDA_COMPILER=!CUDA_PATH!\bin\nvcc.exe" ^
    -DCMAKE_CUDA_ARCHITECTURES="75;86;89"
  if errorlevel 1 exit /b 1
  echo [+] Building Release...
  "!CMAKE!" --build "!BDIR!" --config Release -j
  if errorlevel 1 exit /b 1
  if exist "!BDIR!\Release\keyhunt.exe" (
    copy /y "!BDIR!\Release\keyhunt.exe" keyhunt_cuda.exe >nul
    copy /y "!BDIR!\Release\keyhunt.exe" "%USERPROFILE%\Desktop\keyhunt_cuda.exe" >nul
  ) else (
    echo [E] Release\keyhunt.exe missing
    exit /b 1
  )
) else (
  echo [+] Building...
  "!CMAKE!" --build "!BDIR!" -j
  if errorlevel 1 exit /b 1
  if not exist "!BDIR!\keyhunt.exe" (
    echo [E] keyhunt.exe missing
    exit /b 1
  )
  copy /y "!BDIR!\keyhunt.exe" keyhunt_cuda.exe >nul
  copy /y "!BDIR!\keyhunt.exe" "%USERPROFILE%\Desktop\keyhunt_cuda.exe" >nul
)

echo [+] OK
dir keyhunt_cuda.exe
endlocal
