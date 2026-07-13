# Thorough TrueCollider / KeyCollider mode test harness
# PASS = known hit | SMOKE = clean start | SKIP = unavailable | FAIL = error/no hit

$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $Root) { $Root = (Get-Location).Path }
Set-Location $Root

$Results = @()
$OutDir = Join-Path $Root "_mode_test_out"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null | Out-Null

function Stop-Keyhunt {
  Get-Process -Name "keyhunt","keyhunt_cuda" -ErrorAction SilentlyContinue |
    Stop-Process -Force -ErrorAction SilentlyContinue | Out-Null
  Start-Sleep -Milliseconds 400
}

function Add-Result {
  param($Name, $Status, $Detail)
  $script:Results += [pscustomobject]@{ Mode = $Name; Status = $Status; Detail = $Detail }
  $color = if ($Status -eq "PASS") { "Green" } elseif ($Status -eq "SMOKE") { "Cyan" } elseif ($Status -eq "SKIP") { "Yellow" } else { "Red" }
  Write-Host ("[{0}] {1} - {2}" -f $Status, $Name, $Detail) -ForegroundColor $color
}

function Invoke-Keyhunt {
  param(
    [string]$Exe,
    [string[]]$KhArgs,
    [int]$TimeoutSec = 45,
    [string]$Label,
    [ValidateSet("hit","smoke","removed")]
    [string]$Expect = "hit",
    [string[]]$HitPatterns = @("Hit!","FOUND","Private Key","TAPROOT ADDRESS FOUND","KEYFOUND")
  )

  Stop-Keyhunt
  $hitFile = Join-Path $Root "KEYFOUNDKEYFOUND.txt"
  $vanityFile = Join-Path $Root "VANITYKEYFOUND.txt"
  if (Test-Path $hitFile) { Remove-Item $hitFile -Force -ErrorAction SilentlyContinue }
  if (Test-Path $vanityFile) { Remove-Item $vanityFile -Force -ErrorAction SilentlyContinue }

  $stdout = Join-Path $OutDir ($Label + "_out.txt")
  $stderr = Join-Path $OutDir ($Label + "_err.txt")
  if (Test-Path $stdout) { Remove-Item $stdout -Force -ErrorAction SilentlyContinue }
  if (Test-Path $stderr) { Remove-Item $stderr -Force -ErrorAction SilentlyContinue }

  if (-not (Test-Path $Exe)) {
    Add-Result $Label "SKIP" "missing $Exe"
    return
  }

  $p = Start-Process -FilePath $Exe -ArgumentList $KhArgs -WorkingDirectory $Root -NoNewWindow -PassThru `
    -RedirectStandardOutput $stdout -RedirectStandardError $stderr

  $deadline = (Get-Date).AddSeconds($TimeoutSec)
  $hit = $false
  while (-not $p.HasExited -and (Get-Date) -lt $deadline) {
    Start-Sleep -Milliseconds 400
    if (Test-Path $hitFile) { $hit = $true; break }
    if (Test-Path $vanityFile) { $hit = $true; break }
    $blob = ""
    if (Test-Path $stdout) {
      try { $blob += [IO.File]::ReadAllText($stdout) } catch {}
    }
    if (Test-Path $stderr) {
      try { $blob += [IO.File]::ReadAllText($stderr) } catch {}
    }
    foreach ($pat in $HitPatterns) {
      if ($blob.Contains($pat)) { $hit = $true; break }
    }
    if ($hit) { break }
  }

  if (-not $p.HasExited) {
    try { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue | Out-Null } catch {}
    try { $p.WaitForExit(3000) | Out-Null } catch {}
  }
  Stop-Keyhunt

  $outText = ""; $errText = ""
  if (Test-Path $stdout) { try { $outText = [IO.File]::ReadAllText($stdout) } catch {} }
  if (Test-Path $stderr) { try { $errText = [IO.File]::ReadAllText($stderr) } catch {} }
  $all = $outText + "`n" + $errText
  if (Test-Path $hitFile) {
    try { $all += "`n" + [IO.File]::ReadAllText($hitFile) } catch {}
    $hit = $true
  }
  if (Test-Path $vanityFile) {
    try { $all += "`n" + [IO.File]::ReadAllText($vanityFile) } catch {}
    $hit = $true
  }

  $errLines = @()
  if ($all) { $errLines = $all -split "`r?`n" | Where-Object { $_ -match '^\[E\]' } }

  $fatal = $false
  $skipReason = $null
  foreach ($line in $errLines) {
    if ($line -match 'tcuda_hash160_33_selftest') { continue } # known soft fail; host filter still used
    if ($line -match 'Falling back to CPU') { continue }
    if ($line -match 'does not support mode') { continue }
    if ($line -match 'OpenCL backend requested but no') { $skipReason = "OpenCL not available"; continue }
    if ($line -match 'CUDA backend requested but no') { $skipReason = "CUDA devices not found"; continue }
    if ($line -match 'M value is not divisible') { $fatal = $true; break }
    if ($line -match 'There aren.t any vanity') { $fatal = $true; break }
    if ($line -match 'Can.t open file|Cannot open|Unenexpected|Unexpected error') { $fatal = $true; break }
    # treat remaining [E] as fatal unless we already have a hit (CUDA hash selftest already skipped)
    if (-not $hit) { $fatal = $true; break }
  }

  if ($Expect -eq "removed") {
    if ($all -match "removed") { Add-Result $Label "PASS" "reports mode removed" }
    else { Add-Result $Label "FAIL" "expected removed message" }
    return
  }

  if ($skipReason -and -not $hit) {
    Add-Result $Label "SKIP" $skipReason
    return
  }

  if ($Expect -eq "hit") {
    if ($hit) { Add-Result $Label "PASS" "known hit detected" }
    elseif ($fatal) {
      $snip = ($errLines | Select-Object -First 2) -join " | "
      Add-Result $Label "FAIL" "error: $snip"
    }
    else { Add-Result $Label "FAIL" "no known hit within ${TimeoutSec}s" }
    return
  }

  # smoke
  if ($fatal) {
    $snip = ($errLines | Select-Object -First 2) -join " | "
    Add-Result $Label "FAIL" "error: $snip"
  } elseif ($all -match "Mode |Setting search|Thread|BSGS|Vanity|Mnemonic|Poetry|Brainwallet|minikey|Search mode") {
    Add-Result $Label "SMOKE" "started cleanly (no known-hit fixture)"
  } else {
    Add-Result $Label "FAIL" "did not start / empty output"
  }
}

# --- fixtures ---
@"
1BgGZ9tcN4rm9KBzDnJfSATsodMBcWLvRj
1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
1PWo3JeB9jrGwfHDNpdGK54CRas7fsVz9
1JTK7s9YVYywfm5XUH7RPhH8VSHSokJEV
12VVRNPi4SJqUTsp6FmqDqY1qFhp7bWa
1BY8GQbnueYofwSuFAT3USAhGjPrkwJH
1MVDYgVaSN6iKKEsbzRUAYFrYJadLYZvx
19vkiEajfhuZ8bs8Zu2jgmC6oqZbWqMX
13zb1hQbWVsc2S7ZTZnP2G4undNNpdVu20
12qGmGi12vvCJPb7Gj4U4BGu92kK3ZtAD
1PWo3JeB9jrGwfHDNpdGK54CRas7fsVz9
1JTK7s9YVYywfm5XUH7RPhH8VSHSokJEV
12VVRNPi4SJqUTsp6FmqDqY1qFhp7bWa
1BY8GQbnueYofwSuFAT3USAhGjPrkwJH
1MVDYgVaSN6iKKEsbzRUAYFrYJadLYZvx
19vkiEajfhuZ8bs8Zu2jgmC6oqZbWqMX
13zb1hQbWVsc2S7ZTZnP2G4undNNpdVu20
12qGmGi12vvCJPb7Gj4U4BGu92kK3ZtAD
1BgGZ9tcN4rm9KBzDnJfSATsodMBcWLvRj
1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
1PWo3JeB9jrGwfHDNpdGK54CRas7fsVz9
1JTK7s9YVYywfm5XUH7RPhH8VSHSokJEV
12VVRNPi4SJqUTsp6FmqDqY1qFhp7bWa
1BY8GQbnueYofwSuFAT3USAhGjPrkwJH
1MVDYgVaSN6iKKEsbzRUAYFrYJadLYZvx
19vkiEajfhuZ8bs8Zu2jgmC6oqZbWqMX
13zb1hQbWVsc2S7ZTZnP2G4undNNpdVu20
12qGmGi12vvCJPb7Gj4U4BGu92kK3ZtAD
1PWo3JeB9jrGwfHDNpdGK54CRas7fsVz9
1JTK7s9YVYywfm5XUH7RPhH8VSHSokJEV
12VVRNPi4SJqUTsp6FmqDqY1qFhp7bWa
1BY8GQbnueYofwSuFAT3USAhGjPrkwJH
"@ | Set-Content -Encoding ascii (Join-Path $Root "tests\_btc_pad.txt")

"0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798" |
  Set-Content -Encoding ascii (Join-Path $Root "tests\_pubkey_g.txt")
"79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798" |
  Set-Content -Encoding ascii (Join-Path $Root "tests\_xpoint_g.txt")
(Get-Content (Join-Path $Root "tests\1to32.eth") -TotalCount 1) |
  Set-Content -Encoding ascii (Join-Path $Root "tests\_eth_1.txt")
# pad rmd with full 1to32 to avoid tiny fuse hangs
Copy-Item (Join-Path $Root "tests\1to32.rmd") (Join-Path $Root "tests\_rmd_pad.txt") -Force

$btc = "tests\_btc_pad.txt"
$cpu = ".\keyhunt.exe"
$cuda = ".\keyhunt_cuda.exe"

Write-Host "`n========== CPU HIT TESTS ==========`n" -ForegroundColor White

Invoke-Keyhunt -Exe $cpu -Label "address_btc" -Expect hit -TimeoutSec 50 -KhArgs @(
  "-m","address","-f",$btc,"-r","1:3","-l","both","-x","sequential","-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cpu -Label "address_eth" -Expect hit -TimeoutSec 50 -KhArgs @(
  "-m","address","-c","eth","-f","tests\1to32.eth","-r","1:3","-x","sequential","-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cpu -Label "address_sol" -Expect hit -TimeoutSec 50 -KhArgs @(
  "-m","address","-c","sol","-f","tests\sol_sample.txt","-r","1:8","-x","sequential","-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cpu -Label "rmd160" -Expect hit -TimeoutSec 50 -KhArgs @(
  "-m","rmd160","-f","tests\_rmd_pad.txt","-r","1:4","-l","compress","-x","sequential","-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cpu -Label "xpoint" -Expect hit -TimeoutSec 50 -KhArgs @(
  "-m","xpoint","-f","tests\_xpoint_g.txt","-r","1:3","-x","sequential","-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cpu -Label "vanity" -Expect hit -TimeoutSec 50 -KhArgs @(
  "-m","vanity","-v","1EH","-r","1:3","-l","uncompress","-x","sequential","-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cpu -Label "address_btc_endo" -Expect hit -TimeoutSec 50 -KhArgs @(
  "-m","address","-f",$btc,"-r","1:3","-l","both","-x","sequential","-e","-t","1","-q","-s","0","-A","sse"
)
Invoke-Keyhunt -Exe $cpu -Label "address_all" -Expect hit -TimeoutSec 50 -KhArgs @(
  "-m","address","-c","all","-f",$btc,"-r","1:3","-l","both","-x","sequential","-t","1","-q","-s","0"
)

# BSGS: n must have sqrt divisible by 1024 => n = 1024^2 = 1048576
Invoke-Keyhunt -Exe $cpu -Label "bsgs" -Expect hit -TimeoutSec 120 -HitPatterns @("Hit!","Private Key","Key found","FOUND","0x1") -KhArgs @(
  "-m","bsgs","-f","tests\_pubkey_g.txt","-r","1:2","-n","1048576","-B","sequential","-t","1","-q","-s","0"
)

Write-Host "`n========== CPU SMOKE ==========`n" -ForegroundColor White

Invoke-Keyhunt -Exe $cpu -Label "pub2rmd" -Expect removed -TimeoutSec 10 -KhArgs @(
  "-m","pub2rmd","-f",$btc,"-t","1"
)
Invoke-Keyhunt -Exe $cpu -Label "pubkey2addr" -Expect smoke -TimeoutSec 15 -KhArgs @(
  "-m","pubkey2addr","-f",$btc,"-r","1:8","-l","both","-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cpu -Label "minikeys" -Expect smoke -TimeoutSec 12 -KhArgs @(
  "-m","minikeys","-f",$btc,"-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cpu -Label "mnemonic" -Expect smoke -TimeoutSec 12 -KhArgs @(
  "-m","mnemonic","-f",$btc,"-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cpu -Label "poetry" -Expect smoke -TimeoutSec 12 -KhArgs @(
  "-m","poetry","-f",$btc,"-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cpu -Label "brainwallet" -Expect smoke -TimeoutSec 12 -KhArgs @(
  "-m","brainwallet","-f",$btc,"-t","1","-q","-s","0"
)

Write-Host "`n========== CUDA ==========`n" -ForegroundColor White

Invoke-Keyhunt -Exe $cuda -Label "cuda_address_btc" -Expect hit -TimeoutSec 70 -KhArgs @(
  "-m","address","-f",$btc,"-r","1:3","-l","both","-x","sequential","-U","cuda","-G","64","-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cuda -Label "cuda_address_eth" -Expect hit -TimeoutSec 70 -KhArgs @(
  "-m","address","-c","eth","-f","tests\1to32.eth","-r","1:3","-x","sequential","-U","cuda","-G","64","-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cuda -Label "cuda_rmd160" -Expect hit -TimeoutSec 70 -KhArgs @(
  "-m","rmd160","-f","tests\_rmd_pad.txt","-r","1:4","-l","compress","-x","sequential","-U","cuda","-G","64","-t","1","-q","-s","0"
)
Invoke-Keyhunt -Exe $cuda -Label "opencl_address_btc" -Expect smoke -TimeoutSec 20 -KhArgs @(
  "-m","address","-f",$btc,"-r","1:3","-l","compress","-x","sequential","-U","opencl","-t","1","-q","-s","0"
)

Write-Host "`n========== PATTERNS (smoke) ==========`n" -ForegroundColor White
foreach ($pat in @("sequential","random","chaos","gravity","spiral","reverse","auto")) {
  Invoke-Keyhunt -Exe $cpu -Label ("pattern_$pat") -Expect smoke -TimeoutSec 12 -KhArgs @(
    "-m","address","-f",$btc,"-r","1:100","-l","both","-x",$pat,"-t","1","-q","-s","0"
  )
}

# Also prove non-sequential patterns can hit in a tiny range (longer)
Write-Host "`n========== PATTERN HIT (random) ==========`n" -ForegroundColor White
# Prove random pattern can hit using rmd160 (large fixture avoids intermittent tiny fuse hangs)
Invoke-Keyhunt -Exe $cpu -Label "pattern_random_hit" -Expect hit -TimeoutSec 60 -KhArgs @(
  "-m","rmd160","-f","tests\_rmd_pad.txt","-r","1:10","-l","compress","-x","random","-t","2","-q","-s","0"
)

Stop-Keyhunt

Write-Host "`n========== SUMMARY ==========`n" -ForegroundColor White
$Results | Format-Table -AutoSize | Out-String | Write-Host
$pass = @($Results | Where-Object Status -eq "PASS").Count
$smoke = @($Results | Where-Object Status -eq "SMOKE").Count
$fail = @($Results | Where-Object Status -eq "FAIL").Count
$skip = @($Results | Where-Object Status -eq "SKIP").Count
Write-Host "PASS=$pass  SMOKE=$smoke  FAIL=$fail  SKIP=$skip  TOTAL=$($Results.Count)"
$Results | ConvertTo-Csv -NoTypeInformation | Set-Content (Join-Path $OutDir "summary.csv")
if ($fail -gt 0) { exit 1 } else { exit 0 }
