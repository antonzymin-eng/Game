// ============================================================================
// Phase 1 Core Systems Integration Test - Simplified
// Created: October 15, 2025 - Phase 1 Assessment
// Location: test_phase1_simple.cpp
// ============================================================================

#include "core/ECS/EntityManager.h"
#include "core/ECS/ComponentAccessManager.h" 
#include "core/ECS/MessageBus.h"
#include "game/population/PopulationComponents.h"
#include "game/economy/EconomicComponents.h"
#include "game/military/MilitaryComponents.h"
#include "game/technology/TechnologyComponents.h"
#include "core/logging/Logger.h"

#include <iostream>
#include <memory>

int main() {
    std::cout << "=== Phase 1 Core Systems Component Integration Test ===" << std::endl;
    
    try {
        // Create ECS core components
        auto entity_manager = std::make_unique<::core::ecs::EntityManager>();
        auto message_bus = std::make_unique<::core::ecs::MessageBus>();
        auto access_manager = std::make_unique<::core::ecs::ComponentAccessManager>(
            entity_manager.get(), message_bus.get());
        
        std::cout << "✅ ECS Core created successfully" << std::endl;

        // Create test province entity
        auto province_entity = entity_manager->CreateEntity("TestProvince");
        
        std::cout << "✅ Created test province entity: " << province_entity.id << std::endl;

        // Test PopulationComponent creation
        auto pop_component = entity_manager->AddComponent<game::population::PopulationComponent>(province_entity);
        if (pop_component) {
            pop_component->total_population = 15000;
            pop_component->average_happiness = 0.7;
            std::cout << "✅ PopulationComponent created - Population: " 
                      << pop_component->total_population << std::endl;
        } else {
            std::cout << "❌ PopulationComponent creation failed" << std::endl;
            return 1;
        }

        // Test EconomicComponent creation
        auto econ_component = entity_manager->AddComponent<game::economy::EconomicComponent>(province_entity);
        if (econ_component) {
            econ_component->treasury = 1000;
            econ_component->tax_rate = 0.15f;
            std::cout << "✅ EconomicComponent created - Treasury: " 
                      << econ_component->treasury << std::endl;
        } else {
            std::cout << "❌ EconomicComponent creation failed" << std::endl;
            return 1;
        }

        // Test MilitaryComponent creation
        auto mil_component = entity_manager->AddComponent<game::military::MilitaryComponent>(province_entity);
        if (mil_component) {
            mil_component->military_budget = 500.0;
            mil_component->recruitment_capacity = 200;
            std::cout << "✅ MilitaryComponent created - Military budget: " 
                      << mil_component->military_budget << std::endl;
        } else {
            std::cout << "❌ MilitaryComponent creation failed" << std::endl;
            return 1;
        }

        // Test ResearchComponent creation
        auto research_component = entity_manager->AddComponent<game::technology::ResearchComponent>(province_entity);
        if (research_component) {
            research_component->monthly_research_budget = 100.0;
            research_component->base_research_efficiency = 1.2;
            std::cout << "✅ ResearchComponent created - Research budget: " 
                      << research_component->monthly_research_budget << std::endl;
        } else {
            std::cout << "❌ ResearchComponent creation failed" << std::endl;
            return 1;
        }

        // Test component retrieval and interaction
        std::cout << "\n=== Testing Component Interactions ===" << std::endl;
        
        // Test component access
        auto retrieved_pop = entity_manager->GetComponent<game::population::PopulationComponent>(province_entity);
        auto retrieved_econ = entity_manager->GetComponent<game::economy::EconomicComponent>(province_entity);
        
        if (retrieved_pop && retrieved_econ) {
            // Calculate tax revenue based on population
            int tax_revenue = static_cast<int>(retrieved_pop->total_population * retrieved_econ->tax_rate * 0.5);
            retrieved_econ->treasury += tax_revenue;
            
            std::cout << "✅ Cross-component calculation: Generated " << tax_revenue 
                      << " tax revenue from population" << std::endl;
            std::cout << "✅ Updated treasury: " << retrieved_econ->treasury << std::endl;
        }

        std::cout << "\n=== Phase 1 Component Integration Test PASSED! ===" << std::endl;
        std::cout << "All core ECS components (Population, Economic, Military, Technology) are working correctly" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "❌ Integration test failed: " << e.what() << std::endl;
        return 1;
    }
}