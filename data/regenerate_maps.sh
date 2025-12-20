#!/bin/bash
#
# Complete Map Regeneration Workflow
#
# This script runs the full map generation pipeline:
# 1. Download missing GeoJSON data
# 2. Generate individual country maps
# 3. Calculate adjacencies
# 4. Combine into Europe map
#

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "============================================================"
echo "Europe Map Regeneration"
echo "============================================================"
echo ""

# Check Python availability
if ! command -v python3 &> /dev/null; then
    echo "ERROR: python3 not found!"
    echo "Please install Python 3.6 or later"
    exit 1
fi

echo "Python version:"
python3 --version
echo ""

# Step 1: Download missing GeoJSON data
echo "------------------------------------------------------------"
echo "Step 1: Downloading missing GeoJSON data"
echo "------------------------------------------------------------"
echo ""

if [ -f "download_missing_geojson.py" ]; then
    echo "Running download_missing_geojson.py..."
    python3 download_missing_geojson.py

    if [ $? -eq 0 ]; then
        echo "✓ Download complete"
    else
        echo "⚠ Download failed or skipped (may continue with existing files)"
    fi
else
    echo "⚠ download_missing_geojson.py not found, skipping download"
fi

echo ""

# Step 2: Generate individual country maps
echo "------------------------------------------------------------"
echo "Step 2: Generating individual country maps from GeoJSON"
echo "------------------------------------------------------------"
echo ""

if [ -f "generate_europe_maps.py" ]; then
    echo "Running generate_europe_maps.py..."
    python3 generate_europe_maps.py

    if [ $? -eq 0 ]; then
        echo "✓ Country maps generated"
    else
        echo "✗ Error generating country maps"
        exit 1
    fi
else
    echo "✗ ERROR: generate_europe_maps.py not found!"
    exit 1
fi

echo ""

# Step 3: Calculate adjacencies for individual maps
echo "------------------------------------------------------------"
echo "Step 3: Calculating province adjacencies"
echo "------------------------------------------------------------"
echo ""

if [ -f "calculate_adjacencies.py" ]; then
    echo "Running calculate_adjacencies.py..."
    python3 calculate_adjacencies.py

    if [ $? -eq 0 ]; then
        echo "✓ Adjacencies calculated"
    else
        echo "✗ Error calculating adjacencies"
        exit 1
    fi
else
    echo "⚠ calculate_adjacencies.py not found, skipping"
fi

echo ""

# Step 4: Combine into Europe map with cross-border adjacencies
echo "------------------------------------------------------------"
echo "Step 4: Combining into Europe map"
echo "------------------------------------------------------------"
echo ""

if [ -f "update_combined_europe_with_adjacencies.py" ]; then
    echo "Running update_combined_europe_with_adjacencies.py..."
    python3 update_combined_europe_with_adjacencies.py

    if [ $? -eq 0 ]; then
        echo "✓ Combined map created"
    else
        echo "✗ Error creating combined map"
        exit 1
    fi
elif [ -f "update_combined_europe.py" ]; then
    echo "⚠ Using basic combiner (no cross-border adjacencies)"
    python3 update_combined_europe.py
else
    echo "✗ ERROR: No map combiner script found!"
    exit 1
fi

echo ""

# Verification
echo "------------------------------------------------------------"
echo "Verification"
echo "------------------------------------------------------------"
echo ""

COMBINED_MAP="maps/map_europe_combined.json"

if [ -f "$COMBINED_MAP" ]; then
    echo "✓ Combined map exists: $COMBINED_MAP"

    # Check file size
    SIZE=$(stat -f%z "$COMBINED_MAP" 2>/dev/null || stat -c%s "$COMBINED_MAP" 2>/dev/null || echo "unknown")
    echo "  File size: $SIZE bytes"

    # Count provinces (requires jq)
    if command -v jq &> /dev/null; then
        PROVINCE_COUNT=$(jq '.map_region.provinces | length' "$COMBINED_MAP" 2>/dev/null || echo "unknown")
        echo "  Province count: $PROVINCE_COUNT"

        # Count total adjacencies
        ADJ_COUNT=$(jq '[.map_region.provinces[].neighbors | length] | add' "$COMBINED_MAP" 2>/dev/null || echo "unknown")
        echo "  Total adjacencies: $ADJ_COUNT"

        # Average neighbors
        if [ "$PROVINCE_COUNT" != "unknown" ] && [ "$ADJ_COUNT" != "unknown" ]; then
            AVG=$(echo "scale=1; $ADJ_COUNT / $PROVINCE_COUNT" | bc 2>/dev/null || echo "unknown")
            echo "  Average neighbors: $AVG"
        fi
    else
        echo "  (Install 'jq' for detailed statistics)"
    fi
else
    echo "✗ ERROR: Combined map not found!"
    exit 1
fi

echo ""
echo "============================================================"
echo "Map Regeneration Complete!"
echo "============================================================"
echo ""
echo "Next steps:"
echo "  1. Test in game: ./build/apps/main"
echo "  2. Check rendering and adjacencies"
echo "  3. Report any issues"
echo ""
