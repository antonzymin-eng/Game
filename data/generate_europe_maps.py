#!/usr/bin/env python3
"""
Generate map files for European regions from NUTS1 GeoJSON data.
"""

import json
import os
from pathlib import Path
from typing import Dict, List, Tuple

# Country code to country name mapping
COUNTRY_NAMES = {
    'AL': 'albania',
    'AT': 'austria',
    'BA': 'bosnia_herzegovina',
    'BE': 'belgium',
    'BG': 'bulgaria',
    'CH': 'switzerland',
    'CY': 'cyprus',
    'CZ': 'czech_republic',
    'DE': 'germany',
    'DK': 'denmark',
    'EE': 'estonia',
    'EL': 'greece',
    'ES': 'spain',
    'FI': 'finland',
    'FR': 'france',
    'HR': 'croatia',
    'HU': 'hungary',
    'IE': 'ireland',
    'IS': 'iceland',
    'IT': 'italy',
    'LI': 'liechtenstein',
    'LT': 'lithuania',
    'LU': 'luxembourg',
    'LV': 'latvia',
    'ME': 'montenegro',
    'MK': 'north_macedonia',
    'MT': 'malta',
    'NL': 'netherlands',
    'NO': 'norway',
    'PL': 'poland',
    'PT': 'portugal',
    'RO': 'romania',
    'RS': 'serbia',
    'SE': 'sweden',
    'SI': 'slovenia',
    'SK': 'slovakia',
    'UK': 'united_kingdom',
    'XK': 'kosovo'
}

# Countries that already have map files (skip these)
EXISTING_COUNTRIES = {'austria', 'belgium', 'france', 'germany', 'ireland', 'italy',
                      'luxembourg', 'netherlands', 'portugal', 'spain', 'switzerland'}

def convert_coordinates(lon: float, lat: float) -> Dict[str, float]:
    """
    Convert lat/lon to game coordinates.
    Uses a simple equirectangular projection centered on Europe.
    """
    # Center point approximately in central Europe (10°E, 50°N)
    center_lon = 10.0
    center_lat = 50.0

    # Scale factor to convert degrees to game units
    # Adjust this to match the scale of existing maps
    scale = 50.0

    x = (lon - center_lon) * scale
    y = (lat - center_lat) * scale

    return {"x": round(x, 2), "y": round(y, 2)}

def extract_polygon_coords(geometry) -> List[List[Dict[str, float]]]:
    """Extract and convert polygon coordinates from GeoJSON geometry."""
    polygons = []

    if geometry['type'] == 'Polygon':
        # Single polygon
        for ring in geometry['coordinates']:
            converted_ring = [convert_coordinates(lon, lat) for lon, lat in ring]
            polygons.append(converted_ring)
    elif geometry['type'] == 'MultiPolygon':
        # Multiple polygons
        for polygon in geometry['coordinates']:
            for ring in polygon:
                converted_ring = [convert_coordinates(lon, lat) for lon, lat in ring]
                polygons.append(converted_ring)

    return polygons

def calculate_center(boundary: List[Dict[str, float]]) -> Dict[str, float]:
    """Calculate the centroid of a polygon."""
    if not boundary:
        return {"x": 0.0, "y": 0.0}

    x_sum = sum(point['x'] for point in boundary)
    y_sum = sum(point['y'] for point in boundary)
    n = len(boundary)

    return {
        "x": round(x_sum / n, 2),
        "y": round(y_sum / n, 2)
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

def create_province(region_id: int, region_name: str, boundary: List[Dict[str, float]],
                   country_name: str) -> Dict:
    """Create a province data structure."""
    center = calculate_center(boundary)

    # Default values - these should be customized per region
    return {
        "id": region_id,
        "name": region_name,
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
        "religion": "catholic",
        "climate": "temperate"
    }

def generate_map_file(country_code: str, regions: List[Dict], output_dir: Path):
    """Generate a map file for a country."""
    country_name = COUNTRY_NAMES.get(country_code, country_code.lower())

    # Skip if already exists
    if country_name in EXISTING_COUNTRIES:
        print(f"Skipping {country_name} (already exists)")
        return

    provinces = []
    province_id = 100

    for region in regions:
        region_name = region['properties']['na']
        geometry = region['geometry']

        # Extract polygons (use the largest one as the main boundary)
        polygons = extract_polygon_coords(geometry)
        if polygons:
            # Use the largest polygon
            main_polygon = max(polygons, key=len)

            province = create_province(province_id, region_name, main_polygon, country_name)
            provinces.append(province)
            province_id += 1

    if not provinces:
        print(f"No provinces found for {country_name}")
        return

    # Calculate bounds
    bounds = calculate_bounds(provinces)

    # Create map data structure
    map_data = {
        "map_region": {
            "id": f"{country_name}_real",
            "name": f"{country_name.replace('_', ' ').title()} Real",
            "description": f"Real geographic boundaries for {country_name}",
            "coordinate_system": "cartesian_2d",
            "unit": "game_units",
            "bounds": bounds,
            "provinces": provinces,
            "sea_zones": [],
            "trade_nodes": []
        }
    }

    # Write to file
    output_file = output_dir / f"map_{country_name}_real.json"
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(map_data, f, indent=2, ensure_ascii=False)

    print(f"Created {output_file.name} with {len(provinces)} provinces")

def main():
    """Main function to generate all map files."""
    # Paths
    script_dir = Path(__file__).parent
    data_dir = script_dir / 'maps'
    geojson_dir = data_dir / 'geojson_source'
    nuts1_file = geojson_dir / 'europe_nuts1_2024.json'

    # Load NUTS1 data
    print(f"Loading {nuts1_file}")
    with open(nuts1_file, 'r', encoding='utf-8') as f:
        nuts1_data = json.load(f)

    # Group regions by country
    countries = {}
    for feature in nuts1_data['features']:
        region_id = feature['properties']['id']
        country_code = region_id[:2]

        if country_code not in countries:
            countries[country_code] = []
        countries[country_code].append(feature)

    # Generate map files for each country
    print(f"\nFound {len(countries)} countries")
    print(f"Generating map files...\n")

    for country_code in sorted(countries.keys()):
        generate_map_file(country_code, countries[country_code], data_dir)

    print("\nDone!")

if __name__ == '__main__':
    main()
