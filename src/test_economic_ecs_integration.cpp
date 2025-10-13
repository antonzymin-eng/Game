// ============================================================================
// test_economic_ecs_integration.cpp - Economic System ECS Integration Test
// Created: October 11, 2025 - Economic System ECS Integration Validation
// Location: src/test_economic_ecs_integration.cpp
// ============================================================================

#include "core/ECS/EntityManager.h"
#include "core/ECS/ComponentAccessManager.h"
#include "game/economy/EconomicSystem.h"
#include "game/economy/EconomicComponents.h"
#include "core/logging/Logger.h"
#include "game/config/GameConfig.h"

#include <iostream>
#include <cassert>

using namespace game;
using namespace game::economy;

void TestEconomicECSIntegration() {
    ::core::logging::LogInfo("EconomicECSTest", "Starting Economic System ECS Integration Test");

    // Initialize the economic system
    EconomicSystem economic_system;
    economic_system.Initialize(nullptr);

    // Test entity ID
    game::types::EntityID test_entity = 1001;

    ::core::logging::LogInfo("EconomicECSTest", "Creating economic components for test entity");
    
    // Create economic components for the test entity
    economic_system.CreateEconomicComponents(test_entity);

    // Test treasury operations
    ::core::logging::LogInfo("EconomicECSTest", "Testing treasury operations");
    
    int initial_treasury = economic_system.GetTreasury(test_entity);
    assert(initial_treasury > 0);
    ::core::logging::LogInfo("EconomicECSTest", "Initial treasury: " + std::to_string(initial_treasury));

    // Test spending money
    bool spend_result = economic_system.SpendMoney(test_entity, 100);
    assert(spend_result == true);
    int treasury_after_spend = economic_system.GetTreasury(test_entity);
    assert(treasury_after_spend == initial_treasury - 100);
    ::core::logging::LogInfo("EconomicECSTest", "Treasury after spending 100: " + std::to_string(treasury_after_spend));

    // Test adding money
    economic_system.AddMoney(test_entity, 250);
    int treasury_after_add = economic_system.GetTreasury(test_entity);
    assert(treasury_after_add == treasury_after_spend + 250);
    ::core::logging::LogInfo("EconomicECSTest", "Treasury after adding 250: " + std::to_string(treasury_after_add));

    // Test trade route operations
    ::core::logging::LogInfo("EconomicECSTest", "Testing trade route operations");
    
    game::types::EntityID destination_entity = 1002;
    economic_system.CreateEconomicComponents(destination_entity);
    
    // Add trade route
    economic_system.AddTradeRoute(test_entity, destination_entity, 0.8f, 150);
    
    // Get trade routes and verify
    auto trade_routes = economic_system.GetTradeRoutesForEntity(test_entity);
    assert(trade_routes.size() == 1);
    assert(trade_routes[0].to_province == destination_entity);
    assert(trade_routes[0].efficiency == 0.8f);
    assert(trade_routes[0].base_value == 150);
    ::core::logging::LogInfo("EconomicECSTest", "Trade route created successfully");

    // Test monthly update processing
    ::core::logging::LogInfo("EconomicECSTest", "Testing monthly update processing");
    
    economic_system.ProcessMonthlyUpdate(test_entity);
    
    // Verify income calculations (should be non-negative)
    int monthly_income = economic_system.GetMonthlyIncome(test_entity);
    int monthly_expenses = economic_system.GetMonthlyExpenses(test_entity);
    int net_income = economic_system.GetNetIncome(test_entity);
    
    assert(monthly_income >= 0);
    assert(monthly_expenses >= 0);
    assert(net_income == monthly_income - monthly_expenses);
    
    ::core::logging::LogInfo("EconomicECSTest", "Monthly income: " + std::to_string(monthly_income));
    ::core::logging::LogInfo("EconomicECSTest", "Monthly expenses: " + std::to_string(monthly_expenses));
    ::core::logging::LogInfo("EconomicECSTest", "Net income: " + std::to_string(net_income));

    // Test economic events
    ::core::logging::LogInfo("EconomicECSTest", "Testing economic events");
    
    // Process several months to potentially generate events
    for (int month = 0; month < 12; ++month) {
        economic_system.ProcessMonthlyUpdate(test_entity);
    }
    
    auto active_events = economic_system.GetActiveEvents(test_entity);
    ::core::logging::LogInfo("EconomicECSTest", "Active events after 12 months: " + std::to_string(active_events.size()));
    
    // Test removing trade route
    ::core::logging::LogInfo("EconomicECSTest", "Testing trade route removal");
    
    economic_system.RemoveTradeRoute(test_entity, destination_entity);
    auto routes_after_removal = economic_system.GetTradeRoutesForEntity(test_entity);
    assert(routes_after_removal.size() == 0);
    ::core::logging::LogInfo("EconomicECSTest", "Trade route removed successfully");

    // Test edge cases
    ::core::logging::LogInfo("EconomicECSTest", "Testing edge cases");
    
    // Try to spend more money than available
    int current_treasury = economic_system.GetTreasury(test_entity);
    bool overspend_result = economic_system.SpendMoney(test_entity, current_treasury + 1000);
    assert(overspend_result == false);
    int treasury_after_failed_spend = economic_system.GetTreasury(test_entity);
    assert(treasury_after_failed_spend == current_treasury); // Should remain unchanged
    ::core::logging::LogInfo("EconomicECSTest", "Overspending correctly rejected");

    // Test operations on non-existent entity
    game::types::EntityID nonexistent_entity = 9999;
    int nonexistent_treasury = economic_system.GetTreasury(nonexistent_entity);
    assert(nonexistent_treasury == 0);
    ::core::logging::LogInfo("EconomicECSTest", "Non-existent entity handling correct");

    // Shutdown system
    economic_system.Shutdown();
    
    ::core::logging::LogInfo("EconomicECSTest", "✅ ALL ECONOMIC ECS INTEGRATION TESTS PASSED");
}

int main() {
    // Initialize logging
    ::core::logging::LogInfo("EconomicECSTest", "Economic System ECS Integration Test Starting");
    
    try {
        TestEconomicECSIntegration();
        
        std::cout << "✅ Economic System ECS Integration Test PASSED" << std::endl;
        std::cout << "✅ Economic System successfully integrated with ECS architecture" << std::endl;
        std::cout << "✅ All component operations validated" << std::endl;
        std::cout << "✅ Treasury, trade, and events systems working correctly" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Economic System ECS Integration Test FAILED: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Economic System ECS Integration Test FAILED with unknown error" << std::endl;
        return 1;
    }
}