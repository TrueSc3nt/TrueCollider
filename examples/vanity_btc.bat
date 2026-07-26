@echo off
REM =============================================================================
REM BTC vanity prefix search — no -f file; prefix via -v
REM -e enables GLV endomorphism on CPU (do not use -e with CUDA).
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
set THREADS=8
set PREFIX=1Cool
echo [+] keyhunt.exe -m vanity -v %PREFIX% -e -t %THREADS% -q -s 10
keyhunt.exe -m vanity -v %PREFIX% -e -t %THREADS% -q -s 10
exit /b %ERRORLEVEL%
