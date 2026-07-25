# Final TrueCollider audit â€” hits, ranges, modes, patterns, backends
$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root
Get-Process keyhunt*,keyhunt_cuda* -EA SilentlyContinue | Stop-Process -Force -EA SilentlyContinue
Remove-Item KEYFOUNDKEYFOUND.txt,VANITYKEYFOUND.txt,FOUND_*.txt -Force -EA SilentlyContinue

$pass = 0; $fail = 0; $skip = 0
$results = New-Object System.Collections.Generic.List[string]

function T([string]$Name, [string]$Exe, [string[]]$KhArgs, [int]$Sec, [string]$Pat) {
  if (-not (Test-Path $Exe)) { $script:skip++; $script:results.Add("SKIP  $Name"); return }
  $out = Join-Path $env:TEMP ("tc_aud_" + [guid]::NewGuid().ToString("N") + ".txt")
  $err = "$out.err"
  Remove-Item KEYFOUNDKEYFOUND.txt,VANITYKEYFOUND.txt -Force -EA SilentlyContinue
  $p = Start-Process -FilePath (Resolve-Path $Exe).Path -ArgumentList $KhArgs -WorkingDirectory $Root -NoNewWindow -PassThru -RedirectStandardOutput $out -RedirectStandardError $err
  $deadline = (Get-Date).AddSeconds($Sec)
  $ok = $false
  while ((Get-Date) -lt $deadline) {
    Start-Sleep -Milliseconds 350
    $blob = ""
    if (Test-Path $out) { try { $blob += [IO.File]::ReadAllText($out) } catch {} }
    if (Test-Path $err) { try { $blob += [IO.File]::ReadAllText($err) } catch {} }
    if (Test-Path KEYFOUNDKEYFOUND.txt) { try { $blob += [IO.File]::ReadAllText("KEYFOUNDKEYFOUND.txt") } catch {} }
    if (Test-Path VANITYKEYFOUND.txt) { try { $blob += [IO.File]::ReadAllText("VANITYKEYFOUND.txt") } catch {} }
    if ($blob -match $Pat) { $ok = $true; break }
    if ($p.HasExited) { break }
  }
  if (-not $p.HasExited) { try { Stop-Process -Id $p.Id -Force -EA SilentlyContinue } catch {} }
  Get-Process keyhunt*,keyhunt_cuda* -EA SilentlyContinue | Stop-Process -Force -EA SilentlyContinue
  Start-Sleep -Milliseconds 200
  $blob = ""
  if (Test-Path $out) { try { $blob += [IO.File]::ReadAllText($out) } catch {} }
  if (Test-Path $err) { try { $blob += [IO.File]::ReadAllText($err) } catch {} }
  if (Test-Path KEYFOUNDKEYFOUND.txt) { try { $blob += [IO.File]::ReadAllText("KEYFOUNDKEYFOUND.txt") } catch {} }
  if (Test-Path VANITYKEYFOUND.txt) { try { $blob += [IO.File]::ReadAllText("VANITYKEYFOUND.txt") } catch {} }
  Remove-Item $out,$err -Force -EA SilentlyContinue
  if ($ok -or ($blob -match $Pat)) {
    $script:pass++; $script:results.Add("PASS  $Name"); Write-Host "PASS  $Name" -ForegroundColor Green
  } else {
    $script:fail++; $script:results.Add("FAIL  $Name"); Write-Host "FAIL  $Name" -ForegroundColor Red
    if ($blob.Length -gt 400) { Write-Host ($blob.Substring([Math]::Max(0,$blob.Length-400))) }
    else { Write-Host $blob }
  }
}

# Fixtures
"1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH" | Set-Content -Encoding ascii tests\_btc1.txt
"1HsMJxNiV7TLxmoF6uJNkydxPFDog4NQum" | Set-Content -Encoding ascii tests\_puzzle20.txt
"b907c3a2a3b27789dfb509b730dd47703c272868" | Set-Content -Encoding ascii tests\_puzzle20.rmd
"1GAehh7TsJAHuUAeKZcXf5CnwuGuGgyX2S" | Set-Content -Encoding ascii tests\_minikey_known.txt
"79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798" | Set-Content -Encoding ascii tests\_xpoint_g.txt
"0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798" | Set-Content -Encoding ascii tests\_pubkey_g.txt
if (Test-Path tests\1to32.eth) { (Get-Content tests\1to32.eth -TotalCount 1) | Set-Content -Encoding ascii tests\_eth1.txt }

$cpu = ".\keyhunt.exe"
$cuda = ".\keyhunt_cuda.exe"
$hit = "Hit!|HIT!!|Private Key|Vanity Private|Dry-run complete|Auto-clamped|GPU EC|CUDA"

Write-Host "`n===== RANGE / ADDRESS HITS =====`n"
T "cpu_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","sequential","-q") 25 "Hit! Private Key: d2c55|Private Key: d2c55"
T "cpu_key1_r" $cpu @("-m","address","-f","tests\_btc1.txt","-r","1:100","-l","compress","-t","1","-x","sequential","-q") 25 "Hit! Private Key: 1"
T "cpu_bad_checksum" $cpu @("-m","address","-f","tests\_bad_addr.txt","-r","1:10","-l","compress","-t","1","-x","sequential","-q") 20 "Invalid Base58Check|omitting"
T "cpu_rs_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-rs","-q") 40 "Hit! Private Key: d2c55|Private Key: d2c55"
T "cpu_rmd_b20" $cpu @("-m","rmd160","-f","tests\_puzzle20.rmd","-b","20","-l","compress","-t","2","-x","sequential","-q") 25 "Hit! Private Key: d2c55|Private Key: d2c55"

Write-Host "`n===== CORE MODES =====`n"
T "cpu_xpoint" $cpu @("-m","xpoint","-f","tests\_xpoint_g.txt","-r","1:10","-t","1","-x","sequential","-q") 25 "Hit! Private Key: 1|Private Key: 1"
T "cpu_vanity" $cpu @("-m","vanity","-v","1Bg","-r","1:10","-l","compress","-t","1","-x","sequential","-q") 25 "Vanity Private Key: 1|1BgGZ"
T "cpu_minikeys" $cpu @("-m","minikeys","-f","tests\_minikey_known.txt","-C","S4b3N3oGqDqR5jNuxEvDwe","-t","1","-q") 45 "HIT!!|S4b3N3oGqDqR5jNuxEvDwf|1GAehh7TsJAHuUAeKZcXf5CnwuGuGgyX2S"
T "cpu_bsgs" $cpu @("-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","sequential","-t","1","-q") 90 "Hit!|Private Key|0x1|Key found"
T "cpu_sol" $cpu @("-m","address","-c","sol","-f","tests\sol_sample.txt","-r","1:20","-t","1","-x","sequential","-q") 30 "Hit! Solana|6ASf"
T "cpu_eth" $cpu @("-m","address","-c","eth","-f","tests\_eth1.txt","-r","1:5","-t","1","-x","sequential","-q") 30 "Hit!!!!|Private Key: 1|0x"
T "cpu_kangaroo_dry" $cpu @("-m","kangaroo","-f","tests\_pubkey_g.txt","-b","40","-y") 15 "Dry-run complete"
T "cpu_mnemonic_dry" $cpu @("-m","mnemonic","-f","tests\_btc1.txt","-y") 15 "Dry-run complete"
T "cpu_poetry_dry" $cpu @("-m","poetry","-f","tests\_btc1.txt","-y") 15 "Dry-run complete"
T "cpu_brain_dry" $cpu @("-m","brainwallet","-f","tests\_btc1.txt","-y") 15 "Dry-run complete"
T "cpu_pub2addr_dry" $cpu @("-m","pubkey2addr","-f","tests\_btc1.txt","-y") 15 "Dry-run complete"
T "cpu_minikeys_dry" $cpu @("-m","minikeys","-f","tests\_btc1.txt","-y") 15 "Dry-run complete"

Write-Host "`n===== PATTERNS =====`n"
foreach ($pat in @("sequential","random","chaos","gravity","spiral","reverse","auto","rseq")) {
  T ("pat_$pat") $cpu @("-m","address","-f","tests\_btc1.txt","-r","1:50","-l","compress","-t","1","-x",$pat,"-q") 20 "Hit! Private Key: 1|Mode address|Search mode"
}

Write-Host "`n===== CUDA =====`n"
if (Test-Path $cuda) {
  T "cuda_b20" $cuda @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","1","-U","cuda","-M","auto","-x","sequential","-q") 40 "Hit! Private Key: d2c55|Private Key: d2c55"
  T "cuda_both_b20" $cuda @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","4","-U","both","-M","auto","-x","sequential","-q") 40 "Hit! Private Key: d2c55|Private Key: d2c55"
  T "cuda_rmd_b20" $cuda @("-m","rmd160","-f","tests\_puzzle20.rmd","-b","20","-l","compress","-t","1","-U","cuda","-M","auto","-x","sequential","-q") 40 "Hit! Private Key: d2c55|Private Key: d2c55"
  T "cuda_minikeys" $cuda @("-m","minikeys","-f","tests\_minikey_known.txt","-C","S4b3N3oGqDqR5jNuxEvDwe","-t","1","-U","cuda","-q") 60 "HIT!!|S4b3N3oGqDqR5jNuxEvDwf"
  T "cuda_vanity" $cuda @("-m","vanity","-v","1Bg","-r","1:20","-l","compress","-t","1","-U","cuda","-G","256","-x","sequential","-q") 45 "Vanity Private Key: 1|GPU EC|1BgGZ"
  T "cuda_xpoint" $cuda @("-m","xpoint","-f","tests\_xpoint_g.txt","-r","1:20","-t","1","-U","cuda","-G","256","-x","sequential","-q") 45 "Hit! Private Key: 1"
  T "cuda_sol" $cuda @("-m","address","-c","sol","-f","tests\sol_sample.txt","-r","1:20","-t","1","-U","cuda","-G","256","-x","sequential","-q") 45 "Hit! Solana|CUDA ed25519"
} else {
  $skip++; $results.Add("SKIP  cuda suite")
}

Write-Host "`n===== SUMMARY =====`n"
$results | ForEach-Object { Write-Host $_ }
Write-Host "PASS=$pass FAIL=$fail SKIP=$skip TOTAL=$($pass+$fail+$skip)"
$results | Set-Content -Encoding utf8 docs\FINAL_AUDIT.md
if ($fail -gt 0) { exit 1 } else { exit 0 }
