#!/usr/bin/env python3
"""
Generate game map files from 11th century historical GeoJSON data.
Uses data from aourednik/historical-basemaps for year 1100 CE.
"""

import json
from pathlib import Path
from typing import Dict, List, Tuple

# European bounding box (roughly)
EUROPE_BOUNDS = {
    'min_lon': -25,
    'max_lon': 60,
    'min_lat': 35,
    'max_lat': 75
}

# Map historical entity names to game-friendly IDs
ENTITY_NAME_MAP = {
    'Kievan Rus': 'kievan_rus',
    'Holy Roman Empire': 'holy_roman_empire',
    'Kingdom of France': 'france',
    'England': 'england',
    'Scotland': 'scotland',
    'Denmark': 'denmark',
    'Norway': 'norway',
    'Sweden': 'sweden',
    'Poland': 'poland',
    'Hungary': 'hungary',
    'Byzantine Empire': 'byzantine_empire',
    'Castilla': 'castile',
    'León': 'leon',
    'Aragón': 'aragon',
    'Navarre': 'navarre',
    'Portugal': 'portugal',
    'Croatia': 'croatia',
    'Serbia': 'serbia',
    'Britany': 'brittany',
    'Burgandy': 'burgundy',
    'Venice': 'venice',
    'Papal States': 'papal_states',
    'Dutchy of Benevento': 'benevento',
    'Corsica': 'corsica',
    'Sardinia': 'sardinia',
    'Cyprus': 'cyprus',
    'Icelandic Commonwealth': 'iceland',
    'Principality of Polotsk': 'polotsk',
    'Bulgargarians': 'bulgars',
    'Bulgar Khanate': 'volga_bulgars',
    'Volga Bulgars': 'volga_bulgars',
    'Celtic kingdoms': 'celtic_kingdoms',
    'Prussians': 'prussians',
    'Kingdom of Georgia': 'georgia',
    'Armenia': 'armenia',
    'Almoravid dynasty': 'almoravids',
    'Seljuk Empire': 'seljuks',
    'Finno-Ugric taiga hunter-gatherers': 'finno_ugric_tribes',
    'Sámi': 'sami',
    'Cuman-Kipchak confederation': 'cumans'
}

def convert_coordinates(lon: float, lat: float) -> Dict[str, float]:
    """
    Convert lat/lon to game coordinates using equirectangular projection.
    Same system as modern maps.
    """
    center_lon = 10.0
    center_lat = 50.0
    scale = 50.0

    x = (lon - center_lon) * scale
    y = (lat - center_lat) * scale

    return {"x": round(x, 2), "y": round(y, 2)}

def is_in_europe(geometry: Dict) -> bool:
    """Check if geometry is in European bounds."""
    if not geometry or 'coordinates' not in geometry:
        return False

    try:
        coords = geometry['coordinates']
        geom_type = geometry['type']

        # Get a sample coordinate based on geometry type
        if geom_type == 'Polygon':
            # Polygon: coords[0][0] is [lon, lat]
            lon, lat = coords[0][0]
        elif geom_type == 'MultiPolygon':
            # MultiPolygon: coords[0][0][0] is [lon, lat]
            lon, lat = coords[0][0][0]
        else:
            return False

        return (EUROPE_BOUNDS['min_lon'] <= lon <= EUROPE_BOUNDS['max_lon'] and
                EUROPE_BOUNDS['min_lat'] <= lat <= EUROPE_BOUNDS['max_lat'])
    except (IndexError, TypeError, KeyError):
        return False

def convert_geojson_polygon(coordinates: List) -> List[Dict[str, float]]:
    """Convert GeoJSON polygon coordinates to game format."""
    converted = []

    for lon, lat in coordinates:
        converted.append(convert_coordinates(lon, lat))

    return converted

def extract_main_polygon(geometry: Dict) -> List[Dict[str, float]]:
    """Extract the main polygon from GeoJSON geometry."""
    coords = geometry['coordinates']
    geom_type = geometry['type']

    if geom_type == 'Polygon':
        # Use the outer ring (first ring)
        return convert_geojson_polygon(coords[0])
    elif geom_type == 'MultiPolygon':
        # Use the largest polygon
        polygons = []
        for polygon in coords:
            converted = convert_geojson_polygon(polygon[0])
            polygons.append(converted)
        # Return the longest (likely the main territory)
        return max(polygons, key=len) if polygons else []

    return []

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

def determine_terrain(name: str, partof: str) -> str:
    """Determine terrain type based on entity name and location."""
    name_lower = name.lower()

    if any(x in name_lower for x in ['mountain', 'highland', 'alps']):
        return 'mountains'
    elif any(x in name_lower for x in ['forest', 'taiga']):
        return 'forest'
    elif any(x in name_lower for x in ['coast', 'island', 'maritime']):
        return 'coast'
    elif any(x in name_lower for x in ['steppe', 'plain', 'kipchak', 'cuman']):
        return 'plains'
    else:
        return 'plains'

def determine_religion(name: str, partof: str) -> str:
    """Determine religion based on entity."""
    name_lower = name.lower()

    if any(x in name_lower for x in ['byzantine', 'rus', 'kiev', 'serbia', 'bulgaria', 'georgia']):
        return 'orthodox'
    elif any(x in name_lower for x in ['almoravid', 'seljuk', 'emirate']):
        return 'sunni'
    elif any(x in name_lower for x in ['papal', 'holy roman']):
        return 'catholic'
    elif 'pagan' in name_lower or 'prussian' in name_lower or 'sami' in name_lower:
        return 'pagan'
    else:
        # Default for Western/Central Europe
        return 'catholic'

def create_province(province_id: int, feature: Dict, realm_id: str) -> Dict:
    """Create a province from a GeoJSON feature."""
    props = feature['properties']
    name = props.get('NAME', 'Unknown Territory')
    partof = props.get('PARTOF', '')

    boundary = extract_main_polygon(feature['geometry'])
    if not boundary:
        return None

    center = calculate_center(boundary)
    terrain = determine_terrain(name, partof)
    religion = determine_religion(name, partof)

    return {
        "id": province_id,
        "name": name,
        "owner_realm": realm_id,
        "terrain_type": terrain,
        "center": center,
        "base_tax": 4,
        "base_production": 4,
        "base_manpower": 4,
        "development": 12,
        "boundary": boundary,
        "features": [],
        "trade_goods": "grain",
        "culture": realm_id,
        "religion": religion,
        "climate": "temperate",
        "historical_year": 1100
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

def generate_entity_maps(output_dir: Path):
    """Generate map files for all 11th century European entities."""
    # Load historical data
    historical_file = Path('data/maps/geojson_source/historical/world_1100.geojson')

    print(f"Loading {historical_file}...")
    with open(historical_file, 'r', encoding='utf-8') as f:
        geojson_data = json.load(f)

    # Group features by entity
    entities = {}

    for feature in geojson_data['features']:
        if not feature.get('geometry'):
            continue

        # Check if in Europe
        if not is_in_europe(feature['geometry']):
            continue

        props = feature['properties']
        name = props.get('NAME')

        if not name or name == 'Unknown':
            continue

        # Get or create entity ID
        entity_id = ENTITY_NAME_MAP.get(name, name.lower().replace(' ', '_').replace('-', '_'))

        if entity_id not in entities:
            entities[entity_id] = {
                'name': name,
                'features': []
            }

        entities[entity_id]['features'].append(feature)

    print(f"\nFound {len(entities)} European entities in year 1100")
    print(f"\nGenerating map files...\n")

    # Generate map for each entity
    for entity_id, entity_data in sorted(entities.items()):
        provinces = []
        province_id = 100

        for feature in entity_data['features']:
            province = create_province(province_id, feature, entity_id)
            if province:
                provinces.append(province)
                province_id += 1

        if not provinces:
            continue

        # Calculate bounds
        bounds = calculate_bounds(provinces)

        # Create map data
        map_data = {
            "map_region": {
                "id": f"{entity_id}_1100",
                "name": f"{entity_data['name']} (1100 CE)",
                "description": f"Historical borders of {entity_data['name']} in year 1100 CE",
                "coordinate_system": "cartesian_2d",
                "unit": "game_units",
                "bounds": bounds,
                "provinces": provinces,
                "sea_zones": [],
                "trade_nodes": [],
                "historical_year": 1100
            }
        }

        # Write to file
        output_file = output_dir / f"map_{entity_id}_1100.json"
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(map_data, f, indent=2, ensure_ascii=False)

        print(f"  Created {output_file.name} with {len(provinces)} provinces")

    return entities

def generate_combined_historical_map(entities: Dict, output_dir: Path):
    """Generate a combined map of all 11th century Europe."""
    print(f"\n\nGenerating combined historical Europe map...\n")

    historical_file = Path('data/maps/geojson_source/historical/world_1100.geojson')

    with open(historical_file, 'r', encoding='utf-8') as f:
        geojson_data = json.load(f)

    all_provinces = []
    province_id = 100

    for entity_id, entity_data in sorted(entities.items()):
        for feature in entity_data['features']:
            province = create_province(province_id, feature, entity_id)
            if province:
                all_provinces.append(province)
                province_id += 1

    if not all_provinces:
        print("No provinces found!")
        return

    # Calculate bounds
    bounds = calculate_bounds(all_provinces)

    # Create combined map
    map_data = {
        "map_region": {
            "id": "europe_1100",
            "name": "Europe 1100 CE",
            "description": f"Historical map of Europe in year 1100 CE with {len(entities)} polities and {len(all_provinces)} provinces",
            "coordinate_system": "cartesian_2d",
            "unit": "game_units",
            "bounds": bounds,
            "provinces": all_provinces,
            "sea_zones": [],
            "trade_nodes": [],
            "historical_year": 1100
        }
    }

    # Write combined map
    output_file = output_dir / "map_europe_1100.json"
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(map_data, f, indent=2, ensure_ascii=False)

    print(f"Created {output_file.name} with {len(all_provinces)} provinces from {len(entities)} polities")

def main():
    """Main function to generate all historical maps."""
    script_dir = Path(__file__).parent
    output_dir = script_dir / 'data' / 'maps' / 'historical_1100'
    output_dir.mkdir(exist_ok=True)

    print("="*70)
    print("GENERATING 11TH CENTURY HISTORICAL MAPS (Year 1100 CE)")
    print("="*70)
    print(f"\nOutput directory: {output_dir}\n")

    # Generate individual entity maps
    entities = generate_entity_maps(output_dir)

    # Generate combined map
    generate_combined_historical_map(entities, output_dir)

    print("\n" + "="*70)
    print("HISTORICAL MAP GENERATION COMPLETE!")
    print("="*70)

    return entities

if __name__ == '__main__':
    entities = main()
