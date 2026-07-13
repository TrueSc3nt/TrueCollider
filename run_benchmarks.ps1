# Benchmark TrueCollider modes; write JSON + CSV of measured rates.
$ErrorActionPreference = "Continue"
$Root = "C:\Users\loulo\Desktop\updayingkeyunt\TrueCollider-master"
Set-Location $Root
$OutDir = Join-Path $Root "_bench_out"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
Get-Process keyhunt* -EA SilentlyContinue | Stop-Process -Force -EA SilentlyContinue

$btc = "tests\_btc_pad.txt"
if (-not (Test-Path $btc)) {
@"
1BgGZ9tcN4rm9KBzDnJfSATsodMBcWLvRj
1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
1PWo3JeB9jrGwfHDNpdGK54CRas7fsVz9
1JTK7s9YVYywfm5XUH7RPhH8VSHSokJEV
1MVDYgVaSN6iKKEsbzRUAYFrYJadLYZvx
1PWo3JeB9jrGwfHDNpdGK54CRas7fsVz9
1JTK7s9YVYywfm5XUH7RPhH8VSHSokJEV
1MVDYgVaSN6iKKEsbzRUAYFrYJadLYZvx
1BgGZ9tcN4rm9KBzDnJfSATsodMBcWLvRj
1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
1PWo3JeB9jrGwfHDNpdGK54CRas7fsVz9
1JTK7s9YVYywfm5XUH7RPhH8VSHSokJEV
1MVDYgVaSN6iKKEsbzRUAYFrYJadLYZvx
1BgGZ9tcN4rm9KBzDnJfSATsodMBcWLvRj
1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm
1PWo3JeB9jrGwfHDNpdGK54CRas7fsVz9
"@ | Set-Content -Encoding ascii $btc
}
if (-not (Test-Path "tests\_pubkey_g.txt")) {
  "0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798" | Set-Content -Encoding ascii "tests\_pubkey_g.txt"
}
if (-not (Test-Path "tests\_xpoint_g.txt")) {
  "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798" | Set-Content -Encoding ascii "tests\_xpoint_g.txt"
}

function Format-Rate([double]$r) {
  if ($r -ge 1e12) { return ("{0:N2} Tkeys/s" -f ($r/1e12)) }
  if ($r -ge 1e9)  { return ("{0:N2} Gkeys/s" -f ($r/1e9)) }
  if ($r -ge 1e6)  { return ("{0:N2} Mkeys/s" -f ($r/1e6)) }
  if ($r -ge 1e3)  { return ("{0:N2} Kkeys/s" -f ($r/1e3)) }
  return ("{0:N0} keys/s" -f $r)
}

function Parse-Speed([string]$text) {
  # Match: ... : 123456 keys/s   or  : 1.23 Mkeys/s  or mnemonics/s
  $m = [regex]::Matches($text, ':\s*~?([\d,\.]+)\s*([MGTP]?)(keys|mnemonics)/s')
  if ($m.Count -eq 0) {
    $m = [regex]::Matches($text, '([\d,\.]+)\s*([MGTP]?)(keys|mnemonics)/s')
  }
  $best = 0.0
  $unit = "keys/s"
  foreach ($x in $m) {
    $num = [double]($x.Groups[1].Value -replace ',','')
    $pref = $x.Groups[2].Value
    $u = $x.Groups[3].Value
    $mult = 1.0
    switch ($pref) {
      "M" { $mult = 1e6 }
      "G" { $mult = 1e9 }
      "T" { $mult = 1e12 }
      "P" { $mult = 1e15 }
    }
    $v = $num * $mult
    if ($v -gt $best) { $best = $v; $unit = "$u/s" }
  }
  return @{ Rate = $best; Unit = $unit; Display = if ($best -gt 0) { Format-Rate $best } else { "n/a" } }
}

function Run-Bench {
  param(
    [string]$Name,
    [string]$Exe,
    [string[]]$KhArgs,
    [int]$Seconds = 18,
    [string]$Backend = "CPU",
    [string]$Notes = ""
  )
  Get-Process keyhunt* -EA SilentlyContinue | Stop-Process -Force -EA SilentlyContinue
  Start-Sleep -Milliseconds 300
  $stdout = Join-Path $OutDir ($Name + "_out.txt")
  $stderr = Join-Path $OutDir ($Name + "_err.txt")
  Remove-Item $stdout,$stderr -EA SilentlyContinue

  if (-not (Test-Path $Exe)) {
    Write-Host "[SKIP] $Name - missing $Exe"
    return [pscustomobject]@{ Mode=$Name; Backend=$Backend; Rate=0; Display="n/a"; Notes="binary missing"; Raw="" }
  }

  Write-Host "[RUN] $Name ($Backend) ..." -ForegroundColor Cyan
  $p = Start-Process -FilePath $Exe -ArgumentList $KhArgs -WorkingDirectory $Root -NoNewWindow -PassThru `
    -RedirectStandardOutput $stdout -RedirectStandardError $stderr
  Start-Sleep -Seconds $Seconds
  if (-not $p.HasExited) {
    try { Stop-Process -Id $p.Id -Force -EA SilentlyContinue } catch {}
    try { $p.WaitForExit(4000) | Out-Null } catch {}
  }
  Get-Process keyhunt* -EA SilentlyContinue | Stop-Process -Force -EA SilentlyContinue

  $all = ""
  if (Test-Path $stdout) { $all += [IO.File]::ReadAllText($stdout) }
  if (Test-Path $stderr) { $all += "`n" + [IO.File]::ReadAllText($stderr) }
  $parsed = Parse-Speed $all
  # Prefer last Total line
  $totals = [regex]::Matches($all, '\[\*?\+\] Total[^\r\n]+')
  $rawLine = ""
  if ($totals.Count -gt 0) { $rawLine = $totals[$totals.Count-1].Value.Trim() }
  elseif ($parsed.Rate -gt 0) { $rawLine = $parsed.Display }

  Write-Host ("  -> {0}  |  {1}" -f $parsed.Display, $rawLine) -ForegroundColor Green
  return [pscustomobject]@{
    Mode = $Name
    Backend = $Backend
    Rate = [math]::Round($parsed.Rate, 0)
    Display = $parsed.Display
    Notes = $Notes
    Raw = $rawLine
  }
}

$results = @()
$cpu = ".\keyhunt.exe"
$cuda = ".\keyhunt_cuda.exe"
# Wide bit range so we don't finish early; stats every 5s
$range = "100000000000000000:1ffffffffffffffff"

# --- CPU modes ---
$results += Run-Bench "address_btc" $cpu @(
  "-m","address","-f",$btc,"-r",$range,"-l","compress","-x","random","-t","8","-q","-s","5","-A","sse"
) -Notes "8 threads, SSE hash160, compress"
$results += Run-Bench "address_btc_endo" $cpu @(
  "-m","address","-f",$btc,"-r",$range,"-l","compress","-x","random","-e","-t","8","-q","-s","5","-A","sse"
) -Notes "8 threads + endomorphism -e"
$results += Run-Bench "address_eth" $cpu @(
  "-m","address","-c","eth","-f","tests\1to32.eth","-r",$range,"-x","random","-t","8","-q","-s","5"
) -Notes "8 threads, keccak"
$results += Run-Bench "address_sol" $cpu @(
  "-m","address","-c","sol","-f","tests\sol_sample.txt","-r",$range,"-x","random","-t","8","-q","-s","5"
) -Notes "8 threads, ed25519"
$results += Run-Bench "rmd160" $cpu @(
  "-m","rmd160","-f","tests\1to32.rmd","-r",$range,"-l","compress","-x","random","-t","8","-q","-s","5","-A","sse"
) -Notes "8 threads, SSE"
$results += Run-Bench "rmd160_endo" $cpu @(
  "-m","rmd160","-f","tests\1to32.rmd","-r",$range,"-l","compress","-x","random","-e","-t","8","-q","-s","5","-A","sse"
) -Notes "8 threads + -e"
$results += Run-Bench "xpoint" $cpu @(
  "-m","xpoint","-f","tests\_xpoint_g.txt","-r",$range,"-x","random","-t","8","-q","-s","5"
) -Notes "8 threads"
$results += Run-Bench "vanity" $cpu @(
  "-m","vanity","-v","1Love","-r",$range,"-l","compress","-x","random","-t","8","-q","-s","5","-e"
) -Notes "8 threads + -e, prefix 1Love"
$results += Run-Bench "pubkey2addr" $cpu @(
  "-m","pubkey2addr","-f",$btc,"-r",$range,"-l","compress","-t","8","-q","-s","5"
) -Notes "8 threads"
$results += Run-Bench "minikeys" $cpu @(
  "-m","minikeys","-f",$btc,"-t","8","-q","-s","5"
) -Notes "8 threads"
$results += Run-Bench "mnemonic" $cpu @(
  "-m","mnemonic","-f",$btc,"-t","8","-q","-s","5"
) -Notes "8 threads BIP39 (CPU only)"
$results += Run-Bench "poetry" $cpu @(
  "-m","poetry","-f",$btc,"-t","4","-q","-s","5"
) -Notes "4 threads"
$results += Run-Bench "brainwallet" $cpu @(
  "-m","brainwallet","-f",$btc,"-t","4","-q","-s","5"
) -Notes "4 threads"
# BSGS: after table build, measure giant-step rate. Use saved blooms if possible; small-ish table.
$results += Run-Bench "bsgs" $cpu @(
  "-m","bsgs","-f","tests\_pubkey_g.txt","-b","40","-n","1048576","-B","random","-t","8","-q","-s","5"
) -Seconds 45 -Notes "8 threads, -b 40, -n 1M (includes table build time in wall clock; rate from stats)"

# --- CUDA ---
$results += Run-Bench "cuda_address_btc" $cuda @(
  "-m","address","-f",$btc,"-r",$range,"-l","compress","-x","random","-U","cuda","-G","128","-t","1","-q","-s","5"
) -Backend "CUDA" -Notes "RTX 3060 Ti, -G 128, t=1, GPU EC + host hash"
$results += Run-Bench "cuda_address_eth" $cuda @(
  "-m","address","-c","eth","-f","tests\1to32.eth","-r",$range,"-x","random","-U","cuda","-G","128","-t","1","-q","-s","5"
) -Backend "CUDA" -Notes "GPU EC + host keccak"
$results += Run-Bench "cuda_rmd160" $cuda @(
  "-m","rmd160","-f","tests\1to32.rmd","-r",$range,"-l","compress","-x","random","-U","cuda","-G","128","-t","1","-q","-s","5"
) -Backend "CUDA" -Notes "GPU EC + host hash160"

# Modes with no GPU path (document)
$results += [pscustomobject]@{ Mode="mnemonic_gpu"; Backend="CUDA"; Rate=0; Display="n/a"; Notes="Not implemented - mnemonic stays CPU"; Raw="" }
$results += [pscustomobject]@{ Mode="vanity_gpu"; Backend="CUDA"; Rate=0; Display="n/a"; Notes="Not implemented - vanity stays CPU"; Raw="" }
$results += [pscustomobject]@{ Mode="bsgs_gpu"; Backend="CUDA"; Rate=0; Display="n/a"; Notes="Not implemented - BSGS stays CPU"; Raw="" }
$results += [pscustomobject]@{ Mode="sol_gpu"; Backend="CUDA"; Rate=0; Display="n/a"; Notes="Not implemented - Solana stays CPU"; Raw="" }

$results | Export-Csv -NoTypeInformation (Join-Path $OutDir "bench.csv")
$results | ConvertTo-Json | Set-Content (Join-Path $OutDir "bench.json")
$results | Format-Table -AutoSize | Out-String | Write-Host
$results | ConvertTo-Json -Compress | Set-Content (Join-Path $OutDir "bench_compact.txt")
Write-Host "Wrote $_bench_out\bench.csv"
