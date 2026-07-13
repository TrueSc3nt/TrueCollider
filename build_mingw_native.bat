@echo off
REM Native Windows MinGW build of keyhunt.exe (CPU + AVX-512; no CUDA required for -A avx512 -e)
setlocal
set MINGW=C:\msys64\mingw64
set PATH=%MINGW%\bin;%PATH%
set ROOT=%~dp0
cd /d "%ROOT%"

if not exist "%MINGW%\bin\g++.exe" (
  echo [E] MinGW g++ not found at %MINGW%\bin\g++.exe
  exit /b 1
)

echo [+] Building keyhunt.exe with MinGW...
"%MINGW%\bin\mingw32-make.exe" clean
"%MINGW%\bin\mingw32-make.exe" -j%NUMBER_OF_PROCESSORS% ^
  OS=MINGW64 ARCH=x86_64 ^
  CXX="%MINGW%\bin\g++.exe" CC="%MINGW%\bin\gcc.exe" ^
  TARGET=keyhunt.exe ^
  "LIBS=-lws2_32 -lwinpthread -static -static-libgcc -static-libstdc++" ^
  "CXXFLAGS=-Wall -Wextra -Wno-deprecated-copy -O2 -m64 -mssse3 -ftree-vectorize -DCPU_GRP_SIZE=1024 -DHAVE_SSE -DOS_WINDOWS" ^
  "CFLAGS=-Wall -Wextra -O2 -m64 -mssse3 -DCPU_GRP_SIZE=1024 -DHAVE_SSE -DOS_WINDOWS"

if exist keyhunt.exe (
  echo.
  echo [+] Build OK: %ROOT%keyhunt.exe
  keyhunt.exe -h
) else (
  echo [E] Build failed
  exit /b 1
)
endlocal
