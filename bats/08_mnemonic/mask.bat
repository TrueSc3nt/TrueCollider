@echo off
REM =============================================================================
REM Mnemonic mask submode (--submode mask)
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "%~dp0..\_common.bat" cpu || exit /b 1
echo 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA> tests\_mnemonic_abandon.txt
keyhunt.exe -m mnemonic -w 12 -L english -D 1 -t 1 -q -f tests\_mnemonic_abandon.txt --submode mask --seed "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon ? about"
exit /b %ERRORLEVEL%

