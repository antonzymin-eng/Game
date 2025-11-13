#!/usr/bin/env python3
"""
Combine multiple country maps into a single combined Europe map
"""

import json
import sys
import glob

def combine_maps(map_files, output_file):
    """Combine multiple map JSON files into one"""

    all_provinces = []
    province_id = 100  # Start from 100
    min_x, min_y = float('inf'), float('inf')
    max_x, max_y = float('-inf'), float('-inf')

    for map_file in map_files:
        print(f"Loading {map_file}...")
        with open(map_file, 'r', encoding='utf-8') as f:
            data = json.load(f)

        map_region = data.get('map_region', {})
        provinces = map_region.get('provinces', [])

        # Renumber province IDs to avoid conflicts
        for province in provinces:
            province['id'] = province_id
            province_id += 1

            # Update bounds
            for point in province['boundary']:
                min_x = min(min_x, point['x'])
                max_x = max(max_x, point['x'])
                min_y = min(min_y, point['y'])
                max_y = max(max_y, point['y'])

            all_provinces.append(province)

        print(f"  Added {len(provinces)} provinces")

    # Create combined map
    combined_map = {
        "map_region": {
            "id": "europe_combined",
            "name": "Western Europe Combined",
            "description": "Combined map of multiple European countries with real geographic boundaries",
            "coordinate_system": "cartesian_2d",
            "unit": "game_units",
            "bounds": {
                "min_x": round(min_x, 2),
                "max_x": round(max_x, 2),
                "min_y": round(min_y, 2),
                "max_y": round(max_y, 2)
            },
            "provinces": all_provinces,
            "sea_zones": [],
            "trade_nodes": []
        }
    }

    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(combined_map, f, indent=2, ensure_ascii=False)

    print(f"\n✓ Combined {len(all_provinces)} total provinces")
    print(f"✓ Output written to {output_file}")
    print(f"✓ Bounds: X({min_x:.2f}, {max_x:.2f}), Y({min_y:.2f}, {max_y:.2f})")

    return combined_map

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: combine_maps.py <output_file> <map_file1> <map_file2> ...")
        print("  or: combine_maps.py <output_file> (will use all map_*_real.json files)")
        sys.exit(1)

    output_file = sys.argv[1]

    if len(sys.argv) > 2:
        map_files = sys.argv[2:]
    else:
        # Find all map_*_real.json files
        map_files = glob.glob('data/maps/map_*_real.json')
        map_files.sort()

    print(f"Combining {len(map_files)} map files...\n")
    combine_maps(map_files, output_file)
