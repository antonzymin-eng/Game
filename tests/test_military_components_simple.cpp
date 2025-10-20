// ============================================================================
// test_military_components_simple.cpp - Simple Military Components Test
// Created: October 11, 2025 - Military Components Validation
// Location: src/test_military_components_simple.cpp
// ============================================================================

#include "game/military/MilitaryComponents.h"
#include <iostream>
#include <cassert>

using namespace game::military;

void TestMilitaryComponents() {
    std::cout << "[MilitaryComponentTest] Testing Military Components directly" << std::endl;

    // Test MilitaryComponent creation
    MilitaryComponent military_comp;
    military_comp.recruitment_capacity = 1000;
    military_comp.training_facilities = 0.5;
    military_comp.military_budget = 200.0;

    assert(military_comp.recruitment_capacity == 1000);
    assert(military_comp.training_facilities == 0.5);
    assert(military_comp.military_budget == 200.0);

    std::cout << "[MilitaryComponentTest] ✅ MilitaryComponent test passed" << std::endl;

    // Test ArmyComponent creation
    ArmyComponent army_comp;
    army_comp.army_name = "test_army_001";
    army_comp.commander_id = 12345;
    army_comp.total_strength = 500;
    army_comp.supply_level = 0.8;

    assert(army_comp.army_name == "test_army_001");
    assert(army_comp.commander_id == 12345);
    assert(army_comp.total_strength == 500);
    assert(army_comp.supply_level == 0.8);

    std::cout << "[MilitaryComponentTest] ✅ ArmyComponent test passed" << std::endl;

    // Test FortificationComponent creation  
    FortificationComponent fort_comp;
    fort_comp.garrison_capacity = 200;
    fort_comp.siege_resistance = 0.3;
    fort_comp.structural_integrity = 0.9;

    assert(fort_comp.garrison_capacity == 200);
    assert(fort_comp.siege_resistance == 0.3);
    assert(fort_comp.structural_integrity == 0.9);

    std::cout << "[MilitaryComponentTest] ✅ FortificationComponent test passed" << std::endl;

    // Test CombatComponent creation
    CombatComponent combat_comp;
    combat_comp.battle_name = "Test Battle";
    combat_comp.attacker_casualties = 100;
    combat_comp.defender_casualties = 150;
    combat_comp.battle_duration = 2.5;

    assert(combat_comp.battle_name == "Test Battle");
    assert(combat_comp.attacker_casualties == 100);
    assert(combat_comp.defender_casualties == 150);
    assert(combat_comp.battle_duration == 2.5);

    std::cout << "[MilitaryComponentTest] ✅ CombatComponent test passed" << std::endl;

    // Test MilitaryEventsComponent creation
    MilitaryEventsComponent events_comp;
    events_comp.recruitment_drives.push_back("Recruited 50 spearmen");
    events_comp.battle_history.push_back("Victory at River Crossing");
    events_comp.supply_disruptions.push_back("Supply train delayed");

    assert(events_comp.recruitment_drives.size() == 1);
    assert(events_comp.battle_history.size() == 1);
    assert(events_comp.supply_disruptions.size() == 1);
    assert(events_comp.recruitment_drives[0] == "Recruited 50 spearmen");

    std::cout << "[MilitaryComponentTest] ✅ MilitaryEventsComponent test passed" << std::endl;

    // Test MilitaryUnit structure
    MilitaryUnit unit;
    unit.type = UnitType::SPEARMEN;
    unit.max_strength = 100;
    unit.current_strength = 85;
    unit.experience = 0.4;
    unit.equipment_quality = 0.6;

    assert(unit.type == UnitType::SPEARMEN);
    assert(unit.max_strength == 100);
    assert(unit.current_strength == 85);
    assert(unit.experience == 0.4);
    assert(unit.equipment_quality == 0.6);

    std::cout << "[MilitaryComponentTest] ✅ MilitaryUnit structure test passed" << std::endl;

    // Test Commander structure
    Commander commander;
    commander.name = "Sir William";
    commander.martial_skill = 0.8;
    commander.charisma = 0.7;
    commander.loyalty = 0.9;
    commander.experience = 0.65;

    assert(commander.name == "Sir William");
    assert(commander.martial_skill == 0.8);
    assert(commander.charisma == 0.7);
    assert(commander.loyalty == 0.9);
    assert(commander.experience == 0.65);

    std::cout << "[MilitaryComponentTest] ✅ Commander structure test passed" << std::endl;

    std::cout << "[MilitaryComponentTest] ✅ ALL MILITARY COMPONENT TESTS PASSED" << std::endl;
}

int main() {
    try {
        TestMilitaryComponents();
        std::cout << "✅ Military Components Test Suite: SUCCESS" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "❌ Military Components Test Suite: FAILED - " << e.what() << std::endl;
        return 1;
    }
}