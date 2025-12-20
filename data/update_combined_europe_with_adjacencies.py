#!/usr/bin/env python3
"""
Update the combined Europe map with proper cross-border adjacencies.

This script:
1. Combines all country maps into one
2. Renumbers province IDs
3. Recalculates adjacencies including cross-border neighbors
"""

import json
import sys
from pathlib import Path
from typing import Dict, List

# Import adjacency calculation functions
sys.path.insert(0, str(Path(__file__).parent))
from calculate_adjacencies import (
    calculate_adaptive_tolerance,
    are_neighbors,
    calculate_border_length
)

# All European countries to include (west of Ural Mountains)
ALL_EUROPEAN_COUNTRIES = [
    # Western Europe
    'france', 'belgium', 'netherlands', 'luxembourg', 'ireland',
    # Iberia
    'spain', 'portugal',
    # Central Europe
    'germany', 'switzerland', 'austria', 'liechtenstein',
    # Italy
    'italy',
    # Northern Europe
    'norway', 'sweden', 'finland', 'denmark', 'iceland',
    # Eastern Europe
    'poland', 'czech_republic', 'slovakia', 'hungary', 'romania', 'bulgaria',
    # Baltic States
    'estonia', 'latvia', 'lithuania',
    # Former Soviet States
    'ukraine', 'belarus', 'moldova', 'russia_european',
    # Balkans
    'greece', 'albania', 'north_macedonia', 'serbia', 'montenegro',
    'bosnia_herzegovina', 'croatia', 'slovenia', 'kosovo',
    # Mediterranean Islands
    'cyprus', 'malta',
    # United Kingdom
    'united_kingdom',
]

def load_country_map(country_name: str, maps_dir: Path) -> Dict:
    """Load a country map file."""
    map_file = maps_dir / f"map_{country_name}_real.json"
    if not map_file.exists():
        print(f"  Warning: {map_file.name} not found, skipping...")
        return None

    try:
        with open(map_file, 'r', encoding='utf-8') as f:
            return json.load(f)
    except Exception as e:
        print(f"  Error loading {map_file.name}: {e}")
        return None

def calculate_combined_bounds(all_provinces: List[Dict]) -> Dict[str, float]:
    """Calculate bounding box for all provinces."""
    if not all_provinces:
        return {"min_x": 0, "max_x": 0, "min_y": 0, "max_y": 0}

    all_x = []
    all_y = []

    for province in all_provinces:
        for point in province.get('boundary', []):
            all_x.append(point['x'])
            all_y.append(point['y'])

    if not all_x:
        return {"min_x": 0, "max_x": 0, "min_y": 0, "max_y": 0}

    return {
        "min_x": round(min(all_x), 2),
        "max_x": round(max(all_x), 2),
        "min_y": round(min(all_y), 2),
        "max_y": round(max(all_y), 2)
    }

def recalculate_all_adjacencies(provinces: List[Dict], tolerance: float = None):
    """
    Recalculate all adjacencies for the combined map.
    This ensures cross-border adjacencies are properly calculated.
    """
    print("\nRecalculating adjacencies for combined map...")

    if tolerance is None:
        tolerance = calculate_adaptive_tolerance(provinces)

    # Clear existing neighbors and create ID mapping
    old_id_to_index = {}
    for i, province in enumerate(provinces):
        province['neighbors'] = []
        old_id_to_index[province['id']] = i

    total_adjacencies = 0
    total_comparisons = (len(provinces) * (len(provinces) - 1)) // 2
    comparisons_done = 0
    last_reported_percent = 0

    print(f"  Processing {len(provinces)} provinces...")
    print(f"  Total comparisons needed: {total_comparisons}")

    # Calculate adjacencies between all province pairs
    for i in range(len(provinces)):
        prov1 = provinces[i]
        boundary1 = prov1.get('boundary', [])

        if len(boundary1) < 3:
            continue

        for j in range(i + 1, len(provinces)):
            prov2 = provinces[j]
            boundary2 = prov2.get('boundary', [])

            if len(boundary2) < 3:
                continue

            # Quick neighbor check
            if not are_neighbors(boundary1, boundary2, tolerance):
                comparisons_done += 1
                continue

            # Calculate border length
            border_length = calculate_border_length(boundary1, boundary2, tolerance)

            # Only add if significant border
            if border_length > tolerance:
                prov1['neighbors'].append({
                    'id': prov2['id'],
                    'border_length': round(border_length, 2)
                })
                prov2['neighbors'].append({
                    'id': prov1['id'],
                    'border_length': round(border_length, 2)
                })
                total_adjacencies += 1

            comparisons_done += 1

            # Progress reporting
            if total_comparisons > 1000:
                percent = (comparisons_done * 100) // total_comparisons
                if percent >= last_reported_percent + 10:
                    print(f"  Progress: {percent}% ({comparisons_done}/{total_comparisons} comparisons, {total_adjacencies} adjacencies found)")
                    last_reported_percent = percent

    print(f"\n  ✓ Found {total_adjacencies} adjacencies")

    # Statistics
    neighbor_counts = [len(p.get('neighbors', [])) for p in provinces]
    if neighbor_counts:
        avg_neighbors = sum(neighbor_counts) / len(neighbor_counts)
        max_neighbors = max(neighbor_counts)
        isolated = sum(1 for c in neighbor_counts if c == 0)

        print(f"  ✓ Average neighbors per province: {avg_neighbors:.1f}")
        print(f"  ✓ Max neighbors: {max_neighbors}")

        if isolated > 0:
            print(f"  ⚠ WARNING: {isolated} isolated provinces (islands?)")

def main():
    """Main function to update the combined Europe map."""
    script_dir = Path(__file__).parent
    maps_dir = script_dir / 'maps'

    print("=" * 70)
    print("Update Combined Europe Map with Adjacencies")
    print("=" * 70)
    print()

    all_provinces = []
    province_id = 100
    countries_included = []

    print("Loading country maps...\n")

    for country_name in ALL_EUROPEAN_COUNTRIES:
        country_map = load_country_map(country_name, maps_dir)
        if not country_map:
            continue

        # Extract provinces and renumber them
        num_provinces = len(country_map['map_region'].get('provinces', []))
        for province in country_map['map_region']['provinces']:
            # Store old ID for reference
            province['_old_id'] = province['id']
            province['_country'] = country_name

            # Assign new global ID
            province['id'] = province_id
            all_provinces.append(province)
            province_id += 1

        countries_included.append(country_name)
        print(f"  ✓ Added {num_provinces} provinces from {country_name}")

    if not all_provinces:
        print("\nERROR: No provinces found!")
        return

    print(f"\nTotal provinces loaded: {len(all_provinces)}")

    # Calculate combined bounds
    print("\nCalculating map bounds...")
    bounds = calculate_combined_bounds(all_provinces)
    print(f"  Bounds: x=[{bounds['min_x']}, {bounds['max_x']}], y=[{bounds['min_y']}, {bounds['max_y']}]")

    # Recalculate all adjacencies (including cross-border)
    recalculate_all_adjacencies(all_provinces)

    # Clean up temporary fields
    for province in all_provinces:
        province.pop('_old_id', None)
        province.pop('_country', None)

    # Create combined map data
    map_data = {
        "map_region": {
            "id": "europe_combined",
            "name": "Europe Combined",
            "description": f"Combined map of all European countries west of Ural Mountains with real geographic boundaries. Includes {len(countries_included)} countries with {len(all_provinces)} provinces.",
            "coordinate_system": "cartesian_2d",
            "unit": "game_units",
            "bounds": bounds,
            "provinces": all_provinces,
            "sea_zones": [],
            "trade_nodes": []
        }
    }

    # Write to file
    output_file = maps_dir / "map_europe_combined.json"

    # Create backup
    if output_file.exists():
        backup_file = output_file.with_suffix('.json.backup')
        if not backup_file.exists():
            import shutil
            shutil.copy2(output_file, backup_file)
            print(f"\nCreated backup: {backup_file.name}")

    print(f"\nWriting combined map to {output_file.name}...")
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(map_data, f, indent=2, ensure_ascii=False)

    print()
    print("=" * 70)
    print("SUCCESS!")
    print("=" * 70)
    print(f"\nUpdated {output_file.name}")
    print(f"  {len(all_provinces)} provinces from {len(countries_included)} countries")
    print(f"  Countries: {', '.join(sorted(countries_included))}")
    print()

if __name__ == '__main__':
    main()
