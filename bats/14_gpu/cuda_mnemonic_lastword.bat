@echo off
REM Custom CUDA edition: mnemonic last-word known hit
call "%~dp0..\_common.bat" cuda || exit /b 1
echo 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA> tests\_mnemonic_abandon.txt
keyhunt_cuda.exe -m mnemonic -w 12 -L english -D 1 -t 1 -q -s 0 -U cuda -M auto -f tests\_mnemonic_abandon.txt --seed "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon ?"
exit /b %ERRORLEVEL%
