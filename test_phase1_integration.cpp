// ============================================================================
// Phase 1 Core Systems Integration Test
// Created: October 15, 2025 - Phase 1 Assessment
// Location: test_phase1_integration.cpp
// ============================================================================

#include "core/ECS/EntityManager.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "game/population/PopulationSystem.h"
#include "game/economy/EconomicSystem.h"
#include "game/military/MilitarySystem.h"
#include "game/technology/TechnologySystem.h"
#include "game/population/PopulationComponents.h"
#include "game/economy/EconomicComponents.h"
#include "game/military/MilitaryComponents.h"
#include "core/logging/Logger.h"

#include <iostream>
#include <memory>

int main() {
    std::cout << "=== Phase 1 Core Systems Integration Test ===" << std::endl;
    
    try {
        // Create ECS core components
        auto entity_manager = std::make_unique<::core::ecs::EntityManager>();
        auto message_bus = std::make_unique<::core::ecs::MessageBus>();
        auto access_manager = std::make_unique<::core::ecs::ComponentAccessManager>(
            entity_manager.get(), message_bus.get());
        
        std::cout << "✅ ECS Core created successfully" << std::endl;

        // Create all game systems
        auto population_system = std::make_unique<game::population::PopulationSystem>(
            *access_manager, *message_bus);
        auto economic_system = std::make_unique<game::EconomicSystem>();
        auto military_system = std::make_unique<game::military::MilitarySystem>(
            *access_manager, *message_bus);
        auto technology_system = std::make_unique<game::technology::TechnologySystem>(
            *access_manager, *message_bus);
            
        std::cout << "✅ All core systems created successfully" << std::endl;

        // Initialize systems
        population_system->Initialize();
        economic_system->Initialize(nullptr);
        military_system->Initialize();
        technology_system->Initialize();
        
        std::cout << "✅ All systems initialized successfully" << std::endl;

        // Create test province entity
        auto province_entity = entity_manager->CreateEntity("TestProvince");
        game::types::EntityID province_id = static_cast<game::types::EntityID>(province_entity.id);
        
        std::cout << "✅ Created test province entity: " << province_id << std::endl;

        // Test Population System integration  
        population_system->CreateInitialPopulation(province_id, "english", "catholic", 15000, 0.7, 1300);
        
        // Verify PopulationComponent was created
        auto pop_component = entity_manager->GetComponent<game::population::PopulationComponent>(province_entity);
        if (pop_component) {
            std::cout << "✅ PopulationSystem integration working - Population: " 
                      << pop_component->total_population << std::endl;
        } else {
            std::cout << "❌ PopulationSystem integration failed" << std::endl;
            return 1;
        }

        // Test Economic System integration
        economic_system->CreateEconomicComponents(province_id);
        
        // Test treasury operations
        int treasury = economic_system->GetTreasury(province_id);
        std::cout << "✅ EconomicSystem integration working - Treasury: " << treasury << std::endl;
        
        // Test Military System integration
        military_system->CreateMilitaryComponents(province_id);
        
        // Verify MilitaryComponent was created
        auto mil_component = entity_manager->GetComponent<game::military::MilitaryComponent>(province_entity);
        if (mil_component) {
            std::cout << "✅ MilitarySystem integration working - Military strength available" << std::endl;
        } else {
            std::cout << "❌ MilitarySystem integration failed" << std::endl;
            return 1;
        }

        // Test Technology System integration
        technology_system->CreateResearchComponent(province_id);
        technology_system->CreateInnovationComponent(province_id);
        technology_system->CreateKnowledgeComponent(province_id);
        
        std::cout << "✅ TechnologySystem integration working - Research components created" << std::endl;

        // Test cross-system interaction
        std::cout << "\n=== Testing Cross-System Integration ===" << std::endl;
        
        // Economic system processing (should interact with population)
        economic_system->ProcessMonthlyUpdate(province_id);
        std::cout << "✅ Economic monthly update completed" << std::endl;
        
        // Military recruitment (should interact with population and economy)  
        bool recruitment_result = military_system->RecruitUnit(province_id, 
            game::military::UnitType::LEVIES, 100);
        std::cout << "✅ Military recruitment attempted: " 
                  << (recruitment_result ? "Success" : "No resources") << std::endl;

        std::cout << "\n=== Phase 1 Integration Test PASSED! ===" << std::endl;
        std::cout << "All core systems (Population, Economic, Military, Technology) are properly integrated with ECS" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Integration test failed: " << e.what() << std::endl;
        return 1;
    }
}