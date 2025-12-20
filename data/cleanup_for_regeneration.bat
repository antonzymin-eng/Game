@echo off
REM ============================================================
REM Cleanup Script for Map Regeneration
REM ============================================================
REM This script deletes old GeoJSON and map files for countries
REM that need to be regenerated with province limits.
REM ============================================================

echo ============================================================
echo Map Regeneration Cleanup
echo ============================================================
echo.
echo This will delete the following files so they can be regenerated:
echo   - GeoJSON source files for: Ukraine, Belarus, Moldova, Russia, UK
echo   - Generated map files for: Ukraine, Belarus, Moldova, Russia, UK
echo.
echo This is necessary because the new scripts limit provinces to
echo prevent excessive adjacency calculations (Russia: 8 instead of 59).
echo.

pause

REM Delete GeoJSON source files
echo.
echo Deleting GeoJSON source files...
if exist "maps\geojson_source\ukraine_nuts1.geojson" (
    del "maps\geojson_source\ukraine_nuts1.geojson"
    echo   Deleted ukraine_nuts1.geojson
)
if exist "maps\geojson_source\belarus_nuts1.geojson" (
    del "maps\geojson_source\belarus_nuts1.geojson"
    echo   Deleted belarus_nuts1.geojson
)
if exist "maps\geojson_source\moldova_nuts1.geojson" (
    del "maps\geojson_source\moldova_nuts1.geojson"
    echo   Deleted moldova_nuts1.geojson
)
if exist "maps\geojson_source\russia_nuts1.geojson" (
    del "maps\geojson_source\russia_nuts1.geojson"
    echo   Deleted russia_nuts1.geojson
)
if exist "maps\geojson_source\united_kingdom_nuts1.geojson" (
    del "maps\geojson_source\united_kingdom_nuts1.geojson"
    echo   Deleted united_kingdom_nuts1.geojson
)

REM Delete generated map files
echo.
echo Deleting generated map files...
if exist "maps\map_ukraine_real.json" (
    del "maps\map_ukraine_real.json"
    echo   Deleted map_ukraine_real.json
)
if exist "maps\map_belarus_real.json" (
    del "maps\map_belarus_real.json"
    echo   Deleted map_belarus_real.json
)
if exist "maps\map_moldova_real.json" (
    del "maps\map_moldova_real.json"
    echo   Deleted map_moldova_real.json
)
if exist "maps\map_russia_european_real.json" (
    del "maps\map_russia_european_real.json"
    echo   Deleted map_russia_european_real.json
)
if exist "maps\map_united_kingdom_real.json" (
    del "maps\map_united_kingdom_real.json"
    echo   Deleted map_united_kingdom_real.json
)

REM Also delete backup files if they exist
if exist "maps\map_ukraine_real.json.backup" del "maps\map_ukraine_real.json.backup"
if exist "maps\map_belarus_real.json.backup" del "maps\map_belarus_real.json.backup"
if exist "maps\map_moldova_real.json.backup" del "maps\map_moldova_real.json.backup"
if exist "maps\map_russia_european_real.json.backup" del "maps\map_russia_european_real.json.backup"
if exist "maps\map_united_kingdom_real.json.backup" del "maps\map_united_kingdom_real.json.backup"

echo.
echo ============================================================
echo Cleanup Complete!
echo ============================================================
echo.
echo Next steps:
echo   1. Run: .\regenerate_maps.bat
echo   2. The new files will have limited province counts:
echo      - Russia: 8 provinces (federal districts)
echo      - Ukraine: 8 provinces (major regions)
echo      - Belarus: 7 provinces (major regions)
echo      - Moldova: 5 provinces
echo      - UK: 12 provinces
echo.
pause
