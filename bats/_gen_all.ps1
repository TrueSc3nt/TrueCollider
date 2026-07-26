# Generates comprehensive TrueCollider mode bats under bats/
$ErrorActionPreference = "Stop"
$Bats = Split-Path -Parent $MyInvocation.MyCommand.Path
$Root = Split-Path -Parent $Bats

$dirs = @(
  '00_build','01_address','02_rmd160','03_xpoint','04_bsgs','05_kangaroo',
  '06_vanity','07_minikeys','08_mnemonic','09_poetry','10_brainwallet',
  '11_pubkey2addr','12_patterns','13_crypto','14_gpu','15_research','16_options','17_puzzles'
)
foreach ($d in $dirs) {
  New-Item -ItemType Directory -Force -Path (Join-Path $Bats $d) | Out-Null
}

# Shared helper (lives in bats/)
@'
@echo off
REM bats\_common.bat â€” cd to repo root, require CPU or CUDA exe
REM Usage: call "%~dp0..\_common.bat" cpu
REM    or: call "%~dp0..\_common.bat" cuda
setlocal EnableExtensions EnableDelayedExpansion
set "TC_ROOT=%~dp0.."
cd /d "%TC_ROOT%" || exit /b 1

set "NEED=%~1"
if /I "%NEED%"=="" set "NEED=cpu"

if /I "%NEED%"=="cuda" (
  set "EXE=keyhunt_cuda.exe"
  set "HINT=bats\00_build\build_cuda.bat"
) else (
  set "EXE=keyhunt.exe"
  set "HINT=bats\00_build\build_cpu.bat"
)

if not exist "%EXE%" (
  echo [E] %EXE% not found. Build with %HINT%
  endlocal & exit /b 1
)
for %%F in ("%EXE%") do set "SZ=%%~zF"
if "!SZ!"=="0" (
  echo [E] %EXE% is 0 bytes ^(corrupt^). Rebuild with %HINT%
  endlocal & exit /b 1
)
endlocal & (
  set "TC_EXE=%EXE%"
  cd /d "%TC_ROOT%"
)
exit /b 0
'@ | Set-Content -Encoding ASCII (Join-Path $Bats '_common.bat')

function Write-Bat($RelPath, $Title, $Need, $BodyLines, $Notes = @()) {
  $full = Join-Path $Bats $RelPath
  $dir = Split-Path $full -Parent
  if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
  $depth = ($RelPath -split '[\\/]').Count - 1
  $common = if ($depth -ge 1) { '%~dp0..\_common.bat' } else { '%~dp0_common.bat' }
  $sb = New-Object System.Text.StringBuilder
  [void]$sb.AppendLine('@echo off')
  [void]$sb.AppendLine('REM =============================================================================')
  [void]$sb.AppendLine("REM $Title")
  foreach ($n in $Notes) { [void]$sb.AppendLine("REM $n") }
  [void]$sb.AppendLine('REM =============================================================================')
  [void]$sb.AppendLine('setlocal EnableExtensions EnableDelayedExpansion')
  [void]$sb.AppendLine("call `"$common`" $Need || exit /b 1")
  foreach ($line in $BodyLines) { [void]$sb.AppendLine($line) }
  [void]$sb.AppendLine('exit /b %ERRORLEVEL%')
  $sb.ToString() | Set-Content -Encoding ASCII $full
}

# ---- 00_build ----
Write-Bat '00_build\build_cpu.bat' 'Build CPU keyhunt.exe (MinGW)' 'cpu' @(
  'call "%~dp0build_mingw_native.bat"'
) @('Does not need existing exe — calls bats/00_build/build_mingw_native.bat.') | Out-Null

# Fix build bats specially â€” they shouldn't require exe first
@'
@echo off
REM Build CPU keyhunt.exe (MinGW)
setlocal
if not exist "%~dp0build_mingw_native.bat" (echo [E] missing build_mingw_native.bat & exit /b 1)
call "%~dp0build_mingw_native.bat"
exit /b %ERRORLEVEL%
'@ | Set-Content -Encoding ASCII (Join-Path $Bats '00_build\build_cpu.bat')

@'
@echo off
REM Build CUDA keyhunt_cuda.exe (VS2022 + CUDA Toolkit)
setlocal
if not exist "%~dp0build_cuda_vs2022.bat" (echo [E] missing build_cuda_vs2022.bat & exit /b 1)
call "%~dp0build_cuda_vs2022.bat"
exit /b %ERRORLEVEL%
'@ | Set-Content -Encoding ASCII (Join-Path $Bats '00_build\build_cuda.bat')

# build_cuda_msvc.bat is the real MSVC script (not a thin wrapper) — do not overwrite it here.

# ---- 01_address ----
Write-Bat '01_address\btc_puzzle66.bat' 'BTC address â€” puzzle #66 range' 'cpu' @(
  'if not exist "tests\66.txt" (echo [E] missing tests\66.txt & exit /b 1)',
  'echo [+] BTC puzzle 66 (no -e â€” endomorphism does not help bit-ranges)',
  'keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -A auto -t 8 -q -s 10'
) @('Ctrl+C to stop. Hits -> KEYFOUNDKEYFOUND.txt / FOUND_BTC.txt')

Write-Bat '01_address\btc_key1_hit.bat' 'BTC address â€” guaranteed hit (privkey 1)' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc1.txt -r 1:100 -l compress -t 1 -x sequential -q'
)

Write-Bat '01_address\btc_puzzle20_hit.bat' 'BTC address â€” puzzle #20 known hit' 'cpu' @(
  'keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x sequential -q'
)

Write-Bat '01_address\btc_random.bat' 'BTC address â€” random walk full space' 'cpu' @(
  'keyhunt.exe -m address -f tests\66.txt -l compress -R -t 8 -q -s 10'
) @('Bare -R = random (not research). Do not confuse with --submode.')

Write-Bat '01_address\btc_rs.bat' 'BTC address â€” random-sequential (-rs)' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc_1to2.txt -r 1:1000 -rs -n 0x400 -l compress -t 2 -s 5'
)

Write-Bat '01_address\btc_compress.bat' 'BTC â€” compressed only (-l compress)' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc1.txt -r 1:50 -l compress -t 1 -x sequential -q'
)

Write-Bat '01_address\btc_uncompress.bat' 'BTC â€” uncompressed only' 'cpu' @(
  'if not exist "tests\_btc_key1_u.txt" (echo [E] missing uncompressed fixture & exit /b 1)',
  'keyhunt.exe -m address -f tests\_btc_key1_u.txt -r 1:50 -l uncompress -t 1 -x sequential -q'
)

Write-Bat '01_address\btc_both.bat' 'BTC â€” compress + uncompress (-l both)' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc1.txt -r 1:50 -l both -t 1 -x sequential -q'
)

Write-Bat '01_address\btc_endo_vanity_space.bat' 'BTC + endomorphism (large space / vanity style)' 'cpu' @(
  'echo [W] -e helps full-space/vanity; not small puzzle bit-ranges',
  'keyhunt.exe -m address -f tests\66.txt -l compress -e -A auto -t 8 -q -s 10'
)

Write-Bat '01_address\btc_bip84_derive.bat' 'BTC address + BIP-84 derivation path' 'cpu' @(
  'keyhunt.exe -m address -f tests\66.txt -p "m/84''/0''/0''/0" -D 5 -l compress -t 4 -q -s 10 -V'
)

Write-Bat '01_address\btc_bip44_derive.bat' 'BTC address + BIP-44 derivation' 'cpu' @(
  'keyhunt.exe -m address -f tests\66.txt -p "m/44''/0''/0''/0" -D 5 -l compress -t 4 -q -s 10'
)

Write-Bat '01_address\btc_balance_N.bat' 'BTC address + public API balance check (-N)' 'cpu' @(
  'echo [i] Needs curl + network. Smoke on keys 1..2:',
  'keyhunt.exe -m address -f tests\_btc_1to2.txt -r 1:2 -l compress -N -t 1'
)

Write-Bat '01_address\eth.bat' 'Ethereum address search' 'cpu' @(
  'keyhunt.exe -m address -c eth -f tests\_eth1.txt -r 1:20 -t 1 -x sequential -q'
)

Write-Bat '01_address\sol_hit.bat' 'Solana â€” tiny range guaranteed hit' 'cpu' @(
  'keyhunt.exe -m address -c sol -f tests\sol_sample.txt -r 1:8 -t 1 -q'
)

Write-Bat '01_address\sol_open.bat' 'Solana â€” open-ended scan' 'cpu' @(
  'keyhunt.exe -m address -c sol -f tests\sol_sample.txt -t 4 -q -s 5'
)

Write-Bat '01_address\timestamp_window.bat' 'Address search around Unix timestamp (-T)' 'cpu' @(
  'echo [+] ~4B key window from timestamp (demo uses tiny -b too if combined elsewhere)',
  'keyhunt.exe -m address -f tests\66.txt -T 1421345234 -b 40 -l compress -t 4 -q -s 10'
)

Write-Bat '01_address\strip_zeros.bat' 'Address â€” strip leading zero bytes (-Z) with -b' 'cpu' @(
  'keyhunt.exe -m address -f tests\66.txt -b 40 -Z 2 -l compress -t 4 -q -s 10'
)

# ---- 02_rmd160 ----
Write-Bat '02_rmd160\puzzle66.bat' 'RMD160 puzzle 66' 'cpu' @(
  'keyhunt.exe -m rmd160 -f tests\66.rmd -b 66 -l compress -t 8 -x sequential -q -s 10'
)

Write-Bat '02_rmd160\puzzle20_hit.bat' 'RMD160 puzzle 20 known hit' 'cpu' @(
  'keyhunt.exe -m rmd160 -f tests\_puzzle20.rmd -b 20 -l compress -t 2 -x sequential -q'
)

Write-Bat '02_rmd160\gravity.bat' 'RMD160 + gravity pattern' 'cpu' @(
  'keyhunt.exe -m rmd160 -f tests\66.rmd -b 66 -l compress -x gravity -t 8 -q -s 10'
)

Write-Bat '02_rmd160\with_derive.bat' 'RMD160 + BIP-44 derivation' 'cpu' @(
  'keyhunt.exe -m rmd160 -f tests\66.rmd -p "m/44''/0''/0''/0" -D 5 -l compress -t 4 -q -s 10'
)

# ---- 03_xpoint ----
Write-Bat '03_xpoint\g_hit.bat' 'X-point â€” generator G (privkey 1)' 'cpu' @(
  'keyhunt.exe -m xpoint -f tests\_xpoint_g.txt -r 1:20 -t 1 -x sequential -q'
)

Write-Bat '03_xpoint\spiral.bat' 'X-point + spiral pattern' 'cpu' @(
  'keyhunt.exe -m xpoint -f tests\_xpoint_g.txt -b 40 -x spiral -t 4 -q -s 10'
)

# ---- 04_bsgs ----
Write-Bat '04_bsgs\dry_run.bat' 'BSGS dry-run (-k auto -y)' 'cpu' @(
  'keyhunt.exe -m bsgs -f tests\125.txt -b 125 -k auto -y'
)

Write-Bat '04_bsgs\sequential_tiny.bat' 'BSGS sequential â€” tiny range hit (key 1)' 'cpu' @(
  'keyhunt.exe -m bsgs -f tests\_pubkey_g.txt -r 1:2 -n 1048576 -B sequential -t 1 -q'
)

Write-Bat '04_bsgs\random.bat' 'BSGS random giant steps' 'cpu' @(
  'keyhunt.exe -m bsgs -f tests\125.txt -b 125 -R -k 512 -t 4 -S -q -s 10'
)

Write-Bat '04_bsgs\backward.bat' 'BSGS backward' 'cpu' @(
  'keyhunt.exe -m bsgs -f tests\125.txt -b 125 -B backward -k 256 -t 4 -q -s 10'
)

Write-Bat '04_bsgs\both.bat' 'BSGS both (top/bottom)' 'cpu' @(
  'keyhunt.exe -m bsgs -f tests\125.txt -b 125 -B both -k 256 -t 4 -q -s 10'
)

Write-Bat '04_bsgs\dance.bat' 'BSGS dance' 'cpu' @(
  'keyhunt.exe -m bsgs -f tests\125.txt -b 125 -B dance -k 256 -t 4 -q -s 10'
)

Write-Bat '04_bsgs\rseq.bat' 'BSGS random-sequential (-B rseq / --walk)' 'cpu' @(
  'keyhunt.exe -m bsgs -f tests\125.txt -b 125 -B rseq --walk 2M -k 256 -t 4 -q -s 10'
)

# ---- 05_kangaroo ----
Write-Bat '05_kangaroo\tiny_hit.bat' 'Kangaroo tiny range (key 1)' 'cpu' @(
  'keyhunt.exe -m kangaroo -f tests\_pubkey_g.txt -r 1:1000 -t 1 -q'
)

Write-Bat '05_kangaroo\bit40.bat' 'Kangaroo -b 40 known-hit (DP)' 'cpu' @(
  'if not exist "tests\_pubkey_b40.txt" (',
  '  echo [E] Missing fixture: tests\_pubkey_b40.txt',
  '  exit /b 1',
  ')',
  'keyhunt.exe -m kangaroo -f tests\_pubkey_b40.txt -b 40 -t 4 -q'
)

Write-Bat '05_kangaroo\cuda.bat' 'Kangaroo CUDA' 'cuda' @(
  'keyhunt_cuda.exe -m kangaroo -f tests\_pubkey_g.txt -r 1:1000 -U cuda -t 1 -q'
)

# ---- 06_vanity ----
Write-Bat '06_vanity\prefix_1Cool.bat' 'Vanity prefix 1Cool + endomorphism' 'cpu' @(
  'keyhunt.exe -m vanity -v 1Cool -e -t 8 -q -s 10'
)

Write-Bat '06_vanity\prefix_1Bg_hit.bat' 'Vanity 1Bg â€” hits privkey 1 quickly' 'cpu' @(
  'keyhunt.exe -m vanity -v 1Bg -r 1:20 -l compress -t 1 -x sequential -q'
)

# ---- 07_minikeys ----
Write-Bat '07_minikeys\known_hit.bat' 'Minikeys known hit (S4b3...Dwf)' 'cpu' @(
  'keyhunt.exe -m minikeys -f tests\_minikey_known.txt -C S4b3N3oGqDqR5jNuxEvDwe -t 1 -q'
)

Write-Bat '07_minikeys\grind.bat' 'Minikeys open grind vs puzzle66 addr' 'cpu' @(
  'keyhunt.exe -m minikeys -f tests\66.txt -t 4 -q -s 10'
)

# ---- 08_mnemonic ----
Write-Bat '08_mnemonic\lastword_hit.bat' 'Mnemonic last-word recovery (guaranteed hit)' 'cpu' @(
  'echo 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA> tests\_mnemonic_abandon.txt',
  'keyhunt.exe -m mnemonic -w 12 -L english -D 1 -t 1 -q -s 5 -f tests\_mnemonic_abandon.txt --seed "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon ?"'
)

Write-Bat '08_mnemonic\fullseed_hit.bat' 'Mnemonic fully-known seed (one-shot)' 'cpu' @(
  'echo 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA> tests\_mnemonic_abandon.txt',
  'keyhunt.exe -m mnemonic -w 12 -L english -D 1 -t 1 -q -f tests\_mnemonic_abandon.txt --seed "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"'
)

Write-Bat '08_mnemonic\random_12.bat' 'Mnemonic random 12-word English grind' 'cpu' @(
  'keyhunt.exe -m mnemonic -w 12 -L english -D 5 -f tests\66.txt -t 4 -q -s 10'
)

Write-Bat '08_mnemonic\random_24.bat' 'Mnemonic random 24-word' 'cpu' @(
  'keyhunt.exe -m mnemonic -w 24 -L english -D 5 -f tests\66.txt -t 4 -q -s 10'
)

Write-Bat '08_mnemonic\all_langs.bat' 'Mnemonic all BIP-39 languages (-L all)' 'cpu' @(
  'keyhunt.exe -m mnemonic -w 12 -L all -D 1 -f tests\66.txt -t 4 -q -s 10'
)

Write-Bat '08_mnemonic\eth.bat' 'Mnemonic Ethereum (-W)' 'cpu' @(
  'keyhunt.exe -m mnemonic -w 12 -L english -W -D 5 -f tests\_eth1.txt -t 4 -q -s 10'
)

Write-Bat '08_mnemonic\mask.bat' 'Mnemonic mask submode (--submode mask)' 'cpu' @(
  'echo 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA> tests\_mnemonic_abandon.txt',
  'keyhunt.exe -m mnemonic -w 12 -L english -D 1 -t 1 -q -f tests\_mnemonic_abandon.txt --submode mask --seed "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon ? about"'
)

# ---- 09_poetry ----
Write-Bat '09_poetry\grind.bat' 'Poetry mode grind' 'cpu' @(
  'if not exist "tests\poetry.txt" (echo [E] missing poetry wordlist & exit /b 1)',
  'keyhunt.exe -m poetry -f tests\66.txt -t 4 -q -s 10'
)

Write-Bat '09_poetry\dry.bat' 'Poetry dry-run' 'cpu' @(
  'keyhunt.exe -m poetry -f tests\66.txt -y'
)

# ---- 10_brainwallet ----
Write-Bat '10_brainwallet\words3.bat' 'Brainwallet 3-word + mutations' 'cpu' @(
  'if not exist "tests\brainwalletwords.txt" (echo [E] missing wordlist & exit /b 1)',
  'keyhunt.exe -m brainwallet -w 3 -f tests\66.txt -t 4 -q -s 10'
)

Write-Bat '10_brainwallet\dry.bat' 'Brainwallet dry-run' 'cpu' @(
  'keyhunt.exe -m brainwallet -f tests\66.txt -y'
)

# ---- 11_pubkey2addr ----
Write-Bat '11_pubkey2addr\btc_auto.bat' 'pubkey2addr BTC -x auto' 'cpu' @(
  'keyhunt.exe -m pubkey2addr -f tests\66.txt -x auto -t 4 -q -s 10'
)

Write-Bat '11_pubkey2addr\btc_hit_range.bat' 'pubkey2addr sequential hit in tiny range' 'cpu' @(
  'keyhunt.exe -m pubkey2addr -f tests\_btc1.txt -r 1:20 -x sequential -t 1 -q'
)

Write-Bat '11_pubkey2addr\eth.bat' 'pubkey2addr ETH' 'cpu' @(
  'keyhunt.exe -m pubkey2addr -c eth -f tests\_eth1.txt -t 4 -q -s 10'
)

# ---- 12_patterns (-x) ----
$patterns = @('sequential','random','chaos','gravity','spiral','reverse','auto','rseq','hilbert','sobol','halton')
foreach ($p in $patterns) {
  Write-Bat "12_patterns\x_$p.bat" "Search pattern -x $p (address key1)" 'cpu' @(
    "keyhunt.exe -m address -f tests\_btc1.txt -r 1:80 -l compress -t 1 -x $p -q"
  ) @("Pattern: $p")
}

Write-Bat '12_patterns\rs_flag.bat' 'Random-sequential via -rs flag' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc1.txt -r 1:200 -rs -n 0x400 -l compress -t 1 -q'
)

Write-Bat '12_patterns\walk_2M.bat' 'Collider --mode rseq --walk 2M' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc1.txt -r 1:1000000 --mode rseq --walk 2M -l compress -t 2 -q -s 5'
)

# ---- 13_crypto ----
$cryptos = @(
  @{c='btc'; f='tests\_btc1.txt'; note='Bitcoin'},
  @{c='eth'; f='tests\_eth1.txt'; note='Ethereum'},
  @{c='sol'; f='tests\sol_sample.txt'; note='Solana'}
)
foreach ($cr in $cryptos) {
  Write-Bat "13_crypto\c_$($cr.c).bat" "Currency -c $($cr.c) ($($cr.note))" 'cpu' @(
    "if not exist `"$($cr.f)`" (echo [E] missing $($cr.f) & exit /b 1)",
    "keyhunt.exe -m address -c $($cr.c) -f $($cr.f) -t 2 -q -s 5"
  )
}

# Cryptos without fixtures â†’ dry-run style / user file placeholder
foreach ($c in @('ltc','doge','xrp','bch','btg','etc','troot')) {
  Write-Bat "13_crypto\c_$c`_needs_targets.bat" "Currency -c $c (edit TARGETS=)" 'cpu' @(
    "set `"TARGETS=targets_$c.txt`"",
    "if not exist `"%TARGETS%`" (",
    "  echo Create %TARGETS% with one $c address per line, then re-run.",
    "  exit /b 1",
    ")",
    "keyhunt.exe -m address -c $c -f %TARGETS% -t 4 -q -s 10"
  ) @('No bundled fixture - provide your own target file.')
}

Write-Bat '13_crypto\c_auto.bat' 'Auto-detect currency from file (-c auto)' 'cpu' @(
  'keyhunt.exe -m address -c auto -f tests\_btc1.txt -r 1:20 -t 1 -x sequential -q'
)

Write-Bat '13_crypto\c_all.bat' 'Search all currencies (-c all) â€” needs mixed file' 'cpu' @(
  'set "TARGETS=targets_mixed.txt"',
  'if not exist "%TARGETS%" (',
  '  echo Create targets_mixed.txt with mixed addresses, then re-run.',
  '  exit /b 1',
  ')',
  'keyhunt.exe -m address -c all -f %TARGETS% -t 4 -q -s 10'
)

# ---- 14_gpu ----
Write-Bat '14_gpu\cuda_address_66.bat' 'CUDA address puzzle 66' 'cuda' @(
  'keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -t 1 -q -s 5'
) @('Do NOT pass -e with -U cuda')

Write-Bat '14_gpu\cuda_puzzle20_hit.bat' 'CUDA puzzle 20 known hit' 'cuda' @(
  'keyhunt_cuda.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -U cuda -M auto -t 1 -x sequential -q'
)

Write-Bat '14_gpu\cuda_both_hybrid.bat' 'Hybrid -U both (CPU+CUDA threads)' 'cuda' @(
  'keyhunt_cuda.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -U both -M auto -t 4 -x sequential -q'
) @('-U both != -l both')

Write-Bat '14_gpu\cuda_rmd160.bat' 'CUDA rmd160' 'cuda' @(
  'keyhunt_cuda.exe -m rmd160 -f tests\_puzzle20.rmd -b 20 -l compress -U cuda -M auto -t 1 -x sequential -q'
)

Write-Bat '14_gpu\cuda_sol.bat' 'CUDA Solana' 'cuda' @(
  'keyhunt_cuda.exe -m address -c sol -f tests\sol_sample.txt -r 1:20 -U cuda -G 256 -t 1 -x sequential -q'
)

Write-Bat '14_gpu\cuda_vanity.bat' 'CUDA vanity' 'cuda' @(
  'keyhunt_cuda.exe -m vanity -v 1Bg -r 1:50 -l compress -U cuda -G 256 -t 1 -x sequential -q'
)

Write-Bat '14_gpu\cuda_xpoint.bat' 'CUDA xpoint' 'cuda' @(
  'keyhunt_cuda.exe -m xpoint -f tests\_xpoint_g.txt -r 1:50 -U cuda -G 256 -t 1 -x sequential -q'
)

Write-Bat '14_gpu\cuda_minikeys.bat' 'CUDA minikeys known hit' 'cuda' @(
  'keyhunt_cuda.exe -m minikeys -f tests\_minikey_known.txt -C S4b3N3oGqDqR5jNuxEvDwe -U cuda -t 1 -q'
)

Write-Bat '14_gpu\cuda_mnemonic_lastword.bat' 'CUDA mnemonic last-word hit' 'cuda' @(
  'echo 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA> tests\_mnemonic_abandon.txt',
  'keyhunt_cuda.exe -m mnemonic -w 12 -L english -D 1 -t 1 -U cuda -M auto -q -f tests\_mnemonic_abandon.txt --seed "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon ?"'
)

Write-Bat '14_gpu\cuda_dry.bat' 'CUDA dry-run memory plan' 'cuda' @(
  'keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -y'
)

Write-Bat '14_gpu\cuda_batch_G.bat' 'CUDA with explicit -G batch' 'cuda' @(
  'keyhunt_cuda.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -U cuda -G 128 -M auto -t 1 -x sequential -q'
)

# ---- 15_research / special modes ----
Write-Bat '15_research\shadow160_dry.bat' 'Mode shadow160 (prefix hash160)' 'cpu' @(
  'keyhunt.exe -m shadow160 -f tests\_puzzle20.rmd -b 20 -y'
)

Write-Bat '15_research\weakrng_milksad_dry.bat' 'Mode weakrng / milksad dry' 'cpu' @(
  'keyhunt.exe -m weakrng --submode milksad -T 1514764800:1514851200 -f tests\_btc1.txt -y'
)

Write-Bat '15_research\hex_mask_hit.bat' 'hex-mask â€” last nibble free (hits key 1)' 'cpu' @(
  'keyhunt.exe -m hex-mask --key-mask 000000000000000000000000000000000000000000000000000000000000000? -f tests\_btc1.txt -t 1 -q'
)

Write-Bat '15_research\wif_mask_dry.bat' 'wif-mask dry-run' 'cpu' @(
  'keyhunt.exe -m wif-mask --wif-mask L????????????????????????????????????? -f tests\_btc1.txt -y'
)

Write-Bat '15_research\kangaroo_mod_dry.bat' 'kangaroo-mod dry-run' 'cpu' @(
  'keyhunt.exe -m kangaroo-mod -f tests\_pubkey_g.txt -b 40 --mod-step 8 --mod-rem 0 -y'
)

Write-Bat '15_research\hybrid_dl_dry.bat' 'hybrid-dl (HerdHandoff) dry' 'cpu' @(
  'keyhunt.exe -m hybrid-dl -f tests\125.txt -b 40 -H 20 -y'
)

Write-Bat '15_research\gaudry_dry.bat' 'gaudry / ResidueHerd dry' 'cpu' @(
  'keyhunt.exe -m gaudry -f tests\_pubkey_g.txt -b 40 --mod-step 4 --mod-rem 1 -y'
)

Write-Bat '15_research\CreateAccountWithSeed_dry.bat' 'Solana CreateAccountWithSeed dry' 'cpu' @(
  'keyhunt.exe -m CreateAccountWithSeed -f tests\sol_sample.txt -y'
)

# ---- 16_options ----
Write-Bat '16_options\dry_run_cpu.bat' 'Dry-run -y (CPU plan)' 'cpu' @(
  'keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -A auto -y'
)

Write-Bat '16_options\vector_sse.bat' 'CPU vector -A sse' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc1.txt -r 1:50 -l compress -A sse -t 1 -x sequential -q'
)

Write-Bat '16_options\vector_auto.bat' 'CPU vector -A auto' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc1.txt -r 1:50 -l compress -A auto -t 1 -x sequential -q'
)

Write-Bat '16_options\vector_none.bat' 'CPU vector -A none (scalar)' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc1.txt -r 1:50 -l compress -A none -t 1 -x sequential -q'
)

Write-Bat '16_options\quiet_stats.bat' 'Quiet -q + stats -s 5' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc1.txt -r 1:100 -l compress -t 1 -x sequential -q -s 5'
)

Write-Bat '16_options\verbose_path.bat' 'Verbose derivation -V' 'cpu' @(
  'keyhunt.exe -m address -f tests\66.txt -p "m/84''/0''/0''/0" -D 3 -V -t 2 -q -s 10'
)

Write-Bat '16_options\stride.bat' 'Custom stride -I' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc1.txt -r 1:200 -I 2 -l compress -t 1 -x sequential -q'
)

Write-Bat '16_options\bloom_mult.bat' 'Bloom size multiplier -z' 'cpu' @(
  'keyhunt.exe -m address -f tests\66.txt -b 40 -z 4 -l compress -t 4 -q -s 10'
)

Write-Bat '16_options\random_R_q.bat' 'Legacy -R -q (must NOT warn about submode)' 'cpu' @(
  'keyhunt.exe -m address -f tests\_btc1.txt -r 1:50 -l compress -R -q -t 1 -x sequential'
)

# ---- 17_puzzles ----
Write-Bat '17_puzzles\b40_demo.bat' 'Puzzle-style -b 40 demo (small)' 'cpu' @(
  'keyhunt.exe -m address -f tests\66.txt -b 40 -l compress -R -A auto -t 8 -q -s 10'
)

Write-Bat '17_puzzles\b66.bat' 'Puzzle 66 address' 'cpu' @(
  'keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -A auto -t 8 -q -s 10'
)

Write-Bat '17_puzzles\b72_timestamp.bat' 'Puzzle 72 + funding timestamp' 'cpu' @(
  'if not exist "tests\unsolvedpuzzles.rmd" (',
  '  echo [i] Using 66.rmd as stand-in if unsolved file missing',
  '  keyhunt.exe -m rmd160 -f tests\66.rmd -b 72 -T 1421345234 -l compress -t 8 -x auto -q -s 10',
  ') else (',
  '  keyhunt.exe -m rmd160 -f tests\unsolvedpuzzles.rmd -b 72 -T 1421345234 -l compress -t 8 -x auto -q -s 10',
  ')'
)

Write-Bat '17_puzzles\b20_solved_hit.bat' 'Solved puzzle #20 (verify tooling)' 'cpu' @(
  'keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x sequential -q'
)

# ---- RUN_ALL_SMOKE ----
@'
@echo off
REM Run quick known-hit + dry-run smoke across major modes (CPU; CUDA if present)
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0.."
if not exist keyhunt.exe (
  echo [E] Build first: bats\00_build\build_cpu.bat
  exit /b 1
)
set PASS=0
set FAIL=0

call :run "addr_key1" keyhunt.exe -m address -f tests\_btc1.txt -r 1:20 -l compress -t 1 -x sequential -q
call :run "puzzle20" keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x sequential -q
call :run "rmd20" keyhunt.exe -m rmd160 -f tests\_puzzle20.rmd -b 20 -l compress -t 2 -x sequential -q
call :run "xpoint" keyhunt.exe -m xpoint -f tests\_xpoint_g.txt -r 1:10 -t 1 -x sequential -q
call :run "vanity" keyhunt.exe -m vanity -v 1Bg -r 1:10 -l compress -t 1 -x sequential -q
call :run "sol" keyhunt.exe -m address -c sol -f tests\sol_sample.txt -r 1:8 -t 1 -q
call :run "eth" keyhunt.exe -m address -c eth -f tests\_eth1.txt -r 1:5 -t 1 -x sequential -q
call :run "kangaroo" keyhunt.exe -m kangaroo -f tests\_pubkey_g.txt -r 1:1000 -t 1 -q
call :run "bsgs" keyhunt.exe -m bsgs -f tests\_pubkey_g.txt -r 1:2 -n 1048576 -B sequential -t 1 -q
call :run "minikeys" keyhunt.exe -m minikeys -f tests\_minikey_known.txt -C S4b3N3oGqDqR5jNuxEvDwe -t 1 -q
echo 1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA> tests\_mnemonic_abandon.txt
call :run "mnemonic" keyhunt.exe -m mnemonic -w 12 -L english -D 1 -t 1 -q -f tests\_mnemonic_abandon.txt --seed "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
call :run "dry" keyhunt.exe -m address -f tests\66.txt -b 66 -y

if exist keyhunt_cuda.exe (
  call :run "cuda20" keyhunt_cuda.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -U cuda -M auto -t 1 -x sequential -q
)

echo.
echo PASS=%PASS% FAIL=%FAIL%
if %FAIL% GTR 0 exit /b 1
exit /b 0

:run
set "NAME=%~1"
shift
echo ===== %NAME% =====
%*
if errorlevel 1 (
  echo FAIL %NAME%
  set /a FAIL+=1
) else (
  echo PASS %NAME%
  set /a PASS+=1
)
goto :eof
'@ | Set-Content -Encoding ASCII (Join-Path $Bats 'RUN_ALL_SMOKE.bat')

# Count and README
$all = @(Get-ChildItem $Bats -Recurse -Filter *.bat)
$readmePath = Join-Path $Bats 'README.md'
@(
  '# TrueCollider - full mode bat library',
  '',
  'Double-click any script, or run from repo root:',
  '',
  '```bat',
  'bats\01_address\btc_puzzle66.bat',
  '```',
  '',
  'Every script cds to the repository root via bats\_common.bat.',
  '',
  '## Folders',
  '',
  '| Folder | Covers |',
  '|--------|--------|',
  '| 00_build | CPU / CUDA builds |',
  '| 01_address | -m address ranges, compress, endo, BIP paths, ETH/SOL, -N, -T, -Z |',
  '| 02_rmd160 | -m rmd160 |',
  '| 03_xpoint | -m xpoint |',
  '| 04_bsgs | -m bsgs + -B modes |',
  '| 05_kangaroo | -m kangaroo CPU/CUDA |',
  '| 06_vanity | -m vanity |',
  '| 07_minikeys | -m minikeys |',
  '| 08_mnemonic | BIP-39 random / lastword / mask / ETH / all langs |',
  '| 09_poetry | -m poetry |',
  '| 10_brainwallet | -m brainwallet |',
  '| 11_pubkey2addr | -m pubkey2addr |',
  '| 12_patterns | All -x patterns + -rs / --walk |',
  '| 13_crypto | -c currencies (btc/eth/sol + placeholders) |',
  '| 14_gpu | -U cuda / both, batch, mnemonic CUDA |',
  '| 15_research | shadow160, weakrng, hex-mask, wif-mask, kangaroo-mod, hybrid-dl, gaudry |',
  '| 16_options | -y, -A, -q, -V, -I, -z, -R -q |',
  '| 17_puzzles | Puzzle bit-range recipes |',
  '',
  'Smoke everything quick: bats\RUN_ALL_SMOKE.bat',
  '',
  '## Important rules',
  '',
  '- Build first: bats\00_build\build_cpu.bat (and build_cuda.bat for GPU).',
  '- Do NOT use -e with -U cuda / -U both.',
  '- -e does NOT help small puzzle -b ranges; omit it for puzzles.',
  '- Bare -R = random search. Research uses --submode NAME (or -R NAME only if NAME is a real submode).',
  '- -U both = hybrid CPU+CUDA threads. -l both = compress+uncompress.',
  '',
  "## Counts",
  '',
  "Generated $($all.Count) bat files (including helpers / smoke).",
  '',
  'Full flag docs: [../README.md](../README.md) / [../docs/COMMANDS.md](../docs/COMMANDS.md)'
) | Set-Content -Encoding UTF8 $readmePath

Write-Host "Generated $($all.Count) bat files under $Bats"
$all | Group-Object { $_.Directory.Name } | Sort-Object Name | ForEach-Object {
  Write-Host ("  {0,-20} {1}" -f $_.Name, $_.Count)
}

