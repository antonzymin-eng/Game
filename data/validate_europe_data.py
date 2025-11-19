#!/usr/bin/env python3
"""
Validate all generated European map and nation data.
"""

import json
from pathlib import Path
from typing import Dict, List, Set

def validate_map_structure(map_file: Path) -> List[str]:
    """Validate a map file structure."""
    errors = []

    try:
        with open(map_file, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Check top-level structure
        if 'map_region' not in data:
            errors.append(f"{map_file.name}: Missing 'map_region' key")
            return errors

        map_region = data['map_region']

        # Required fields
        required_fields = ['id', 'name', 'description', 'coordinate_system', 'unit', 'bounds', 'provinces']
        for field in required_fields:
            if field not in map_region:
                errors.append(f"{map_file.name}: Missing required field '{field}'")

        # Check bounds
        if 'bounds' in map_region:
            bounds = map_region['bounds']
            required_bounds = ['min_x', 'max_x', 'min_y', 'max_y']
            for bound in required_bounds:
                if bound not in bounds:
                    errors.append(f"{map_file.name}: Missing bound '{bound}'")

        # Check provinces
        if 'provinces' in map_region:
            provinces = map_region['provinces']
            if not isinstance(provinces, list):
                errors.append(f"{map_file.name}: 'provinces' should be a list")
            elif len(provinces) == 0:
                errors.append(f"{map_file.name}: No provinces defined")
            else:
                # Validate each province
                province_ids = set()
                for i, province in enumerate(provinces):
                    # Check required fields
                    required_province_fields = ['id', 'name', 'owner_realm', 'terrain_type',
                                                'center', 'boundary']
                    for field in required_province_fields:
                        if field not in province:
                            errors.append(f"{map_file.name}: Province {i} missing '{field}'")

                    # Check for duplicate IDs
                    if 'id' in province:
                        if province['id'] in province_ids:
                            errors.append(f"{map_file.name}: Duplicate province ID {province['id']}")
                        province_ids.add(province['id'])

                    # Check center coordinates
                    if 'center' in province:
                        if not isinstance(province['center'], dict):
                            errors.append(f"{map_file.name}: Province {i} center not a dict")
                        elif 'x' not in province['center'] or 'y' not in province['center']:
                            errors.append(f"{map_file.name}: Province {i} center missing x or y")

                    # Check boundary
                    if 'boundary' in province:
                        if not isinstance(province['boundary'], list):
                            errors.append(f"{map_file.name}: Province {i} boundary not a list")
                        elif len(province['boundary']) < 3:
                            errors.append(f"{map_file.name}: Province {i} boundary has < 3 points")

    except json.JSONDecodeError as e:
        errors.append(f"{map_file.name}: JSON decode error - {str(e)}")
    except Exception as e:
        errors.append(f"{map_file.name}: Unexpected error - {str(e)}")

    return errors

def validate_nation_structure(nation_file: Path) -> List[str]:
    """Validate a nation file structure."""
    errors = []

    try:
        with open(nation_file, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Check top-level structure
        if 'nations' not in data:
            errors.append(f"{nation_file.name}: Missing 'nations' key")
            return errors

        nations = data['nations']
        if not isinstance(nations, list):
            errors.append(f"{nation_file.name}: 'nations' should be a list")
            return errors

        if len(nations) == 0:
            errors.append(f"{nation_file.name}: No nations defined")
            return errors

        # Validate each nation
        nation_ids = set()
        for i, nation in enumerate(nations):
            # Check required fields
            required_fields = ['id', 'name', 'adjective', 'culture_group', 'primary_culture',
                              'religion', 'government_type', 'capital', 'color']
            for field in required_fields:
                if field not in nation:
                    errors.append(f"{nation_file.name}: Nation {i} missing '{field}'")

            # Check for duplicate IDs
            if 'id' in nation:
                if nation['id'] in nation_ids:
                    errors.append(f"{nation_file.name}: Duplicate nation ID {nation['id']}")
                nation_ids.add(nation['id'])

            # Check color
            if 'color' in nation:
                if not isinstance(nation['color'], dict):
                    errors.append(f"{nation_file.name}: Nation {i} color not a dict")
                elif 'r' not in nation['color'] or 'g' not in nation['color'] or 'b' not in nation['color']:
                    errors.append(f"{nation_file.name}: Nation {i} color missing r, g, or b")

    except json.JSONDecodeError as e:
        errors.append(f"{nation_file.name}: JSON decode error - {str(e)}")
    except Exception as e:
        errors.append(f"{nation_file.name}: Unexpected error - {str(e)}")

    return errors

def count_files_and_provinces():
    """Count all map files and provinces."""
    maps_dir = Path('data/maps')
    nations_dir = Path('data/nations')

    # Count map files
    map_files = list(maps_dir.glob('map_*_real.json'))
    regional_files = list(maps_dir.glob('map_*.json'))
    regional_files = [f for f in regional_files if '_real.json' not in f.name and f.name != 'map_europe_combined.json']

    print(f"\n📊 File Statistics:")
    print(f"   Individual country maps: {len(map_files)}")
    print(f"   Regional grouping maps: {len(regional_files)}")

    # Count provinces in combined map
    combined_file = maps_dir / 'map_europe_combined.json'
    if combined_file.exists():
        with open(combined_file, 'r') as f:
            combined_data = json.load(f)
            province_count = len(combined_data['map_region']['provinces'])
            print(f"   Combined Europe provinces: {province_count}")

    # Count nation files and nations
    nation_files = list(nations_dir.glob('nations_*.json'))
    total_nations = 0
    for nf in nation_files:
        with open(nf, 'r') as f:
            data = json.load(f)
            total_nations += len(data['nations'])

    print(f"   Nation data files: {len(nation_files)}")
    print(f"   Total nations defined: {total_nations}")

def main():
    """Main validation function."""
    print("🔍 Validating European map and nation data...\n")

    maps_dir = Path('data/maps')
    nations_dir = Path('data/nations')

    all_errors = []

    # Validate all map files
    print("Validating map files...")
    map_files = sorted(maps_dir.glob('map_*.json'))
    valid_maps = 0
    for map_file in map_files:
        errors = validate_map_structure(map_file)
        if errors:
            all_errors.extend(errors)
        else:
            valid_maps += 1

    print(f"   ✓ {valid_maps}/{len(map_files)} map files valid")

    # Validate all nation files
    print("\nValidating nation files...")
    nation_files = sorted(nations_dir.glob('nations_*.json'))
    valid_nations = 0
    for nation_file in nation_files:
        errors = validate_nation_structure(nation_file)
        if errors:
            all_errors.extend(errors)
        else:
            valid_nations += 1

    print(f"   ✓ {valid_nations}/{len(nation_files)} nation files valid")

    # Count files and provinces
    count_files_and_provinces()

    # Report errors
    if all_errors:
        print(f"\n❌ Found {len(all_errors)} errors:\n")
        for error in all_errors:
            print(f"   • {error}")
        return 1
    else:
        print("\n✅ All validation checks passed!")
        print("\n🎉 European regions and geography data is complete and valid!")
        return 0

if __name__ == '__main__':
    exit(main())
