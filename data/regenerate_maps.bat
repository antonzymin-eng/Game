@echo off
REM ============================================================
REM Europe Map Regeneration - Windows Version
REM ============================================================

echo ============================================================
echo Europe Map Regeneration
echo ============================================================
echo.

REM Check Python availability
where python >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Python not found!
    echo Please install Python from https://www.python.org/downloads/
    echo Make sure to check "Add Python to PATH" during installation
    pause
    exit /b 1
)

echo Python version:
python --version
echo.

REM Step 1: Download missing GeoJSON data
echo ------------------------------------------------------------
echo Step 1: Downloading missing GeoJSON data
echo ------------------------------------------------------------
echo.

if exist download_missing_geojson.py (
    echo Running download_missing_geojson.py...
    python download_missing_geojson.py
    if %errorlevel% equ 0 (
        echo ✓ Download complete
    ) else (
        echo ⚠ Download failed or skipped ^(may continue with existing files^)
    )
) else (
    echo ⚠ download_missing_geojson.py not found, skipping download
)

echo.

REM Step 2: Generate individual country maps
echo ------------------------------------------------------------
echo Step 2: Generating individual country maps from GeoJSON
echo ------------------------------------------------------------
echo.

if exist generate_europe_maps.py (
    echo Running generate_europe_maps.py...
    python generate_europe_maps.py
    if %errorlevel% equ 0 (
        echo ✓ Country maps generated
    ) else (
        echo ✗ Error generating country maps
        pause
        exit /b 1
    )
) else (
    echo ✗ ERROR: generate_europe_maps.py not found!
    pause
    exit /b 1
)

echo.

REM Step 3: Calculate adjacencies
echo ------------------------------------------------------------
echo Step 3: Calculating province adjacencies
echo ------------------------------------------------------------
echo.

if exist calculate_adjacencies.py (
    echo Running calculate_adjacencies.py...
    python calculate_adjacencies.py
    if %errorlevel% equ 0 (
        echo ✓ Adjacencies calculated
    ) else (
        echo ✗ Error calculating adjacencies
        pause
        exit /b 1
    )
) else (
    echo ⚠ calculate_adjacencies.py not found, skipping
)

echo.

REM Step 4: Combine into Europe map
echo ------------------------------------------------------------
echo Step 4: Combining into Europe map
echo ------------------------------------------------------------
echo.

if exist update_combined_europe_with_adjacencies.py (
    echo Running update_combined_europe_with_adjacencies.py...
    python update_combined_europe_with_adjacencies.py
    if %errorlevel% equ 0 (
        echo ✓ Combined map created
    ) else (
        echo ✗ Error creating combined map
        pause
        exit /b 1
    )
) else if exist update_combined_europe.py (
    echo ⚠ Using basic combiner ^(no cross-border adjacencies^)
    python update_combined_europe.py
) else (
    echo ✗ ERROR: No map combiner script found!
    pause
    exit /b 1
)

echo.

REM Verification
echo ------------------------------------------------------------
echo Verification
echo ------------------------------------------------------------
echo.

set COMBINED_MAP=maps\map_europe_combined.json

if exist "%COMBINED_MAP%" (
    echo ✓ Combined map exists: %COMBINED_MAP%

    for %%I in ("%COMBINED_MAP%") do echo   File size: %%~zI bytes

    REM Check if jq is available
    where jq >nul 2>&1
    if %errorlevel% equ 0 (
        echo.
        echo Statistics:
        for /f "delims=" %%i in ('jq ".map_region.provinces | length" "%COMBINED_MAP%"') do echo   Province count: %%i
        for /f "delims=" %%i in ('jq "[.map_region.provinces[].neighbors | length] | add" "%COMBINED_MAP%"') do echo   Total adjacencies: %%i
    ) else (
        echo   ^(Install jq for detailed statistics: https://stedolan.github.io/jq/^)
    )
) else (
    echo ✗ ERROR: Combined map not found!
    pause
    exit /b 1
)

echo.
echo ============================================================
echo Map Regeneration Complete!
echo ============================================================
echo.
echo Next steps:
echo   1. Test in game
echo   2. Check rendering and adjacencies
echo   3. Report any issues
echo.
pause
