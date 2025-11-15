#!/usr/bin/env python3
"""
Generate regional grouping map files by combining individual country maps.
"""

import json
import os
from pathlib import Path
from typing import Dict, List

# Regional groupings
REGIONS = {
    'northern_europe': {
        'name': 'Northern Europe',
        'description': 'Scandinavia and Iceland',
        'countries': ['norway', 'sweden', 'finland', 'denmark', 'iceland']
    },
    'eastern_europe': {
        'name': 'Eastern Europe',
        'description': 'Poland, Czech Republic, Slovakia, Hungary, Romania, and Bulgaria',
        'countries': ['poland', 'czech_republic', 'slovakia', 'hungary', 'romania', 'bulgaria']
    },
    'baltic_states': {
        'name': 'Baltic States',
        'description': 'Estonia, Latvia, and Lithuania',
        'countries': ['estonia', 'latvia', 'lithuania']
    },
    'balkans': {
        'name': 'Balkans',
        'description': 'Greece and Western Balkans',
        'countries': ['greece', 'albania', 'north_macedonia', 'serbia', 'montenegro',
                     'bosnia_herzegovina', 'croatia', 'slovenia', 'kosovo']
    },
    'mediterranean_islands': {
        'name': 'Mediterranean Islands',
        'description': 'Cyprus and Malta',
        'countries': ['cyprus', 'malta']
    }
}

def load_country_map(country_name: str, maps_dir: Path) -> Dict:
    """Load a country map file."""
    map_file = maps_dir / f"map_{country_name}_real.json"
    if not map_file.exists():
        print(f"Warning: {map_file} not found")
        return None

    with open(map_file, 'r', encoding='utf-8') as f:
        return json.load(f)

def calculate_combined_bounds(all_provinces: List[Dict]) -> Dict[str, float]:
    """Calculate bounding box for all provinces."""
    if not all_provinces:
        return {"min_x": 0, "max_x": 0, "min_y": 0, "max_y": 0}

    all_x = []
    all_y = []

    for province in all_provinces:
        for point in province['boundary']:
            all_x.append(point['x'])
            all_y.append(point['y'])

    return {
        "min_x": round(min(all_x), 2),
        "max_x": round(max(all_x), 2),
        "min_y": round(min(all_y), 2),
        "max_y": round(max(all_y), 2)
    }

def generate_regional_map(region_id: str, region_info: Dict, maps_dir: Path):
    """Generate a regional map file by combining country maps."""
    print(f"\nGenerating {region_id}...")

    all_provinces = []
    province_id = 100

    for country_name in region_info['countries']:
        country_map = load_country_map(country_name, maps_dir)
        if not country_map:
            continue

        # Extract provinces and renumber them
        for province in country_map['map_region']['provinces']:
            province['id'] = province_id
            all_provinces.append(province)
            province_id += 1
            print(f"  Added {province['name']} from {country_name}")

    if not all_provinces:
        print(f"No provinces found for {region_id}")
        return

    # Calculate combined bounds
    bounds = calculate_combined_bounds(all_provinces)

    # Create regional map data
    map_data = {
        "map_region": {
            "id": region_id,
            "name": region_info['name'],
            "description": region_info['description'],
            "coordinate_system": "cartesian_2d",
            "unit": "game_units",
            "bounds": bounds,
            "provinces": all_provinces,
            "sea_zones": [],
            "trade_nodes": []
        }
    }

    # Write to file
    output_file = maps_dir / f"map_{region_id}.json"
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(map_data, f, indent=2, ensure_ascii=False)

    print(f"Created {output_file.name} with {len(all_provinces)} provinces")

def main():
    """Main function to generate all regional grouping files."""
    script_dir = Path(__file__).parent
    maps_dir = script_dir / 'data' / 'maps'

    print("Generating regional grouping files...\n")

    for region_id, region_info in REGIONS.items():
        generate_regional_map(region_id, region_info, maps_dir)

    print("\nDone!")

if __name__ == '__main__':
    main()
