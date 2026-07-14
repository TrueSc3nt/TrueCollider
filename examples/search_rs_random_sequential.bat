@echo off
REM =============================================================================
REM Random-sequential (-rs) — Mivvvy-style: random start, walk N keys, reseed
REM Tiny range smoke against tests\_btc_1to2.txt (privkeys 1 and 2).
REM CPU by default; set USE_CUDA=1 to prefer keyhunt_cuda.exe.
REM Ctrl+C to stop (reseed loop does not end on its own).
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if "%USE_CUDA%"=="1" (
  if exist "keyhunt_cuda.exe" set "EXE=keyhunt_cuda.exe"
)
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat or build_cuda.bat first.
  exit /b 1
)
set THREADS=2
set NCHUNK=0x400
echo [+] Random-sequential smoke:
if /I "%EXE%"=="keyhunt_cuda.exe" (
  echo [+] %EXE% -m address -f tests\_btc_1to2.txt -r 1:1000 -rs -n %NCHUNK% -l compress -U cuda -M auto -t 1 -s 5
  "%EXE%" -m address -f tests\_btc_1to2.txt -r 1:1000 -rs -n %NCHUNK% -l compress -U cuda -M auto -t 1 -s 5
) else (
  echo [+] %EXE% -m address -f tests\_btc_1to2.txt -r 1:1000 -rs -n %NCHUNK% -l compress -t %THREADS% -s 5
  "%EXE%" -m address -f tests\_btc_1to2.txt -r 1:1000 -rs -n %NCHUNK% -l compress -t %THREADS% -s 5
)
exit /b %ERRORLEVEL%
