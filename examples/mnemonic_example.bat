@echo off
REM =============================================================================
REM BIP-39 mnemonic — last-word recovery smoke (guaranteed hit)
REM Known phrase: abandon x11 + about → 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA
REM Open-ended random grind: remove --seed and use -f your_targets.txt
REM For ETH: add -W and use an ETH target file.
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
call "%~dp0_check_exe.bat" keyhunt.exe "examples\build_cpu.bat" || exit /b 1
if not exist "tests\bip39\english.txt" (
  echo [E] Missing BIP39 wordlist: tests\bip39\english.txt
  exit /b 1
)
echo 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA> tests\_mnemonic_abandon.txt
set THREADS=1
echo [+] Last-word recovery ^(abandon... ? → about^)
keyhunt.exe -m mnemonic -w 12 -L english -D 1 -t %THREADS% -q -s 5 -f tests\_mnemonic_abandon.txt --seed "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon ?"
exit /b %ERRORLEVEL%
