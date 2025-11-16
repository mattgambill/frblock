#!/usr/bin/env pwsh
<# 
  PowerShell equivalent of build-release.sh.
  Cleans the build directory, configures CMake for Release (tests off),
  and builds with parallelism.
#>

$ErrorActionPreference = 'Stop'

if (Test-Path -Path 'build') {
  Remove-Item -Path 'build' -Recurse -Force
}

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF

# Forward a parallel build hint; works with Ninja/Make and is ignored by generators that don't use -j.
$jobs = [Math]::Max(1, [Environment]::ProcessorCount)
cmake --build build --config Release -- -j $jobs
