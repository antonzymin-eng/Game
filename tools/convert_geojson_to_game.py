#!/usr/bin/env python3
"""
Convert real GeoJSON (lat/lon) to game coordinate format
Based on the coordinate conversion in include/map/loaders/GeoJSONLoader.h
"""

import json
import sys

# Coordinate conversion settings from GeoJSONLoader.h
EUROPE_MIN_LAT = 35.0
EUROPE_MAX_LAT = 72.0
EUROPE_MIN_LON = -15.0
EUROPE_MAX_LON = 45.0
GAME_WORLD_SIZE = 1000.0  # -500 to +500

def lat_lon_to_game(latitude, longitude):
    """Convert lat/lon to game coordinates"""
    # Normalize to 0-1 range
    lat_norm = (latitude - EUROPE_MIN_LAT) / (EUROPE_MAX_LAT - EUROPE_MIN_LAT)
    lon_norm = (longitude - EUROPE_MIN_LON) / (EUROPE_MAX_LON - EUROPE_MIN_LON)

    # Convert to game coordinates (-500 to +500)
    game_x = (lon_norm * GAME_WORLD_SIZE) - (GAME_WORLD_SIZE / 2)
    game_y = (GAME_WORLD_SIZE / 2) - (lat_norm * GAME_WORLD_SIZE)  # Flip Y axis

    return {"x": round(game_x, 2), "y": round(game_y, 2)}

def simplify_polygon(coordinates, max_points=100):
    """Simplify polygon to reduce points (basic decimation)"""
    if len(coordinates) <= max_points:
        return coordinates

    step = len(coordinates) // max_points
    simplified = [coordinates[i] for i in range(0, len(coordinates), step)]
    # Always keep first and last point
    if simplified[-1] != coordinates[-1]:
        simplified.append(coordinates[-1])

    return simplified

def calculate_center(boundary):
    """Calculate center point of polygon"""
    if not boundary:
        return {"x": 0, "y": 0}

    sum_x = sum(p["x"] for p in boundary)
    sum_y = sum(p["y"] for p in boundary)
    count = len(boundary)

    return {
        "x": round(sum_x / count, 2),
        "y": round(sum_y / count, 2)
    }

def convert_geojson_to_game(geojson_path, output_path, region_name="france", simplify=True):
    """Convert GeoJSON file to game format"""

    with open(geojson_path, 'r', encoding='utf-8') as f:
        geojson = json.load(f)

    provinces = []
    province_id = 100

    for feature in geojson['features']:
        properties = feature.get('properties', {})
        geometry = feature.get('geometry', {})

        # Extract coordinates (handle Polygon and MultiPolygon)
        coords = []
        if geometry['type'] == 'Polygon':
            coords = geometry['coordinates'][0]  # Outer ring
        elif geometry['type'] == 'MultiPolygon':
            # Use the largest polygon
            coords = max(geometry['coordinates'], key=lambda p: len(p[0]))[0]

        if not coords:
            continue

        # Convert lat/lon to game coordinates
        boundary = [lat_lon_to_game(lat, lon) for lon, lat in coords]

        # Simplify polygon if needed
        if simplify and len(boundary) > 50:
            boundary = simplify_polygon(boundary, max_points=50)

        # Calculate center
        center = calculate_center(boundary)

        # Create province entry
        province = {
            "id": province_id,
            "name": properties.get('nom', f'Province {province_id}'),
            "owner_realm": "france",  # Default
            "terrain_type": "plains",  # Default, could be enhanced
            "center": center,
            "base_tax": 5,
            "base_production": 5,
            "base_manpower": 5,
            "development": 15,
            "boundary": boundary,
            "features": [],
            "trade_goods": "grain",
            "culture": "french",
            "religion": "catholic",
            "climate": "temperate"
        }

        provinces.append(province)
        province_id += 1

    # Calculate overall bounds
    all_x = [p['x'] for prov in provinces for p in prov['boundary']]
    all_y = [p['y'] for prov in provinces for p in prov['boundary']]

    bounds = {
        "min_x": round(min(all_x), 2),
        "max_x": round(max(all_x), 2),
        "min_y": round(min(all_y), 2),
        "max_y": round(max(all_y), 2)
    }

    # Create game format output
    output = {
        "map_region": {
            "id": region_name,
            "name": region_name.replace('_', ' ').title(),
            "description": f"Real geographic boundaries for {region_name}",
            "coordinate_system": "cartesian_2d",
            "unit": "game_units",
            "bounds": bounds,
            "provinces": provinces,
            "sea_zones": [],
            "trade_nodes": []
        }
    }

    with open(output_path, 'w', encoding='utf-8') as f:
        json.dump(output, f, indent=2, ensure_ascii=False)

    print(f"✓ Converted {len(provinces)} provinces from {geojson_path}")
    print(f"✓ Output written to {output_path}")
    print(f"✓ Bounds: X({bounds['min_x']}, {bounds['max_x']}), Y({bounds['min_y']}, {bounds['max_y']})")

    return output

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: convert_geojson_to_game.py <input.geojson> <output.json> [region_name]")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    region_name = sys.argv[3] if len(sys.argv) > 3 else "france"

    convert_geojson_to_game(input_file, output_file, region_name)
