@echo off
REM =============================================================================
REM Kangaroo — tiny range demo (generator G pubkey = key 1)
REM Ranges <= 2^24: sequential / GPU batch scan; larger: DP kangaroo.
REM Runs CPU first; optional second CUDA pass if keyhunt_cuda.exe is present.
REM Do NOT use -e with -U cuda.
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
if not exist "tests\_pubkey_g.txt" (
  echo [E] Missing fixture: tests\_pubkey_g.txt
  exit /b 1
)

call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
echo [+] CPU: keyhunt.exe -m kangaroo -f tests\_pubkey_g.txt -r 1:1000
keyhunt.exe -m kangaroo -f tests\_pubkey_g.txt -r 1:1000
if errorlevel 1 exit /b %ERRORLEVEL%

if exist "keyhunt_cuda.exe" (
  for %%F in (keyhunt_cuda.exe) do set "SZ=%%~zF"
  if "!SZ!"=="0" (
    echo [E] keyhunt_cuda.exe is 0 bytes ^(corrupt^). Rebuild with examples\build_cuda.bat
    exit /b 1
  )
  echo.
  echo [+] CUDA: keyhunt_cuda.exe -m kangaroo -f tests\_pubkey_g.txt -r 1:1000 -U cuda
  keyhunt_cuda.exe -m kangaroo -f tests\_pubkey_g.txt -r 1:1000 -U cuda
  exit /b %ERRORLEVEL%
)
echo.
echo [i] keyhunt_cuda.exe not found — skipped CUDA pass. Use examples\build_cuda.bat
exit /b 0
