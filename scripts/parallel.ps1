<#
  Finds all files with the given extension under the current directory
  and runs the provided command against each file in parallel.
  Usage:
    ./parallel.ps1 -FileExtension .txt -Command 'wc -l'
    ./parallel.ps1 .txt 'cmd /c find /v /c ""'
  Optional:
    -Throttle <int> : limit concurrent jobs (default = CPU count)
#>

param(
  [Parameter(Mandatory = $true, Position = 0)]
  [string] $FileExtension,
  [Parameter(Mandatory = $true, Position = 1)]
  [string] $Command,
  [int] $Throttle = [Math]::Max(1, [Environment]::ProcessorCount)
)

$ErrorActionPreference = 'Stop'

function Wait-For-OpenSlot {
  param([int] $Limit)
  while ((Get-Job -State Running).Count -ge $Limit) {
    Receive-Job -State Completed -AutoRemoveJob | Out-Null
    Start-Sleep -Milliseconds 100
  }
}

$files = Get-ChildItem -Path . -Recurse -File -Filter "*$FileExtension"
foreach ($file in $files) {
  Wait-For-OpenSlot -Limit $Throttle
  Start-Job -ScriptBlock {
    param($cmd, $path)
    $call = "$cmd `"$path`""
    cmd.exe /c $call | Out-String
  } -ArgumentList $Command, $file.FullName | Out-Null
}

while ((Get-Job).Count -gt 0) {
  Receive-Job -State Completed -AutoRemoveJob | Out-Null
  Start-Sleep -Milliseconds 100
}
