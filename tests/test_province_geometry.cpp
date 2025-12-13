// ============================================================================
// test_province_geometry.cpp - Unit Tests for Province Geometry Functions
// Tests for adjacency detection, border length calculation, and edge cases
// ============================================================================

#include "map/ProvinceGeometry.h"
#include "map/GeographicUtils.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace game::map;

// Test helper
#define TEST(name) \
    std::cout << "Running test: " << #name << "..." << std::endl; \
    test_##name(); \
    std::cout << "  ✓ PASSED" << std::endl;

#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        std::cerr << "  ✗ FAILED: " << #expr << " at line " << __LINE__ << std::endl; \
        return false; \
    }

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_NEAR(a, b, epsilon) \
    if (std::abs((a) - (b)) > (epsilon)) { \
        std::cerr << "  ✗ FAILED: " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ") at line " << __LINE__ << std::endl; \
        return false; \
    }

// ============================================================================
// Test Cases
// ============================================================================

bool test_collinear_overlapping_segments() {
    // Two provinces sharing a collinear border
    std::vector<Coordinate> prov1 = {
        {0, 0}, {10, 0}, {10, 10}, {0, 10}  // Square
    };
    std::vector<Coordinate> prov2 = {
        {10, 0}, {20, 0}, {20, 10}, {10, 10}  // Adjacent square
    };

    auto result = ProvinceGeometry::CheckAdjacency(prov1, prov2, 0.001);

    ASSERT_TRUE(result.are_neighbors);
    ASSERT_NEAR(result.border_length, 10.0, 0.01);  // Shared edge length = 10

    return true;
}

bool test_crossing_intersection() {
    // Two provinces that touch at a crossing (not collinear)
    std::vector<Coordinate> prov1 = {
        {0, 0}, {10, 0}, {10, 10}, {0, 10}
    };
    std::vector<Coordinate> prov2 = {
        {5, -5}, {15, -5}, {15, 5}, {5, 5}  // Crosses bottom edge
    };

    auto result = ProvinceGeometry::CheckAdjacency(prov1, prov2, 0.001);

    ASSERT_TRUE(result.are_neighbors);  // They do intersect
    ASSERT_NEAR(result.border_length, 0.0, 0.01);  // But border length is 0 (crossing, not overlap)

    return true;
}

bool test_point_touch() {
    // Two provinces that touch at a single point only
    std::vector<Coordinate> prov1 = {
        {0, 0}, {10, 0}, {10, 10}, {0, 10}
    };
    std::vector<Coordinate> prov2 = {
        {10, 10}, {20, 10}, {20, 20}, {10, 20}  // Touch at (10,10)
    };

    auto result = ProvinceGeometry::CheckAdjacency(prov1, prov2, 0.001);

    ASSERT_TRUE(result.are_neighbors);  // They touch
    ASSERT_NEAR(result.border_length, 0.0, 0.01);  // Point has zero length

    return true;
}

bool test_non_neighbors() {
    // Two provinces that don't touch at all
    std::vector<Coordinate> prov1 = {
        {0, 0}, {10, 0}, {10, 10}, {0, 10}
    };
    std::vector<Coordinate> prov2 = {
        {20, 20}, {30, 20}, {30, 30}, {20, 30}  // Far away
    };

    auto result = ProvinceGeometry::CheckAdjacency(prov1, prov2, 0.001);

    ASSERT_FALSE(result.are_neighbors);
    ASSERT_NEAR(result.border_length, 0.0, 0.01);

    return true;
}

bool test_partial_border_overlap() {
    // Provinces sharing only part of an edge
    std::vector<Coordinate> prov1 = {
        {0, 0}, {10, 0}, {10, 10}, {0, 10}
    };
    std::vector<Coordinate> prov2 = {
        {10, 3}, {20, 3}, {20, 7}, {10, 7}  // Shares edge from (10,3) to (10,7)
    };

    auto result = ProvinceGeometry::CheckAdjacency(prov1, prov2, 0.001);

    ASSERT_TRUE(result.are_neighbors);
    ASSERT_NEAR(result.border_length, 4.0, 0.01);  // Shared length = 4

    return true;
}

bool test_degenerate_province() {
    // Province with less than 3 points (invalid)
    std::vector<Coordinate> prov1 = {
        {0, 0}, {10, 0}  // Only 2 points
    };
    std::vector<Coordinate> prov2 = {
        {10, 0}, {20, 0}, {20, 10}, {10, 10}
    };

    auto result = ProvinceGeometry::CheckAdjacency(prov1, prov2, 0.001);

    ASSERT_FALSE(result.are_neighbors);  // Invalid boundary
    ASSERT_NEAR(result.border_length, 0.0, 0.01);

    return true;
}

bool test_duplicate_points_removal() {
    // Boundary with duplicate consecutive points
    std::vector<Coordinate> boundary = {
        {0, 0}, {0, 0}, {10, 0}, {10, 0}, {10, 10}, {0, 10}
    };

    ProvinceGeometry::RemoveDuplicatePoints(boundary, 0.001);

    ASSERT_TRUE(boundary.size() == 4);  // Should remove 2 duplicates
    ASSERT_NEAR(boundary[0].x, 0.0, 0.01);
    ASSERT_NEAR(boundary[1].x, 10.0, 0.01);

    return true;
}

bool test_valid_boundary() {
    std::vector<Coordinate> valid = {{0, 0}, {10, 0}, {10, 10}};
    std::vector<Coordinate> invalid = {{0, 0}, {10, 0}};

    ASSERT_TRUE(ProvinceGeometry::IsValidBoundary(valid));
    ASSERT_FALSE(ProvinceGeometry::IsValidBoundary(invalid));

    return true;
}

bool test_multiple_shared_edges() {
    // Complex provinces with multiple shared edge segments
    std::vector<Coordinate> prov1 = {
        {0, 0}, {5, 0}, {10, 0}, {10, 10}, {0, 10}
    };
    std::vector<Coordinate> prov2 = {
        {10, 0}, {15, 0}, {20, 0}, {20, 10}, {15, 10}, {10, 10}
    };

    auto result = ProvinceGeometry::CheckAdjacency(prov1, prov2, 0.001);

    ASSERT_TRUE(result.are_neighbors);
    ASSERT_NEAR(result.border_length, 10.0, 0.1);  // Shared edge from (10,0) to (10,10)

    return true;
}

bool test_tolerance_handling() {
    // Test that tolerance allows near-misses
    std::vector<Coordinate> prov1 = {
        {0, 0}, {10, 0}, {10, 10}, {0, 10}
    };
    std::vector<Coordinate> prov2 = {
        {10.0001, 0}, {20, 0}, {20, 10}, {10.0001, 10}  // Slightly offset
    };

    // Should be neighbors with tolerance = 0.001
    auto result1 = ProvinceGeometry::CheckAdjacency(prov1, prov2, 1.0);
    ASSERT_TRUE(result1.are_neighbors);

    // Should NOT be neighbors with very small tolerance
    auto result2 = ProvinceGeometry::CheckAdjacency(prov1, prov2, 0.00001);
    ASSERT_FALSE(result2.are_neighbors);

    return true;
}

bool test_zero_length_edge_handling() {
    // Province with a zero-length edge (degenerate)
    std::vector<Coordinate> prov1 = {
        {0, 0}, {0, 0}, {10, 0}, {10, 10}, {0, 10}  // Duplicate at start
    };
    std::vector<Coordinate> prov2 = {
        {10, 0}, {20, 0}, {20, 10}, {10, 10}
    };

    // Should still work (zero-length edges are skipped)
    auto result = ProvinceGeometry::CheckAdjacency(prov1, prov2, 0.001);
    ASSERT_TRUE(result.are_neighbors);

    return true;
}

bool test_complex_border() {
    // Provinces with jagged borders
    std::vector<Coordinate> prov1 = {
        {0, 0}, {10, 0}, {10, 5}, {8, 5}, {8, 7}, {10, 7}, {10, 10}, {0, 10}
    };
    std::vector<Coordinate> prov2 = {
        {10, 0}, {20, 0}, {20, 10}, {10, 10}, {10, 7}, {12, 7}, {12, 5}, {10, 5}
    };

    auto result = ProvinceGeometry::CheckAdjacency(prov1, prov2, 0.001);

    ASSERT_TRUE(result.are_neighbors);
    ASSERT_TRUE(result.border_length > 0.0);  // Has some shared border

    return true;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n=== Province Geometry Unit Tests ===" << std::endl;
    std::cout << "Testing adjacency detection, border length calculation, and edge cases\n" << std::endl;

    int passed = 0;
    int failed = 0;

    #define RUN_TEST(name) \
        try { \
            TEST(name); \
            passed++; \
        } catch (...) { \
            std::cerr << "  ✗ FAILED: Exception thrown" << std::endl; \
            failed++; \
        }

    RUN_TEST(collinear_overlapping_segments);
    RUN_TEST(crossing_intersection);
    RUN_TEST(point_touch);
    RUN_TEST(non_neighbors);
    RUN_TEST(partial_border_overlap);
    RUN_TEST(degenerate_province);
    RUN_TEST(duplicate_points_removal);
    RUN_TEST(valid_boundary);
    RUN_TEST(multiple_shared_edges);
    RUN_TEST(tolerance_handling);
    RUN_TEST(zero_length_edge_handling);
    RUN_TEST(complex_border);

    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    if (failed == 0) {
        std::cout << "\n✓ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ Some tests failed!" << std::endl;
        return 1;
    }
}
