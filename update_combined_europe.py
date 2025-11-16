#!/usr/bin/env python3
"""
Update the combined Europe map to include all European countries.
"""

import json
from pathlib import Path
from typing import Dict, List

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
    # Turkey (European part - if included)
    'tr'
]

def load_country_map(country_name: str, maps_dir: Path) -> Dict:
    """Load a country map file."""
    map_file = maps_dir / f"map_{country_name}_real.json"
    if not map_file.exists():
        print(f"Warning: {map_file} not found, skipping...")
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

def main():
    """Main function to update the combined Europe map."""
    script_dir = Path(__file__).parent
    maps_dir = script_dir / 'data' / 'maps'

    print("Updating combined Europe map...\n")

    all_provinces = []
    province_id = 100
    countries_included = []

    for country_name in ALL_EUROPEAN_COUNTRIES:
        country_map = load_country_map(country_name, maps_dir)
        if not country_map:
            continue

        # Extract provinces and renumber them
        num_provinces = len(country_map['map_region']['provinces'])
        for province in country_map['map_region']['provinces']:
            province['id'] = province_id
            all_provinces.append(province)
            province_id += 1

        countries_included.append(country_name)
        print(f"  Added {num_provinces} provinces from {country_name}")

    if not all_provinces:
        print("No provinces found!")
        return

    # Calculate combined bounds
    bounds = calculate_combined_bounds(all_provinces)

    # Create combined map data
    map_data = {
        "map_region": {
            "id": "europe_combined",
            "name": "Europe Combined",
            "description": f"Combined map of all European countries west of Ural Mountains with real geographic boundaries. Includes {len(countries_included)} countries.",
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
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(map_data, f, indent=2, ensure_ascii=False)

    print(f"\nUpdated {output_file.name} with {len(all_provinces)} provinces from {len(countries_included)} countries")
    print(f"Countries included: {', '.join(sorted(countries_included))}")

if __name__ == '__main__':
    main()
