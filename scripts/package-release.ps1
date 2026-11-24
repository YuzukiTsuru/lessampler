# Build, test, and package a release with MSVC. Run from a developer prompt.
param(
    [string]$Preset = "ci-msvc-max"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

cmake --preset $Preset
if ($LASTEXITCODE -ne 0) { throw "configure failed" }

cmake --build --preset $Preset -j
if ($LASTEXITCODE -ne 0) { throw "build failed" }

ctest --preset $Preset --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "tests failed" }

cmake --build --preset $Preset --target package
if ($LASTEXITCODE -ne 0) { throw "package failed" }

Write-Host ""
Write-Host "Release artifacts:"
Get-ChildItem build -Recurse -File |
    Where-Object { $_.Name -like "lessampler-*.zip*" } |
    ForEach-Object { Write-Host "  $($_.FullName)" }
