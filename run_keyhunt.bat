@echo off
title Keyhunt + Collider Search Modes
color 0A
echo =============================================================
echo  Keyhunt + Collider Search Modes
echo  Run from WSL (Windows Subsystem for Linux)
echo =============================================================
echo.
echo Usage: run_keyhunt.bat [arguments]
echo.
echo Examples:
echo   run_keyhunt.bat -m address -f tests/66.txt -b 66 -l compress -x chaos -q -t 8
echo   run_keyhunt.bat -m bsgs -f tests/125.txt -b 125 -x auto -q -s 10 -R
echo   run_keyhunt.bat -h
echo.
echo Search modes (-x): sequential, random, chaos, gravity, spiral, reverse, auto
echo.
set "SCRIPT_DIR=%~dp0"
set "WSL_DIR=%SCRIPT_DIR:\=/%"
set "WSL_DIR=/mnt/c/%WSL_DIR:~3%"

if "%~1"=="" (
    wsl -d Ubuntu -- bash -c "cd '%WSL_DIR%' && ./keyhunt -h"
) else (
    wsl -d Ubuntu -- bash -c "cd '%WSL_DIR%' && ./keyhunt %*"
)
