#!/usr/bin/env python3
"""
Cleanup script for FULL map regeneration.

Deletes ALL GeoJSON source files and map files so the entire European map
can be regenerated from scratch with correct province limits.
"""

import os
from pathlib import Path
import glob

def main():
    script_dir = Path(__file__).parent
    geojson_dir = script_dir / 'maps' / 'geojson_source'
    maps_dir = script_dir / 'maps'

    print("=" * 70)
    print("FULL Map Regeneration Cleanup")
    print("=" * 70)
    print()
    print("WARNING: This will delete ALL generated map files and GeoJSON sources!")
    print()
    print("This allows complete regeneration with correct province limits for")
    print("all European countries, preventing the 'all provinces = france' bug.")
    print()

    response = input("Continue? (yes/no): ").strip().lower()
    if response not in ['yes', 'y']:
        print("Cancelled.")
        return

    deleted_count = 0

    # Delete ALL GeoJSON source files
    print()
    print("Deleting ALL GeoJSON source files...")
    if geojson_dir.exists():
        for geojson_file in geojson_dir.glob('*.geojson'):
            geojson_file.unlink()
            print(f"  ✓ Deleted {geojson_file.name}")
            deleted_count += 1

    # Delete ALL generated map_*_real.json files
    print()
    print("Deleting ALL map_*_real.json files...")
    if maps_dir.exists():
        for map_file in maps_dir.glob('map_*_real.json'):
            map_file.unlink()
            print(f"  ✓ Deleted {map_file.name}")
            deleted_count += 1

            # Also delete backup if exists
            backup_path = map_file.with_suffix('.json.backup')
            if backup_path.exists():
                backup_path.unlink()
                print(f"  ✓ Deleted {map_file.name}.backup")
                deleted_count += 1

    # Delete combined map
    print()
    print("Deleting combined map...")
    combined_map = maps_dir / 'map_europe_combined.json'
    if combined_map.exists():
        combined_map.unlink()
        print(f"  ✓ Deleted map_europe_combined.json")
        deleted_count += 1

    print()
    print("=" * 70)
    print("Cleanup Complete!")
    print("=" * 70)
    print()
    print(f"Deleted {deleted_count} files")
    print()
    print("Next steps:")
    print("  1. Run: .\\regenerate_maps.bat (Windows) or ./regenerate_maps.sh (Linux)")
    print("  2. This will:")
    print("     - Download GeoJSON for ALL European countries")
    print("     - Generate map files with correct owner_realm values")
    print("     - Calculate adjacencies")
    print("     - Create combined map")
    print("  3. Then rebuild the game to copy new files to build directory")
    print()

if __name__ == '__main__':
    main()
