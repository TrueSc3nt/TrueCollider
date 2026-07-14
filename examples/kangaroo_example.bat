@echo off
REM =============================================================================
REM Kangaroo — tiny range demo (generator G pubkey = key 1)
REM Ranges <= 2^24: sequential / GPU batch scan; larger: DP kangaroo.
REM CUDA: use keyhunt_cuda.exe -U cuda when built.
REM =============================================================================
setlocal
cd /d "%~dp0.."
set "EXE=keyhunt.exe"
if exist "keyhunt_cuda.exe" set "EXE=keyhunt_cuda.exe"
if not exist "%EXE%" (
  echo [E] %EXE% not found. Run examples\build_cpu.bat or build_cuda_vs2022.bat first.
  exit /b 1
)
echo [+] CPU: %EXE% -m kangaroo -f tests\_pubkey_g.txt -r 1:1000
"%EXE%" -m kangaroo -f tests\_pubkey_g.txt -r 1:1000
if errorlevel 1 exit /b %ERRORLEVEL%
echo.
if /I "%EXE%"=="keyhunt_cuda.exe" (
  echo [+] CUDA: %EXE% -m kangaroo -f tests\_pubkey_g.txt -r 1:1000 -U cuda
  "%EXE%" -m kangaroo -f tests\_pubkey_g.txt -r 1:1000 -U cuda
)
exit /b %ERRORLEVEL%
