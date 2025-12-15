// ============================================================================
// test_province_adjacency_integration.cpp - Integration Tests for Province Adjacency
// Tests the complete province adjacency computation pipeline
// ============================================================================

#include "map/loaders/ProvinceBuilder.h"
#include "map/ProvinceGeometry.h"
#include "map/MapData.h"
#include <iostream>
#include <cassert>
#include <chrono>

using namespace game::map;
using namespace game::map::loaders;

#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        std::cerr << "  ✗ FAILED: " << #expr << " at line " << __LINE__ << std::endl; \
        return false; \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "  ✗ FAILED: " << #a << " (" << (a) << ") != " << #b << " (" << (b) << ") at line " << __LINE__ << std::endl; \
        return false; \
    }

// ============================================================================
// Test Helper Functions
// ============================================================================

ProvinceData CreateSquareProvince(uint32_t id, const std::string& name,
                                  double x, double y, double size) {
    ProvinceData prov;
    prov.id = id;
    prov.name = name;
    prov.boundary = {
        {x, y},
        {x + size, y},
        {x + size, y + size},
        {x, y + size}
    };
    prov.center = {x + size/2, y + size/2};
    prov.bounds = {x, y, x + size, y + size};
    return prov;
}

bool HasNeighbor(const ProvinceData& prov, uint32_t neighbor_id) {
    for (const auto& neighbor : prov.detailed_neighbors) {
        if (neighbor.neighbor_id == neighbor_id) return true;
    }
    return false;
}

double GetBorderLength(const ProvinceData& prov, uint32_t neighbor_id) {
    for (const auto& neighbor : prov.detailed_neighbors) {
        if (neighbor.neighbor_id == neighbor_id) {
            return neighbor.border_length;
        }
    }
    return 0.0;
}

// ============================================================================
// Integration Tests
// ============================================================================

bool test_basic_adjacency_computation() {
    std::cout << "Test: Basic adjacency computation" << std::endl;

    // Create 3 provinces in a row
    std::vector<ProvinceData> provinces;
    provinces.push_back(CreateSquareProvince(1, "Province 1", 0, 0, 10));
    provinces.push_back(CreateSquareProvince(2, "Province 2", 10, 0, 10));
    provinces.push_back(CreateSquareProvince(3, "Province 3", 20, 0, 10));

    // Compute adjacency
    ProvinceBuilder builder;
    auto result = builder.LinkProvinces(provinces, 1.0);
    ASSERT_TRUE(result.IsSuccess());

    // Verify adjacency
    ASSERT_EQ(provinces[0].detailed_neighbors.size(), 1);  // Province 1 has 1 neighbor
    ASSERT_EQ(provinces[1].detailed_neighbors.size(), 2);  // Province 2 has 2 neighbors
    ASSERT_EQ(provinces[2].detailed_neighbors.size(), 1);  // Province 3 has 1 neighbor

    ASSERT_TRUE(HasNeighbor(provinces[0], 2));   // 1 -> 2
    ASSERT_TRUE(HasNeighbor(provinces[1], 1));   // 2 -> 1
    ASSERT_TRUE(HasNeighbor(provinces[1], 3));   // 2 -> 3
    ASSERT_TRUE(HasNeighbor(provinces[2], 2));   // 3 -> 2

    // Verify border lengths
    double border_1_2 = GetBorderLength(provinces[0], 2);
    double border_2_1 = GetBorderLength(provinces[1], 1);
    ASSERT_TRUE(border_1_2 > 9.9 && border_1_2 < 10.1);  // ~10
    ASSERT_TRUE(border_2_1 > 9.9 && border_2_1 < 10.1);  // ~10

    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

bool test_no_adjacency() {
    std::cout << "Test: Non-adjacent provinces" << std::endl;

    // Create 2 provinces far apart
    std::vector<ProvinceData> provinces;
    provinces.push_back(CreateSquareProvince(1, "Province 1", 0, 0, 10));
    provinces.push_back(CreateSquareProvince(2, "Province 2", 100, 100, 10));

    // Compute adjacency
    ProvinceBuilder builder;
    auto result = builder.LinkProvinces(provinces, 1.0);
    ASSERT_TRUE(result.IsSuccess());

    // Verify no adjacency
    ASSERT_EQ(provinces[0].detailed_neighbors.size(), 0);
    ASSERT_EQ(provinces[1].detailed_neighbors.size(), 0);

    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

bool test_grid_adjacency() {
    std::cout << "Test: 2x2 grid adjacency" << std::endl;

    // Create 2x2 grid of provinces
    std::vector<ProvinceData> provinces;
    provinces.push_back(CreateSquareProvince(1, "NW", 0, 0, 10));
    provinces.push_back(CreateSquareProvince(2, "NE", 10, 0, 10));
    provinces.push_back(CreateSquareProvince(3, "SW", 0, 10, 10));
    provinces.push_back(CreateSquareProvince(4, "SE", 10, 10, 10));

    // Compute adjacency
    ProvinceBuilder builder;
    auto result = builder.LinkProvinces(provinces, 1.0);
    ASSERT_TRUE(result.IsSuccess());

    // Verify each corner province has 2 neighbors
    ASSERT_EQ(provinces[0].detailed_neighbors.size(), 2);  // NW: NE, SW
    ASSERT_EQ(provinces[1].detailed_neighbors.size(), 2);  // NE: NW, SE
    ASSERT_EQ(provinces[2].detailed_neighbors.size(), 2);  // SW: NW, SE
    ASSERT_EQ(provinces[3].detailed_neighbors.size(), 2);  // SE: NE, SW

    // Verify specific adjacencies
    ASSERT_TRUE(HasNeighbor(provinces[0], 2));  // NW -> NE
    ASSERT_TRUE(HasNeighbor(provinces[0], 3));  // NW -> SW
    ASSERT_TRUE(HasNeighbor(provinces[1], 1));  // NE -> NW
    ASSERT_TRUE(HasNeighbor(provinces[1], 4));  // NE -> SE

    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

bool test_bidirectional_relationships() {
    std::cout << "Test: Bidirectional relationship validation" << std::endl;

    // Create adjacent provinces
    std::vector<ProvinceData> provinces;
    provinces.push_back(CreateSquareProvince(1, "A", 0, 0, 10));
    provinces.push_back(CreateSquareProvince(2, "B", 10, 0, 10));

    // Compute adjacency
    ProvinceBuilder builder;
    auto result = builder.LinkProvinces(provinces, 1.0);
    ASSERT_TRUE(result.IsSuccess());

    // Verify bidirectional
    ASSERT_TRUE(HasNeighbor(provinces[0], 2));  // A -> B
    ASSERT_TRUE(HasNeighbor(provinces[1], 1));  // B -> A

    // Verify symmetric border lengths
    double border_AB = GetBorderLength(provinces[0], 2);
    double border_BA = GetBorderLength(provinces[1], 1);
    ASSERT_TRUE(std::abs(border_AB - border_BA) < 0.01);

    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

bool test_adaptive_tolerance() {
    std::cout << "Test: Adaptive tolerance calculation" << std::endl;

    // Create provinces of varying sizes
    std::vector<ProvinceData> provinces;
    provinces.push_back(CreateSquareProvince(1, "Tiny", 0, 0, 1));
    provinces.push_back(CreateSquareProvince(2, "Small", 10, 10, 10));
    provinces.push_back(CreateSquareProvince(3, "Large", 50, 50, 100));

    // Compute adjacency with adaptive tolerance (tolerance <= 0)
    ProvinceBuilder builder;
    auto result = builder.LinkProvinces(provinces, 0.0);  // Triggers adaptive tolerance
    ASSERT_TRUE(result.IsSuccess());

    // Should complete without errors
    // (Adaptive tolerance is calculated from median province size)
    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

bool test_performance_large_dataset() {
    std::cout << "Test: Performance with larger province set" << std::endl;

    // Create 10x10 grid (100 provinces)
    std::vector<ProvinceData> provinces;
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 10; ++j) {
            uint32_t id = i * 10 + j + 1;
            provinces.push_back(CreateSquareProvince(id, "Province " + std::to_string(id),
                                                    i * 10.0, j * 10.0, 10.0));
        }
    }

    // Measure time
    auto start = std::chrono::high_resolution_clock::now();

    ProvinceBuilder builder;
    auto result = builder.LinkProvinces(provinces, 1.0);
    ASSERT_TRUE(result.IsSuccess());

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "  Computed adjacency for 100 provinces in " << duration.count() << "ms" << std::endl;

    // Verify adjacency counts
    int total_neighbors = 0;
    for (const auto& prov : provinces) {
        total_neighbors += prov.detailed_neighbors.size();
    }
    std::cout << "  Total neighbor relationships: " << total_neighbors / 2 << std::endl;

    // Corner provinces should have 2 neighbors
    ASSERT_EQ(provinces[0].detailed_neighbors.size(), 2);  // Top-left corner
    ASSERT_EQ(provinces[9].detailed_neighbors.size(), 2);  // Top-right corner

    // Edge provinces should have 3 neighbors
    ASSERT_EQ(provinces[5].detailed_neighbors.size(), 3);  // Top edge

    // Center provinces should have 4 neighbors
    ASSERT_EQ(provinces[55].detailed_neighbors.size(), 4);  // Center

    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

bool test_border_length_accuracy() {
    std::cout << "Test: Border length calculation accuracy" << std::endl;

    // Create provinces with known shared border length
    std::vector<ProvinceData> provinces;
    provinces.push_back(CreateSquareProvince(1, "A", 0, 0, 10));   // 10x10 square
    provinces.push_back(CreateSquareProvince(2, "B", 10, 0, 10));  // Adjacent 10x10 square

    // Compute adjacency
    ProvinceBuilder builder;
    auto result = builder.LinkProvinces(provinces, 1.0);
    ASSERT_TRUE(result.IsSuccess());

    // Expected border length is 10 (height of shared edge)
    double border_length = GetBorderLength(provinces[0], 2);
    ASSERT_TRUE(border_length > 9.9 && border_length < 10.1);

    std::cout << "  Border length: " << border_length << " (expected ~10.0)" << std::endl;
    std::cout << "  ✓ PASSED" << std::endl;
    return true;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n=== Province Adjacency Integration Tests ===" << std::endl;
    std::cout << "Testing complete adjacency computation pipeline\n" << std::endl;

    int passed = 0;
    int failed = 0;

    #define RUN_TEST(name) \
        try { \
            if (test_##name()) { \
                passed++; \
            } else { \
                failed++; \
            } \
        } catch (const std::exception& e) { \
            std::cerr << "  ✗ FAILED: Exception: " << e.what() << std::endl; \
            failed++; \
        } catch (...) { \
            std::cerr << "  ✗ FAILED: Unknown exception" << std::endl; \
            failed++; \
        }

    RUN_TEST(basic_adjacency_computation);
    RUN_TEST(no_adjacency);
    RUN_TEST(grid_adjacency);
    RUN_TEST(bidirectional_relationships);
    RUN_TEST(adaptive_tolerance);
    RUN_TEST(performance_large_dataset);
    RUN_TEST(border_length_accuracy);

    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    if (failed == 0) {
        std::cout << "\n✓ All integration tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗ Some integration tests failed!" << std::endl;
        return 1;
    }
}
