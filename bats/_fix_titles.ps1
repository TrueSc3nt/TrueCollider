$Bats = Split-Path -Parent $MyInvocation.MyCommand.Path
Get-ChildItem $Bats -Recurse -Filter *.bat | ForEach-Object {
  $t = [IO.File]::ReadAllText($_.FullName)
  $n = $t -replace '\?\?\+','-' -replace '\?\?\?','-'
  if ($n -ne $t) {
    [IO.File]::WriteAllText($_.FullName, $n)
    Write-Host "fixed $($_.Name)"
  }
}
$n = @(Get-ChildItem $Bats -Recurse -Filter *.bat).Count
Write-Host "Total bats: $n"
