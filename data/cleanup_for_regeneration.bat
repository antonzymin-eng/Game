@echo off
REM ============================================================
REM Cleanup Script for FULL Map Regeneration
REM ============================================================
REM Deletes ALL GeoJSON source files and map files so the
REM entire European map can be regenerated from scratch with
REM correct province limits.
REM ============================================================

echo ============================================================
echo FULL Map Regeneration Cleanup
echo ============================================================
echo.
echo WARNING: This will delete ALL generated map files and GeoJSON sources!
echo.
echo This allows complete regeneration with correct province limits for
echo all European countries, preventing the 'all provinces = france' bug.
echo.

set /p response="Continue? (yes/no): "
if /i not "%response%"=="yes" if /i not "%response%"=="y" (
    echo Cancelled.
    exit /b
)

REM Delete ALL GeoJSON source files
echo.
echo Deleting ALL GeoJSON source files...
del /q "maps\geojson_source\*.geojson" 2>nul
if errorlevel 1 (
    echo   No .geojson files found
) else (
    echo   Deleted all .geojson files
)

REM Delete ALL generated map_*_real.json files
echo.
echo Deleting ALL map_*_real.json files...
del /q "maps\map_*_real.json" 2>nul
if errorlevel 1 (
    echo   No map_*_real.json files found
) else (
    echo   Deleted all map_*_real.json files
)

REM Delete backup files
echo.
echo Deleting backup files...
del /q "maps\*.backup" 2>nul
if errorlevel 1 (
    echo   No backup files found
) else (
    echo   Deleted all backup files
)

REM Delete combined map
echo.
echo Deleting combined map...
if exist "maps\map_europe_combined.json" (
    del /q "maps\map_europe_combined.json"
    echo   Deleted map_europe_combined.json
) else (
    echo   map_europe_combined.json not found
)

echo.
echo ============================================================
echo Cleanup Complete!
echo ============================================================
echo.
echo Next steps:
echo   1. Run: .\regenerate_maps.bat
echo   2. This will:
echo      - Download GeoJSON for ALL European countries (~36 countries)
echo      - Generate map files with correct owner_realm values
echo      - Calculate adjacencies
echo      - Create combined map
echo   3. Then rebuild the game: cmake --build build\windows-vs-release --config Release
echo   4. The map will now be right-side up (Y-axis fixed)
echo.

pause
