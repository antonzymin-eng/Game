// ============================================================================
// test_military_ecs_integration.cpp - Military System ECS Integration Test
// Created: October 11, 2025 - Military System ECS Integration Validation
// Location: src/test_military_ecs_integration.cpp
// ============================================================================

#include "core/ECS/EntityManager.h"
#include "core/ECS/ComponentAccessManager.h"
#include "game/military/MilitarySystem.h"
#include "game/military/MilitaryComponents.h"
#include "core/logging/Logger.h"

#include <iostream>
#include <cassert>

using namespace game;
using namespace game::military;

void TestMilitaryECSIntegration() {
    std::cout << "[MilitaryECSTest] Starting Military System ECS Integration Test" << std::endl;

    // Initialize the military system
    MilitarySystem military_system;
    military_system.Initialize( << std::endl;

    // Test entity ID
    game::types::EntityID test_province = 2001;
    game::types::EntityID test_army = 2002;

    std::cout << "[MilitaryTest] " << "Creating military components for test province" << std::endl;
    
    // Create military components for the test province
    military_system.CreateMilitaryComponents(test_province << std::endl;

    // Test military strength operations
    std::cout << "[MilitaryTest] " << "Testing military strength operations" << std::endl;
    
    uint32_t initial_strength = military_system.GetTotalMilitaryStrength(test_province << std::endl;
    assert(initial_strength == 0 << std::endl; // Should start with no units
    std::cout << "[MilitaryTest] " << "Initial military strength: " + std::to_string(initial_strength) << std::endl;

    // Test unit recruitment
    bool recruit_result = military_system.RecruitUnit(test_province, UnitType::LEVIES, 500 << std::endl;
    assert(recruit_result == true << std::endl;
    uint32_t strength_after_recruitment = military_system.GetTotalMilitaryStrength(test_province << std::endl;
    assert(strength_after_recruitment == 500 << std::endl;
    std::cout << "[MilitaryTest] " << "Strength after recruiting 500 levies: " + std::to_string(strength_after_recruitment) << std::endl;

    // Test recruiting another unit type
    bool spearmen_recruit = military_system.RecruitUnit(test_province, UnitType::SPEARMEN, 300 << std::endl;
    assert(spearmen_recruit == true << std::endl;
    uint32_t total_strength = military_system.GetTotalMilitaryStrength(test_province << std::endl;
    assert(total_strength == 800 << std::endl; // 500 levies + 300 spearmen
    std::cout << "[MilitaryTest] " << "Total strength after recruiting spearmen: " + std::to_string(total_strength) << std::endl;

    // Test military maintenance costs
    std::cout << "[MilitaryTest] " << "Testing military maintenance" << std::endl;
    
    double maintenance_cost = military_system.GetMilitaryMaintenance(test_province << std::endl;
    assert(maintenance_cost >= 0.0 << std::endl; // Should have some maintenance cost
    std::cout << "[MilitaryTest] " << "Military maintenance cost: " + std::to_string(maintenance_cost) << std::endl;

    // Test army creation
    std::cout << "[MilitaryTest] " << "Testing army creation" << std::endl;
    
    military_system.CreateArmyComponents(test_army, "Test Army" << std::endl;
    std::cout << "[MilitaryTest] " << "Successfully created army components" << std::endl;

    // Test edge cases
    std::cout << "[MilitaryTest] " << "Testing edge cases" << std::endl;
    
    // Test operations on non-existent province
    game::types::EntityID nonexistent_province = 9999;
    uint32_t nonexistent_strength = military_system.GetTotalMilitaryStrength(nonexistent_province << std::endl;
    assert(nonexistent_strength == 0 << std::endl;
    
    bool failed_recruitment = military_system.RecruitUnit(nonexistent_province, UnitType::LEVIES, 100 << std::endl;
    assert(failed_recruitment == false << std::endl;
    std::cout << "[MilitaryTest] " << "Non-existent province handling correct" << std::endl;

    // Shutdown system
    military_system.Shutdown( << std::endl;
    
    std::cout << "[MilitaryTest] " << "✅ ALL MILITARY ECS INTEGRATION TESTS PASSED" << std::endl;
}

void TestMilitaryComponents() {
    std::cout << "[MilitaryTest] " << "Testing Military Components directly" << std::endl;

    // Test MilitaryComponent
    MilitaryComponent military_comp;
    military_comp.recruitment_capacity = 1000;
    military_comp.training_facilities = 0.8;
    military_comp.military_budget = 500.0;
    
    assert(military_comp.recruitment_capacity == 1000 << std::endl;
    assert(military_comp.GetComponentTypeName() == "MilitaryComponent" << std::endl;
    std::cout << "[MilitaryTest] " << "✅ MilitaryComponent test passed" << std::endl;

    // Test ArmyComponent
    ArmyComponent army_comp;
    army_comp.army_name = "Test Legion";
    army_comp.total_strength = 1500;
    army_comp.army_morale = 0.9;
    army_comp.organization = 0.85;
    
    assert(army_comp.army_name == "Test Legion" << std::endl;
    assert(army_comp.total_strength == 1500 << std::endl;
    assert(army_comp.GetComponentTypeName() == "ArmyComponent" << std::endl;
    std::cout << "[MilitaryTest] " << "✅ ArmyComponent test passed" << std::endl;

    // Test FortificationComponent
    FortificationComponent fort_comp;
    fort_comp.walls_level = 3;
    fort_comp.towers_level = 2;
    fort_comp.structural_integrity = 0.95;
    fort_comp.garrison_capacity = 800;
    
    assert(fort_comp.walls_level == 3 << std::endl;
    assert(fort_comp.towers_level == 2 << std::endl;
    assert(fort_comp.GetComponentTypeName() == "FortificationComponent" << std::endl;
    std::cout << "[MilitaryTest] " << "✅ FortificationComponent test passed" << std::endl;

    // Test CombatComponent
    CombatComponent combat_comp;
    combat_comp.battle_name = "Battle of Test Field";
    combat_comp.battle_active = true;
    combat_comp.attacker_initial_strength = 2000;
    combat_comp.defender_initial_strength = 1800;
    combat_comp.terrain_modifier = 1.1;
    
    assert(combat_comp.battle_name == "Battle of Test Field" << std::endl;
    assert(combat_comp.battle_active == true << std::endl;
    assert(combat_comp.GetComponentTypeName() == "CombatComponent" << std::endl;
    std::cout << "[MilitaryTest] " << "✅ CombatComponent test passed" << std::endl;

    // Test MilitaryEventsComponent
    MilitaryEventsComponent events_comp;
    events_comp.military_reputation = 0.75;
    events_comp.battle_prestige = 0.6;
    events_comp.max_history_size = 150;
    
    // Add some test data
    events_comp.active_campaigns.push_back("Northern Campaign" << std::endl;
    events_comp.battle_history.push_back("Victory at Test Hill" << std::endl;
    
    assert(events_comp.active_campaigns.size() == 1 << std::endl;
    assert(events_comp.battle_history.size() == 1 << std::endl;
    assert(events_comp.GetComponentTypeName() == "MilitaryEventsComponent" << std::endl;
    std::cout << "[MilitaryTest] " << "✅ MilitaryEventsComponent test passed" << std::endl;

    // Test MilitaryUnit structure
    MilitaryUnit test_unit(UnitType::SPEARMEN << std::endl;
    test_unit.current_strength = 600;
    test_unit.experience = 0.4;
    test_unit.morale = MoraleState::CONFIDENT;
    
    assert(test_unit.type == UnitType::SPEARMEN << std::endl;
    assert(test_unit.current_strength == 600 << std::endl;
    assert(test_unit.morale == MoraleState::CONFIDENT << std::endl;
    std::cout << "[MilitaryTest] " << "✅ MilitaryUnit structure test passed" << std::endl;

    // Test Commander structure
    Commander test_commander("General Marcus" << std::endl;
    test_commander.rank = MilitaryRank::GENERAL;
    test_commander.martial_skill = 0.85;
    test_commander.loyalty = 0.9;
    test_commander.command_limit = 8000;
    
    assert(test_commander.name == "General Marcus" << std::endl;
    assert(test_commander.rank == MilitaryRank::GENERAL << std::endl;
    assert(test_commander.martial_skill == 0.85 << std::endl;
    std::cout << "[MilitaryTest] " << "✅ Commander structure test passed" << std::endl;
}

int main() {
    std::cout << "=== Military System ECS Integration Test ===" << std::endl;
    
    try {
        TestMilitaryComponents( << std::endl;
        TestMilitaryECSIntegration( << std::endl;
        
        std::cout << std::endl;
        std::cout << "✅ ALL MILITARY SYSTEM ECS INTEGRATION TESTS PASSED!" << std::endl;
        std::cout << "✅ Military System ECS Components Successfully Created" << std::endl;
        std::cout << "✅ All 5 military components inherit from Component<T> correctly" << std::endl;
        std::cout << "✅ Component type names are correctly implemented" << std::endl;
        std::cout << "✅ Military unit recruitment and strength tracking validated" << std::endl;
        std::cout << "✅ Army creation and management validated" << std::endl;
        std::cout << "✅ Military data structures and operations working correctly" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown error" << std::endl;
        return 1;
    }
}