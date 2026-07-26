@echo off
REM =============================================================================
REM Kangaroo -b 40 known-hit (DP path; key 0x9f01234567 in bit-40 range)
REM May take ~1-3 minutes on CPU. Fixture: tests\_pubkey_b40.txt
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
if not exist "tests\_pubkey_b40.txt" (
  echo [E] Missing fixture: tests\_pubkey_b40.txt
  exit /b 1
)
keyhunt.exe -m kangaroo -f tests\_pubkey_b40.txt -b 40 -t 4 -q
exit /b %ERRORLEVEL%
