@echo off
call "%~dp0..\_common.bat" cuda || exit /b 1
REM Custom CUDA edition: packed hash160 via MIT converter
python "%CD%\scripts\addr_to_hash160.py" "%CD%\tests\66.txt" -o "%CD%\tests\_66.hash160" --sort
if errorlevel 1 (
  echo [E] converter failed - need Python 3
  exit /b 1
)
"%TC_EXE%" -m address -f "%CD%\tests\_66.hash160" -b 66 -l compress -U cuda -M auto -t 1 -x sequential -n 0x10000 -q -s 0
exit /b %ERRORLEVEL%
