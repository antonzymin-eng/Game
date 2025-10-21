// ============================================================================
// main_minimal.cpp - Minimal Build Test for Mechanica Imperii
// Created: 2025-01-13 17:00:00 1
// Basic SDL2 initialization to test compilation
// ============================================================================

#include <iostream>
#include <chrono>
#include <memory>
#include <exception>

// Platform compatibility layer (includes SDL2, OpenGL, Json, ImGui)
#include "utils/PlatformCompat.h"

// CRITICAL FIX: Core Type System
#include "core/types/game_types.h"

// Core ECS and Architecture
#include "core/ECS/EntityManager.h"
#include "core/ECS/MessageBus.h"

// Test ECS Component Template
#include "game/realm/RealmComponents.h"

// Game Configuration
#include "game/config/GameConfig.h"

// UI System
#include "ui/UI.h"


int main(int argc, char* argv[]) {
    try {
        std::cout << "=== Mechanica Imperii - Minimal Build Test ===" << std::endl;

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
            return 1;
        }
        std::cout << "SDL initialized successfully" << std::endl;

        // Initialize GameConfig system
        try {
            auto& game_config = game::config::GameConfig::Instance();
            if (game_config.LoadFromFile("config/GameConfig.json")) {
                std::cout << "GameConfig loaded successfully" << std::endl;
            } else {
                std::cerr << "GameConfig failed to load from file." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "GameConfig initialization failed: " << e.what() << std::endl;
        }

        // Toast test
        ui::Toast::Show("ECS Component Template Test", 3.0f);

        // Test ECS Component Template
        std::cout << "Testing core::ecs::Component<T> template..." << std::endl;

        // Test component type IDs
        auto realmTypeId = ::core::ecs::Component<game::realm::RealmComponent>::GetStaticTypeID();
        auto dynastyTypeId = ::core::ecs::Component<game::realm::DynastyComponent>::GetStaticTypeID();
        auto councilTypeId = ::core::ecs::Component<game::realm::CouncilComponent>::GetStaticTypeID();
        auto lawsTypeId = ::core::ecs::Component<game::realm::LawsComponent>::GetStaticTypeID();

        std::cout << "RealmComponent TypeID: " << realmTypeId << std::endl;
        std::cout << "DynastyComponent TypeID: " << dynastyTypeId << std::endl;
        std::cout << "CouncilComponent TypeID: " << councilTypeId << std::endl;
        std::cout << "LawsComponent TypeID: " << lawsTypeId << std::endl;

        // Create test components
        game::realm::RealmComponent testRealm(123);
        testRealm.realmName = "Test Kingdom";

        std::cout << "Created RealmComponent: " << testRealm.realmName
                  << " (ID: " << testRealm.realmId << ")" << std::endl;
        std::cout << "Component TypeID: " << testRealm.GetTypeID() << std::endl;
        std::cout << "Component Type Name: " << testRealm.GetComponentTypeName() << std::endl;

        // Test cloning
        auto clonedRealm = testRealm.Clone();
        std::cout << "Successfully cloned component!" << std::endl;

        std::cout << "✅ ECS Component Template Test PASSED!" << std::endl;

        std::cout << "Main loop starting..." << std::endl;

        // Test entity manager creation
        auto entity_manager = std::make_unique<core::ecs::EntityManager>();
        auto entity_id = entity_manager->CreateEntity();

        std::cout << "Created test entity with ID: " << entity_id.id << std::endl;

        // Test message bus creation
        auto message_bus = std::make_unique<core::ecs::MessageBus>();

        std::cout << "Core systems initialized successfully" << std::endl;

        // === Add PopulationSystem (bottom-up) ===
        // #include "game/population/PopulationSystem.h" at top if not present
        // Uncomment and initialize as needed:
        // auto population_system = std::make_unique<game::population::PopulationSystem>();
        // std::cout << "PopulationSystem initialized successfully" << std::endl;

        std::cout << "=== Build Test Complete ===" << std::endl;

        SDL_Quit();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        SDL_Quit();
        return 1;
    }
}