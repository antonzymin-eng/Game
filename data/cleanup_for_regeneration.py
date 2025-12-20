#!/usr/bin/env python3
"""
Cleanup script for map regeneration.

Deletes old GeoJSON and map files for countries that need to be regenerated
with province limits to prevent excessive adjacency calculations.
"""

import os
from pathlib import Path

def main():
    script_dir = Path(__file__).parent
    geojson_dir = script_dir / 'maps' / 'geojson_source'
    maps_dir = script_dir / 'maps'

    print("=" * 70)
    print("Map Regeneration Cleanup")
    print("=" * 70)
    print()
    print("This will delete the following files so they can be regenerated:")
    print("  - GeoJSON source files for: Ukraine, Belarus, Moldova, Russia, UK")
    print("  - Generated map files for: Ukraine, Belarus, Moldova, Russia, UK")
    print()
    print("This is necessary because the new scripts limit provinces to")
    print("prevent excessive adjacency calculations (Russia: 8 instead of 59).")
    print()

    # Files to delete
    geojson_files = [
        'ukraine_nuts1.geojson',
        'belarus_nuts1.geojson',
        'moldova_nuts1.geojson',
        'russia_nuts1.geojson',
        'united_kingdom_nuts1.geojson'
    ]

    map_files = [
        'map_ukraine_real.json',
        'map_belarus_real.json',
        'map_moldova_real.json',
        'map_russia_european_real.json',
        'map_united_kingdom_real.json'
    ]

    # Delete GeoJSON source files
    print("Deleting GeoJSON source files...")
    deleted_count = 0
    for filename in geojson_files:
        file_path = geojson_dir / filename
        if file_path.exists():
            file_path.unlink()
            print(f"  ✓ Deleted {filename}")
            deleted_count += 1
        else:
            print(f"  - {filename} (not found)")

    print()

    # Delete generated map files
    print("Deleting generated map files...")
    for filename in map_files:
        file_path = maps_dir / filename
        if file_path.exists():
            file_path.unlink()
            print(f"  ✓ Deleted {filename}")
            deleted_count += 1

            # Also delete backup if exists
            backup_path = file_path.with_suffix('.json.backup')
            if backup_path.exists():
                backup_path.unlink()
                print(f"  ✓ Deleted {filename}.backup")
        else:
            print(f"  - {filename} (not found)")

    print()
    print("=" * 70)
    print("Cleanup Complete!")
    print("=" * 70)
    print()
    print(f"Deleted {deleted_count} files")
    print()
    print("Next steps:")
    print("  1. Run: python regenerate_maps.bat (Windows) or ./regenerate_maps.sh (Linux)")
    print("  2. The new files will have limited province counts:")
    print("     - Russia: 8 provinces (federal districts)")
    print("     - Ukraine: 8 provinces (major regions)")
    print("     - Belarus: 7 provinces (major regions)")
    print("     - Moldova: 5 provinces")
    print("     - UK: 12 provinces")
    print()

if __name__ == '__main__':
    main()
