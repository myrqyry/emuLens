param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$BuildType = "Release"
)

$ErrorActionPreference = "Stop"

Write-Host "================================================" -ForegroundColor Cyan
Write-Host "  Building EmuLens for Windows (emuLensFX module)" -ForegroundColor Cyan
Write-Host "================================================" -ForegroundColor Cyan

$InstallPrefix = Join-Path $env:APPDATA "obs-studio\plugins\emuLensFX"
Write-Host "Install prefix: $InstallPrefix" -ForegroundColor Green

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "cmake was not found in PATH. Install CMake first."
}

New-Item -ItemType Directory -Path build -Force | Out-Null
Set-Location build

cmake -S .. -B . `
    -G "Visual Studio 17 2022" `
    -A x64 `
    -DCMAKE_INSTALL_PREFIX="$InstallPrefix" `
    -DOBS_PLUGIN_DESTINATION="bin/64bit" `
    -DOBS_DATA_DESTINATION="data"

cmake --build . --config $BuildType --parallel
cmake --install . --config $BuildType --prefix "$InstallPrefix"

Write-Host ""
Write-Host "✓ Installation complete. Restart OBS Studio." -ForegroundColor Green
Write-Host "Plugin installed to: $InstallPrefix" -ForegroundColor Cyan
