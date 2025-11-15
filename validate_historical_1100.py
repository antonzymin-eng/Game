#!/usr/bin/env python3
"""
Comprehensive validation for 11th century historical data.
"""

import json
from pathlib import Path
from typing import Dict, List, Set

def validate_map_file(map_file: Path) -> List[str]:
    """Validate a historical map file."""
    errors = []

    try:
        with open(map_file, 'r', encoding='utf-8') as f:
            data = json.load(f)

        # Check structure
        if 'map_region' not in data:
            errors.append(f"{map_file.name}: Missing map_region")
            return errors

        mr = data['map_region']

        # Check required fields
        required = ['id', 'name', 'description', 'coordinate_system', 'unit', 'bounds', 'provinces']
        for field in required:
            if field not in mr:
                errors.append(f"{map_file.name}: Missing field '{field}'")

        # Check historical year marker
        if 'historical_year' not in mr:
            errors.append(f"{map_file.name}: Missing historical_year marker")
        elif mr['historical_year'] != 1100:
            errors.append(f"{map_file.name}: Incorrect historical_year {mr['historical_year']}")

        # Check ID format
        if 'id' in mr and not mr['id'].endswith('_1100'):
            errors.append(f"{map_file.name}: ID should end with '_1100'")

        # Check provinces
        if 'provinces' in mr:
            provinces = mr['provinces']
            if not provinces:
                errors.append(f"{map_file.name}: Empty provinces list")

            province_ids = set()
            for i, prov in enumerate(provinces):
                # Check required province fields
                req_prov = ['id', 'name', 'owner_realm', 'terrain_type', 'center', 'boundary', 'religion']
                for field in req_prov:
                    if field not in prov:
                        errors.append(f"{map_file.name}: Province {i} missing '{field}'")

                # Check for duplicate province IDs
                if 'id' in prov:
                    if prov['id'] in province_ids:
                        errors.append(f"{map_file.name}: Duplicate province ID {prov['id']}")
                    province_ids.add(prov['id'])

                # Check boundary
                if 'boundary' in prov:
                    if not isinstance(prov['boundary'], list):
                        errors.append(f"{map_file.name}: Province {i} boundary not a list")
                    elif len(prov['boundary']) < 3:
                        errors.append(f"{map_file.name}: Province {i} boundary has < 3 points")

                # Check historical year in province
                if 'historical_year' in prov and prov['historical_year'] != 1100:
                    errors.append(f"{map_file.name}: Province {i} wrong historical_year")

                # Validate religion values
                valid_religions = ['catholic', 'orthodox', 'sunni', 'pagan', 'shia']
                if 'religion' in prov and prov['religion'] not in valid_religions:
                    errors.append(f"{map_file.name}: Province {i} invalid religion '{prov['religion']}'")

        # Check bounds
        if 'bounds' in mr:
            bounds = mr['bounds']
            if not all(k in bounds for k in ['min_x', 'max_x', 'min_y', 'max_y']):
                errors.append(f"{map_file.name}: Incomplete bounds")
            elif bounds['min_x'] >= bounds['max_x'] or bounds['min_y'] >= bounds['max_y']:
                errors.append(f"{map_file.name}: Invalid bounds (min >= max)")

    except json.JSONDecodeError as e:
        errors.append(f"{map_file.name}: JSON error - {str(e)}")
    except Exception as e:
        errors.append(f"{map_file.name}: Error - {str(e)}")

    return errors

def validate_nation_file(nation_file: Path) -> List[str]:
    """Validate the 11th century nations file."""
    errors = []

    try:
        with open(nation_file, 'r', encoding='utf-8') as f:
            data = json.load(f)

        if 'nations' not in data:
            errors.append(f"{nation_file.name}: Missing 'nations' key")
            return errors

        nations = data['nations']
        nation_ids = set()

        for i, nation in enumerate(nations):
            # Check required fields
            required = ['id', 'name', 'adjective', 'culture_group', 'primary_culture',
                       'religion', 'government_type', 'capital', 'color', 'historical_info',
                       'national_ideas', 'starting_attributes']

            for field in required:
                if field not in nation:
                    errors.append(f"{nation_file.name}: Nation {i} missing '{field}'")

            # Check duplicate IDs
            if 'id' in nation:
                if nation['id'] in nation_ids:
                    errors.append(f"{nation_file.name}: Duplicate nation ID {nation['id']}")
                nation_ids.add(nation['id'])

            # Check color
            if 'color' in nation:
                color = nation['color']
                if not all(k in color for k in ['r', 'g', 'b']):
                    errors.append(f"{nation_file.name}: Nation {i} incomplete color")
                elif not all(0 <= color[k] <= 255 for k in ['r', 'g', 'b']):
                    errors.append(f"{nation_file.name}: Nation {i} invalid color values")

            # Check historical_info
            if 'historical_info' in nation:
                hist = nation['historical_info']
                if not all(k in hist for k in ['founded', 'notable_rulers', 'historical_notes']):
                    errors.append(f"{nation_file.name}: Nation {i} incomplete historical_info")

            # Check national_ideas structure
            if 'national_ideas' in nation:
                ideas = nation['national_ideas']
                if not all(k in ideas for k in ['traditions', 'ideas', 'ambition']):
                    errors.append(f"{nation_file.name}: Nation {i} incomplete national_ideas")

                # Check we have 7 ideas
                if 'ideas' in ideas and len(ideas['ideas']) != 7:
                    errors.append(f"{nation_file.name}: Nation {i} should have 7 ideas, has {len(ideas['ideas'])}")

    except json.JSONDecodeError as e:
        errors.append(f"{nation_file.name}: JSON error - {str(e)}")
    except Exception as e:
        errors.append(f"{nation_file.name}: Error - {str(e)}")

    return errors

def check_coordinate_consistency(maps_dir: Path) -> List[str]:
    """Check that coordinates are reasonable and consistent."""
    errors = []

    combined_file = maps_dir / 'map_europe_1100.json'
    if not combined_file.exists():
        errors.append("Combined map file not found")
        return errors

    try:
        with open(combined_file, 'r') as f:
            data = json.load(f)

        bounds = data['map_region']['bounds']
        provinces = data['map_region']['provinces']

        # Check that all province coordinates are within bounds
        for prov in provinces:
            for point in prov.get('boundary', []):
                x, y = point.get('x', 0), point.get('y', 0)

                if x < bounds['min_x'] or x > bounds['max_x']:
                    errors.append(f"Province {prov.get('name', 'unknown')} has x={x} outside bounds")
                if y < bounds['min_y'] or y > bounds['max_y']:
                    errors.append(f"Province {prov.get('name', 'unknown')} has y={y} outside bounds")

        # Check bounds are reasonable for historical Europe
        # Historical maps include polities extending beyond modern Europe:
        # - Cumans (Central Asian steppe)
        # - Seljuks (Anatolia, Persia)
        # - Almoravids (North Africa)
        # - Finno-Ugric tribes (Siberia)
        # So we allow larger ranges than modern Europe
        if bounds['max_x'] - bounds['min_x'] > 10000:
            errors.append(f"X range unreasonably large: {bounds['max_x'] - bounds['min_x']}")
        if bounds['max_y'] - bounds['min_y'] > 5000:
            errors.append(f"Y range unreasonably large: {bounds['max_y'] - bounds['min_y']}")

    except Exception as e:
        errors.append(f"Coordinate check error: {str(e)}")

    return errors

def check_data_consistency(maps_dir: Path, nation_file: Path) -> List[str]:
    """Check consistency between maps and nations."""
    errors = []

    try:
        # Get all polity IDs from maps
        map_polities = set()
        for map_file in maps_dir.glob('map_*_1100.json'):
            if map_file.name == 'map_europe_1100.json':
                continue

            with open(map_file, 'r') as f:
                data = json.load(f)
                map_id = data['map_region']['id'].replace('_1100', '')
                map_polities.add(map_id)

        # Get all nation IDs
        nation_ids = set()
        if nation_file.exists():
            with open(nation_file, 'r') as f:
                data = json.load(f)
                for nation in data['nations']:
                    nation_ids.add(nation['id'])

        # Major polities should have nation definitions
        major_polities = {'kievan_rus', 'holy_roman_empire', 'france', 'england',
                         'byzantine_empire', 'castile', 'poland', 'hungary'}

        missing_nations = major_polities - nation_ids
        if missing_nations:
            errors.append(f"Major polities missing nation definitions: {missing_nations}")

        # Nations should have corresponding maps
        missing_maps = nation_ids - map_polities
        if missing_maps:
            errors.append(f"Nations without corresponding maps: {missing_maps}")

    except Exception as e:
        errors.append(f"Consistency check error: {str(e)}")

    return errors

def validate_generation_script() -> List[str]:
    """Validate the generation script itself."""
    errors = []

    script_path = Path('generate_historical_1100.py')
    if not script_path.exists():
        errors.append("Generation script not found")
        return errors

    try:
        # Check syntax
        with open(script_path, 'r') as f:
            code = f.read()
        compile(code, str(script_path), 'exec')

        # Check for required functions
        required_functions = ['convert_coordinates', 'is_in_europe', 'extract_main_polygon',
                            'create_province', 'generate_entity_maps', 'main']

        for func in required_functions:
            if f'def {func}(' not in code:
                errors.append(f"Missing function: {func}")

        # Check for proper error handling
        if 'try:' not in code or 'except' not in code:
            errors.append("Script lacks error handling")

    except SyntaxError as e:
        errors.append(f"Script syntax error: {str(e)}")
    except Exception as e:
        errors.append(f"Script validation error: {str(e)}")

    return errors

def main():
    """Main validation function."""
    print("="*75)
    print(" VALIDATING 11TH CENTURY HISTORICAL DATA")
    print("="*75)
    print()

    all_errors = []

    # Validate generation script
    print("1. Validating generation script...")
    script_errors = validate_generation_script()
    if script_errors:
        all_errors.extend(script_errors)
        print(f"   ❌ {len(script_errors)} errors found")
    else:
        print("   ✓ Script valid")

    # Validate map files
    print("\n2. Validating map files...")
    maps_dir = Path('data/maps/historical_1100')

    if not maps_dir.exists():
        print("   ❌ Maps directory not found!")
        all_errors.append("Maps directory missing")
    else:
        map_files = list(maps_dir.glob('map_*_1100.json'))
        print(f"   Found {len(map_files)} map files")

        map_errors_count = 0
        for map_file in map_files:
            errors = validate_map_file(map_file)
            if errors:
                all_errors.extend(errors)
                map_errors_count += 1

        if map_errors_count > 0:
            print(f"   ❌ {map_errors_count} files with errors")
        else:
            print(f"   ✓ All {len(map_files)} map files valid")

    # Validate nation file
    print("\n3. Validating nation definitions...")
    nation_file = Path('data/nations/nations_11th_century.json')

    if not nation_file.exists():
        print("   ❌ Nation file not found!")
        all_errors.append("Nation file missing")
    else:
        nation_errors = validate_nation_file(nation_file)
        if nation_errors:
            all_errors.extend(nation_errors)
            print(f"   ❌ {len(nation_errors)} errors found")
        else:
            with open(nation_file, 'r') as f:
                data = json.load(f)
                print(f"   ✓ All {len(data['nations'])} nations valid")

    # Check coordinate consistency
    print("\n4. Checking coordinate consistency...")
    if maps_dir.exists():
        coord_errors = check_coordinate_consistency(maps_dir)
        if coord_errors:
            all_errors.extend(coord_errors)
            print(f"   ❌ {len(coord_errors)} coordinate issues")
        else:
            print("   ✓ Coordinates consistent")

    # Check data consistency
    print("\n5. Checking data consistency...")
    if maps_dir.exists() and nation_file.exists():
        consistency_errors = check_data_consistency(maps_dir, nation_file)
        if consistency_errors:
            all_errors.extend(consistency_errors)
            print(f"   ⚠️  {len(consistency_errors)} consistency notes")
        else:
            print("   ✓ Data consistent")

    # Summary
    print("\n" + "="*75)
    if all_errors:
        print(f"❌ VALIDATION FAILED - {len(all_errors)} issues found\n")
        print("ISSUES:")
        for i, error in enumerate(all_errors[:20], 1):  # Show first 20
            print(f"  {i}. {error}")
        if len(all_errors) > 20:
            print(f"  ... and {len(all_errors) - 20} more")
        return 1
    else:
        print("✅ ALL VALIDATIONS PASSED!")
        print()

        # Print statistics
        if maps_dir.exists():
            map_files = list(maps_dir.glob('map_*_1100.json'))
            combined = maps_dir / 'map_europe_1100.json'

            print("📊 STATISTICS:")
            print(f"   Total map files: {len(map_files)}")
            print(f"   Individual polity maps: {len(map_files) - 1}")
            print(f"   Combined Europe map: 1")

            if combined.exists():
                with open(combined, 'r') as f:
                    data = json.load(f)
                    print(f"   Total provinces: {len(data['map_region']['provinces'])}")

            if nation_file.exists():
                with open(nation_file, 'r') as f:
                    data = json.load(f)
                    print(f"   Nation definitions: {len(data['nations'])}")

        print("\n✨ 11th century historical data is production-ready!")
        print("="*75)
        return 0

if __name__ == '__main__':
    exit(main())
