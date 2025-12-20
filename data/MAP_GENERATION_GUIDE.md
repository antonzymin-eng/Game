# Europe Map Generation Guide

Complete guide for generating and updating the Europe map with proper province boundaries and adjacencies.

## Overview

The map generation system converts real-world geographic data (GeoJSON) into game map files with proper province boundaries and neighbor relationships.

## Quick Start

### Option 1: Full Regeneration (Recommended)

```bash
cd /home/user/Game/data

# Step 1: Download missing GeoJSON data
python3 download_missing_geojson.py

# Step 2: Generate individual country maps
python3 generate_europe_maps.py

# Step 3: Calculate adjacencies for each country
python3 calculate_adjacencies.py

# Step 4: Combine into Europe map with cross-border adjacencies
python3 update_combined_europe_with_adjacencies.py
```

### Option 2: Update Adjacencies Only

If you already have map files but adjacencies are wrong:

```bash
cd /home/user/Game/data

# Recalculate adjacencies for existing maps
python3 calculate_adjacencies.py

# Combine with cross-border adjacencies
python3 update_combined_europe_with_adjacencies.py
```

---

## File Structure

```
data/
├── maps/
│   ├── geojson_source/          # Source GeoJSON files
│   │   ├── france_nuts1.geojson
│   │   ├── germany_nuts1.geojson
│   │   ├── ukraine_nuts1.geojson (after download)
│   │   └── ...
│   ├── map_france_real.json     # Individual country maps
│   ├── map_germany_real.json
│   └── map_europe_combined.json # Combined map (loaded by game)
│
├── download_missing_geojson.py   # Download GeoJSON data
├── generate_europe_maps.py       # Convert GeoJSON → map JSON
├── calculate_adjacencies.py      # Add neighbor relationships
└── update_combined_europe_with_adjacencies.py  # Combine maps
```

---

## Scripts Explained

### 1. `download_missing_geojson.py`

**Purpose**: Downloads missing GeoJSON administrative boundary data from Natural Earth Data.

**Downloads**:
- Ukraine (NUTS1 equivalent regions)
- Belarus
- Moldova
- Russia (European part only, west of 60°E)
- United Kingdom

**Requirements**:
- Internet connection
- Python 3.6+
- Optional: `fiona` or `gdal-bin` for shapefile processing

**Install dependencies** (if needed):
```bash
# Ubuntu/Debian
sudo apt-get install python3-fiona gdal-bin

# Or with pip
pip3 install fiona
```

**Output**: GeoJSON files in `data/maps/geojson_source/`

---

### 2. `generate_europe_maps.py`

**Purpose**: Converts GeoJSON lat/lon coordinates to game coordinates and creates individual country map files.

**What it does**:
- Reads NUTS1 GeoJSON data
- Converts WGS84 lat/lon → game Cartesian coordinates
- Creates one map file per country (e.g., `map_france_real.json`)
- Calculates province centers and bounding boxes

**Coordinate System**:
- Center: 10°E, 50°N (Central Europe)
- Scale: 50 game units per degree
- Game coordinates: Cartesian 2D

**Output**: Individual `map_{country}_real.json` files

---

### 3. `calculate_adjacencies.py`

**Purpose**: Calculates which provinces share borders and adds neighbor relationships.

**Algorithm**:
- Compares all province boundary edges
- Detects shared edges (collinear overlapping segments)
- Calculates shared border length
- Uses adaptive tolerance (0.5% of median province diagonal)

**Features**:
- Bidirectional neighbor relationships
- Border length calculation (used for diplomacy/influence)
- Identifies isolated provinces (islands)
- Creates backups before modifying files

**Output**: Updates `map_{country}_real.json` files with `neighbors` field

---

### 4. `update_combined_europe_with_adjacencies.py`

**Purpose**: Combines all country maps into a single Europe map with cross-border adjacencies.

**What it does**:
- Loads all individual country maps
- Renumbers province IDs globally (starts at 100)
- **Recalculates adjacencies** including cross-border neighbors
- Combines into `map_europe_combined.json`

**Why recalculate adjacencies?**
- Province IDs change when combined
- Cross-border adjacencies (France-Germany, Poland-Ukraine, etc.)
- Ensures consistency across the entire map

**Output**: `map_europe_combined.json` (this is what the game loads)

---

## Understanding Adjacency Calculation

### The Problem

Province adjacency is determined by geometric intersection of boundaries. The challenge:

1. **Floating-point precision**: Coordinates may not match exactly
2. **Tolerance needed**: Points within tolerance distance are considered "touching"
3. **Too strict tolerance**: Provinces that should be neighbors aren't detected
4. **Too loose tolerance**: False positives, unrelated provinces marked as neighbors

### The Solution

**Adaptive Tolerance**:
```python
tolerance = median_province_diagonal * 0.005  # 0.5% of typical province size
```

**Shared Border Detection**:
- Checks if polygon edges are collinear and overlapping
- Calculates actual shared border length
- Only marks as neighbors if border length > tolerance

**Why 0.5%?**
- 0.1% (C++ default): Too strict, misses valid neighbors
- 1.0%: Too loose, creates false neighbors
- 0.5%: Sweet spot for real geographic data

### Adjacency Data Format

```json
{
  "id": 100,
  "name": "Île-de-France",
  "neighbors": [
    {
      "id": 101,
      "border_length": 45.32
    },
    {
      "id": 102,
      "border_length": 38.67
    }
  ]
}
```

---

## Common Issues & Solutions

### Issue: "No features found for [country]"

**Cause**: GeoJSON file doesn't contain data for that country

**Solution**:
1. Check the GeoJSON file:
   ```bash
   jq '.features[0].properties' data/maps/geojson_source/ukraine_nuts1.geojson
   ```
2. Verify country code matches (ISO 3166-1)
3. Download from alternative source (Eurostat, OpenStreetMap)

---

### Issue: "X isolated provinces"

**Cause**: Islands, or tolerance too strict

**Islands** (expected isolated provinces):
- Iceland
- Malta
- Cyprus
- Island regions of Norway, Sweden, Finland

**Unexpected isolated provinces**:
- Check if province has valid boundary data
- Increase tolerance if boundaries are slightly misaligned
- Verify coordinate system conversion

---

### Issue: "Provinces rendering as rectangles"

**Cause**: Using placeholder data instead of real GeoJSON

**Check**:
```bash
# Count boundary points (should be 30-100+ for real data, 5 for placeholders)
jq '.map_region.provinces[0].boundary | length' data/maps/map_ukraine_real.json
```

**Solution**:
1. Run `download_missing_geojson.py`
2. Regenerate maps
3. If still rectangles, check GeoJSON source quality

---

### Issue: "Adjacencies are incorrect"

**Symptoms**:
- Provinces that should touch aren't neighbors
- Provinces far apart are marked as neighbors

**Debug**:
1. Check tolerance used:
   ```bash
   grep "tolerance" <output_from_calculate_adjacencies.py>
   ```

2. Visualize boundaries:
   ```bash
   # Extract a problem province's boundary
   jq '.map_region.provinces[] | select(.name == "Province Name")' map.json
   ```

3. Adjust tolerance manually:
   ```python
   # In calculate_adjacencies.py
   add_adjacencies_to_map(map_file, tolerance=2.0)  # Force specific tolerance
   ```

---

## Testing & Verification

### 1. Verify Map Loads

```bash
cd /home/user/Game
./build/apps/main
```

Check logs for:
```
[MapInit] Step 3: Loading province data from data/maps/map_europe_combined.json...
[MapDataLoader] Loaded X provinces
```

### 2. Check Adjacency Graph

```cpp
// In C++, run validation
ProvinceGraph graph;
graph.Build(provinces);
bool valid = graph.ValidateGraph();  // Should return true
```

### 3. Visual Inspection

Run the game and verify:
- No rectangular provinces (except placeholders)
- Borders between countries align properly
- Province colors change at borders (indicating different owners)

### 4. Statistics

```bash
# Count adjacencies
jq '[.map_region.provinces[].neighbors | length] | add' data/maps/map_europe_combined.json

# Average neighbors per province (should be ~5-7)
jq '[.map_region.provinces[].neighbors | length] | (add / length)' data/maps/map_europe_combined.json

# Find isolated provinces
jq '.map_region.provinces[] | select(.neighbors | length == 0) | .name' data/maps/map_europe_combined.json
```

---

## Advanced: Manual GeoJSON Sources

If automatic download doesn't work:

### Eurostat GISCO (EU countries only)

1. Visit: https://ec.europa.eu/eurostat/web/gisco/geodata
2. Download: "NUTS 2021 - SHP/GeoJSON"
3. Extract NUTS level 1 for your country
4. Save as `{country}_nuts1.geojson`

### Natural Earth (Global)

1. Visit: https://www.naturalearthdata.com/
2. Download: "Admin 1 – States, Provinces" (10m)
3. Filter by country using QGIS or `ogr2ogr`:
   ```bash
   ogr2ogr -where "admin='Ukraine'" ukraine_nuts1.geojson ne_10m_admin_1_states_provinces.shp
   ```

### OpenStreetMap (Most up-to-date)

Use Overpass API to query administrative boundaries:

```bash
curl -X POST https://overpass-api.de/api/interpreter \
  --data "[out:json];area[name='Ukraine'];(rel(area)[admin_level=4];);out geom;" \
  > ukraine_osm.json
```

Then convert to standard GeoJSON format.

---

## Coordinate System Reference

### WGS84 (Input - GeoJSON)
- Latitude: -90° to +90° (south to north)
- Longitude: -180° to +180° (west to east)
- Europe: roughly 35°N to 72°N, -15°W to 45°E

### Game Coordinates (Output - Map JSON)
- Cartesian 2D plane
- Origin: 10°E, 50°N (Central Europe)
- Scale: 50 units/degree
- X-axis: West (-) to East (+)
- Y-axis: South (-) to North (+)

### Conversion Formula
```python
center_lon = 10.0
center_lat = 50.0
scale = 50.0

x = (lon - center_lon) * scale
y = (lat - center_lat) * scale
```

**Example**:
- Paris (2.35°E, 48.86°N) → (-382.5, -57.0)
- Berlin (13.40°E, 52.52°N) → (170.0, 126.0)
- Kyiv (30.52°E, 50.45°N) → (1026.0, 22.5)

---

## Performance Notes

### Map Generation Speed

- **download_missing_geojson.py**: 10-30 seconds (download time)
- **generate_europe_maps.py**: 1-5 seconds
- **calculate_adjacencies.py**:
  - Single country: 1-10 seconds
  - Depends on province count: O(n²) comparisons
- **update_combined_europe_with_adjacencies.py**:
  - 500 provinces: ~30 seconds
  - 1000 provinces: ~2 minutes
  - Scales as O(n²)

### Optimization Tips

For large maps (>1000 provinces):

1. **Spatial indexing**: Use QuadTree/R-tree for bounding box checks
2. **Parallel processing**: Process country maps in parallel
3. **Incremental updates**: Only recalculate affected provinces
4. **Cache results**: Store adjacency graph separately

---

## Troubleshooting

### Python errors: "No module named 'fiona'"

```bash
pip3 install fiona
# or
sudo apt-get install python3-fiona
```

### ogr2ogr not found

```bash
sudo apt-get install gdal-bin
```

### Download fails (SSL errors)

```bash
# Try with updated certificates
pip3 install --upgrade certifi
```

### Out of memory during adjacency calculation

Reduce batch size or process countries individually:
```bash
# Instead of combined map
python3 calculate_adjacencies.py  # Process each country separately
# Then load in game and calculate cross-border in C++
```

---

## Next Steps

After generating maps:

1. **Test in game**: Verify map loads and renders correctly
2. **Check performance**: Monitor frame rate with full map
3. **Tune LOD system**: Adjust level-of-detail thresholds if needed
4. **Add sea zones**: Define naval regions (future enhancement)
5. **Historical maps**: Apply same process to historical_1100 data

---

## References

- **Natural Earth Data**: https://www.naturalearthdata.com/
- **Eurostat GISCO**: https://ec.europa.eu/eurostat/web/gisco/geodata
- **OpenStreetMap**: https://www.openstreetmap.org/
- **NUTS Classification**: https://ec.europa.eu/eurostat/web/nuts
- **GeoJSON Spec**: https://geojson.org/

---

## Support

Issues with map generation? Check:

1. **Logs**: Look for error messages in script output
2. **Backups**: Scripts create `.backup` files before modifying
3. **Documentation**: See `docs/MAP_DATA_ISSUES.md` for known issues
4. **Validation**: Run `ValidateGraph()` in C++ to check data integrity
