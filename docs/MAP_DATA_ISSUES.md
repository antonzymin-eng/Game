# Europe Map Rendering Issues - Analysis & Recommendations

**Date**: 2025-11-30
**Issue**: Former Soviet states rendering as squares instead of proper polygon shapes

---

## Problem Summary

**Symptoms**:
- Ukraine, Belarus, Moldova, and Russia (European) provinces render as **perfect squares/rectangles**
- Map units not properly joined at borders
- 23 provinces total have placeholder geometry (5-9 boundary points)

**Root Cause**: **Missing or placeholder GeoJSON source data**

---

## Affected Provinces (23 total)

### Ukraine (5 provinces)
- Kyiv and Central Ukraine
- Western Ukraine
- Southern Ukraine
- Eastern Ukraine
- Northern Ukraine
- **All have 5 points** (perfect rectangles)

### Belarus (6 provinces)
- Minsk Region
- Brest Region
- Gomel Region
- Grodno Region
- Mogilev Region
- Vitebsk Region
- **All have 5 points** (perfect rectangles)

### Moldova (3 provinces)
- Central Moldova
- Northern Moldova
- Southern Moldova
- **All have 5 points** (perfect rectangles)

### Russia - European Part (5 provinces)
- Central Federal District
- Northwestern Federal District
- Southern Federal District
- North Caucasian Federal District
- Volga Federal District
- **All have 5 points** (perfect rectangles)

### France (4 provinces)
- Province 100, 101, 111
- **Have 5-9 points** (suspicious, likely placeholders)

---

## Technical Analysis

### Example: Ukraine "Kyiv and Central Ukraine" Boundary
```json
[
  {"x": 926.0, "y": -77.5},   // Bottom-left corner
  {"x": 1126.0, "y": -77.5},  // Bottom-right corner
  {"x": 1126.0, "y": 122.5},  // Top-right corner
  {"x": 926.0, "y": 122.5},   // Top-left corner
  {"x": 926.0, "y": -77.5}    // Close polygon
]
```
This is a **200×200 unit perfect square** - clearly a placeholder!

### Comparison: Proper Province Data
Île-de-France (France):
- **52 boundary points**
- Irregular, natural shape
- Smooth curved borders

---

## Data Generation Pipeline

### Source Files Status

**✅ Western Europe** (have proper NUTS1 GeoJSON data):
```
belgium_nuts1.geojson    (12 KB) ✓
france_nuts1.geojson     (60 KB) ✓
germany_nuts1.geojson    (70 KB) ✓
italy_nuts1.geojson      (38 KB) ✓
spain_nuts1.geojson      (47 KB) ✓
netherlands_nuts1.geojson (13 KB) ✓
portugal_nuts1.geojson   (8.4 KB) ✓
switzerland_nuts1.geojson (7.0 KB) ✓
```

**❌ Eastern Europe** (missing or placeholder):
```
NO ukraine_nuts1.geojson     ✗
NO belarus_nuts1.geojson     ✗
NO moldova_nuts1.geojson     ✗
NO russia_nuts1.geojson      ✗
uk_nuts1.geojson (14 bytes)  ✗ Empty placeholder
```

### Data Flow
```
GeoJSON Source Files (data/maps/geojson_source/)
    ↓
generate_europe_maps.py (converts lat/lon → game coordinates)
    ↓
map_{country}_real.json (individual country maps)
    ↓
update_combined_europe.py (combines all countries)
    ↓
map_europe_combined.json (final combined map)
```

**Issue**: For countries without GeoJSON source, someone manually created placeholder rectangles in the individual country map files.

---

## Solutions & Recommendations

### Option 1: Get Proper GeoJSON Data (RECOMMENDED)

**Sources for NUTS1 GeoJSON data**:
1. **Eurostat GISCO** (official EU statistical office)
   - URL: https://ec.europa.eu/eurostat/web/gisco/geodata/reference-data/administrative-units-statistical-units
   - Download NUTS 2021 dataset
   - Available for all EU member states

2. **Natural Earth** (global administrative boundaries)
   - URL: https://www.naturalearthdata.com/downloads/10m-cultural-vectors/
   - Download "Admin 1 – States, Provinces"
   - Includes Ukraine, Belarus, Russia, Moldova

3. **OpenStreetMap** (via Overpass API)
   - Query administrative boundaries (admin_level=4 for NUTS1 equivalent)
   - Good coverage for Eastern Europe

**Steps**:
1. Download NUTS1 or equivalent GeoJSON for:
   - Ukraine (UKR)
   - Belarus (BLR)
   - Moldova (MDA)
   - Russia - European part (RUS)
   - UK (GBR) - fix empty files

2. Save to `data/maps/geojson_source/` as:
   - `ukraine_nuts1.geojson`
   - `belarus_nuts1.geojson`
   - `moldova_nuts1.geojson`
   - `russia_european_nuts1.geojson`
   - `uk_nuts1.geojson`

3. Run generation scripts:
   ```bash
   cd data
   python generate_europe_maps.py
   python update_combined_europe.py
   ```

4. Rebuild game with new map data

---

### Option 2: Use Historical Boundaries (Alternative)

Use the existing historical 1100 AD data which has better coverage:

**Check**:
```bash
ls data/maps/historical_1100/
```

These files might have better polygon data for Eastern European realms.

---

### Option 3: Accept Placeholders (Temporary)

If getting proper data is not a priority:
- Current rectangles are **functional** but **not visually accurate**
- Game is playable with placeholder provinces
- Can be improved later

**Trade-offs**:
- ✗ Not historically/geographically accurate
- ✗ Poor visual aesthetics
- ✓ Game works functionally
- ✓ Can be improved incrementally

---

## Quick Fix: Hide Placeholder Provinces (Optional)

If you want to remove the rectangles until proper data is available:

**Edit** `data/update_combined_europe.py`:
```python
# Skip countries with placeholder data
SKIP_PLACEHOLDER_COUNTRIES = ['ukraine', 'belarus', 'moldova', 'russia_european']

for country_name in ALL_EUROPEAN_COUNTRIES:
    if country_name in SKIP_PLACEHOLDER_COUNTRIES:
        print(f"  Skipping {country_name} (placeholder data)")
        continue

    country_map = load_country_map(country_name, maps_dir)
    # ... rest of code
```

Then regenerate:
```bash
cd data
python update_combined_europe.py
```

This will create a map with only Western/Central Europe (proper polygons).

---

## Expected Results After Fix

**With Proper GeoJSON Data**:
- Ukraine provinces: 30-100+ boundary points (natural borders)
- Belarus provinces: 40-80+ boundary points
- Moldova provinces: 30-60+ boundary points
- Russia provinces: 50-150+ boundary points
- Smooth, curved, realistic borders
- Proper neighbor adjacency

**Current Status** (with placeholders):
- All former Soviet provinces: 5 points (squares)
- Sharp 90-degree corners
- No natural geography
- Gaps between provinces

---

## Technical Note: Why 5 Points?

A polygon needs minimum 3 points to be valid. 5 points creates a closed rectangle:
```
Point 1: (x1, y1) ────────── Point 2: (x2, y1)
         │                            │
         │                            │
Point 4: (x1, y2) ────────── Point 3: (x2, y2)
         │
Point 5: (x1, y1) ← Close polygon
```

This is the **minimal placeholder** that renders without crashing but looks terrible.

---

## Recommended Action Plan

1. **Immediate** (5 minutes):
   - Document this issue for the team
   - Mark affected provinces on map

2. **Short-term** (1-2 hours):
   - Download NUTS1 GeoJSON from Eurostat/Natural Earth
   - Place in geojson_source directory
   - Regenerate maps

3. **Long-term** (1 day):
   - Validate all 133 provinces have proper geometry
   - Add elevation data
   - Implement historical boundary transitions

---

## Files to Update

**If adding proper GeoJSON data**:
- `data/maps/geojson_source/ukraine_nuts1.geojson` (NEW)
- `data/maps/geojson_source/belarus_nuts1.geojson` (NEW)
- `data/maps/geojson_source/moldova_nuts1.geojson` (NEW)
- `data/maps/geojson_source/russia_european_nuts1.geojson` (NEW)
- `data/maps/geojson_source/uk_nuts1.geojson` (REPLACE - currently empty)

**Then regenerate**:
- Run `python data/generate_europe_maps.py`
- Run `python data/update_combined_europe.py`
- Rebuild game
- Test

---

## Conclusion

The map rendering **works correctly** - it's displaying exactly what's in the data file. The issue is **placeholder geometry** in the source data for Eastern European countries.

**Solution**: Get proper NUTS1 or administrative boundary GeoJSON files for the affected countries.

**Temporary workaround**: Accept the rectangles as placeholders until proper data is available.

**Status**: Not a code bug - this is a **data quality issue**.

---

**Next Steps**: Obtain GeoJSON files or use historical boundary data.
