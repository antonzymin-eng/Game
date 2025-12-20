#!/usr/bin/env python3
"""
Calculate province adjacencies for map files.

This script adds neighbor relationships to map JSON files by detecting
when province boundaries share edges.
"""

import json
import math
from pathlib import Path
from typing import Dict, List, Tuple, Set

def distance(p1: Dict[str, float], p2: Dict[str, float]) -> float:
    """Calculate Euclidean distance between two points."""
    dx = p1['x'] - p2['x']
    dy = p1['y'] - p2['y']
    return math.sqrt(dx * dx + dy * dy)

def points_close(p1: Dict[str, float], p2: Dict[str, float], tolerance: float) -> bool:
    """Check if two points are within tolerance distance."""
    return distance(p1, p2) < tolerance

def segments_intersect(a1: Dict, a2: Dict, b1: Dict, b2: Dict, tolerance: float) -> Tuple[bool, float]:
    """
    Check if two line segments intersect or share an edge.
    Returns (intersects: bool, shared_length: float)
    """
    # Check if segments are collinear (share an edge)
    # This is the key for detecting shared borders

    # Quick bounding box check first
    a_min_x = min(a1['x'], a2['x']) - tolerance
    a_max_x = max(a1['x'], a2['x']) + tolerance
    a_min_y = min(a1['y'], a2['y']) - tolerance
    a_max_y = max(a1['y'], a2['y']) + tolerance

    b_min_x = min(b1['x'], b2['x']) - tolerance
    b_max_x = max(b1['x'], b2['x']) + tolerance
    b_min_y = min(b1['y'], b2['y']) - tolerance
    b_max_y = max(b1['y'], b2['y']) + tolerance

    # No intersection if bounding boxes don't overlap
    if (a_max_x < b_min_x or b_max_x < a_min_x or
        a_max_y < b_min_y or b_max_y < a_min_y):
        return False, 0.0

    # Check if endpoints are close (shared vertices)
    endpoints_close = (
        points_close(a1, b1, tolerance) or
        points_close(a1, b2, tolerance) or
        points_close(a2, b1, tolerance) or
        points_close(a2, b2, tolerance)
    )

    # Calculate vectors
    dx_a = a2['x'] - a1['x']
    dy_a = a2['y'] - a1['y']
    dx_b = b2['x'] - b1['x']
    dy_b = b2['y'] - b1['y']

    len_a = math.sqrt(dx_a * dx_a + dy_a * dy_a)
    len_b = math.sqrt(dx_b * dx_b + dy_b * dy_b)

    if len_a < tolerance or len_b < tolerance:
        return endpoints_close, 0.0

    # Normalize vectors
    nx_a = dx_a / len_a
    ny_a = dy_a / len_a
    nx_b = dx_b / len_b
    ny_b = dy_b / len_b

    # Check if vectors are parallel (collinear segments)
    cross = abs(nx_a * ny_b - ny_a * nx_b)

    if cross < 0.01:  # Nearly parallel
        # Check if segments overlap
        # Project all points onto the line direction
        # Using segment A's direction as reference

        proj_a1 = 0.0
        proj_a2 = len_a
        proj_b1 = (b1['x'] - a1['x']) * nx_a + (b1['y'] - a1['y']) * ny_a
        proj_b2 = (b2['x'] - a1['x']) * nx_a + (b2['y'] - a1['y']) * ny_a

        # Check if points are close to the line
        perp_dist_b1 = abs((b1['x'] - a1['x']) * (-ny_a) + (b1['y'] - a1['y']) * nx_a)
        perp_dist_b2 = abs((b2['x'] - a1['x']) * (-ny_a) + (b2['y'] - a1['y']) * nx_a)

        if perp_dist_b1 < tolerance and perp_dist_b2 < tolerance:
            # Segments are collinear, check overlap
            overlap_start = max(proj_a1, min(proj_b1, proj_b2))
            overlap_end = min(proj_a2, max(proj_b1, proj_b2))
            overlap_length = max(0.0, overlap_end - overlap_start)

            if overlap_length > tolerance:
                return True, overlap_length

    # Check for standard line intersection
    if endpoints_close:
        return True, 0.0

    return False, 0.0

def calculate_border_length(boundary1: List[Dict], boundary2: List[Dict], tolerance: float) -> float:
    """
    Calculate the total shared border length between two provinces.
    """
    total_length = 0.0

    # Compare all edge pairs
    for i in range(len(boundary1)):
        p1_start = boundary1[i]
        p1_end = boundary1[(i + 1) % len(boundary1)]

        for j in range(len(boundary2)):
            p2_start = boundary2[j]
            p2_end = boundary2[(j + 1) % len(boundary2)]

            intersects, shared_length = segments_intersect(
                p1_start, p1_end, p2_start, p2_end, tolerance
            )

            if intersects and shared_length > 0:
                total_length += shared_length

    return total_length

def are_neighbors(boundary1: List[Dict], boundary2: List[Dict], tolerance: float) -> bool:
    """
    Check if two provinces share a border.
    Faster check that returns as soon as an intersection is found.
    """
    for i in range(len(boundary1)):
        p1_start = boundary1[i]
        p1_end = boundary1[(i + 1) % len(boundary1)]

        for j in range(len(boundary2)):
            p2_start = boundary2[j]
            p2_end = boundary2[(j + 1) % len(boundary2)]

            intersects, _ = segments_intersect(
                p1_start, p1_end, p2_start, p2_end, tolerance
            )

            if intersects:
                return True

    return False

def calculate_adaptive_tolerance(provinces: List[Dict]) -> float:
    """
    Calculate an adaptive tolerance based on province sizes.
    Uses 0.5% of median province diagonal.
    """
    diagonals = []

    for province in provinces:
        boundary = province.get('boundary', [])
        if len(boundary) < 3:
            continue

        xs = [p['x'] for p in boundary]
        ys = [p['y'] for p in boundary]

        width = max(xs) - min(xs)
        height = max(ys) - min(ys)
        diagonal = math.sqrt(width * width + height * height)
        diagonals.append(diagonal)

    if not diagonals:
        return 1.0  # Default fallback

    # Use median for robustness
    diagonals.sort()
    median = diagonals[len(diagonals) // 2]

    # Use 0.5% of median diagonal (more permissive than C++ 0.1%)
    tolerance = median * 0.005

    print(f"  Median province diagonal: {median:.2f}")
    print(f"  Calculated tolerance: {tolerance:.2f}")

    return tolerance

def add_adjacencies_to_map(map_file: Path, tolerance: float = None):
    """
    Add neighbor adjacencies to a map file.
    """
    print(f"\nProcessing {map_file.name}...")

    # Load map data
    with open(map_file, 'r', encoding='utf-8') as f:
        data = json.load(f)

    if 'map_region' not in data:
        print(f"  ERROR: No map_region in {map_file.name}")
        return

    provinces = data['map_region'].get('provinces', [])
    if not provinces:
        print(f"  ERROR: No provinces in {map_file.name}")
        return

    print(f"  Found {len(provinces)} provinces")

    # Calculate adaptive tolerance if not provided
    if tolerance is None:
        tolerance = calculate_adaptive_tolerance(provinces)

    # Clear existing neighbors
    for province in provinces:
        province['neighbors'] = []

    # Calculate adjacencies
    total_adjacencies = 0
    max_neighbors = 0
    isolated_provinces = []

    for i in range(len(provinces)):
        prov1 = provinces[i]
        boundary1 = prov1.get('boundary', [])

        if len(boundary1) < 3:
            print(f"  WARNING: Province {prov1.get('name', prov1.get('id'))} has invalid boundary")
            continue

        for j in range(i + 1, len(provinces)):
            prov2 = provinces[j]
            boundary2 = prov2.get('boundary', [])

            if len(boundary2) < 3:
                continue

            # Quick check first
            if not are_neighbors(boundary1, boundary2, tolerance):
                continue

            # Calculate border length
            border_length = calculate_border_length(boundary1, boundary2, tolerance)

            # Only add as neighbors if they share significant border
            if border_length > tolerance:
                # Add bidirectional neighbors with border length
                prov1['neighbors'].append({
                    'id': prov2['id'],
                    'border_length': round(border_length, 2)
                })
                prov2['neighbors'].append({
                    'id': prov1['id'],
                    'border_length': round(border_length, 2)
                })

                total_adjacencies += 1

    # Calculate statistics
    for province in provinces:
        neighbor_count = len(province.get('neighbors', []))
        max_neighbors = max(max_neighbors, neighbor_count)
        if neighbor_count == 0:
            isolated_provinces.append(province.get('name', province.get('id')))

    print(f"  ✓ Found {total_adjacencies} adjacencies")
    print(f"  ✓ Max neighbors: {max_neighbors}")

    if isolated_provinces:
        print(f"  ⚠ WARNING: {len(isolated_provinces)} isolated provinces:")
        for name in isolated_provinces[:5]:  # Show first 5
            print(f"    - {name}")
        if len(isolated_provinces) > 5:
            print(f"    ... and {len(isolated_provinces) - 5} more")

    # Save updated map
    backup_file = map_file.with_suffix('.json.backup')
    if not backup_file.exists():
        import shutil
        shutil.copy2(map_file, backup_file)
        print(f"  Created backup: {backup_file.name}")

    with open(map_file, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2, ensure_ascii=False)

    print(f"  ✓ Updated {map_file.name}")

def main():
    """Main function."""
    script_dir = Path(__file__).parent
    maps_dir = script_dir / 'maps'

    print("=" * 70)
    print("Calculate Province Adjacencies")
    print("=" * 70)
    print()

    # Get all map files
    map_files = sorted(maps_dir.glob('map_*_real.json'))

    if not map_files:
        print("No map files found!")
        return

    print(f"Found {len(map_files)} map files")
    print()

    # Process each map file
    for map_file in map_files:
        try:
            add_adjacencies_to_map(map_file)
        except Exception as e:
            print(f"  ERROR processing {map_file.name}: {e}")
            import traceback
            traceback.print_exc()

    print()
    print("=" * 70)
    print("Adjacency calculation complete!")
    print("=" * 70)
    print()
    print("Next step:")
    print("  Run: python update_combined_europe.py")
    print()

if __name__ == '__main__':
    main()
