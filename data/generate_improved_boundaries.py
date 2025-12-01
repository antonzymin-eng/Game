#!/usr/bin/env python3
"""
Generate improved polygon boundaries for Eastern European countries.
Uses approximate real geographic positions to create better-looking provinces.
"""

import json
import math
from pathlib import Path
from typing import List, Dict, Tuple

# Approximate center coordinates (longitude, latitude) for provinces
# These are rough approximations based on real geography
PROVINCE_CENTERS = {
    # Ukraine
    "ukraine": {
        "Kyiv and Central Ukraine": (30.5, 50.5),
        "Western Ukraine": (24.0, 49.0),
        "Southern Ukraine": (32.0, 47.0),
        "Eastern Ukraine": (37.0, 49.0),
        "Northern Ukraine": (31.0, 51.5),
    },
    # Belarus
    "belarus": {
        "Minsk Region": (27.5, 53.9),
        "Brest Region": (25.0, 52.1),
        "Gomel Region": (31.0, 52.4),
        "Grodno Region": (25.3, 53.7),
        "Mogilev Region": (30.3, 53.9),
        "Vitebsk Region": (28.8, 55.2),
    },
    # Moldova
    "moldova": {
        "Central Moldova": (28.8, 47.0),
        "Northern Moldova": (27.5, 48.0),
        "Southern Moldova": (28.8, 46.0),
    },
    # Russia (European part)
    "russia_european": {
        "Central Federal District": (38.0, 55.0),
        "Northwestern Federal District": (32.0, 60.0),
        "Southern Federal District": (42.0, 48.0),
        "North Caucasian Federal District": (43.0, 44.0),
        "Volga Federal District": (48.0, 54.0),
    }
}

# Province sizes (approximate width/height in degrees)
PROVINCE_SIZES = {
    "ukraine": (3.5, 2.5),
    "belarus": (2.5, 2.0),
    "moldova": (1.5, 1.5),
    "russia_european": (5.0, 4.0),
}

def convert_coordinates(lon: float, lat: float) -> Dict[str, float]:
    """Convert lat/lon to game coordinates matching existing map."""
    # Center point approximately in central Europe (10°E, 50°N)
    center_lon = 10.0
    center_lat = 50.0

    # Scale factor to convert degrees to game units
    scale = 50.0

    x = (lon - center_lon) * scale
    y = (lat - center_lat) * scale

    return {"x": round(x, 2), "y": round(y, 2)}

def generate_irregular_polygon(center_lon: float, center_lat: float,
                               width: float, height: float,
                               num_points: int = 40) -> List[Dict[str, float]]:
    """
    Generate an irregular polygon around a center point.
    Creates a more natural-looking shape than a perfect rectangle.
    """
    points = []

    for i in range(num_points):
        angle = (2 * math.pi * i) / num_points

        # Add some randomness to radius for irregular shape
        # Use deterministic "random" based on angle for consistency
        variation = 0.85 + 0.3 * math.sin(angle * 3.7) * math.cos(angle * 2.3)

        # Calculate point position
        radius_x = (width / 2) * variation
        radius_y = (height / 2) * variation

        lon = center_lon + radius_x * math.cos(angle)
        lat = center_lat + radius_y * math.sin(angle)

        point = convert_coordinates(lon, lat)
        points.append(point)

    # Close the polygon
    if points:
        points.append(points[0])

    return points

def calculate_center(boundary: List[Dict[str, float]]) -> Dict[str, float]:
    """Calculate the centroid of a polygon."""
    if not boundary:
        return {"x": 0.0, "y": 0.0}

    x_sum = sum(point['x'] for point in boundary)
    y_sum = sum(point['y'] for point in boundary)
    n = len(boundary) - 1  # Subtract 1 because last point duplicates first

    return {
        "x": round(x_sum / n, 2),
        "y": round(y_sum / n, 2)
    }

def generate_country_map(country_name: str, province_data: Dict, size: Tuple[float, float]) -> Dict:
    """Generate a complete map file for a country."""
    provinces = []
    province_id = 1

    for province_name, (center_lon, center_lat) in province_data.items():
        # Generate irregular polygon
        boundary = generate_irregular_polygon(
            center_lon, center_lat,
            size[0], size[1],
            num_points=40
        )

        # Calculate center
        center = calculate_center(boundary)

        # Create province data
        province = {
            "id": province_id,
            "name": province_name,
            "owner_realm": country_name,
            "terrain_type": "plains",
            "center": center,
            "boundary": boundary,
            "features": []
        }

        provinces.append(province)
        province_id += 1

    # Calculate bounding box
    all_x = []
    all_y = []
    for prov in provinces:
        for point in prov['boundary']:
            all_x.append(point['x'])
            all_y.append(point['y'])

    bounds = {
        "min_x": round(min(all_x), 2),
        "max_x": round(max(all_x), 2),
        "min_y": round(min(all_y), 2),
        "max_y": round(max(all_y), 2)
    }

    # Create map structure
    map_data = {
        "map_region": {
            "id": f"{country_name}_real",
            "name": country_name.replace('_', ' ').title(),
            "coordinate_system": "cartesian_2d",
            "unit": "game_units",
            "bounds": bounds,
            "provinces": provinces
        }
    }

    return map_data

def main():
    """Generate improved map files for Eastern European countries."""
    script_dir = Path(__file__).parent
    maps_dir = script_dir / 'maps'

    print("Generating improved boundary data for Eastern European countries...\n")

    for country_name, province_data in PROVINCE_CENTERS.items():
        size = PROVINCE_SIZES[country_name]

        print(f"Generating {country_name}...")
        print(f"  Provinces: {len(province_data)}")
        print(f"  Points per province: ~40")

        map_data = generate_country_map(country_name, province_data, size)

        # Write to file
        output_file = maps_dir / f"map_{country_name}_real.json"
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(map_data, f, indent=2, ensure_ascii=False)

        print(f"  ✓ Saved to {output_file.name}\n")

    print("✓ All country maps generated successfully!")
    print("\nNext step: Run update_combined_europe.py to merge into combined map")

if __name__ == '__main__':
    main()
