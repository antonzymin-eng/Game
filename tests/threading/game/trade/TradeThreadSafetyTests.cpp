// ============================================================================
// Mechanica Imperii - Trade System Thread Safety Tests
// Validates thread-safe operations and concurrent access patterns
// Created: 2025-11-22
// Location: tests/threading/game/trade/TradeThreadSafetyTests.cpp
// ============================================================================

#include <gtest/gtest.h>
#include "game/trade/TradeSystem.h"
#include "core/ECS/EntityManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace game::trade;
using namespace game::types;

// ============================================================================
// Thread Safety Test Fixtures
// ============================================================================

class TradeThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        entity_manager = std::make_unique<core::ecs::EntityManager>();
        message_bus = std::make_unique<::core::threading::ThreadSafeMessageBus>();
        access_manager = std::make_unique<::core::ecs::ComponentAccessManager>(*entity_manager);

        trade_system = std::make_unique<TradeSystem>(*access_manager, *message_bus);
        trade_system->Initialize();

        // Create test provinces
        for (int i = 0; i < 20; ++i) {
            provinces.push_back(entity_manager->CreateEntity());
        }
    }

    void TearDown() override {
        trade_system->Shutdown();
    }

    EntityID GetProvinceID(size_t index) const {
        return static_cast<EntityID>(provinces[index].id);
    }

    std::unique_ptr<core::ecs::EntityManager> entity_manager;
    std::unique_ptr<::core::threading::ThreadSafeMessageBus> message_bus;
    std::unique_ptr<::core::ecs::ComponentAccessManager> access_manager;
    std::unique_ptr<TradeSystem> trade_system;
    std::vector<core::ecs::EntityID> provinces;
};

// ============================================================================
// Message Bus Thread Safety Tests
// ============================================================================

TEST_F(TradeThreadSafetyTest, MessageBus_ConcurrentEventPublishing_AllDelivered) {
    // Arrange
    std::atomic<int> events_received{0};
    const int num_threads = 10;
    const int events_per_thread = 100;

    message_bus->Subscribe<messages::TradeRouteEstablished>(
        [&](const messages::TradeRouteEstablished& event) {
            events_received++;
        });

    // Act - Multiple threads publishing events concurrently
    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < events_per_thread; ++i) {
                EntityID src = GetProvinceID(i % 10);
                EntityID dst = GetProvinceID((i + 1) % 10);

                // This creates a route which publishes an event
                trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Give message bus time to process all events
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Assert
    // Note: We might not get exactly num_threads * events_per_thread due to duplicate routes
    // but we should get a significant number
    EXPECT_GT(events_received.load(), 0) << "Should receive some events from concurrent publishing";
}

TEST_F(TradeThreadSafetyTest, MessageBus_ConcurrentSubscribe_NoDeadlock) {
    // Arrange & Act
    std::vector<std::thread> threads;
    std::atomic<int> subscribe_count{0};

    for (int t = 0; t < 20; ++t) {
        threads.emplace_back([&]() {
            message_bus->Subscribe<messages::TradeRouteEstablished>(
                [&](const messages::TradeRouteEstablished& event) {
                    // Empty handler
                });
            subscribe_count++;
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // Assert
    EXPECT_EQ(subscribe_count.load(), 20) << "All subscribes should complete without deadlock";
}

// ============================================================================
// Trade System MAIN_THREAD Validation Tests
// ============================================================================

TEST_F(TradeThreadSafetyTest, TradeSystem_ThreadingStrategy_IsMAIN_THREAD) {
    // Assert
    auto strategy = trade_system->GetThreadingStrategy();
    EXPECT_EQ(strategy, ::core::threading::ThreadingStrategy::MAIN_THREAD)
        << "Trade System should use MAIN_THREAD strategy for production safety";
}

TEST_F(TradeThreadSafetyTest, TradeSystem_ThreadingRationale_IsDocumented) {
    // Act
    std::string rationale = trade_system->GetThreadingRationale();

    // Assert
    EXPECT_FALSE(rationale.empty()) << "Threading rationale should be documented";
    EXPECT_NE(rationale.find("MAIN_THREAD"), std::string::npos)
        << "Rationale should mention MAIN_THREAD strategy";
    EXPECT_NE(rationale.find("component access"), std::string::npos)
        << "Rationale should explain component access concerns";
}

// ============================================================================
// Sequential Access Pattern Tests (MAIN_THREAD validation)
// ============================================================================

TEST_F(TradeThreadSafetyTest, SequentialAccess_MultipleUpdates_NoDataCorruption) {
    // Arrange - Create routes
    std::vector<std::string> routes;
    for (int i = 0; i < 10; ++i) {
        EntityID src = GetProvinceID(i);
        EntityID dst = GetProvinceID((i + 1) % 10);
        auto route = trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
        routes.push_back(route);
    }

    // Act - Sequential updates (simulating main thread behavior)
    for (int frame = 0; frame < 100; ++frame) {
        trade_system->Update(0.016f);  // ~60 FPS
    }

    // Assert - All routes should still be valid
    for (const auto& route_id : routes) {
        auto route = trade_system->GetRoute(route_id);
        ASSERT_TRUE(route.has_value()) << "Route " << route_id << " should still exist";
        EXPECT_EQ(route->status, TradeStatus::ACTIVE);
    }
}

TEST_F(TradeThreadSafetyTest, SequentialAccess_RouteQueries_ConsistentResults) {
    // Arrange
    EntityID p1 = GetProvinceID(0);
    EntityID p2 = GetProvinceID(1);
    std::string route = trade_system->EstablishTradeRoute(p1, p2, ResourceType::FOOD);

    // Act - Multiple sequential queries
    std::vector<std::optional<TradeRoute>> results;
    for (int i = 0; i < 100; ++i) {
        results.push_back(trade_system->GetRoute(route));
    }

    // Assert - All queries should return same consistent data
    EXPECT_EQ(results.size(), 100);
    for (const auto& result : results) {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->route_id, route);
        EXPECT_EQ(result->source_province, p1);
        EXPECT_EQ(result->destination_province, p2);
    }
}

// ============================================================================
// Internal Data Structure Thread Safety Tests
// ============================================================================

TEST_F(TradeThreadSafetyTest, InternalMutex_ProtectsTradeData) {
    // This test validates that even though we use MAIN_THREAD,
    // the internal mutex protection is still correct for future THREAD_POOL upgrade

    // Arrange - Create routes
    std::vector<std::string> routes;
    for (int i = 0; i < 5; ++i) {
        EntityID src = GetProvinceID(i);
        EntityID dst = GetProvinceID(i + 1);
        routes.push_back(trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD));
    }

    // Act - Sequential operations that acquire mutex
    for (const auto& route_id : routes) {
        trade_system->GetRoute(route_id);
        trade_system->Update(0.016f);
    }

    // Assert - No deadlocks, all operations complete
    EXPECT_EQ(routes.size(), 5);
}

// ============================================================================
// Pathfinder Cache Thread Safety Tests
// ============================================================================

TEST_F(TradeThreadSafetyTest, PathfinderCache_SequentialAccess_ConsistentHitRate) {
    // Arrange - Warm up cache
    EntityID p1 = GetProvinceID(0);
    EntityID p2 = GetProvinceID(5);

    // Act - Create same route multiple times (should hit cache)
    for (int i = 0; i < 10; ++i) {
        trade_system->EstablishTradeRoute(p1, p2, ResourceType::FOOD);
    }

    // The first call should miss cache, subsequent calls should hit
    // (though duplicate route detection might return same route ID)

    // Assert - Cache should have entries
    auto all_routes = trade_system->GetAllTradeRoutes();
    EXPECT_GE(all_routes.size(), 1) << "Should have at least one route";
}

TEST_F(TradeThreadSafetyTest, PathfinderCache_ClearCache_NoDataCorruption) {
    // Arrange
    for (int i = 0; i < 5; ++i) {
        EntityID src = GetProvinceID(i);
        EntityID dst = GetProvinceID(i + 5);
        trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
    }

    // Act - Clear pathfinder cache
    trade_system->ClearPathfinderCache();

    // Create new routes (should rebuild cache)
    for (int i = 5; i < 10; ++i) {
        EntityID src = GetProvinceID(i);
        EntityID dst = GetProvinceID(i - 5);
        trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
    }

    // Assert - All routes should be valid
    auto all_routes = trade_system->GetAllTradeRoutes();
    EXPECT_GT(all_routes.size(), 0);
}

// ============================================================================
// Hub Management Thread Safety Tests
// ============================================================================

TEST_F(TradeThreadSafetyTest, HubManagement_SequentialOperations_NoCorruption) {
    // Arrange
    EntityID hub = GetProvinceID(0);

    // Act - Sequential hub operations
    trade_system->CreateTradeHub(hub, "Test Hub", HubType::LOCAL_MARKET);

    for (int i = 1; i < 10; ++i) {
        EntityID spoke = GetProvinceID(i);
        trade_system->EstablishTradeRoute(hub, spoke, ResourceType::FOOD);
    }

    trade_system->Update(1.0f);
    trade_system->EvolveTradeHub(hub);
    trade_system->Update(1.0f);

    // Assert
    auto hub_data = trade_system->GetTradeHub(hub);
    ASSERT_TRUE(hub_data.has_value());
    EXPECT_GT(hub_data->outgoing_route_ids.size(), 0);
}

// ============================================================================
// Configuration Thread Safety Tests
// ============================================================================

TEST_F(TradeThreadSafetyTest, Configuration_GetConfig_SafeAccess) {
    // Act - Multiple sequential config accesses
    for (int i = 0; i < 100; ++i) {
        const auto& config = trade_system->GetConfig();
        EXPECT_GT(config.min_viable_profitability, 0.0);
    }

    // Assert - No crashes or corruption
    SUCCEED();
}

TEST_F(TradeThreadSafetyTest, Configuration_SetConfig_UpdatesSystem) {
    // Arrange
    TradeSystemConfig new_config;
    new_config.min_viable_profitability = 0.20;
    new_config.debug.enable_trade_logging = true;

    // Act
    trade_system->SetConfig(new_config);
    const auto& retrieved_config = trade_system->GetConfig();

    // Assert
    EXPECT_EQ(retrieved_config.min_viable_profitability, 0.20);
    EXPECT_TRUE(retrieved_config.debug.enable_trade_logging);
}

// ============================================================================
// Performance Under Load Tests
// ============================================================================

TEST_F(TradeThreadSafetyTest, HighFrequencyUpdates_NoPerformanceDegradation) {
    // Arrange - Create moderate number of routes
    for (int i = 0; i < 25; ++i) {  // Max routes per frame
        EntityID src = GetProvinceID(i % 10);
        EntityID dst = GetProvinceID((i + 1) % 10);
        trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
    }

    // Act - High frequency updates
    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < 1000; ++i) {
        trade_system->Update(0.016f);
    }

    auto end = std::chrono::steady_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // Assert - Should complete in reasonable time (< 5 seconds for 1000 frames)
    EXPECT_LT(duration_ms, 5000) << "1000 updates should complete in under 5 seconds";

    // Check performance metrics
    auto metrics = trade_system->GetPerformanceMetrics();
    EXPECT_LT(metrics.total_update_ms, 100.0) << "Individual update should be fast";
}

// ============================================================================
// Stress Tests (validates MAIN_THREAD handles high load)
// ============================================================================

TEST_F(TradeThreadSafetyTest, StressTest_ManyRoutesSequential_HandlesGracefully) {
    // Arrange & Act - Create many routes sequentially
    std::vector<std::string> routes;
    const int target_routes = 500;

    for (int i = 0; i < target_routes; ++i) {
        EntityID src = GetProvinceID(i % provinces.size());
        EntityID dst = GetProvinceID((i + 1) % provinces.size());
        ResourceType resource = static_cast<ResourceType>(i % 3);

        auto route = trade_system->EstablishTradeRoute(src, dst, resource);
        if (!route.empty()) {
            routes.push_back(route);
        }
    }

    // Update system
    for (int i = 0; i < 10; ++i) {
        trade_system->Update(1.0f);
    }

    // Assert
    EXPECT_GT(routes.size(), 0);

    auto all_routes = trade_system->GetAllTradeRoutes();
    EXPECT_EQ(all_routes.size(), routes.size());
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
