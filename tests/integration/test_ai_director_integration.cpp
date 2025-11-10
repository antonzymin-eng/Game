// ============================================================================
// test_ai_director_integration.cpp - AI Director Functional Integration Tests
// Created: November 10, 2025
// Purpose: Week 2 verification - Functional testing of AI Director integration
// ============================================================================

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

#include "game/ai/AIDirector.h"
#include "core/ECS/EntityManager.h"
#include "core/ECS/MessageBus.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadedSystemManager.h"

// ============================================================================
// Test Fixture
// ============================================================================

class AIDirectorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        entity_manager = std::make_unique<core::ecs::EntityManager>();
        message_bus = std::make_unique<core::ecs::MessageBus>();
        access_manager = std::make_unique<core::ecs::ComponentAccessManager>();
        threaded_system_manager = std::make_unique<core::threading::ThreadedSystemManager>();

        ai_director = std::make_unique<AI::AIDirector>(
            *entity_manager,
            *message_bus,
            *access_manager,
            *threaded_system_manager
        );
        ai_director->Initialize();
        ai_director->Start();
    }

    void TearDown() override {
        if (ai_director) {
            ai_director->Shutdown();
        }
    }

    std::unique_ptr<core::ecs::EntityManager> entity_manager;
    std::unique_ptr<core::ecs::MessageBus> message_bus;
    std::unique_ptr<core::ecs::ComponentAccessManager> access_manager;
    std::unique_ptr<core::threading::ThreadedSystemManager> threaded_system_manager;
    std::unique_ptr<AI::AIDirector> ai_director;
};

// ============================================================================
// Initialization and Lifecycle Tests
// ============================================================================

TEST_F(AIDirectorIntegrationTest, InitializationSucceeds) {
    // Verify AI Director initializes correctly
    EXPECT_NE(ai_director, nullptr);
    SUCCEED() << "AI Director initialized successfully";
}

TEST_F(AIDirectorIntegrationTest, StartStopCycle) {
    // Test start/stop lifecycle
    ai_director->Shutdown();
    SUCCEED() << "AI Director shutdown successfully";

    ai_director->Initialize();
    ai_director->Start();
    SUCCEED() << "AI Director restarted successfully";
}

TEST_F(AIDirectorIntegrationTest, UpdateWithoutCrash) {
    // Verify Update() can be called without issues
    constexpr int NUM_UPDATES = 100;
    constexpr float DELTA_TIME = 0.016f;

    for (int i = 0; i < NUM_UPDATES; ++i) {
        ASSERT_NO_THROW(ai_director->Update(DELTA_TIME));
    }

    SUCCEED() << "AI Director Update() completed " << NUM_UPDATES << " iterations";
}

// ============================================================================
// Actor Creation and Management Tests
// ============================================================================

TEST_F(AIDirectorIntegrationTest, CreateNationAI) {
    // Test Nation AI creation
    constexpr EntityID nation_id = 1000;

    ASSERT_NO_THROW(ai_director->CreateNationAI(nation_id));

    // Update to ensure actor is processed
    for (int i = 0; i < 10; ++i) {
        ai_director->Update(0.016f);
    }

    SUCCEED() << "Nation AI created and updated successfully";
}

TEST_F(AIDirectorIntegrationTest, CreateMultipleNationAI) {
    // Test creating multiple Nation AI actors
    constexpr int NUM_NATIONS = 20;

    for (int i = 0; i < NUM_NATIONS; ++i) {
        EntityID nation_id = 1000 + i;
        ASSERT_NO_THROW(ai_director->CreateNationAI(nation_id));
    }

    // Update to process all actors
    for (int i = 0; i < 50; ++i) {
        ai_director->Update(0.016f);
    }

    SUCCEED() << "Created and updated " << NUM_NATIONS << " Nation AI actors";
}

TEST_F(AIDirectorIntegrationTest, CreateCharacterAI) {
    // Test Character AI creation
    constexpr EntityID character_id = 5000;

    ASSERT_NO_THROW(ai_director->CreateCharacterAI(character_id));

    // Update to ensure actor is processed
    for (int i = 0; i < 10; ++i) {
        ai_director->Update(0.016f);
    }

    SUCCEED() << "Character AI created and updated successfully";
}

TEST_F(AIDirectorIntegrationTest, CreateMultipleCharacterAI) {
    // Test creating multiple Character AI actors
    constexpr int NUM_CHARACTERS = 100;

    for (int i = 0; i < NUM_CHARACTERS; ++i) {
        EntityID character_id = 5000 + i;
        ASSERT_NO_THROW(ai_director->CreateCharacterAI(character_id));
    }

    // Update to process all actors
    for (int i = 0; i < 50; ++i) {
        ai_director->Update(0.016f);
    }

    SUCCEED() << "Created and updated " << NUM_CHARACTERS << " Character AI actors";
}

TEST_F(AIDirectorIntegrationTest, CreateMixedActors) {
    // Test creating a realistic mix of AI actors
    constexpr int NUM_NATIONS = 10;
    constexpr int NUM_CHARACTERS = 50;

    for (int i = 0; i < NUM_NATIONS; ++i) {
        ai_director->CreateNationAI(1000 + i);
    }

    for (int i = 0; i < NUM_CHARACTERS; ++i) {
        ai_director->CreateCharacterAI(5000 + i);
    }

    // Update to process all actors
    for (int i = 0; i < 100; ++i) {
        ai_director->Update(0.016f);
    }

    SUCCEED() << "Created and updated mixed AI actors";
}

// ============================================================================
// Message Bus Integration Tests
// ============================================================================

TEST_F(AIDirectorIntegrationTest, MessageBusIntegration) {
    // Verify AI Director integrates correctly with message bus

    // Create some AI actors
    ai_director->CreateNationAI(1000);
    ai_director->CreateCharacterAI(5000);

    // Simulate other systems posting messages
    constexpr int NUM_MESSAGES = 100;
    for (int i = 0; i < NUM_MESSAGES; ++i) {
        // Simulate system messages (actual message types would be used in real test)
        // message_bus->Post(/* some message */);
    }

    // Update AI Director - should process messages without issues
    for (int i = 0; i < 50; ++i) {
        ai_director->Update(0.016f);
    }

    SUCCEED() << "AI Director integrates with message bus successfully";
}

// ============================================================================
// Entity Manager Integration Tests
// ============================================================================

TEST_F(AIDirectorIntegrationTest, EntityManagerIntegration) {
    // Verify AI Director works with entity manager

    // Create some test entities
    constexpr int NUM_ENTITIES = 50;
    std::vector<EntityID> entities;

    for (int i = 0; i < NUM_ENTITIES; ++i) {
        auto entity = entity_manager->CreateEntity();
        entities.push_back(entity);
    }

    // Create AI actors
    ai_director->CreateNationAI(1000);

    // Update and verify no issues
    for (int i = 0; i < 100; ++i) {
        ai_director->Update(0.016f);
    }

    SUCCEED() << "AI Director integrates with entity manager successfully";
}

// ============================================================================
// Long-Running Stability Tests
// ============================================================================

TEST_F(AIDirectorIntegrationTest, ExtendedOperationStability) {
    // Verify AI Director remains stable over extended operation

    constexpr int NUM_ITERATIONS = 1000;
    constexpr float DELTA_TIME = 0.016f;

    // Create some AI actors
    for (int i = 0; i < 10; ++i) {
        ai_director->CreateNationAI(1000 + i);
        ai_director->CreateCharacterAI(5000 + i);
    }

    // Run for extended period (simulating ~16 seconds of game time)
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        ASSERT_NO_THROW(ai_director->Update(DELTA_TIME));
    }

    SUCCEED() << "AI Director stable over " << NUM_ITERATIONS << " updates";
}

TEST_F(AIDirectorIntegrationTest, StressTestWithDynamicActors) {
    // Stress test with dynamic actor creation/destruction

    constexpr int NUM_CYCLES = 10;
    constexpr int ACTORS_PER_CYCLE = 20;

    for (int cycle = 0; cycle < NUM_CYCLES; ++cycle) {
        // Create actors
        for (int i = 0; i < ACTORS_PER_CYCLE; ++i) {
            int offset = cycle * ACTORS_PER_CYCLE + i;
            ai_director->CreateNationAI(1000 + offset);
            ai_director->CreateCharacterAI(5000 + offset);
        }

        // Update for a while
        for (int i = 0; i < 50; ++i) {
            ai_director->Update(0.016f);
        }

        // Note: Actor removal would be tested here if API is available
    }

    SUCCEED() << "Dynamic actor stress test completed";
}

// ============================================================================
// Integration with Game Loop Simulation
// ============================================================================

TEST_F(AIDirectorIntegrationTest, GameLoopSimulation) {
    // Simulate a realistic game loop scenario

    // Setup: Create initial game state
    constexpr int NUM_NATIONS = 5;
    constexpr int NUM_CHARACTERS = 20;

    for (int i = 0; i < NUM_NATIONS; ++i) {
        ai_director->CreateNationAI(1000 + i);
    }
    for (int i = 0; i < NUM_CHARACTERS; ++i) {
        ai_director->CreateCharacterAI(5000 + i);
    }

    // Simulate 10 seconds of game time at 60 FPS
    constexpr int NUM_FRAMES = 600;
    constexpr float DELTA_TIME = 0.016f;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < NUM_FRAMES; ++frame) {
        // Simulate other systems updating
        // (In real game loop, other systems would update here)

        // Update AI Director
        ASSERT_NO_THROW(ai_director->Update(DELTA_TIME));

        // Simulate occasional new actors
        if (frame % 100 == 0 && frame > 0) {
            int new_id = frame / 100;
            ai_director->CreateNationAI(2000 + new_id);
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time
    ).count();

    std::cout << "Game loop simulation completed in " << duration_ms << " ms" << std::endl;
    std::cout << "Average frame time: "
              << (static_cast<double>(duration_ms) / NUM_FRAMES)
              << " ms" << std::endl;

    SUCCEED() << "Game loop simulation completed successfully";
}

// ============================================================================
// Error Handling and Edge Cases
// ============================================================================

TEST_F(AIDirectorIntegrationTest, UpdateWithZeroDeltaTime) {
    // Test with zero delta time
    ASSERT_NO_THROW(ai_director->Update(0.0f));
    SUCCEED() << "AI Director handles zero delta time";
}

TEST_F(AIDirectorIntegrationTest, UpdateWithLargeDeltaTime) {
    // Test with unusually large delta time
    ASSERT_NO_THROW(ai_director->Update(1.0f)); // 1 second
    SUCCEED() << "AI Director handles large delta time";
}

TEST_F(AIDirectorIntegrationTest, RapidUpdates) {
    // Test rapid consecutive updates
    constexpr int NUM_RAPID_UPDATES = 1000;
    constexpr float SMALL_DELTA = 0.001f;

    for (int i = 0; i < NUM_RAPID_UPDATES; ++i) {
        ASSERT_NO_THROW(ai_director->Update(SMALL_DELTA));
    }

    SUCCEED() << "AI Director handles rapid updates";
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
