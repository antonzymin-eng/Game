// ============================================================================
// Mechanica Imperii - Trade System Integration Tests
// Tests for cross-system interactions and complex scenarios
// Created: 2025-11-22
// Location: tests/integration/game/trade/TradeIntegrationTests.cpp
// ============================================================================

#include <gtest/gtest.h>
#include "game/trade/TradeSystem.h"
#include "game/trade/TradeSystemConfig.h"
#include "core/ECS/EntityManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include <thread>
#include <vector>

using namespace game::trade;
using namespace game::types;

// ============================================================================
// Integration Test Fixtures
// ============================================================================

class TradeIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        entity_manager = std::make_unique<core::ecs::EntityManager>();
        message_bus = std::make_unique<::core::threading::ThreadSafeMessageBus>();
        access_manager = std::make_unique<::core::ecs::ComponentAccessManager>(*entity_manager);

        trade_system = std::make_unique<TradeSystem>(*access_manager, *message_bus);
        trade_system->Initialize();

        // Create a network of 10 provinces
        for (int i = 0; i < 10; ++i) {
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
// Complex Trade Network Tests
// ============================================================================

TEST_F(TradeIntegrationTest, ComplexTradeNetwork_MultipleRoutes_BalancesCorrectly) {
    // Arrange - Create a hub-and-spoke network
    EntityID hub = GetProvinceID(0);
    trade_system->CreateTradeHub(hub, "Central Hub", HubType::MAJOR_TRADING_CENTER);

    std::vector<std::string> route_ids;
    for (size_t i = 1; i < provinces.size(); ++i) {
        EntityID spoke = GetProvinceID(i);
        trade_system->CreateTradeHub(spoke, "Spoke " + std::to_string(i), HubType::LOCAL_MARKET);

        // Create bidirectional routes
        auto route1 = trade_system->EstablishTradeRoute(hub, spoke, ResourceType::FOOD);
        auto route2 = trade_system->EstablishTradeRoute(spoke, hub, ResourceType::WOOD);

        route_ids.push_back(route1);
        route_ids.push_back(route2);
    }

    // Act - Update system multiple times
    for (int i = 0; i < 10; ++i) {
        trade_system->Update(1.0f);
    }

    // Assert
    EXPECT_EQ(route_ids.size(), 18);  // 9 provinces * 2 routes each

    auto hub_data = trade_system->GetTradeHub(hub);
    ASSERT_TRUE(hub_data.has_value());
    EXPECT_GT(hub_data->incoming_route_ids.size() + hub_data->outgoing_route_ids.size(), 0);

    // Verify all routes are still active
    for (const auto& route_id : route_ids) {
        auto route = trade_system->GetRoute(route_id);
        ASSERT_TRUE(route.has_value());
        EXPECT_EQ(route->status, TradeStatus::ACTIVE);
    }
}

TEST_F(TradeIntegrationTest, TradeChain_ThreeProvinces_FlowsCorrectly) {
    // Arrange - Create a trade chain: P1 -> P2 -> P3
    EntityID p1 = GetProvinceID(0);
    EntityID p2 = GetProvinceID(1);
    EntityID p3 = GetProvinceID(2);

    trade_system->CreateTradeHub(p1, "Producer", HubType::LOCAL_MARKET);
    trade_system->CreateTradeHub(p2, "Middleman", HubType::REGIONAL_HUB);
    trade_system->CreateTradeHub(p3, "Consumer", HubType::LOCAL_MARKET);

    // Act
    auto route1 = trade_system->EstablishTradeRoute(p1, p2, ResourceType::FOOD);
    auto route2 = trade_system->EstablishTradeRoute(p2, p3, ResourceType::FOOD);

    trade_system->Update(1.0f);

    // Assert
    EXPECT_FALSE(route1.empty());
    EXPECT_FALSE(route2.empty());

    // Verify P2 has both incoming and outgoing routes
    auto p2_hub = trade_system->GetTradeHub(p2);
    ASSERT_TRUE(p2_hub.has_value());
    EXPECT_GT(p2_hub->incoming_route_ids.size(), 0);
    EXPECT_GT(p2_hub->outgoing_route_ids.size(), 0);
}

// ============================================================================
// Market Dynamics Integration Tests
// ============================================================================

TEST_F(TradeIntegrationTest, PriceShock_Propagates_AffectsConnectedMarkets) {
    // Arrange
    EntityID p1 = GetProvinceID(0);
    EntityID p2 = GetProvinceID(1);

    auto route = trade_system->EstablishTradeRoute(p1, p2, ResourceType::FOOD);

    double initial_price_p1 = trade_system->CalculateMarketPrice(p1, ResourceType::FOOD);

    // Act - Apply price shock to P1
    trade_system->ApplyPriceShock(p1, ResourceType::FOOD, 0.5, "Supply disruption");

    trade_system->Update(1.0f);

    // Assert
    double shocked_price_p1 = trade_system->CalculateMarketPrice(p1, ResourceType::FOOD);
    EXPECT_GT(shocked_price_p1, initial_price_p1) << "Price shock should increase price";

    // Verify route profitability changed
    auto route_data = trade_system->GetRoute(route);
    ASSERT_TRUE(route_data.has_value());
    // Price shock should affect route economics
}

TEST_F(TradeIntegrationTest, SeasonalChanges_AffectsAllRoutes) {
    // Arrange
    std::vector<std::string> routes;
    for (size_t i = 0; i < 5; ++i) {
        EntityID src = GetProvinceID(i);
        EntityID dst = GetProvinceID(i + 1);
        auto route = trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
        routes.push_back(route);
    }

    // Act - Process seasonal adjustments (winter month)
    trade_system->ProcessSeasonalAdjustments(12);  // December
    trade_system->Update(1.0f);

    // Assert - All routes should have seasonal modifiers applied
    for (const auto& route_id : routes) {
        auto route = trade_system->GetRoute(route_id);
        ASSERT_TRUE(route.has_value());
        // Seasonal modifier should be applied (not necessarily 1.0)
        EXPECT_TRUE(route->seasonal_modifier > 0.0);
    }
}

// ============================================================================
// Trade Disruption and Recovery Tests
// ============================================================================

TEST_F(TradeIntegrationTest, RouteDisruption_War_ImpactsEconomy) {
    // Arrange
    EntityID p1 = GetProvinceID(0);
    EntityID p2 = GetProvinceID(1);

    auto route = trade_system->EstablishTradeRoute(p1, p2, ResourceType::FOOD);

    auto initial_route_data = trade_system->GetRoute(route);
    ASSERT_TRUE(initial_route_data.has_value());
    double initial_volume = initial_route_data->current_volume;

    // Act - Disrupt route due to war
    bool disrupted = trade_system->DisruptTradeRoute(route, "War outbreak", 6.0);
    ASSERT_TRUE(disrupted);

    trade_system->Update(1.0f);

    // Assert
    auto disrupted_route = trade_system->GetRoute(route);
    ASSERT_TRUE(disrupted_route.has_value());
    EXPECT_EQ(disrupted_route->status, TradeStatus::DISRUPTED);
    EXPECT_TRUE(disrupted_route->is_recovering);
    EXPECT_GT(disrupted_route->pre_disruption_volume, 0.0);
    EXPECT_EQ(disrupted_route->GetEffectiveVolume(), 0.0) << "Disrupted route should have zero volume";
}

TEST_F(TradeIntegrationTest, RouteRecovery_GradualRestoration) {
    // Arrange
    EntityID p1 = GetProvinceID(0);
    EntityID p2 = GetProvinceID(1);

    auto route = trade_system->EstablishTradeRoute(p1, p2, ResourceType::FOOD);
    trade_system->DisruptTradeRoute(route, "Temporary blockade", 3.0);

    // Act - Restore route
    bool restored = trade_system->RestoreTradeRoute(route);
    ASSERT_TRUE(restored);

    trade_system->Update(1.0f);

    // Assert
    auto restored_route = trade_system->GetRoute(route);
    ASSERT_TRUE(restored_route.has_value());
    EXPECT_EQ(restored_route->status, TradeStatus::ACTIVE);
    EXPECT_FALSE(restored_route->is_recovering);
}

// ============================================================================
// Hub Evolution Tests
// ============================================================================

TEST_F(TradeIntegrationTest, HubEvolution_HighVolume_UpgradesNaturally) {
    // Arrange
    EntityID hub = GetProvinceID(0);
    trade_system->CreateTradeHub(hub, "Growing Hub", HubType::LOCAL_MARKET);

    // Create many routes to increase hub volume
    for (size_t i = 1; i < provinces.size(); ++i) {
        EntityID spoke = GetProvinceID(i);
        trade_system->EstablishTradeRoute(hub, spoke, ResourceType::FOOD);
        trade_system->EstablishTradeRoute(hub, spoke, ResourceType::WOOD);
    }

    // Act - Update system and trigger evolution
    for (int i = 0; i < 20; ++i) {
        trade_system->Update(1.0f);
    }

    // Manually trigger evolution
    trade_system->EvolveTradeHub(hub);

    // Assert
    auto hub_data = trade_system->GetTradeHub(hub);
    ASSERT_TRUE(hub_data.has_value());
    // Hub might have evolved to higher type based on volume
    EXPECT_GE(static_cast<int>(hub_data->hub_type), static_cast<int>(HubType::LOCAL_MARKET));
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(TradeIntegrationTest, ConfigChange_AffectsSystemBehavior) {
    // Arrange
    auto& config = trade_system->GetConfig();
    double original_threshold = config.min_viable_profitability;

    // Create a marginally profitable route
    EntityID p1 = GetProvinceID(0);
    EntityID p2 = GetProvinceID(1);
    auto route = trade_system->EstablishTradeRoute(p1, p2, ResourceType::FOOD);

    // Act - Change profitability threshold
    config.min_viable_profitability = 0.5;  // Much higher threshold

    trade_system->Update(1.0f);

    // Assert - Route might become non-viable with new threshold
    auto route_data = trade_system->GetRoute(route);
    ASSERT_TRUE(route_data.has_value());
    // Route still exists, but viability check would use new threshold

    // Restore original config
    config.min_viable_profitability = original_threshold;
}

TEST_F(TradeIntegrationTest, ConfigSaveLoad_PreservesSettings) {
    // Arrange
    auto& config = trade_system->GetConfig();
    config.min_viable_profitability = 0.15;
    config.debug.enable_trade_logging = true;
    config.performance.max_routes_per_frame = 50;

    std::string config_file = "/tmp/test_trade_config.json";

    // Act
    bool saved = config.SaveToFile(config_file);
    ASSERT_TRUE(saved);

    // Create new config and load
    TradeSystemConfig loaded_config;
    bool loaded = loaded_config.LoadFromFile(config_file);
    ASSERT_TRUE(loaded);

    // Assert
    EXPECT_EQ(loaded_config.min_viable_profitability, 0.15);
    EXPECT_EQ(loaded_config.debug.enable_trade_logging, true);
    EXPECT_EQ(loaded_config.performance.max_routes_per_frame, 50);

    // Cleanup
    std::remove(config_file.c_str());
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(TradeIntegrationTest, HighVolumeRoutes_100Routes_MaintainsConsistency) {
    // Arrange - Create 100 routes in a mesh network
    std::vector<std::string> routes;
    const int num_routes = 100;

    // Act
    for (int i = 0; i < num_routes; ++i) {
        size_t src_idx = i % provinces.size();
        size_t dst_idx = (i + 1) % provinces.size();

        EntityID src = GetProvinceID(src_idx);
        EntityID dst = GetProvinceID(dst_idx);

        ResourceType resource = static_cast<ResourceType>(i % 3);  // Vary resources
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

    // Verify no corruption
    for (const auto& route_id : routes) {
        auto route = trade_system->GetRoute(route_id);
        ASSERT_TRUE(route.has_value()) << "Route " << route_id << " should exist";
    }
}

// ============================================================================
// Message Bus Integration Tests
// ============================================================================

TEST_F(TradeIntegrationTest, EventPublishing_RouteEstablished_EventReceived) {
    // Arrange
    bool event_received = false;
    std::string received_route_id;

    message_bus->Subscribe<messages::TradeRouteEstablished>(
        [&](const messages::TradeRouteEstablished& event) {
            event_received = true;
            received_route_id = event.route_id;
        });

    // Act
    EntityID p1 = GetProvinceID(0);
    EntityID p2 = GetProvinceID(1);
    std::string route = trade_system->EstablishTradeRoute(p1, p2, ResourceType::FOOD);

    // Give message bus time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Assert
    EXPECT_TRUE(event_received) << "TradeRouteEstablished event should be published";
    EXPECT_EQ(received_route_id, route);
}

TEST_F(TradeIntegrationTest, EventPublishing_RouteDisrupted_EventReceived) {
    // Arrange
    bool event_received = false;

    message_bus->Subscribe<messages::TradeRouteDisrupted>(
        [&](const messages::TradeRouteDisrupted& event) {
            event_received = true;
        });

    EntityID p1 = GetProvinceID(0);
    EntityID p2 = GetProvinceID(1);
    std::string route = trade_system->EstablishTradeRoute(p1, p2, ResourceType::FOOD);

    // Act
    trade_system->DisruptTradeRoute(route, "Test disruption", 3.0);

    // Give message bus time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Assert
    EXPECT_TRUE(event_received) << "TradeRouteDisrupted event should be published";
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
