# Quick known-hit + dry-run smoke across major modes
$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
if (-not (Test-Path (Join-Path $Root "keyhunt.exe"))) {
  $Root = (Get-Location).Path
}
Set-Location $Root
Get-Process keyhunt*,keyhunt_cuda* -EA SilentlyContinue | Stop-Process -Force -EA SilentlyContinue

$pass = 0; $fail = 0

function Quote-Arg([string]$a) {
  if ($null -eq $a) { return '""' }
  if ($a -match '[\s"]') { return '"' + ($a -replace '"','\"') + '"' }
  return $a
}

function Run-Smoke([string]$Name, [string]$Exe, [string[]]$KhArgs, [int]$Sec = 60, [string]$Pat = ".") {
  if (-not (Test-Path $Exe)) {
    Write-Host "SKIP  $Name"
    return
  }
  Write-Host "===== $Name ====="
  $out = Join-Path $env:TEMP ("tc_smoke_" + [guid]::NewGuid().ToString("N") + ".txt")
  $err = "$out.err"
  Remove-Item KEYFOUNDKEYFOUND.txt,VANITYKEYFOUND.txt,FOUND_BTC.txt,FOUND_ETH.txt,FOUND_SOL.txt -Force -EA SilentlyContinue
  $argLine = ($KhArgs | ForEach-Object { Quote-Arg $_ }) -join ' '
  $p = Start-Process -FilePath (Resolve-Path $Exe).Path -ArgumentList $argLine -WorkingDirectory $Root `
    -NoNewWindow -PassThru -RedirectStandardOutput $out -RedirectStandardError $err
  $deadline = (Get-Date).AddSeconds($Sec)
  $ok = $false
  while ((Get-Date) -lt $deadline) {
    Start-Sleep -Milliseconds 200
    $blob = ""
    foreach ($f in @($out,$err,"KEYFOUNDKEYFOUND.txt","VANITYKEYFOUND.txt","FOUND_ETH.txt","FOUND_SOL.txt")) {
      if (Test-Path $f) { try { $blob += [IO.File]::ReadAllText($f) } catch {} }
    }
    if ($blob -match $Pat) { $ok = $true; break }
    if ($p.HasExited) { break }
  }
  if (-not $p.HasExited) { try { Stop-Process -Id $p.Id -Force -EA SilentlyContinue } catch {} }
  Get-Process keyhunt*,keyhunt_cuda* -EA SilentlyContinue | Stop-Process -Force -EA SilentlyContinue
  $blob = ""
  foreach ($f in @($out,$err,"KEYFOUNDKEYFOUND.txt","VANITYKEYFOUND.txt","FOUND_ETH.txt","FOUND_SOL.txt")) {
    if (Test-Path $f) { try { $blob += [IO.File]::ReadAllText($f) } catch {} }
  }
  Remove-Item $out,$err -Force -EA SilentlyContinue
  if ($ok -or ($blob -match $Pat)) {
    $script:pass++; Write-Host "PASS  $Name" -ForegroundColor Green
  } else {
    $script:fail++; Write-Host "FAIL  $Name" -ForegroundColor Red
    if ($blob.Length -gt 400) { Write-Host $blob.Substring($blob.Length-400) } else { Write-Host $blob }
  }
}

# Fixtures
"1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH" | Set-Content -Encoding ascii tests\_btc1.txt
"1HsMJxNiV7TLxmoF6uJNkydxPFDog4NQum" | Set-Content -Encoding ascii tests\_puzzle20.txt
"b907c3a2a3b27789dfb509b730dd47703c272868" | Set-Content -Encoding ascii tests\_puzzle20.rmd
"79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798" | Set-Content -Encoding ascii tests\_xpoint_g.txt
"0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798" | Set-Content -Encoding ascii tests\_pubkey_g.txt
"1GAehh7TsJAHuUAeKZcXf5CnwuGuGgyX2S" | Set-Content -Encoding ascii tests\_minikey_known.txt
"1LqBGSKuX5yYUonjxT5qGfpUsXKYYWeabA" | Set-Content -Encoding ascii tests\_mnemonic_abandon.txt
if (Test-Path tests\1to32.eth) {
  (Get-Content tests\1to32.eth -TotalCount 1) | Set-Content -Encoding ascii tests\_eth1.txt
}

$cpu = ".\keyhunt.exe"
$cuda = ".\keyhunt_cuda.exe"

Run-Smoke "addr_key1" $cpu @("-m","address","-f","tests\_btc1.txt","-r","1:20","-l","compress","-t","1","-x","sequential","-q") 25 "Private Key: 1"
Run-Smoke "puzzle20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","sequential","-q") 30 "Private Key: d2c55"
Run-Smoke "rmd20" $cpu @("-m","rmd160","-f","tests\_puzzle20.rmd","-b","20","-l","compress","-t","2","-x","sequential","-q") 30 "Private Key: d2c55"
Run-Smoke "xpoint" $cpu @("-m","xpoint","-f","tests\_xpoint_g.txt","-r","1:10","-t","1","-x","sequential","-q") 25 "Private Key: 1"
Run-Smoke "vanity" $cpu @("-m","vanity","-v","1Bg","-r","1:10","-l","compress","-t","1","-x","sequential","-q") 25 "Vanity Private Key: 1|1BgGZ"
Run-Smoke "sol" $cpu @("-m","address","-c","sol","-f","tests\sol_sample.txt","-r","1:20","-t","1","-x","sequential","-q") 30 "Hit! Solana|6ASf|Private Key"
Run-Smoke "eth" $cpu @("-m","address","-c","eth","-f","tests\_eth1.txt","-r","1:5","-t","1","-x","sequential","-q") 25 "Private Key: 1|0x"
Run-Smoke "kangaroo" $cpu @("-m","kangaroo","-f","tests\_pubkey_g.txt","-r","1:1000","-t","1","-q") 40 "Private Key: 1|Hit!|found"
Run-Smoke "bsgs" $cpu @("-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","sequential","-t","1","-q") 90 "Hit!|Private Key|0x1|Key found"
Run-Smoke "bsgs_modfan" $cpu @("-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","modfan","--mod-step","4","-t","2","-q") 90 "Hit!|Private Key|0x1|Key found|ModFan"
Run-Smoke "minikeys" $cpu @("-m","minikeys","-f","tests\_minikey_known.txt","-C","S4b3N3oGqDqR5jNuxEvDwe","-t","1","-q") 50 "S4b3N3oGqDqR5jNuxEvDwf|1GAehh7"
$seedFull = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"
Run-Smoke "mnemonic" $cpu @("-m","mnemonic","-w","12","-L","english","-D","1","-t","1","-q","-f","tests\_mnemonic_abandon.txt","--seed",$seedFull) 40 "MNEMONIC FOUND|1LqBGSKu"
Run-Smoke "pub2addr" $cpu @("-m","pubkey2addr","-f","tests\_btc1.txt","-r","1:20","-x","sequential","-t","1","-q") 25 "Private Key: 1|PUBKEY2ADDR FOUND"
Run-Smoke "poetry_dry" $cpu @("-m","poetry","-f","tests\_btc1.txt","-y") 15 "Dry-run complete"
Run-Smoke "brain_dry" $cpu @("-m","brainwallet","-f","tests\_btc1.txt","-y") 15 "Dry-run complete"
Run-Smoke "keyhole" $cpu @("-m","address","-f","tests\_btc1.txt","-r","1:200","-l","compress","-t","1","-x","keyhole","-q") 30 "Private Key: 1|Keyhole"
Run-Smoke "pocket_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","pocket","--pocket-bits","8","-q") 40 "Private Key: d2c55"
Run-Smoke "wave_b20" $cpu @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-t","2","-x","wave","-q") 40 "Private Key: d2c55"
Run-Smoke "research_shadow160" $cpu @("-m","shadow160","-f","tests\_puzzle20.rmd","-b","20","-y") 15 "Dry-run complete|Shadow160"
Run-Smoke "dry" $cpu @("-m","address","-f","tests\66.txt","-b","66","-y") 15 "Dry-run complete"

if (Test-Path $cuda) {
  Run-Smoke "cuda20" $cuda @("-m","address","-f","tests\_puzzle20.txt","-b","20","-l","compress","-U","cuda","-M","auto","-t","1","-x","sequential","-q") 45 "Private Key: d2c55"
}

Write-Host ""
Write-Host "PASS=$pass FAIL=$fail"
if ($fail -gt 0) { exit 1 } else { exit 0 }
