// ============================================================================
// test_religion_improvements.cpp - Test improvements to ReligionComponents
// ============================================================================

#include "game/religion/ReligionComponents.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace game::religion;

void test_demographics_normalization() {
    std::cout << "Testing demographics normalization...\n";

    RealmReligionComponent realm(1, 1);

    // Manually set percentages that don't sum to 100
    realm.faith_demographics[1] = 40.0;  // Catholic
    realm.faith_demographics[2] = 30.0;  // Orthodox
    realm.faith_demographics[3] = 20.0;  // Protestant
    // Total = 90%, missing 10%

    std::cout << "  Before normalization:\n";
    double total_before = 0.0;
    for (const auto& [faith_id, pct] : realm.faith_demographics) {
        std::cout << "    Faith " << faith_id << ": " << pct << "%\n";
        total_before += pct;
    }
    std::cout << "    Total: " << total_before << "%\n";

    // Normalize
    realm.NormalizeDemographics();

    std::cout << "  After normalization:\n";
    double total_after = 0.0;
    for (const auto& [faith_id, pct] : realm.faith_demographics) {
        std::cout << "    Faith " << faith_id << ": " << pct << "%\n";
        total_after += pct;
    }
    std::cout << "    Total: " << total_after << "%\n";

    // Verify total is 100%
    assert(std::abs(total_after - 100.0) < 0.01);
    std::cout << "  ✓ Demographics normalized correctly to 100%\n";
}

void test_set_faith_percentage() {
    std::cout << "\nTesting SetFaithPercentage auto-normalization...\n";

    RealmReligionComponent realm(1, 1);

    // Set percentages using the new method
    realm.faith_demographics[1] = 50.0;
    realm.faith_demographics[2] = 30.0;

    // Add a new faith with SetFaithPercentage
    realm.SetFaithPercentage(3, 20.0);  // Should auto-normalize

    std::cout << "  Demographics after SetFaithPercentage:\n";
    double total = 0.0;
    for (const auto& [faith_id, pct] : realm.faith_demographics) {
        std::cout << "    Faith " << faith_id << ": " << pct << "%\n";
        total += pct;
    }
    std::cout << "    Total: " << total << "%\n";

    assert(std::abs(total - 100.0) < 0.01);
    std::cout << "  ✓ SetFaithPercentage auto-normalizes correctly\n";
}

void test_zero_total_handling() {
    std::cout << "\nTesting zero total edge case...\n";

    RealmReligionComponent realm(1, 5);  // State faith = 5

    // Clear initial demographics and set all to zero (invalid state)
    realm.faith_demographics.clear();
    realm.faith_demographics[1] = 0.0;
    realm.faith_demographics[2] = 0.0;

    std::cout << "  Before normalization (total = 0):\n";
    for (const auto& [faith_id, pct] : realm.faith_demographics) {
        std::cout << "    Faith " << faith_id << ": " << pct << "%\n";
    }

    // Normalize should reset to 100% state faith
    realm.NormalizeDemographics();

    std::cout << "  After normalizing zero total:\n";
    for (const auto& [faith_id, pct] : realm.faith_demographics) {
        std::cout << "    Faith " << faith_id << ": " << pct << "%\n";
    }
    std::cout << "  Demographics size: " << realm.faith_demographics.size() << "\n";

    assert(realm.faith_demographics.size() == 1);
    assert(realm.faith_demographics[5] == 100.0);
    std::cout << "  ✓ Zero total correctly reset to 100% state faith\n";
}

void test_already_normalized() {
    std::cout << "\nTesting already normalized demographics...\n";

    RealmReligionComponent realm(1, 1);

    // Set demographics that already sum to 100%
    realm.faith_demographics[1] = 60.0;
    realm.faith_demographics[2] = 40.0;

    // Store values before normalization
    double pct1_before = realm.faith_demographics[1];
    double pct2_before = realm.faith_demographics[2];

    // Normalize (should not change anything)
    realm.NormalizeDemographics();

    // Verify no change
    assert(realm.faith_demographics[1] == pct1_before);
    assert(realm.faith_demographics[2] == pct2_before);

    std::cout << "  ✓ Already normalized demographics unchanged\n";
}

void test_faith_id_documentation() {
    std::cout << "\nTesting faith ID permanence (documented behavior)...\n";

    ReligionSystemData religion_data;

    // Register several faiths
    auto id1 = religion_data.RegisterFaith("Faith1", ReligionGroup::CUSTOM);
    auto id2 = religion_data.RegisterFaith("Faith2", ReligionGroup::CUSTOM);
    auto id3 = religion_data.RegisterFaith("Faith3", ReligionGroup::CUSTOM);

    std::cout << "  Registered faith IDs: " << id1 << ", " << id2 << ", " << id3 << "\n";

    // Verify IDs are sequential
    assert(id2 == id1 + 1);
    assert(id3 == id2 + 1);

    std::cout << "  ✓ Faith IDs are permanent and auto-incrementing\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Religion System Improvements Test                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    try {
        test_demographics_normalization();
        test_set_faith_percentage();
        test_zero_total_handling();
        test_already_normalized();
        test_faith_id_documentation();

        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ✓ ALL IMPROVEMENT TESTS PASSED                           ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
