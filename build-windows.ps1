# Windows Build Script for Mechanica Imperii
# Ensures proper CMake configuration with x64 architecture

param(
    [ValidateSet("debug", "release", "vs-debug", "vs-release")]
    [string]$Config = "vs-release",

    [switch]$Clean,
    [switch]$Reconfigure,
    [int]$Jobs = 0
)

$ErrorActionPreference = "Stop"

Write-Host "=== Mechanica Imperii - Windows Build Script ===" -ForegroundColor Cyan
Write-Host "Configuration: $Config" -ForegroundColor Yellow

# Determine preset names
$configPreset = "windows-$Config"
$buildPreset = "windows-$Config"

# Check if vcpkg is available for non-dev builds
if (-not $env:VCPKG_ROOT -and $Config -notlike "*dev*") {
    Write-Host "WARNING: VCPKG_ROOT environment variable not set" -ForegroundColor Yellow
    Write-Host "Expected location: C:\vcpkg" -ForegroundColor Yellow
    $vcpkgPath = "C:\vcpkg"
    if (Test-Path $vcpkgPath) {
        Write-Host "Found vcpkg at $vcpkgPath, setting VCPKG_ROOT..." -ForegroundColor Green
        $env:VCPKG_ROOT = $vcpkgPath
    } else {
        Write-Host "ERROR: vcpkg not found. Please install vcpkg or set VCPKG_ROOT" -ForegroundColor Red
        exit 1
    }
}

# Determine build directory
$buildDir = "build\windows-$Config"

# Clean build directory if requested
if ($Clean) {
    Write-Host "Cleaning build directory: $buildDir" -ForegroundColor Yellow
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
        Write-Host "Build directory cleaned" -ForegroundColor Green
    }
    $Reconfigure = $true
}

# Reconfigure if requested or if build directory doesn't exist
if ($Reconfigure -or -not (Test-Path "$buildDir\CMakeCache.txt")) {
    Write-Host ""
    Write-Host "=== Configuring CMake ===" -ForegroundColor Cyan
    Write-Host "Preset: $configPreset" -ForegroundColor Yellow
    Write-Host "Architecture: x64" -ForegroundColor Yellow
    Write-Host ""

    # Run CMake configure with explicit architecture
    & cmake --preset $configPreset

    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
        Write-Host ""
        Write-Host "Troubleshooting tips:" -ForegroundColor Yellow
        Write-Host "1. Ensure CMake 3.15+ is installed: cmake --version" -ForegroundColor White
        Write-Host "2. For Visual Studio builds, ensure VS 2022 is installed" -ForegroundColor White
        Write-Host "3. Try cleaning: .\build-windows.ps1 -Config $Config -Clean" -ForegroundColor White
        Write-Host "4. Check vcpkg installation: Test-Path `$env:VCPKG_ROOT\vcpkg.exe" -ForegroundColor White
        exit 1
    }

    Write-Host ""
    Write-Host "CMake configuration successful!" -ForegroundColor Green
}

# Build
Write-Host ""
Write-Host "=== Building ===" -ForegroundColor Cyan
Write-Host "Preset: $buildPreset" -ForegroundColor Yellow

$buildArgs = @("--build", "--preset", $buildPreset)

if ($Jobs -gt 0) {
    Write-Host "Parallel jobs: $Jobs" -ForegroundColor Yellow
    $buildArgs += "-j", $Jobs
}

Write-Host ""
& cmake @buildArgs

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Common issues:" -ForegroundColor Yellow
    Write-Host "1. Stale cache - try: .\build-windows.ps1 -Config $Config -Reconfigure" -ForegroundColor White
    Write-Host "2. Missing dependencies - vcpkg should auto-install during configure" -ForegroundColor White
    Write-Host "3. Architecture mismatch - this script enforces x64" -ForegroundColor White
    exit 1
}

Write-Host ""
Write-Host "=== Build Successful! ===" -ForegroundColor Green
Write-Host ""
Write-Host "Executable: $buildDir\bin\mechanica_imperii.exe" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run:" -ForegroundColor Yellow
Write-Host "  .\$buildDir\bin\mechanica_imperii.exe" -ForegroundColor White
Write-Host ""

# Offer to run if debug build
if ($Config -like "*debug*") {
    $runNow = Read-Host "Run the executable now? (y/N)"
    if ($runNow -eq "y" -or $runNow -eq "Y") {
        & ".\$buildDir\bin\mechanica_imperii.exe"
    }
}
