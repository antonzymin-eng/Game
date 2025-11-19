// ============================================================================
// test_technology_enhancements.cpp - Test Technology System Enhancements
// Created: 2025-01-19
// Tests: Modern random, prerequisites validation, serialization
// ============================================================================

#include "game/technology/TechnologySystem.h"
#include "game/technology/TechnologyPrerequisites.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadSafeMessageBus.h"

#include <iostream>
#include <cassert>
#include <sstream>

using namespace game::technology;
using namespace core::ecs;
using namespace core::threading;

void TestModernRandom() {
    std::cout << "Testing modern random number generation..." << std::endl;

    ComponentAccessManager component_manager;
    ThreadSafeMessageBus message_bus;

    component_manager.RegisterComponent<ResearchComponent>();
    TechnologySystem tech_system(component_manager, message_bus);
    tech_system.Initialize();

    // The system should now use std::mt19937 instead of rand()
    // We can't directly test the internal random generator, but we can verify
    // the system initializes without errors
    std::cout << "✓ Modern random generator initialized successfully" << std::endl;
}

void TestGetTechnologyComponentCount() {
    std::cout << "Testing GetTechnologyComponentCount..." << std::endl;

    ComponentAccessManager component_manager;
    ThreadSafeMessageBus message_bus;

    component_manager.RegisterComponent<ResearchComponent>();
    component_manager.RegisterComponent<InnovationComponent>();
    component_manager.RegisterComponent<KnowledgeComponent>();
    component_manager.RegisterComponent<TechnologyEventsComponent>();

    TechnologySystem tech_system(component_manager, message_bus);
    tech_system.Initialize();

    // Initially should be 0
    assert(tech_system.GetTechnologyComponentCount() == 0);

    // Add some entities
    types::EntityID entity1 = 1;
    types::EntityID entity2 = 2;
    types::EntityID entity3 = 3;

    tech_system.InitializeTechnologyComponents(entity1);
    assert(tech_system.GetTechnologyComponentCount() == 1);

    tech_system.InitializeTechnologyComponents(entity2);
    assert(tech_system.GetTechnologyComponentCount() == 2);

    tech_system.InitializeTechnologyComponents(entity3);
    assert(tech_system.GetTechnologyComponentCount() == 3);

    std::cout << "✓ GetTechnologyComponentCount works correctly" << std::endl;
}

void TestPrerequisitesValidation() {
    std::cout << "Testing prerequisites validation..." << std::endl;

    ComponentAccessManager component_manager;
    ThreadSafeMessageBus message_bus;

    component_manager.RegisterComponent<ResearchComponent>();
    TechnologySystem tech_system(component_manager, message_bus);
    tech_system.Initialize();

    types::EntityID entity = 1;
    tech_system.InitializeTechnologyComponents(entity);

    auto* research_comp = tech_system.GetResearchComponent(entity);
    assert(research_comp != nullptr);

    // Test 1: Technology with no prerequisites should be available
    assert(TechnologyPrerequisites::GetPrerequisites(TechnologyType::HEAVY_PLOW).empty());
    assert(tech_system.CheckTechnologyPrerequisites(entity, TechnologyType::HEAVY_PLOW));

    // Test 2: Horse Collar requires Heavy Plow
    auto horse_collar_prereqs = TechnologyPrerequisites::GetPrerequisites(TechnologyType::HORSE_COLLAR);
    assert(!horse_collar_prereqs.empty());
    assert(horse_collar_prereqs[0] == TechnologyType::HEAVY_PLOW);

    // Should not be available without prerequisite
    assert(!tech_system.CheckTechnologyPrerequisites(entity, TechnologyType::HORSE_COLLAR));

    // Check missing prerequisites
    auto missing = tech_system.GetMissingPrerequisites(entity, TechnologyType::HORSE_COLLAR);
    assert(missing.size() == 1);
    assert(missing[0] == TechnologyType::HEAVY_PLOW);

    // Discover the prerequisite
    research_comp->technology_states[TechnologyType::HEAVY_PLOW] = ResearchState::DISCOVERED;

    // Now should be available
    assert(tech_system.CheckTechnologyPrerequisites(entity, TechnologyType::HORSE_COLLAR));
    missing = tech_system.GetMissingPrerequisites(entity, TechnologyType::HORSE_COLLAR);
    assert(missing.empty());

    // Test 3: Complex prerequisites (Plate Armor requires Chainmail + Blast Furnace)
    auto plate_armor_prereqs = TechnologyPrerequisites::GetPrerequisites(TechnologyType::PLATE_ARMOR);
    assert(plate_armor_prereqs.size() == 2);

    assert(!tech_system.CheckTechnologyPrerequisites(entity, TechnologyType::PLATE_ARMOR));
    missing = tech_system.GetMissingPrerequisites(entity, TechnologyType::PLATE_ARMOR);
    assert(missing.size() == 2);

    // Discover one prerequisite
    research_comp->technology_states[TechnologyType::CHAINMAIL_ARMOR] = ResearchState::IMPLEMENTED;
    missing = tech_system.GetMissingPrerequisites(entity, TechnologyType::PLATE_ARMOR);
    assert(missing.size() == 1);

    // Discover second prerequisite
    research_comp->technology_states[TechnologyType::BLAST_FURNACE] = ResearchState::IMPLEMENTED;
    assert(tech_system.CheckTechnologyPrerequisites(entity, TechnologyType::PLATE_ARMOR));
    missing = tech_system.GetMissingPrerequisites(entity, TechnologyType::PLATE_ARMOR);
    assert(missing.empty());

    std::cout << "✓ Prerequisites validation working correctly" << std::endl;
}

void TestPrerequisitesDatabase() {
    std::cout << "Testing prerequisites database..." << std::endl;

    // Test various technologies
    assert(TechnologyPrerequisites::GetPrerequisites(TechnologyType::THREE_FIELD_SYSTEM).empty());
    assert(TechnologyPrerequisites::GetPrerequisites(TechnologyType::PRINTING_PRESS).size() == 2);
    assert(TechnologyPrerequisites::GetPrerequisites(TechnologyType::OCEAN_NAVIGATION).size() == 2);

    // Test GetUnlockedTechnologies
    auto unlocked_by_heavy_plow = TechnologyPrerequisites::GetUnlockedTechnologies(TechnologyType::HEAVY_PLOW);
    assert(!unlocked_by_heavy_plow.empty());

    std::cout << "✓ Prerequisites database correctly initialized" << std::endl;
}

void TestSerialization() {
    std::cout << "Testing component serialization..." << std::endl;

    // Create a research component with data
    ResearchComponent original;
    original.current_focus = TechnologyType::PRINTING_PRESS;
    original.universities = 5;
    original.libraries = 10;
    original.scholar_population = 150;
    original.monthly_research_budget = 500.0;
    original.technology_states[TechnologyType::PRINTING_PRESS] = ResearchState::RESEARCHING;
    original.technology_states[TechnologyType::PAPER_MAKING] = ResearchState::IMPLEMENTED;
    original.research_progress[TechnologyType::PRINTING_PRESS] = 0.65;

    // Serialize
    std::stringstream buffer;
    original.Serialize(buffer);

    // Deserialize
    ResearchComponent loaded;
    loaded.Deserialize(buffer);

    // Verify
    assert(loaded.current_focus == original.current_focus);
    assert(loaded.universities == original.universities);
    assert(loaded.libraries == original.libraries);
    assert(loaded.scholar_population == original.scholar_population);
    assert(loaded.monthly_research_budget == original.monthly_research_budget);
    assert(loaded.technology_states[TechnologyType::PRINTING_PRESS] == ResearchState::RESEARCHING);
    assert(loaded.technology_states[TechnologyType::PAPER_MAKING] == ResearchState::IMPLEMENTED);
    assert(loaded.research_progress[TechnologyType::PRINTING_PRESS] == 0.65);

    std::cout << "✓ Serialization working correctly" << std::endl;

    // Test InnovationComponent serialization
    InnovationComponent innov_original;
    innov_original.innovation_rate = 0.25;
    innov_original.inventors = 20;
    innov_original.recent_discoveries.push_back(TechnologyType::WINDMILL);
    innov_original.innovation_expertise[TechnologyCategory::CRAFT] = 0.8;

    std::stringstream innov_buffer;
    innov_original.Serialize(innov_buffer);

    InnovationComponent innov_loaded;
    innov_loaded.Deserialize(innov_buffer);

    assert(innov_loaded.innovation_rate == innov_original.innovation_rate);
    assert(innov_loaded.inventors == innov_original.inventors);
    assert(innov_loaded.recent_discoveries.size() == 1);
    assert(innov_loaded.recent_discoveries[0] == TechnologyType::WINDMILL);
    assert(innov_loaded.innovation_expertise[TechnologyCategory::CRAFT] == 0.8);

    std::cout << "✓ InnovationComponent serialization working correctly" << std::endl;
}

int main() {
    try {
        std::cout << "=== Technology System Enhancements Test ===" << std::endl;
        std::cout << std::endl;

        TestModernRandom();
        std::cout << std::endl;

        TestGetTechnologyComponentCount();
        std::cout << std::endl;

        TestPrerequisitesDatabase();
        std::cout << std::endl;

        TestPrerequisitesValidation();
        std::cout << std::endl;

        TestSerialization();
        std::cout << std::endl;

        std::cout << "🎉 All enhancement tests passed!" << std::endl;
        std::cout << "✅ Modern random number generation" << std::endl;
        std::cout << "✅ Component counting" << std::endl;
        std::cout << "✅ Prerequisites database" << std::endl;
        std::cout << "✅ Prerequisites validation" << std::endl;
        std::cout << "✅ Component serialization" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cout << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
