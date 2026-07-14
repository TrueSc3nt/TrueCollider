@echo off
REM Native Windows MinGW build of keyhunt.exe (CPU + AVX kernels; no CUDA required)
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

REM ---- Locate MinGW g++ (MSYS2 default, then PATH) ----
set "MINGW="
if defined MINGW_HOME if exist "!MINGW_HOME!\bin\g++.exe" set "MINGW=!MINGW_HOME!"
if not defined MINGW if exist "C:\msys64\mingw64\bin\g++.exe" set "MINGW=C:\msys64\mingw64"
if not defined MINGW if exist "C:\msys64\ucrt64\bin\g++.exe" set "MINGW=C:\msys64\ucrt64"
if not defined MINGW if exist "C:\msys64\clang64\bin\g++.exe" set "MINGW=C:\msys64\clang64"
if not defined MINGW (
  where g++ >nul 2>&1
  if not errorlevel 1 (
    for /f "delims=" %%i in ('where g++') do (
      set "_GPP=%%i"
      goto :gpp_found
    )
  )
  goto :no_mingw
)
goto :have_mingw

:gpp_found
for %%i in ("!_GPP!") do set "MINGW=%%~dpi.."
set "MINGW=!MINGW:\bin\..=!"
set "MINGW=!MINGW:\bin=!"

:have_mingw
if not exist "!MINGW!\bin\g++.exe" goto :no_mingw
set "PATH=!MINGW!\bin;!PATH!"

set "MAKE=!MINGW!\bin\mingw32-make.exe"
if not exist "!MAKE!" set "MAKE=!MINGW!\bin\make.exe"
if not exist "!MAKE!" (
  where mingw32-make >nul 2>&1 && set "MAKE=mingw32-make"
)
if not exist "!MAKE!" if /I not "!MAKE!"=="mingw32-make" (
  where make >nul 2>&1 && set "MAKE=make"
)
if "!MAKE!"=="" (
  echo [E] make / mingw32-make not found near !MINGW!\bin
  echo     Install: pacman -S mingw-w64-x86_64-make
  exit /b 1
)

echo [+] MinGW: !MINGW!
echo [+] Building keyhunt.exe with MinGW...

REM Windows-safe clean (Makefile `rm` may be missing under cmd.exe)
del /Q keyhunt.exe keyhunt keyhunt_nolto.o *.o 2>nul
if exist hash del /Q hash\*.o 2>nul
if exist gpu del /Q gpu\*.o 2>nul
if exist ed25519 del /Q ed25519\*.o 2>nul

"!MAKE!" -j%NUMBER_OF_PROCESSORS% ^
  OS=MINGW64 ARCH=x86_64 ^
  CXX="!MINGW!\bin\g++.exe" CC="!MINGW!\bin\gcc.exe" ^
  TARGET=keyhunt.exe ^
  "LIBS=-lws2_32 -lwinpthread -static -static-libgcc -static-libstdc++" ^
  "CXXFLAGS=-Wall -Wextra -Wno-deprecated-copy -O2 -I. -m64 -mssse3 -ftree-vectorize -DCPU_GRP_SIZE=1024 -DHAVE_SSE -DOS_WINDOWS" ^
  "CFLAGS=-Wall -Wextra -O2 -I. -m64 -mssse3 -DCPU_GRP_SIZE=1024 -DHAVE_SSE -DOS_WINDOWS"
if errorlevel 1 (
  echo [E] Build failed
  exit /b 1
)

if not exist keyhunt.exe (
  echo [E] keyhunt.exe was not produced
  exit /b 1
)

echo.
echo [+] Build OK: %CD%\keyhunt.exe
keyhunt.exe -h >nul 2>&1
if errorlevel 1 (
  echo [!] keyhunt.exe -h returned non-zero ^(binary may still be usable^)
) else (
  echo [+] Smoke: keyhunt.exe -h PASS
)
echo PASS: CPU MinGW build
endlocal
exit /b 0

:no_mingw
echo [E] MinGW g++ not found.
echo     Install MSYS2 from https://www.msys2.org/ then:
echo       pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
echo     Or set MINGW_HOME to your mingw64/ucrt64 root.
exit /b 1
