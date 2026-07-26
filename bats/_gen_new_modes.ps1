# Generate bats for new -x / -B puzzle walk modes
$ErrorActionPreference = "Stop"
$Bats = Split-Path -Parent $MyInvocation.MyCommand.Path

function Write-SimpleBat($RelPath, $Title, $CmdLine) {
  $full = Join-Path $Bats $RelPath
  $dir = Split-Path $full -Parent
  if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
  $depth = ($RelPath -split '[\\/]').Count - 1
  $common = if ($depth -ge 1) { '%~dp0..\_common.bat' } else { '%~dp0_common.bat' }
  @"
@echo off
REM =============================================================================
REM $Title
REM =============================================================================
setlocal EnableExtensions EnableDelayedExpansion
call "$common" cpu || exit /b 1
$CmdLine
exit /b %ERRORLEVEL%
"@ | Set-Content -Encoding ASCII $full
}

$patterns = @(
  'keyhole','pocket','afterimage','driftcompass','twinflame','breadcrumb',
  'clockwork','lottery','wave','waveroulette','density-map'
)
foreach ($p in $patterns) {
  $name = switch ($p) {
    'waveroulette' { 'x_waveroulette.bat' }
    'density-map'  { 'x_density.bat' }
    default        { "x_$p.bat" }
  }
  Write-SimpleBat "12_patterns\$name" "Search pattern -x $p (address key1 smoke)" `
    "keyhunt.exe -m address -f tests\_btc1.txt -r 1:200 -l compress -t 1 -x $p -q"
}

$bsgs = @(
  @{ n='modfan'; e=' --mod-step 4' },
  @{ n='shadowledger'; e=' --shadow-mod 8' },
  @{ n='hybrid'; e='' },
  @{ n='residue'; e=' --mod-step 4 --mod-rem 1' },
  @{ n='freeze-table'; e='' },
  @{ n='compact-dp'; e='' },
  @{ n='dual-range'; e='' }
)
foreach ($b in $bsgs) {
  Write-SimpleBat "04_bsgs\$($b.n).bat" "BSGS -B $($b.n) (tiny pubkey G smoke)" `
    "keyhunt.exe -m bsgs -f tests\_pubkey_g.txt -r 1:2 -n 1048576 -B $($b.n)$($b.e) -t 1 -q"
}

$puzzles = @(
  @{ n='b20_keyhole.bat'; c='keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x keyhole --window-bits 12 -q' },
  @{ n='b20_pocket.bat'; c='keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x pocket --pocket-bits 8 -q' },
  @{ n='b20_afterimage.bat'; c='keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x afterimage --antiloop-dist 8 -q' },
  @{ n='b20_driftcompass.bat'; c='keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x driftcompass -q' },
  @{ n='b20_twinflame.bat'; c='keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x twinflame -q' },
  @{ n='b20_breadcrumb.bat'; c='keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x breadcrumb -q' },
  @{ n='b20_clockwork.bat'; c='keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x clockwork -q' },
  @{ n='b20_lottery.bat'; c='keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x lottery -n 1024 -q' },
  @{ n='b20_wave.bat'; c='keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x wave -q' },
  @{ n='b20_sobol.bat'; c='keyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x sobol -q' },
  @{ n='b72_keyhole_recipe.bat'; c='echo [i] Puzzle 72-160 address/rmd160: -x keyhole --window-bits 40 (hash160; not kangaroo)`nkeyhunt.exe -m address -f tests\_puzzle20.txt -b 20 -l compress -t 2 -x keyhole --window-bits 16 -q' },
  @{ n='b135_bsgs_modfan_recipe.bat'; c='echo [i] Pubkey puzzles: BSGS -B modfan --mod-step M (not address hash160)`nkeyhunt.exe -m bsgs -f tests\_pubkey_g.txt -r 1:2 -n 1048576 -B modfan --mod-step 4 -t 2 -q' },
  @{ n='b135_bsgs_hybrid_recipe.bat'; c='echo [i] Hybrid warmup + random giants; honesty: BSGS not kangaroo sqrtN`nkeyhunt.exe -m bsgs -f tests\_pubkey_g.txt -r 1:2 -n 1048576 -B hybrid -t 2 -q' },
  @{ n='rmd20_pocket.bat'; c='keyhunt.exe -m rmd160 -f tests\_puzzle20.rmd -b 20 -l compress -t 2 -x pocket --pocket-bits 8 -q' }
)
foreach ($p in $puzzles) {
  Write-SimpleBat "17_puzzles\$($p.n)" "Puzzle / walk recipe: $($p.n)" $p.c
}

Write-Host "Wrote new mode bats under 12_patterns, 04_bsgs, 17_puzzles"
