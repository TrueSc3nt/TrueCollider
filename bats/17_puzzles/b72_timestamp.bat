@echo off
REM =============================================================================
REM Puzzle 72 + funding timestamp
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
if not exist "tests\unsolvedpuzzles.rmd" (
  echo [i] Using 66.rmd as stand-in if unsolved file missing
  keyhunt.exe -m rmd160 -f tests\66.rmd -b 72 -T 1421345234 -l compress -t 8 -x auto -q -s 10
) else (
  keyhunt.exe -m rmd160 -f tests\unsolvedpuzzles.rmd -b 72 -T 1421345234 -l compress -t 8 -x auto -q -s 10
)
exit /b %ERRORLEVEL%

