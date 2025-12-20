#!/usr/bin/env python3
"""
Download missing GeoJSON data for European countries from Natural Earth Data.

This script downloads administrative boundary data (admin-1 level) for:
- Ukraine
- Belarus
- Moldova
- Russia (European part)
- United Kingdom

Source: Natural Earth Data (https://www.naturalearthdata.com/)
"""

import json
import os
import urllib.request
from pathlib import Path
from typing import Dict, List

# Natural Earth Data URL for Admin 1 (provinces/states)
# Using 10m (1:10m scale) for good detail
NATURAL_EARTH_URL = "https://www.naturalearthdata.com/http//www.naturalearthdata.com/download/10m/cultural/ne_10m_admin_1_states_provinces.zip"

# Country codes to process
COUNTRIES_TO_PROCESS = {
    'Ukraine': 'UKR',
    'Belarus': 'BLR',
    'Moldova': 'MDA',
    'Russia': 'RUS',
    'United Kingdom': 'GBR'
}

def download_natural_earth_data(output_dir: Path):
    """Download Natural Earth admin-1 dataset."""
    import zipfile
    import tempfile

    print("Downloading Natural Earth admin-1 data...")
    print(f"URL: {NATURAL_EARTH_URL}")

    # Create temp directory
    with tempfile.TemporaryDirectory() as tmpdir:
        zip_path = Path(tmpdir) / "ne_admin1.zip"

        # Download
        print("Downloading... (this may take a minute)")
        urllib.request.urlretrieve(NATURAL_EARTH_URL, zip_path)
        print(f"Downloaded to {zip_path}")

        # Extract
        print("Extracting...")
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(tmpdir)

        # Find the shapefile
        shp_files = list(Path(tmpdir).glob("*.shp"))
        if not shp_files:
            print("ERROR: No shapefile found in download!")
            return None

        shp_file = shp_files[0]
        print(f"Found shapefile: {shp_file}")

        # Convert to GeoJSON using ogr2ogr or fiona
        try:
            import fiona
            from fiona.crs import from_epsg

            print("Processing shapefile with fiona...")
            return process_shapefile_with_fiona(shp_file, output_dir)
        except ImportError:
            print("fiona not available, trying ogr2ogr...")
            return process_shapefile_with_ogr(shp_file, output_dir, tmpdir)

def process_shapefile_with_fiona(shp_file: Path, output_dir: Path):
    """Process shapefile using fiona (Python library)."""
    import fiona

    print(f"Opening {shp_file} with fiona...")

    with fiona.open(shp_file, 'r') as src:
        print(f"Found {len(src)} features in shapefile")
        print(f"Schema: {src.schema}")

        # Group features by country
        country_features = {code: [] for code in COUNTRIES_TO_PROCESS.values()}

        for feature in src:
            props = feature['properties']
            iso_a2 = props.get('iso_a2', '')
            iso_3166_2 = props.get('iso_3166_2', '')
            admin = props.get('admin', '')

            # Match by country code
            for country_name, country_code in COUNTRIES_TO_PROCESS.items():
                if iso_a2 == country_code[:2] or admin == country_name:
                    # For Russia, only include European part (west of 60°E longitude)
                    if country_code == 'RUS':
                        # Check if geometry is west of Ural Mountains (~60°E)
                        geom = feature['geometry']
                        if geom and geom['type'] in ['Polygon', 'MultiPolygon']:
                            # Get centroid longitude (rough approximation)
                            coords = geom['coordinates']
                            if geom['type'] == 'Polygon':
                                lons = [c[0] for c in coords[0]]
                            else:
                                lons = [c[0] for polygon in coords for c in polygon[0]]

                            avg_lon = sum(lons) / len(lons) if lons else 0
                            if avg_lon > 60:  # Skip Asian Russia
                                continue

                    country_features[country_code].append(feature)
                    break

        # Write GeoJSON files for each country
        for country_name, country_code in COUNTRIES_TO_PROCESS.items():
            features = country_features[country_code]
            if not features:
                print(f"WARNING: No features found for {country_name} ({country_code})")
                continue

            output_file = output_dir / f"{country_name.lower().replace(' ', '_')}_nuts1.geojson"

            geojson = {
                "type": "FeatureCollection",
                "features": features
            }

            with open(output_file, 'w', encoding='utf-8') as f:
                json.dump(geojson, f, indent=2, ensure_ascii=False)

            print(f"✓ Created {output_file.name} with {len(features)} provinces")

def process_shapefile_with_ogr(shp_file: Path, output_dir: Path, tmpdir: str):
    """Process shapefile using ogr2ogr command-line tool."""
    import subprocess

    # First convert entire shapefile to GeoJSON
    geojson_file = Path(tmpdir) / "all_admin1.geojson"

    print(f"Converting shapefile to GeoJSON with ogr2ogr...")
    result = subprocess.run([
        'ogr2ogr',
        '-f', 'GeoJSON',
        str(geojson_file),
        str(shp_file)
    ], capture_output=True, text=True)

    if result.returncode != 0:
        print(f"ERROR: ogr2ogr failed: {result.stderr}")
        print("\nPlease install GDAL/OGR or fiona:")
        print("  Ubuntu/Debian: sudo apt-get install gdal-bin python3-fiona")
        print("  Fedora: sudo dnf install gdal python3-fiona")
        print("  macOS: brew install gdal && pip install fiona")
        return

    print(f"Converted to {geojson_file}")

    # Load and filter by country
    with open(geojson_file, 'r', encoding='utf-8') as f:
        data = json.load(f)

    # Group features by country
    country_features = {code: [] for code in COUNTRIES_TO_PROCESS.values()}

    for feature in data.get('features', []):
        props = feature.get('properties', {})
        iso_a2 = props.get('iso_a2', '')
        admin = props.get('admin', '')

        for country_name, country_code in COUNTRIES_TO_PROCESS.items():
            if iso_a2 == country_code[:2] or admin == country_name:
                # For Russia, filter European part only
                if country_code == 'RUS':
                    geom = feature.get('geometry', {})
                    if geom and geom['type'] in ['Polygon', 'MultiPolygon']:
                        coords = geom['coordinates']
                        if geom['type'] == 'Polygon':
                            lons = [c[0] for c in coords[0]]
                        else:
                            lons = [c[0] for polygon in coords for c in polygon[0]]

                        avg_lon = sum(lons) / len(lons) if lons else 0
                        if avg_lon > 60:
                            continue

                country_features[country_code].append(feature)
                break

    # Write GeoJSON files
    for country_name, country_code in COUNTRIES_TO_PROCESS.items():
        features = country_features[country_code]
        if not features:
            print(f"WARNING: No features found for {country_name} ({country_code})")
            continue

        output_file = output_dir / f"{country_name.lower().replace(' ', '_')}_nuts1.geojson"

        geojson = {
            "type": "FeatureCollection",
            "features": features
        }

        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(geojson, f, indent=2, ensure_ascii=False)

        print(f"✓ Created {output_file.name} with {len(features)} provinces")

def main():
    """Main function."""
    script_dir = Path(__file__).parent
    geojson_dir = script_dir / 'maps' / 'geojson_source'

    print("=" * 70)
    print("Download Missing GeoJSON Data for European Map")
    print("=" * 70)
    print()

    # Create output directory
    geojson_dir.mkdir(parents=True, exist_ok=True)

    # Download and process
    download_natural_earth_data(geojson_dir)

    print()
    print("=" * 70)
    print("Download complete!")
    print("=" * 70)
    print()
    print("Next steps:")
    print("  1. Review the generated GeoJSON files in:")
    print(f"     {geojson_dir}")
    print("  2. Run: python generate_europe_maps.py")
    print("  3. Run: python update_combined_europe.py")
    print()

if __name__ == '__main__':
    main()
