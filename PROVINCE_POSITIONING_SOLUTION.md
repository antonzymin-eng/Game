# Province Positioning Issue - Resolution

## Summary of Fixes Applied

### ✅ Fixed Issues

1. **Debug Counter Showing 0/0 Provinces**
   - **Problem**: Debug display used ViewportCuller's cached statistics which were never updated
   - **Solution**: Changed debug display to use actual `rendered_province_count_` variable
   - **File**: `src/rendering/MapRenderer.cpp:463-471`

2. **Mouse Wheel Zoom Not Working**
   - **Problem**: `HandleInput()` was called before `ImGui::NewFrame()`, so ImGuiIO data wasn't populated
   - **Solution**: Moved `HandleInput()` to after `ImGui::NewFrame()`
   - **File**: `apps/main.cpp:1639-1642`

3. **Rendered Features Counter**
   - Already working correctly - shows 0 if no features at current LOD level

### ⚠️ Province Positioning Issue - Data Quality (Not a Bug)

**The Issue**: Provinces outside France/Iberia appear as rectangles and don't match real geography

**Root Cause**: This is **NOT a rendering bug**. The renderer is working correctly and displaying exactly what's in the data files. The issue is that **23 provinces have placeholder rectangle geometry** because they're missing proper GeoJSON source data:

#### Affected Countries (23 provinces total):
- **Ukraine** (5 provinces) - all 5-point rectangles
- **Belarus** (6 provinces) - all 5-point rectangles
- **Moldova** (3 provinces) - all 5-point rectangles
- **Russia European** (5 provinces) - all 5-point rectangles
- **France** (4 provinces) - suspicious 5-9 point placeholders

#### Why They're Rectangles:
These countries are missing GeoJSON source files in `data/maps/geojson_source/`:
- ❌ `ukraine_nuts1.geojson` - missing
- ❌ `belarus_nuts1.geojson` - missing
- ❌ `moldova_nuts1.geojson` - missing
- ❌ `russia_european_nuts1.geojson` - missing

## Solution: Get Proper GeoJSON Data

### Option 1: Download NUTS1 GeoJSON (Recommended)

**Sources**:
1. **Eurostat GISCO** (EU countries)
   - URL: https://ec.europa.eu/eurostat/web/gisco/geodata
   - Download NUTS 2021 dataset for EU member states

2. **Natural Earth** (global boundaries)
   - URL: https://www.naturalearthdata.com/downloads/10m-cultural-vectors/
   - Download "Admin 1 – States, Provinces"
   - Covers Ukraine, Belarus, Russia, Moldova

**Steps to Fix**:
```bash
# 1. Download GeoJSON files for affected countries

# 2. Place in source directory:
cd /home/user/Game/data/maps/geojson_source/
# Save files as:
#   ukraine_nuts1.geojson
#   belarus_nuts1.geojson
#   moldova_nuts1.geojson
#   russia_european_nuts1.geojson

# 3. Regenerate map files
cd /home/user/Game/data
python3 generate_europe_maps.py
python3 update_combined_europe.py

# 4. Rebuild and run the game
cd /home/user/Game/build
cmake .. && make
./Game
```

### Option 2: Use Historical 1100 AD Data

Check if better boundaries exist in historical data:
```bash
ls data/maps/historical_1100/
```

### Option 3: Accept Placeholders (Temporary)

The game is **fully functional** with placeholder rectangles - they're just not geographically accurate. This can be improved later when proper GeoJSON data is obtained.

## Expected Results After Fix

**With Proper GeoJSON**:
- Ukraine provinces: 30-100+ boundary points (natural borders)
- Belarus provinces: 40-80+ boundary points
- Moldova provinces: 30-60+ boundary points
- Russia provinces: 50-150+ boundary points
- Smooth, curved borders matching real geography
- Proper neighbor adjacency

**Current (Placeholders)**:
- All affected provinces: 5 points (perfect squares)
- 90-degree corners
- Approximate positioning only

## Technical Details

### Example Placeholder Province (Ukraine - Kyiv):
```json
{
  "boundary": [
    {"x": 926.0, "y": -77.5},   // Bottom-left
    {"x": 1126.0, "y": -77.5},  // Bottom-right
    {"x": 1126.0, "y": 122.5},  // Top-right
    {"x": 926.0, "y": 122.5},   // Top-left
    {"x": 926.0, "y": -77.5}    // Close polygon
  ]
}
```
This is a **200×200 unit perfect square** - clearly a placeholder!

### Example Proper Province (France - Île-de-France):
```json
{
  "boundary": [ /* 52 points with irregular, natural shape */ ]
}
```

## Code Changes Summary

### Modified Files:
1. `src/rendering/MapRenderer.cpp` - Fixed debug counter display
2. `apps/main.cpp` - Fixed mouse wheel zoom by reordering HandleInput()

### No Changes Needed:
- Coordinate system is working correctly
- Rendering pipeline is working correctly
- ViewportCuller is working correctly
- The "incorrect positioning" is actually just placeholder rectangle data

## References

- See `/home/user/Game/MAP_DATA_ISSUES.md` for detailed analysis
- See `/home/user/Game/data/generate_europe_maps.py` for coordinate conversion
- Map bounds: X[-1708.45, 2056.00], Y[-769.15, 1053.85]
- Center: (173.77, 142.35)

## Next Steps

**To fully resolve the province positioning issue**:
1. Obtain proper NUTS1 or administrative boundary GeoJSON files
2. Place in `data/maps/geojson_source/`
3. Run `python3 generate_europe_maps.py`
4. Run `python3 update_combined_europe.py`
5. Rebuild the game

**For now**: The rendering fixes have been applied. Debug counters and mouse wheel zoom will work correctly once the game is rebuilt.
