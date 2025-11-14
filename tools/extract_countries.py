#!/usr/bin/env python3
"""
Extract specific countries from NUTS level 1 data and convert to game format
"""

import json
import sys

# Country codes for the countries we want to extract
# Based on NUTS naming: ES = Spain, PT = Portugal, UK/UKC/UKD/etc = UK,
# BE = Belgium, NL = Netherlands, LU = Luxembourg, IE = Ireland,
# DE = Germany, CH = Switzerland, IT = Italy, FR = France (already done separately)
COUNTRY_PREFIXES = {
    'ES': 'spain',
    'PT': 'portugal',
    'UK': 'united_kingdom',
    'BE': 'belgium',
    'NL': 'netherlands',
    'LU': 'luxembourg',
    'IE': 'ireland',
    'DE': 'germany',
    'CH': 'switzerland',
    'IT': 'italy',
    'FR': 'france',
}

def extract_countries_from_nuts(nuts_file, output_dir):
    """Extract individual countries from NUTS level 1 data"""

    with open(nuts_file, 'r', encoding='utf-8') as f:
        nuts_data = json.load(f)

    # Group features by country
    countries = {}
    for country_code in COUNTRY_PREFIXES.keys():
        countries[country_code] = []

    for feature in nuts_data['features']:
        nuts_id = feature['properties']['id']

        # Determine country from NUTS ID
        for code in COUNTRY_PREFIXES.keys():
            if nuts_id.startswith(code):
                countries[code].append(feature)
                break

    # Save each country to a separate GeoJSON file
    results = {}
    for country_code, features in countries.items():
        if not features:
            continue

        country_name = COUNTRY_PREFIXES[country_code]
        output_file = f"{output_dir}/{country_name}_nuts1.geojson"

        geojson = {
            "type": "FeatureCollection",
            "features": features
        }

        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(geojson, f, indent=2, ensure_ascii=False)

        results[country_name] = {
            'file': output_file,
            'regions': len(features),
            'code': country_code
        }
        print(f"✓ Extracted {len(features)} regions for {country_name} ({country_code})")

    return results

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: extract_countries.py <nuts_file> <output_dir>")
        sys.exit(1)

    nuts_file = sys.argv[1]
    output_dir = sys.argv[2]

    results = extract_countries_from_nuts(nuts_file, output_dir)

    print(f"\n✓ Total: Extracted {len(results)} countries")
    print(f"✓ Output directory: {output_dir}")
