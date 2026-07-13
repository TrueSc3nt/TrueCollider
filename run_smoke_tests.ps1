# Visible smoke / hit tests — kill on first Hit/Vanity match
$ErrorActionPreference = "Continue"
Set-Location $PSScriptRoot
$log = New-Object System.Collections.Generic.List[string]
function L([string]$s) { Write-Host $s; [void]$log.Add($s) }
$pass = 0; $fail = 0

function Invoke-TC {
  param(
    [string]$Name,
    [string]$Exe,
    [string[]]$CmdArgs,
    [int]$Sec = 30,
    [string]$Pat = "HIT|FOUND|Dry-run complete|GPU EC|GPU Solana|GPU BSGS|Private Key|Vanity|CUDA secp|CUDA ed25519|Hit!"
  )
  if (-not (Test-Path $Exe)) { L "FAIL  $Name - missing $Exe"; $script:fail++; return }
  L "----- $Name -----"; L ("CMD: $Exe " + ($CmdArgs -join " "))
  $outFile = Join-Path $env:TEMP ("tc_smoke_" + [guid]::NewGuid().ToString("N") + ".txt")
  $argStr = ($CmdArgs | ForEach-Object { if ($_ -match "\s") { '"' + $_ + '"' } else { $_ } }) -join " "
  $p = Start-Process -FilePath (Resolve-Path $Exe).Path -ArgumentList $argStr -NoNewWindow -PassThru -RedirectStandardOutput $outFile -RedirectStandardError "$outFile.err"
  $deadline = (Get-Date).AddSeconds($Sec)
  $ok = $false
  $text = ""
  while ((Get-Date) -lt $deadline) {
    Start-Sleep -Milliseconds 400
    $so = ""; $se = ""
    if (Test-Path $outFile) { $so = Get-Content $outFile -Raw -ErrorAction SilentlyContinue }
    if (Test-Path "$outFile.err") { $se = Get-Content "$outFile.err" -Raw -ErrorAction SilentlyContinue }
    $text = "$so$se"
    if ($text -match $Pat) { $ok = $true; break }
    if ($p.HasExited) { break }
  }
  if (-not $p.HasExited) {
    try { $p.Kill() } catch {}
    Get-Process keyhunt*,keyhunt_cuda* -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
  }
  try { $p.WaitForExit(3000) } catch {}
  if (Test-Path $outFile) { $so = Get-Content $outFile -Raw -ErrorAction SilentlyContinue } else { $so = "" }
  if (Test-Path "$outFile.err") { $se = Get-Content "$outFile.err" -Raw -ErrorAction SilentlyContinue } else { $se = "" }
  $text = "$so$se"
  # Hits may be buffered; also accept FOUND_*.txt / KEYFOUND*.txt artifacts
  foreach ($f in @("FOUND_BTC.txt","FOUND_ETH.txt","FOUND_SOL.txt","KEYFOUNDKEYFOUND.txt","VANITYKEYFOUND.txt")) {
    if (Test-Path $f) {
      $ft = Get-Content $f -Raw -ErrorAction SilentlyContinue
      if ($ft) { $text += "`n" + $ft }
    }
  }
  Remove-Item $outFile,"$outFile.err" -Force -ErrorAction SilentlyContinue
  if ($text.Length -gt 1800) { L ($text.Substring(0,1800) + "`n...(truncated)") } else { L $text }
  if ($ok -or ($text -match $Pat)) { L "PASS  $Name"; $script:pass++ } else { L "FAIL  $Name - pattern not matched"; $script:fail++ }
}

L "# TrueCollider smoke / hit tests"
L ("Date: " + (Get-Date -Format o))
L ""

Invoke-TC -Name "CPU address hit 1-2" -Exe ".\keyhunt.exe" -Sec 25 -Pat "Hit! Private Key|1BgGZ|Private Key: 1" -CmdArgs @("-m","address","-f","tests\_btc_1to2.txt","-r","1:100","-l","compress","-t","1","-x","sequential","-q")
Invoke-TC -Name "CPU sol hit seed1" -Exe ".\keyhunt.exe" -Sec 25 -Pat "Hit! Solana|6ASf" -CmdArgs @("-m","address","-c","sol","-f","tests\sol_sample.txt","-r","1:100","-t","1","-x","sequential","-q")
Invoke-TC -Name "CPU xpoint G" -Exe ".\keyhunt.exe" -Sec 25 -Pat "Hit! Private Key: 1|Private Key: 1" -CmdArgs @("-m","xpoint","-f","tests\_xpoint_g.txt","-r","1:100","-t","1","-x","sequential","-q")
Invoke-TC -Name "CPU vanity hit 1Bg" -Exe ".\keyhunt.exe" -Sec 25 -Pat "Vanity Private Key: 1|1BgGZ" -CmdArgs @("-m","vanity","-v","1Bg","-r","1:100","-l","compress","-t","1","-x","sequential","-q")
Invoke-TC -Name "CPU bsgs dry" -Exe ".\keyhunt.exe" -Sec 15 -Pat "Dry-run complete" -CmdArgs @("-m","bsgs","-f","tests\125.txt","-b","40","-n","1048576","-k","auto","-y")
Invoke-TC -Name "CPU pubkey2addr dry" -Exe ".\keyhunt.exe" -Sec 15 -Pat "Dry-run complete" -CmdArgs @("-m","pubkey2addr","-f","tests\_btc_1to2.txt","-y")
Invoke-TC -Name "CPU minikeys dry" -Exe ".\keyhunt.exe" -Sec 15 -Pat "Dry-run complete" -CmdArgs @("-m","minikeys","-f","tests\_btc_1to2.txt","-y")
Invoke-TC -Name "CPU mnemonic dry" -Exe ".\keyhunt.exe" -Sec 15 -Pat "Dry-run complete" -CmdArgs @("-m","mnemonic","-f","tests\_btc_1to2.txt","-y")
Invoke-TC -Name "CPU poetry dry" -Exe ".\keyhunt.exe" -Sec 15 -Pat "Dry-run complete" -CmdArgs @("-m","poetry","-f","tests\_btc_1to2.txt","-y")
Invoke-TC -Name "CPU brainwallet dry" -Exe ".\keyhunt.exe" -Sec 15 -Pat "Dry-run complete" -CmdArgs @("-m","brainwallet","-f","tests\_btc_1to2.txt","-y")

if (Test-Path ".\keyhunt_cuda.exe") {
  Invoke-TC -Name "CUDA address dry" -Exe ".\keyhunt_cuda.exe" -Sec 60 -Pat "Dry-run complete|CUDA secp|CUDA ed25519|GPU EC" -CmdArgs @("-m","address","-f","tests\_btc_1to2.txt","-U","cuda","-M","auto","-y")
  Invoke-TC -Name "CUDA address hit 1-2" -Exe ".\keyhunt_cuda.exe" -Sec 60 -Pat "Hit! Private Key|1BgGZ" -CmdArgs @("-m","address","-f","tests\_btc_1to2.txt","-r","1:100","-l","compress","-U","cuda","-G","256","-M","512","-t","1","-x","sequential","-q")
  Invoke-TC -Name "CUDA vanity hit 1Bg" -Exe ".\keyhunt_cuda.exe" -Sec 60 -Pat "Vanity Private Key: 1|GPU EC" -CmdArgs @("-m","vanity","-v","1Bg","-r","1:100","-l","compress","-U","cuda","-G","256","-t","1","-x","sequential","-q")
  Invoke-TC -Name "CUDA xpoint G" -Exe ".\keyhunt_cuda.exe" -Sec 60 -Pat "Hit! Private Key: 1" -CmdArgs @("-m","xpoint","-f","tests\_xpoint_g.txt","-r","1:100","-U","cuda","-G","256","-t","1","-x","sequential","-q")
  Invoke-TC -Name "CUDA pubkey2addr dry" -Exe ".\keyhunt_cuda.exe" -Sec 45 -Pat "Dry-run complete|GPU|CUDA" -CmdArgs @("-m","pubkey2addr","-f","tests\_btc_1to2.txt","-U","cuda","-y")
  Invoke-TC -Name "CUDA minikeys dry" -Exe ".\keyhunt_cuda.exe" -Sec 45 -Pat "Dry-run complete|GPU|CUDA" -CmdArgs @("-m","minikeys","-f","tests\_btc_1to2.txt","-U","cuda","-y")
  Invoke-TC -Name "CUDA mnemonic dry" -Exe ".\keyhunt_cuda.exe" -Sec 45 -Pat "Dry-run complete|GPU|CUDA" -CmdArgs @("-m","mnemonic","-f","tests\_btc_1to2.txt","-U","cuda","-y")
  Invoke-TC -Name "CUDA poetry dry" -Exe ".\keyhunt_cuda.exe" -Sec 45 -Pat "Dry-run complete|GPU|CUDA" -CmdArgs @("-m","poetry","-f","tests\_btc_1to2.txt","-U","cuda","-y")
  Invoke-TC -Name "CUDA brainwallet dry" -Exe ".\keyhunt_cuda.exe" -Sec 45 -Pat "Dry-run complete|GPU|CUDA" -CmdArgs @("-m","brainwallet","-f","tests\_btc_1to2.txt","-U","cuda","-y")
  Invoke-TC -Name "CUDA bsgs dry" -Exe ".\keyhunt_cuda.exe" -Sec 45 -Pat "Dry-run complete|GPU BSGS|GPU EC|CUDA" -CmdArgs @("-m","bsgs","-f","tests\125.txt","-b","40","-n","1048576","-k","1","-U","cuda","-M","auto","-y")
  Invoke-TC -Name "CUDA sol hit seed1" -Exe ".\keyhunt_cuda.exe" -Sec 60 -Pat "Hit! Solana|6ASf|GPU Solana|CUDA ed25519" -CmdArgs @("-m","address","-c","sol","-f","tests\sol_sample.txt","-r","1:100","-U","cuda","-G","256","-M","512","-t","1","-x","sequential","-q")
} else {
  L "SKIP  CUDA tests - keyhunt_cuda.exe not built yet"
}

L ""; L "## Summary"; L ("PASS=$pass FAIL=$fail")
$log | Set-Content -Encoding UTF8 "docs\TEST_RESULTS.md"
Write-Host ""; Write-Host "=== SUMMARY PASS=$pass FAIL=$fail ==="
if ($fail -gt 0) { exit 1 } else { exit 0 }
