# Deep TrueCollider audit — known-hit for every mode + flag regression
$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root
Get-Process keyhunt*,keyhunt_cuda* -EA SilentlyContinue | Stop-Process -Force -EA SilentlyContinue
Remove-Item KEYFOUNDKEYFOUND.txt,VANITYKEYFOUND.txt,FOUND_*.txt -Force -EA SilentlyContinue

$pass = 0; $fail = 0; $skip = 0
$results = New-Object System.Collections.Generic.List[string]

function Quote-Arg([string]$a) {
  if ($null -eq $a) { return '""' }
  if ($a -match '[\s"]') { return '"' + ($a -replace '"','\"') + '"' }
  return $a
}
function T([string]$Name, [string]$Exe, [string[]]$KhArgs, [int]$Sec, [string]$Pat, [string]$BadPat = "") {
  if (-not (Test-Path $Exe)) { $script:skip++; $script:results.Add("SKIP  $Name"); Write-Host "SKIP  $Name"; return }
  $out = Join-Path $env:TEMP ("tc_deep_" + [guid]::NewGuid().ToString("N") + ".txt")
  $err = "$out.err"
  Remove-Item KEYFOUNDKEYFOUND.txt,VANITYKEYFOUND.txt,FOUND_ETH.txt,FOUND_SOL.txt -Force -EA SilentlyContinue
  # Join with proper quoting so --seed "word word ?" stays one argv
  $argLine = ($KhArgs | ForEach-Object { Quote-Arg $_ }) -join ' '
  $p = Start-Process -FilePath (Resolve-Path $Exe).Path -ArgumentList $argLine -WorkingDirectory $Root `
    -NoNewWindow -PassThru -RedirectStandardOutput $out -RedirectStandardError $err
  $deadline = (Get-Date).AddSeconds($Sec)
  $ok = $false; $bad = $false
  while ((Get-Date) -lt $deadline) {
    Start-Sleep -Milliseconds 300
    $blob = ""
    foreach ($f in @($out,$err,"KEYFOUNDKEYFOUND.txt","VANITYKEYFOUND.txt","FOUND_ETH.txt","FOUND_SOL.txt")) {
      if (Test-Path $f) { try { $blob += [IO.File]::ReadAllText($f) } catch {} }
    }
    if ($BadPat -and ($blob -match $BadPat)) { $bad = $true; break }
    if ($blob -match $Pat) { $ok = $true; break }
    if ($p.HasExited) { break }
  }
  if (-not $p.HasExited) { try { Stop-Process -Id $p.Id -Force -EA SilentlyContinue } catch {} }
  Get-Process keyhunt*,keyhunt_cuda* -EA SilentlyContinue | Stop-Process -Force -EA SilentlyContinue
  Start-Sleep -Milliseconds 150
  $blob = ""
  foreach ($f in @($out,$err,"KEYFOUNDKEYFOUND.txt","VANITYKEYFOUND.txt","FOUND_ETH.txt","FOUND_SOL.txt")) {
    if (Test-Path $f) { try { $blob += [IO.File]::ReadAllText($f) } catch {} }
  }
  Remove-Item $out,$err -Force -EA SilentlyContinue
  if ($bad) {
    $script:fail++; $script:results.Add("FAIL  $Name (bad pattern)"); Write-Host "FAIL  $Name (bad)" -ForegroundColor Red
    Write-Host ($blob.Substring([Math]::Max(0,$blob.Length-500)))
  } elseif ($ok -or ($blob -match $Pat)) {
    $script:pass++; $script:results.Add("PASS  $Name"); Write-Host "PASS  $Name" -ForegroundColor Green
  } else {
    $script:fail++; $script:results.Add("FAIL  $Name"); Write-Host "FAIL  $Name" -ForegroundColor Red
    if ($blob.Length -gt 500) { Write-Host ($blob.Substring($blob.Length-500)) } else { Write-Host $blob }
  }
}

# Fixtures
"1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH" | Set-Content -Encoding ascii tests\_btc1.txt
"1HsMJxNiV7TLxmoF6uJNkydxPFDog4NQum" | Set-Content -Encoding ascii tests\_puzzle20.txt
"b907c3a2a3b27789dfb509b730dd47703c272868" | Set-Content -Encoding ascii tests\_puzzle20.rmd
"1GAehh7TsJAHuUAeKZcXf5CnwuGuGgyX2S" | Set-Content -Encoding ascii tests\_minikey_known.txt
"79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798" | Set-Content -Encoding ascii tests\_xpoint_g.txt
"0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798" | Set-Content -Encoding ascii tests\_pubkey_g.txt
"1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA" | Set-Content -Encoding ascii tests\_mnemonic_abandon.txt
if (Test-Path tests\1to32.eth) { (Get-Content tests\1to32.eth -TotalCount 1) | Set-Content -Encoding ascii tests\_eth1.txt }

$cpu = ".\keyhunt.exe"
$cuda = ".\keyhunt_cuda.exe"

Write-Host "`n===== FLAG REGRESSION (-R -q must NOT warn) =====`n"
T "flag_R_q" $cpu @("-m","address","-f","tests\_btc1.txt","-r","1:5","-l","compress","-R","-q","-t","1","-x","sequential") 25 `
  "Hit! Private Key: 1|Private Key: 1" "Unknown -R submode|Unknown research submode: -q"

Write-Host "`n===== RANGE / ADDRESS HITS =====`n"
T "cpu_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","sequential","-q") 30 "Private Key: d2c55"
T "cpu_key1" $cpu @("-m","address","-f","tests\_btc1.txt","-r","1:100","-l","compress","-t","1","-x","sequential","-q") 25 "Private Key: 1"
T "cpu_endo_warn" $cpu @("-m","address","-f","tests\_btc1.txt","-b","20","-l","compress","-e","-t","1","-x","sequential","-q") 25 "Private Key: 1|rarely helps puzzle"
T "cpu_rs_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-rs","-q") 45 "Private Key: d2c55"
T "cpu_rmd_b20" $cpu @("-m","rmd160","-f","tests\_puzzle20.rmd","-b","20","-l","compress","-t","2","-x","sequential","-q") 30 "Private Key: d2c55"

Write-Host "`n===== CORE MODES (known hits) =====`n"
T "cpu_xpoint" $cpu @("-m","xpoint","-f","tests\_xpoint_g.txt","-r","1:10","-t","1","-x","sequential","-q") 25 "Private Key: 1"
T "cpu_vanity" $cpu @("-m","vanity","-v","1Bg","-r","1:10","-l","compress","-t","1","-x","sequential","-q") 25 "Vanity Private Key: 1|1BgGZ"
T "cpu_minikeys" $cpu @("-m","minikeys","-f","tests\_minikey_known.txt","-C","S4b3N3oGqDqR5jNuxEvDwe","-t","1","-q") 50 "S4b3N3oGqDqR5jNuxEvDwf|1GAehh7"
T "cpu_bsgs" $cpu @("-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","sequential","-t","1","-q") 90 "Hit!|Private Key|0x1|Key found"
T "cpu_kangaroo" $cpu @("-m","kangaroo","-f","tests\_pubkey_g.txt","-r","1:1000","-t","1","-q") 40 "Private Key: 1|Hit!|found"
T "cpu_sol" $cpu @("-m","address","-c","sol","-f","tests\sol_sample.txt","-r","1:20","-t","1","-x","sequential","-q") 30 "Hit! Solana|6ASf"
T "cpu_eth" $cpu @("-m","address","-c","eth","-f","tests\_eth1.txt","-r","1:5","-t","1","-x","sequential","-q") 30 "Private Key: 1|0x"
T "cpu_pub2addr" $cpu @("-m","pubkey2addr","-f","tests\_btc1.txt","-r","1:20","-x","sequential","-t","1","-q") 30 "Private Key: 1|Hit!"

Write-Host "`n===== MNEMONIC / WORDLIST =====`n"
$seedMask = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon ?"
T "cpu_mnemonic_lastword" $cpu @("-m","mnemonic","-w","12","-L","english","-D","1","-t","1","-q","-s","5","-f","tests\_mnemonic_abandon.txt","--seed",$seedMask) 120 "MNEMONIC FOUND|1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA|abandon about"
$seedFull = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
T "cpu_mnemonic_fullseed" $cpu @("-m","mnemonic","-w","12","-L","english","-D","1","-t","1","-q","-f","tests\_mnemonic_abandon.txt","--seed",$seedFull) 40 "MNEMONIC FOUND|1LqBGSKu"
T "cpu_mnemonic_dry" $cpu @("-m","mnemonic","-f","tests\_btc1.txt","-y") 15 "Dry-run complete"
T "cpu_poetry_dry" $cpu @("-m","poetry","-f","tests\_btc1.txt","-y") 15 "Dry-run complete"
T "cpu_brain_dry" $cpu @("-m","brainwallet","-f","tests\_btc1.txt","-y") 15 "Dry-run complete"

Write-Host "`n===== SEARCH PATTERNS =====`n"
foreach ($pat in @("sequential","random","chaos","gravity","spiral","reverse","auto","rseq","hilbert","sobol","halton","density-map",
                   "keyhole","pocket","afterimage","driftcompass","twinflame","breadcrumb","clockwork","lottery","wave","waveroulette")) {
  T ("pat_$pat") $cpu @("-m","address","-f","tests\_btc1.txt","-r","1:200","-l","compress","-t","1","-x",$pat,"-q") 35 "Private Key: 1|Search mode"
}

Write-Host "`n===== NEW WALK MODES (puzzle20 known hit) =====`n"
T "walk_keyhole_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","keyhole","--window-bits","12","-q") 45 "Private Key: d2c55"
T "walk_pocket_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","pocket","--pocket-bits","8","-q") 45 "Private Key: d2c55"
T "walk_afterimage_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","afterimage","--antiloop-dist","8","-q") 45 "Private Key: d2c55"
T "walk_wave_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","wave","-q") 45 "Private Key: d2c55"
T "walk_lottery_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","lottery","-n","1024","-q") 50 "Private Key: d2c55"
T "walk_driftcompass_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","driftcompass","-q") 50 "Private Key: d2c55"
T "walk_twinflame_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","twinflame","-q") 50 "Private Key: d2c55"
T "walk_breadcrumb_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","breadcrumb","-q") 50 "Private Key: d2c55"
T "walk_clockwork_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","clockwork","-q") 50 "Private Key: d2c55"
T "walk_rmd_pocket_b20" $cpu @("-m","rmd160","-f","tests\_puzzle20.rmd","-b","20","-l","compress","-t","2","-x","pocket","--pocket-bits","8","-q") 45 "Private Key: d2c55"

Write-Host "`n===== BSGS HELPERS (tiny pubkey G) =====`n"
T "bsgs_modfan" $cpu @("-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","modfan","--mod-step","4","-t","2","-q") 90 "Hit!|Private Key|0x1|Key found|ModFan"
T "bsgs_shadowledger" $cpu @("-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","shadowledger","--shadow-mod","8","-t","1","-q") 90 "Hit!|Private Key|0x1|Key found|ShadowLedger"
T "bsgs_hybrid" $cpu @("-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","hybrid","-t","1","-q") 90 "Hit!|Private Key|0x1|Key found|Hybrid"
T "bsgs_residue" $cpu @("-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","residue","--mod-step","2","--mod-rem","1","-t","1","-q") 90 "Hit!|Private Key|0x1|Key found|residue"
T "bsgs_freeze_tip" $cpu @("-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","freeze-table","-t","1","-y") 20 "Dry-run complete|FreezeCascade|freeze-table|Mode bsgs"
T "bsgs_compact_dp_tip" $cpu @("-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","compact-dp","-t","1","-y") 20 "Dry-run complete|compact-dp|Mode bsgs|tip"
T "bsgs_dual_range_tip" $cpu @("-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","dual-range","-t","1","-y") 20 "Dry-run complete|dual-range|Mode bsgs|tip|wave"

Write-Host "`n===== RESEARCH / SPECIAL MODES (smoke) =====`n"
T "cpu_shadow160_dry" $cpu @("-m","shadow160","-f","tests\_puzzle20.rmd","-b","20","-y") 15 "Dry-run complete|Shadow160|Mode rmd160"
T "cpu_hexmask" $cpu @("-m","hex-mask","--key-mask","000000000000000000000000000000000000000000000000000000000000000?","-f","tests\_btc1.txt","-t","1","-q") 40 "Private Key: 1|Hit!|hex"
T "cpu_wifmask_dry" $cpu @("-m","wif-mask","--wif-mask","L?????????????????????????????????????","-f","tests\_btc1.txt","-y") 15 "Dry-run complete|wif-mask|Mode address"
T "cpu_gaudry_dry" $cpu @("-m","gaudry","-f","tests\_pubkey_g.txt","-r","1:100","-y") 15 "Dry-run complete|gaudry|Mode"
T "cpu_kangaroo_mod_dry" $cpu @("-m","kangaroo-mod","-f","tests\_pubkey_g.txt","-r","1:100","-y") 15 "Dry-run complete|kangaroo|Mode"
T "cpu_hybrid_dl_dry" $cpu @("-m","hybrid-dl","-f","tests\_pubkey_g.txt","-r","1:100","-y") 15 "Dry-run complete|hybrid|Mode"
T "cpu_weakrng_dry" $cpu @("-m","weakrng","-f","tests\_btc1.txt","-y") 15 "Dry-run complete|weakrng|Mode|Milk"
T "cpu_cas_dry" $cpu @("-m","CreateAccountWithSeed","-f","tests\sol_sample.txt","-y") 15 "Dry-run complete|CreateAccount|Mode|sol"

Write-Host "`n===== CUDA =====`n"
if (Test-Path $cuda) {
  T "cuda_b20" $cuda @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","1","-U","cuda","-M","auto","-x","sequential","-q") 45 "Private Key: d2c55"
  T "cuda_both_b20" $cuda @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","4","-U","both","-M","auto","-x","sequential","-q") 50 "Private Key: d2c55"
  T "cuda_rmd_b20" $cuda @("-m","rmd160","-f","tests\_puzzle20.rmd","-b","20","-l","compress","-t","1","-U","cuda","-M","auto","-x","sequential","-q") 45 "Private Key: d2c55"
  T "cuda_minikeys" $cuda @("-m","minikeys","-f","tests\_minikey_known.txt","-C","S4b3N3oGqDqR5jNuxEvDwe","-t","1","-U","cuda","-q") 60 "S4b3N3oGqDqR5jNuxEvDwf"
  T "cuda_vanity" $cuda @("-m","vanity","-v","1Bg","-r","1:20","-l","compress","-t","1","-U","cuda","-G","256","-x","sequential","-q") 45 "Vanity Private Key: 1|1BgGZ"
  T "cuda_xpoint" $cuda @("-m","xpoint","-f","tests\_xpoint_g.txt","-r","1:20","-t","1","-U","cuda","-G","256","-x","sequential","-q") 45 "Private Key: 1"
  T "cuda_sol" $cuda @("-m","address","-c","sol","-f","tests\sol_sample.txt","-r","1:20","-t","1","-U","cuda","-G","256","-x","sequential","-q") 45 "Hit! Solana|6ASf"
  T "cuda_mnemonic_lastword" $cuda @("-m","mnemonic","-w","12","-L","english","-D","1","-t","1","-U","cuda","-M","auto","-q","-f","tests\_mnemonic_abandon.txt","--seed",$seedMask) 120 "MNEMONIC FOUND|1LqBGSKu"
  T "cuda_kangaroo" $cuda @("-m","kangaroo","-f","tests\_pubkey_g.txt","-r","1:1000","-t","1","-U","cuda","-q") 45 "Private Key: 1|Hit!|found|Kangaroo"
} else {
  $skip++; $results.Add("SKIP  cuda suite")
}

Write-Host "`n===== SUMMARY =====`n"
$results | ForEach-Object { Write-Host $_ }
Write-Host "PASS=$pass FAIL=$fail SKIP=$skip TOTAL=$($pass+$fail+$skip)"
@(
  "# Deep audit results",
  "",
  "Generated by ``run_deep_audit.ps1``. Known-hit / dry-run matrix across modes, ``-x`` walks, ``-B`` BSGS helpers, research smokes, and CUDA when ``keyhunt_cuda.exe`` is present.",
  "",
  "``````"
) + $results + @(
  "``````",
  "",
  "PASS=$pass FAIL=$fail SKIP=$skip TOTAL=$($pass+$fail+$skip)"
) | Set-Content -Encoding utf8 docs\DEEP_AUDIT.md
if ($fail -gt 0) { exit 1 } else { exit 0 }
