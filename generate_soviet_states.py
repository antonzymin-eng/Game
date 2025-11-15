#!/usr/bin/env python3
"""
Generate map files for former Soviet states in Europe.
Since NUTS1 data doesn't cover these, we'll create basic regional maps.
"""

import json
from pathlib import Path
from typing import Dict, List

# Major regions for former Soviet states
# Using simplified regional divisions
FORMER_SOVIET_REGIONS = {
    'ukraine': {
        'country_name': 'Ukraine',
        'capital': 'kyiv',
        'regions': [
            {'name': 'Kyiv and Central Ukraine', 'center_lat': 50.45, 'center_lon': 30.52},
            {'name': 'Western Ukraine', 'center_lat': 49.84, 'center_lon': 24.03},
            {'name': 'Southern Ukraine', 'center_lat': 46.48, 'center_lon': 30.73},
            {'name': 'Eastern Ukraine', 'center_lat': 48.57, 'center_lon': 37.55},
            {'name': 'Northern Ukraine', 'center_lat': 51.49, 'center_lon': 31.28},
        ]
    },
    'belarus': {
        'country_name': 'Belarus',
        'capital': 'minsk',
        'regions': [
            {'name': 'Minsk Region', 'center_lat': 53.90, 'center_lon': 27.56},
            {'name': 'Brest Region', 'center_lat': 52.09, 'center_lon': 25.33},
            {'name': 'Gomel Region', 'center_lat': 52.43, 'center_lon': 31.01},
            {'name': 'Grodno Region', 'center_lat': 53.68, 'center_lon': 25.32},
            {'name': 'Mogilev Region', 'center_lat': 53.90, 'center_lon': 30.33},
            {'name': 'Vitebsk Region', 'center_lat': 55.19, 'center_lon': 28.87},
        ]
    },
    'moldova': {
        'country_name': 'Moldova',
        'capital': 'chisinau',
        'regions': [
            {'name': 'Central Moldova', 'center_lat': 47.01, 'center_lon': 28.86},
            {'name': 'Northern Moldova', 'center_lat': 48.27, 'center_lon': 27.55},
            {'name': 'Southern Moldova', 'center_lat': 46.30, 'center_lon': 28.47},
        ]
    },
    'russia_european': {
        'country_name': 'European Russia',
        'capital': 'moscow',
        'regions': [
            {'name': 'Central Federal District', 'center_lat': 55.75, 'center_lon': 37.62},
            {'name': 'Northwestern Federal District', 'center_lat': 59.93, 'center_lon': 30.36},
            {'name': 'Southern Federal District', 'center_lat': 47.23, 'center_lon': 39.72},
            {'name': 'North Caucasian Federal District', 'center_lat': 44.04, 'center_lon': 43.06},
            {'name': 'Volga Federal District', 'center_lat': 55.79, 'center_lon': 49.12},
        ]
    }
}

def convert_coordinates(lon: float, lat: float) -> Dict[str, float]:
    """Convert lat/lon to game coordinates using the same system as NUTS1."""
    center_lon = 10.0
    center_lat = 50.0
    scale = 50.0

    x = (lon - center_lon) * scale
    y = (lat - center_lat) * scale

    return {"x": round(x, 2), "y": round(y, 2)}

def create_approximate_boundary(center_lat: float, center_lon: float, size: float = 2.0) -> List[Dict[str, float]]:
    """Create an approximate rectangular boundary around a center point."""
    # Create a simple rectangle
    points = []
    offsets = [
        (-size, -size),
        (size, -size),
        (size, size),
        (-size, size),
        (-size, -size)  # Close the polygon
    ]

    for lon_offset, lat_offset in offsets:
        coords = convert_coordinates(center_lon + lon_offset, center_lat + lat_offset)
        points.append(coords)

    return points

def create_province(region_id: int, region_data: Dict, country_name: str) -> Dict:
    """Create a province data structure."""
    center = convert_coordinates(region_data['center_lon'], region_data['center_lat'])
    boundary = create_approximate_boundary(region_data['center_lat'], region_data['center_lon'])

    return {
        "id": region_id,
        "name": region_data['name'],
        "owner_realm": country_name,
        "terrain_type": "plains",
        "center": center,
        "base_tax": 5,
        "base_production": 5,
        "base_manpower": 5,
        "development": 15,
        "boundary": boundary,
        "features": [],
        "trade_goods": "grain",
        "culture": country_name,
        "religion": "orthodox",
        "climate": "continental"
    }

def calculate_bounds(provinces: List[Dict]) -> Dict[str, float]:
    """Calculate bounding box for all provinces."""
    if not provinces:
        return {"min_x": 0, "max_x": 0, "min_y": 0, "max_y": 0}

    all_x = []
    all_y = []

    for province in provinces:
        for point in province['boundary']:
            all_x.append(point['x'])
            all_y.append(point['y'])

    return {
        "min_x": round(min(all_x), 2),
        "max_x": round(max(all_x), 2),
        "min_y": round(min(all_y), 2),
        "max_y": round(max(all_y), 2)
    }

def generate_country_map(country_id: str, country_data: Dict, output_dir: Path):
    """Generate a map file for a country."""
    provinces = []
    province_id = 100

    for region in country_data['regions']:
        province = create_province(province_id, region, country_id)
        provinces.append(province)
        province_id += 1

    if not provinces:
        print(f"No provinces found for {country_id}")
        return

    # Calculate bounds
    bounds = calculate_bounds(provinces)

    # Create map data structure
    map_data = {
        "map_region": {
            "id": f"{country_id}_real",
            "name": f"{country_data['country_name']} Real",
            "description": f"Real geographic boundaries for {country_id}",
            "coordinate_system": "cartesian_2d",
            "unit": "game_units",
            "bounds": bounds,
            "provinces": provinces,
            "sea_zones": [],
            "trade_nodes": []
        }
    }

    # Write to file
    output_file = output_dir / f"map_{country_id}_real.json"
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(map_data, f, indent=2, ensure_ascii=False)

    print(f"Created {output_file.name} with {len(provinces)} provinces")

def main():
    """Main function to generate all former Soviet state maps."""
    script_dir = Path(__file__).parent
    data_dir = script_dir / 'data' / 'maps'

    print("Generating maps for former Soviet states in Europe...\n")

    for country_id, country_data in FORMER_SOVIET_REGIONS.items():
        generate_country_map(country_id, country_data, data_dir)

    print("\nDone!")

if __name__ == '__main__':
    main()
