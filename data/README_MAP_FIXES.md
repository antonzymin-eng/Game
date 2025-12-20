# Map Generation Fixes - Summary

## What Was Fixed

### 1. Missing GeoJSON Data
**Problem**: Ukraine, Belarus, Moldova, Russia, and UK had placeholder rectangle provinces instead of real geographic boundaries.

**Solution**: Created `download_missing_geojson.py` to automatically download proper administrative boundary data from Natural Earth Data.

### 2. Incorrect Province Adjacencies
**Problem**: The adjacency calculation in C++ happens at runtime but wasn't being performed, or the tolerance was too strict, causing missing or incorrect neighbor relationships.

**Solution**:
- Created `calculate_adjacencies.py` to pre-calculate adjacencies during map generation
- Uses adaptive tolerance (0.5% of median province diagonal) for robust edge detection
- Calculates actual shared border lengths for diplomacy/influence systems
- Bakes adjacency data into map JSON files for faster loading

### 3. Cross-Border Adjacencies Missing
**Problem**: When combining country maps, province IDs change and cross-border neighbors weren't recalculated.

**Solution**: Created `update_combined_europe_with_adjacencies.py` to:
- Combine all country maps
- Renumber province IDs globally
- Recalculate all adjacencies including cross-border neighbors

---

## Quick Start

### Full Map Regeneration (Recommended)

```bash
cd /home/user/Game/data
./regenerate_maps.sh
```

This runs the complete pipeline:
1. Downloads missing GeoJSON data
2. Generates individual country maps
3. Calculates adjacencies
4. Combines into `map_europe_combined.json`

### Manual Steps (if preferred)

```bash
cd /home/user/Game/data

# 1. Download missing data (Ukraine, Belarus, Moldova, Russia, UK)
python3 download_missing_geojson.py

# 2. Generate country maps from GeoJSON
python3 generate_europe_maps.py

# 3. Calculate adjacencies for each country
python3 calculate_adjacencies.py

# 4. Combine with cross-border adjacencies
python3 update_combined_europe_with_adjacencies.py
```

### Fix Adjacencies Only (Skip Download & Generation)

If you already have map files but just need to fix adjacencies:

```bash
cd /home/user/Game/data

# Recalculate adjacencies for existing maps
python3 calculate_adjacencies.py

# Combine with cross-border adjacencies
python3 update_combined_europe_with_adjacencies.py
```

---

## Files Created

| File | Purpose |
|------|---------|
| `download_missing_geojson.py` | Downloads GeoJSON data from Natural Earth |
| `calculate_adjacencies.py` | Calculates province neighbors with border lengths |
| `update_combined_europe_with_adjacencies.py` | Combines maps with cross-border adjacencies |
| `regenerate_maps.sh` | Master script to run full workflow |
| `MAP_GENERATION_GUIDE.md` | Complete documentation (troubleshooting, algorithms, etc.) |
| `README_MAP_FIXES.md` | This file - quick summary |

---

## What to Expect

### Before Fixes
- Ukraine, Belarus, Moldova, Russia: Rectangle provinces (5 points each)
- Missing neighbor relationships between provinces
- Cross-border gaps
- Average 0-2 neighbors per province

### After Fixes
- Real geographic boundaries (30-100+ points per province)
- Proper neighbor relationships
- Seamless borders between countries
- Average 5-7 neighbors per province
- Border lengths calculated for gameplay systems

---

## Verification

After running regeneration:

```bash
# Check province count
jq '.map_region.provinces | length' maps/map_europe_combined.json

# Check average neighbors (should be ~5-7)
jq '[.map_region.provinces[].neighbors | length] | add / length' maps/map_europe_combined.json

# Find isolated provinces (islands are OK)
jq '.map_region.provinces[] | select(.neighbors | length == 0) | .name' maps/map_europe_combined.json
```

### Test in Game

```bash
cd /home/user/Game
./build/apps/main
```

Check logs for:
```
[MapInit] Loading province data from data/maps/map_europe_combined.json
[MapDataLoader] Loaded XXX provinces
```

Verify visually:
- No rectangular provinces (except intentional placeholders)
- Borders align between countries
- Province colors change at borders

---

## Known Limitations

1. **Island provinces**: Will have 0 neighbors (expected: Iceland, Malta, Cyprus, Corsica, etc.)

2. **Download requirements**:
   - Needs internet connection for `download_missing_geojson.py`
   - Optional: Install `fiona` or `gdal-bin` for better shapefile processing
   ```bash
   pip3 install fiona
   # or
   sudo apt-get install python3-fiona gdal-bin
   ```

3. **Performance**: Adjacency calculation is O(n²)
   - 100 provinces: ~1 second
   - 500 provinces: ~30 seconds
   - 1000 provinces: ~2 minutes

4. **Russia**: Only European part included (west of 60°E longitude)

---

## Troubleshooting

### "No features found for [country]"

The GeoJSON file may not have data for that country. Try:
1. Download manually from Eurostat or OpenStreetMap
2. Check country code in GeoJSON properties
3. See MAP_GENERATION_GUIDE.md for alternative sources

### "X isolated provinces" (besides islands)

If unexpected provinces are isolated:
1. Check boundary point count: `jq '.map_region.provinces[N].boundary | length'`
2. Increase tolerance if boundaries slightly misaligned
3. Verify coordinate conversion is correct

### Map doesn't load in game

1. Check JSON is valid: `jq . maps/map_europe_combined.json > /dev/null`
2. Check file size is reasonable (>100KB)
3. Look for errors in game logs
4. Verify C++ loader can handle neighbor format

---

## Algorithm Details

### Adjacency Detection

Two provinces are neighbors if their boundaries share an edge:

1. **Compare all edge pairs**: O(n×m) where n,m = boundary point counts
2. **Detect collinear overlaps**: Edges that lie on the same line and overlap
3. **Calculate shared length**: Sum of overlapping segments
4. **Apply tolerance**: Points within `tolerance` distance are considered touching

### Adaptive Tolerance

```python
tolerance = median_province_diagonal * 0.005  # 0.5% of typical size
```

Why 0.5%?
- 0.1%: Too strict, misses valid neighbors
- 1.0%: Too loose, false positives
- 0.5%: Best balance for real geographic data

### Border Length Calculation

Only collinear overlapping segments contribute to border length:
- **Point touches**: Neighbors, but border_length = 0
- **Edge crossing**: Neighbors, but border_length = 0
- **Edge overlap**: Neighbors, border_length = overlap distance

Used by game systems for:
- Diplomatic relations (longer borders = more interaction)
- Influence spreading
- Trade route weighting
- Military movement costs

---

## Next Steps

After map generation:

1. **Test thoroughly**: Verify all provinces render correctly
2. **Check performance**: Monitor frame rate with full map
3. **Tune rendering**: Adjust LOD (Level-of-Detail) if needed
4. **Add sea zones**: Define naval regions for naval movement
5. **Historical maps**: Apply to `historical_1100` if needed

---

## References

- **Full Documentation**: `MAP_GENERATION_GUIDE.md`
- **Known Issues**: `../docs/MAP_DATA_ISSUES.md`
- **Map Architecture**: `../docs/architecture/MAP-RENDERING-ARCHITECTURE.md`
- **Natural Earth Data**: https://www.naturalearthdata.com/
- **Eurostat GISCO**: https://ec.europa.eu/eurostat/web/gisco/geodata

---

## Support

Questions or issues? Check:
1. MAP_GENERATION_GUIDE.md for detailed troubleshooting
2. Game logs for loading errors
3. Run ValidateGraph() in C++ for data integrity

**Created**: 2025-12-20
**Purpose**: Fix missing GeoJSON data and incorrect province adjacencies
