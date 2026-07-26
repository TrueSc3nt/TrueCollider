@echo off
REM =============================================================================
REM Random-sequential (-rs) — random start, walk N keys, reseed
REM Tiny range smoke against tests\_btc_1to2.txt (privkeys 1 and 2).
REM CPU by default; set USE_CUDA=1 to require keyhunt_cuda.exe.
REM Do NOT use -e with -U cuda. Ctrl+C to stop (reseed loop is open-ended).
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
if not exist "tests\_btc_1to2.txt" (
  echo [E] Missing fixture: tests\_btc_1to2.txt
  exit /b 1
)

set THREADS=2
set NCHUNK=0x400

if "%USE_CUDA%"=="1" (
  call "%~dp0_check_exe.bat" keyhunt_cuda.exe "examples\build_cuda.bat" || exit /b 1
  echo [+] CUDA: keyhunt_cuda.exe -m address -f tests\_btc_1to2.txt -r 1:1000 -rs -n %NCHUNK% -l compress -U cuda -M auto -t 1 -s 5
  keyhunt_cuda.exe -m address -f tests\_btc_1to2.txt -r 1:1000 -rs -n %NCHUNK% -l compress -U cuda -M auto -t 1 -s 5
  exit /b %ERRORLEVEL%
)

call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
echo [+] CPU: keyhunt.exe -m address -f tests\_btc_1to2.txt -r 1:1000 -rs -n %NCHUNK% -l compress -t %THREADS% -s 5
keyhunt.exe -m address -f tests\_btc_1to2.txt -r 1:1000 -rs -n %NCHUNK% -l compress -t %THREADS% -s 5
exit /b %ERRORLEVEL%
