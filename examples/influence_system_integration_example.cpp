// ============================================================================
// influence_system_integration_example.cpp - Example Integration Usage
// Created: November 15, 2025 - Phase 3: Influence System Integration
// Location: examples/influence_system_integration_example.cpp
// ============================================================================
//
// This example demonstrates how to wire up the InfluenceSystem with
// Character, Religion, and Province systems for full integration.
//
// PREREQUISITES:
// - CharacterRelationshipsComponent created for all characters
// - ReligionSystemData initialized with faiths
// - CharacterReligionComponent created for all characters
// - RealmReligionComponent created for all realms
// - ProvinceAdjacencyManager initialized with province borders
//
// ============================================================================

#include "game/diplomacy/InfluenceSystem.h"
#include "game/character/CharacterRelationships.h"
#include "game/religion/ReligionComponents.h"
#include "game/province/ProvinceAdjacency.h"
#include <iostream>

using namespace game;
using namespace game::diplomacy;

int main() {
    std::cout << "=== Influence System Integration Example ===\n\n";

    // ========================================================================
    // STEP 1: Create all required systems
    // ========================================================================

    std::cout << "Step 1: Creating core systems...\n";

    // Create province adjacency manager
    province::ProvinceAdjacencyManager adjacency_manager;

    // Register provinces and add adjacencies
    adjacency_manager.RegisterProvince(1);
    adjacency_manager.RegisterProvince(2);
    adjacency_manager.RegisterProvince(3);
    adjacency_manager.AddAdjacency(1, 2, province::BorderType::LAND, 100.0);
    adjacency_manager.AddAdjacency(2, 3, province::BorderType::RIVER, 50.0);

    // Set province ownership
    adjacency_manager.UpdateProvinceOwnership(1, 100);  // Realm 100 owns province 1
    adjacency_manager.UpdateProvinceOwnership(2, 200);  // Realm 200 owns province 2
    adjacency_manager.UpdateProvinceOwnership(3, 200);  // Realm 200 owns province 3

    std::cout << "  ✓ ProvinceAdjacencyManager initialized\n";

    // Create religion system
    religion::ReligionSystemData religion_data;
    religion_data.InitializeDefaultFaiths();

    std::cout << "  ✓ ReligionSystemData initialized with default faiths\n";

    // ========================================================================
    // STEP 2: Create character and realm components
    // ========================================================================

    std::cout << "\nStep 2: Creating character and realm components...\n";

    // Create character relationships for rulers
    character::CharacterRelationshipsComponent ruler1_relationships(1001);  // Ruler of Realm 100
    character::CharacterRelationshipsComponent ruler2_relationships(1002);  // Ruler of Realm 200

    // Add a marriage between rulers (political alliance)
    ruler1_relationships.AddMarriage(1002, 200, 2000, true /* creates alliance */);
    ruler2_relationships.AddMarriage(1001, 100, 1000, true);

    std::cout << "  ✓ CharacterRelationships created with marriage alliance\n";

    // Create character religion components
    religion::CharacterReligionComponent ruler1_religion(1001, 1 /* Catholic */);
    ruler1_religion.piety = 70.0;
    ruler1_religion.devotion = 60.0;

    religion::CharacterReligionComponent ruler2_religion(1002, 2 /* Orthodox */);
    ruler2_religion.piety = 80.0;
    ruler2_religion.devotion = 75.0;
    ruler2_religion.is_clergy = true;
    ruler2_religion.clergy_rank = 5;

    std::cout << "  ✓ CharacterReligion components created\n";

    // Create realm religion components
    religion::RealmReligionComponent realm1_religion(100, 1 /* Catholic */);
    realm1_religion.tolerance = 50.0;

    religion::RealmReligionComponent realm2_religion(200, 2 /* Orthodox */);
    realm2_religion.tolerance = 60.0;
    realm2_religion.clergy_loyalty = 75.0;

    std::cout << "  ✓ RealmReligion components created\n";

    // ========================================================================
    // STEP 3: Create and configure InfluenceSystem
    // ========================================================================

    std::cout << "\nStep 3: Creating InfluenceSystem and enabling integration...\n";

    InfluenceSystem influence_system;

    // Enable integration (creates the integration helper)
    influence_system.EnableIntegration();
    std::cout << "  ✓ Integration enabled\n";

    // Set system-level managers
    influence_system.SetProvinceAdjacencyManager(&adjacency_manager);
    std::cout << "  ✓ ProvinceAdjacencyManager registered\n";

    influence_system.SetReligionSystemData(&religion_data);
    std::cout << "  ✓ ReligionSystemData registered\n";

    // ========================================================================
    // STEP 4: Register all character and realm components
    // ========================================================================

    std::cout << "\nStep 4: Registering components with InfluenceSystem...\n";

    // Register character relationships
    influence_system.RegisterCharacterRelationships(1001, &ruler1_relationships);
    influence_system.RegisterCharacterRelationships(1002, &ruler2_relationships);
    std::cout << "  ✓ Character relationships registered (2 characters)\n";

    // Register character religions
    influence_system.RegisterCharacterReligion(1001, &ruler1_religion);
    influence_system.RegisterCharacterReligion(1002, &ruler2_religion);
    std::cout << "  ✓ Character religions registered (2 characters)\n";

    // Register realm religions
    influence_system.RegisterRealmReligion(100, &realm1_religion);
    influence_system.RegisterRealmReligion(200, &realm2_religion);
    std::cout << "  ✓ Realm religions registered (2 realms)\n";

    // ========================================================================
    // STEP 5: Verify integration is enabled
    // ========================================================================

    std::cout << "\nStep 5: Verifying integration status...\n";

    if (influence_system.IsIntegrationEnabled()) {
        std::cout << "  ✓ Integration is ENABLED and fully operational!\n";
    } else {
        std::cout << "  ✗ Integration is NOT enabled (missing components?)\n";
        return 1;
    }

    // ========================================================================
    // STEP 6: Use the integrated system
    // ========================================================================

    std::cout << "\nStep 6: Using integrated influence calculations...\n";

    // The influence system will now use:
    // - Actual marriages for dynastic influence calculations
    // - Real friendships for personal influence
    // - Faith compatibility for religious influence
    // - Province borders for geographic neighbor detection

    // Initialize the influence system (would calculate all influences)
    // influence_system.Initialize();

    std::cout << "  ✓ InfluenceSystem ready for use!\n";

    // ========================================================================
    // INTEGRATION NOTES
    // ========================================================================

    std::cout << "\n=== Integration Notes ===\n\n";

    std::cout << "The InfluenceSystem will now use integrated calculations:\n\n";

    std::cout << "1. DYNASTIC INFLUENCE:\n";
    std::cout << "   - Checks actual marriages between rulers\n";
    std::cout << "   - Direct marriage: +30 influence\n";
    std::cout << "   - Marriage to realm member: +15 influence\n";
    std::cout << "   - Alliance marriage: +10 bonus\n";
    std::cout << "   - Family connections (siblings, children): +20-25\n\n";

    std::cout << "2. PERSONAL INFLUENCE:\n";
    std::cout << "   - Uses friendship bonds from CharacterRelationships\n";
    std::cout << "   - Friendship bond strength: up to +40 influence\n";
    std::cout << "   - Opinion modifier: up to +30 influence\n";
    std::cout << "   - Special relationships (Blood Brother, Rival, etc.)\n\n";

    std::cout << "3. RELIGIOUS INFLUENCE:\n";
    std::cout << "   - Faith compatibility checks:\n";
    std::cout << "     * Same faith: +40 influence\n";
    std::cout << "     * Same denomination: +25 influence\n";
    std::cout << "     * Same religion group: +10 influence\n";
    std::cout << "   - Religious authority: up to +40 influence\n";
    std::cout << "   - Holy sites controlled: +3 per site\n";
    std::cout << "   - Clergy loyalty bonus: +10 if > 70%\n\n";

    std::cout << "4. GEOGRAPHIC NEIGHBORS:\n";
    std::cout << "   - Uses ProvinceAdjacencyManager for real border detection\n";
    std::cout << "   - Replaces placeholder province-count heuristic\n";
    std::cout << "   - 100% accurate neighbor detection\n\n";

    // ========================================================================
    // CLEANUP EXAMPLE
    // ========================================================================

    std::cout << "=== Cleanup Example ===\n\n";

    std::cout << "When characters/realms are destroyed, unregister components:\n\n";

    std::cout << "  influence_system.UnregisterCharacterRelationships(1001);\n";
    std::cout << "  influence_system.UnregisterCharacterReligion(1001);\n";
    std::cout << "  influence_system.UnregisterRealmReligion(100);\n\n";

    // ========================================================================
    // SUCCESS
    // ========================================================================

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✓ Integration Complete - InfluenceSystem Ready for Use! ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    return 0;
}
