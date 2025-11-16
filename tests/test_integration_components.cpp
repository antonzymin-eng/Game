// ============================================================================
// test_integration_components.cpp - Comprehensive compilation test
// Created: November 15, 2025 - Verification of new integration components
// ============================================================================

#include "game/character/CharacterRelationships.h"
#include "game/religion/ReligionComponents.h"
#include "game/province/ProvinceAdjacency.h"
#include "game/realm/RealmComponents.h"
#include <iostream>
#include <cassert>

using namespace game;

void test_character_relationships() {
    std::cout << "Testing CharacterRelationshipsComponent...\n";

    // Create component
    character::CharacterRelationshipsComponent relationships(100);
    assert(relationships.character_id == 100);

    // Test marriage
    relationships.AddMarriage(200, 300, 400, true);
    assert(relationships.current_spouse == 200);
    assert(relationships.marriages.size() == 1);
    assert(relationships.IsMarriedTo(200));
    assert(relationships.HasMarriageTiesTo(300));

    // Test friendship
    relationships.SetRelationship(250, character::RelationshipType::FRIEND, 75, 80.0);
    assert(relationships.IsFriendsWith(250));
    assert(relationships.GetFriendshipBondStrength(250) == 80.0);

    std::cout << "  ✓ CharacterRelationshipsComponent passed\n";
}

void test_religion_components() {
    std::cout << "Testing ReligionComponents...\n";

    // Create religion system
    religion::ReligionSystemData religion_data;
    religion_data.InitializeDefaultFaiths();

    // Get a faith
    auto catholic_faith = religion_data.GetFaith(1);
    assert(catholic_faith != nullptr);

    // Create character religion
    religion::CharacterReligionComponent char_religion(100, 1);
    char_religion.piety = 75.0;
    char_religion.devotion = 80.0;
    char_religion.is_clergy = true;
    char_religion.clergy_rank = 5;

    double authority = char_religion.GetReligiousAuthority();
    assert(authority > 80.0);  // Should be high due to clergy status

    // Create realm religion
    religion::RealmReligionComponent realm_religion(200, 1);
    assert(realm_religion.IsStateFaith(1));
    assert(realm_religion.GetFaithPercentage(1) == 100.0);

    // Test faith compatibility
    assert(religion_data.AreSameFaith(1, 1));

    std::cout << "  ✓ ReligionComponents passed\n";
}

void test_province_adjacency() {
    std::cout << "Testing ProvinceAdjacency...\n";

    // Create adjacency manager
    province::ProvinceAdjacencyManager adjacency_manager;

    // Register provinces
    adjacency_manager.RegisterProvince(1);
    adjacency_manager.RegisterProvince(2);
    adjacency_manager.RegisterProvince(3);

    // Add adjacencies
    adjacency_manager.AddAdjacency(1, 2, province::BorderType::LAND, 10.0);
    adjacency_manager.AddAdjacency(2, 3, province::BorderType::RIVER, 5.0);

    // Check adjacency
    auto adj1 = adjacency_manager.GetAdjacency(1);
    assert(adj1 != nullptr);
    assert(adj1->IsAdjacentTo(2));
    assert(!adj1->IsAdjacentTo(3));  // Not directly adjacent

    // Update ownership
    adjacency_manager.UpdateProvinceOwnership(1, 100);
    adjacency_manager.UpdateProvinceOwnership(2, 200);
    adjacency_manager.UpdateProvinceOwnership(3, 200);

    // Check realm borders
    assert(adjacency_manager.RealmsShareBorder(100, 200));
    assert(!adjacency_manager.RealmsShareBorder(100, 300));

    // Get neighboring realms
    auto neighbors = adjacency_manager.GetNeighboringRealms(200);
    assert(neighbors.size() == 1);  // Only realm 100 borders realm 200

    std::cout << "  ✓ ProvinceAdjacency passed\n";
}

void test_component_inheritance() {
    std::cout << "Testing ECS Component inheritance...\n";

    // Verify components can be used as IComponent pointers
    auto char_rel = std::make_unique<character::CharacterRelationshipsComponent>(1);
    auto char_religion = std::make_unique<religion::CharacterReligionComponent>(1, 1);
    auto realm_religion = std::make_unique<religion::RealmReligionComponent>(1, 1);
    auto province_adj = std::make_unique<province::ProvinceAdjacencyComponent>(1);

    // Verify polymorphism works
    game::core::IComponent* base1 = char_rel.get();
    game::core::IComponent* base2 = char_religion.get();
    game::core::IComponent* base3 = realm_religion.get();
    game::core::IComponent* base4 = province_adj.get();

    assert(base1 != nullptr);
    assert(base2 != nullptr);
    assert(base3 != nullptr);
    assert(base4 != nullptr);

    std::cout << "  ✓ ECS Component inheritance passed\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Integration Components Verification Test                 ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    try {
        test_character_relationships();
        test_religion_components();
        test_province_adjacency();
        test_component_inheritance();

        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ✓ ALL VERIFICATION TESTS PASSED                          ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ TEST FAILED: " << e.what() << "\n";
        return 1;
    }
}
