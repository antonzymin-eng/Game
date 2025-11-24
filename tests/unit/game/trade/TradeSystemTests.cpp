// ============================================================================
// Mechanica Imperii - Trade System Unit Tests
// Comprehensive test suite for Trade System
// Created: 2025-11-22
// Location: tests/unit/game/trade/TradeSystemTests.cpp
// ============================================================================

#include <gtest/gtest.h>
#include "game/trade/TradeSystem.h"
#include "game/trade/TradeRepository.h"
#include "game/trade/TradeCalculator.h"
#include "core/ECS/EntityManager.h"
#include "core/threading/ThreadSafeMessageBus.h"

using namespace game::trade;
using namespace game::types;

// ============================================================================
// Test Fixtures
// ============================================================================

class TradeSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create ECS infrastructure
        entity_manager = std::make_unique<core::ecs::EntityManager>();
        message_bus = std::make_unique<::core::threading::ThreadSafeMessageBus>();
        access_manager = std::make_unique<::core::ecs::ComponentAccessManager>(*entity_manager);

        // Create trade system
        trade_system = std::make_unique<TradeSystem>(*access_manager, *message_bus);
        trade_system->Initialize();

        // Create test provinces
        province_1 = entity_manager->CreateEntity();
        province_2 = entity_manager->CreateEntity();
        province_3 = entity_manager->CreateEntity();
    }

    void TearDown() override {
        trade_system->Shutdown();
    }

    std::unique_ptr<core::ecs::EntityManager> entity_manager;
    std::unique_ptr<::core::threading::ThreadSafeMessageBus> message_bus;
    std::unique_ptr<::core::ecs::ComponentAccessManager> access_manager;
    std::unique_ptr<TradeSystem> trade_system;

    core::ecs::EntityID province_1;
    core::ecs::EntityID province_2;
    core::ecs::EntityID province_3;
};

// ============================================================================
// Trade Route Management Tests
// ============================================================================

TEST_F(TradeSystemTest, EstablishTradeRoute_ValidRoute_CreatesSuccessfully) {
    // Arrange
    EntityID source = static_cast<EntityID>(province_1.id);
    EntityID dest = static_cast<EntityID>(province_2.id);
    ResourceType resource = ResourceType::FOOD;

    // Act
    std::string route_id = trade_system->EstablishTradeRoute(source, dest, resource);

    // Assert
    EXPECT_FALSE(route_id.empty());
    auto route = trade_system->GetRoute(route_id);
    ASSERT_TRUE(route.has_value());
    EXPECT_EQ(route->source_province, source);
    EXPECT_EQ(route->destination_province, dest);
    EXPECT_EQ(route->resource, resource);
    EXPECT_EQ(route->status, TradeStatus::ACTIVE);
}

TEST_F(TradeSystemTest, EstablishTradeRoute_SameProvince_ReturnsEmpty) {
    // Arrange
    EntityID source = static_cast<EntityID>(province_1.id);
    ResourceType resource = ResourceType::FOOD;

    // Act
    std::string route_id = trade_system->EstablishTradeRoute(source, source, resource);

    // Assert
    EXPECT_TRUE(route_id.empty()) << "Should not create route to same province";
}

TEST_F(TradeSystemTest, EstablishTradeRoute_DuplicateRoute_ReturnsExisting) {
    // Arrange
    EntityID source = static_cast<EntityID>(province_1.id);
    EntityID dest = static_cast<EntityID>(province_2.id);
    ResourceType resource = ResourceType::FOOD;

    // Act
    std::string route_id_1 = trade_system->EstablishTradeRoute(source, dest, resource);
    std::string route_id_2 = trade_system->EstablishTradeRoute(source, dest, resource);

    // Assert
    EXPECT_EQ(route_id_1, route_id_2) << "Duplicate route should return same ID";
}

TEST_F(TradeSystemTest, DisruptTradeRoute_ActiveRoute_TransitionsToDisrupted) {
    // Arrange
    EntityID source = static_cast<EntityID>(province_1.id);
    EntityID dest = static_cast<EntityID>(province_2.id);
    std::string route_id = trade_system->EstablishTradeRoute(source, dest, ResourceType::FOOD);

    // Act
    bool result = trade_system->DisruptTradeRoute(route_id, "War outbreak", 3.0);

    // Assert
    EXPECT_TRUE(result);
    auto route = trade_system->GetRoute(route_id);
    ASSERT_TRUE(route.has_value());
    EXPECT_EQ(route->status, TradeStatus::DISRUPTED);
    EXPECT_TRUE(route->is_recovering);
}

TEST_F(TradeSystemTest, RestoreTradeRoute_DisruptedRoute_RecoversProperly) {
    // Arrange
    EntityID source = static_cast<EntityID>(province_1.id);
    EntityID dest = static_cast<EntityID>(province_2.id);
    std::string route_id = trade_system->EstablishTradeRoute(source, dest, ResourceType::FOOD);
    trade_system->DisruptTradeRoute(route_id, "War", 3.0);

    // Act
    bool result = trade_system->RestoreTradeRoute(route_id);

    // Assert
    EXPECT_TRUE(result);
    auto route = trade_system->GetRoute(route_id);
    ASSERT_TRUE(route.has_value());
    EXPECT_EQ(route->status, TradeStatus::ACTIVE);
    EXPECT_FALSE(route->is_recovering);
}

TEST_F(TradeSystemTest, AbandonTradeRoute_ExistingRoute_RemovesCompletely) {
    // Arrange
    EntityID source = static_cast<EntityID>(province_1.id);
    EntityID dest = static_cast<EntityID>(province_2.id);
    std::string route_id = trade_system->EstablishTradeRoute(source, dest, ResourceType::FOOD);

    // Act
    trade_system->AbandonTradeRoute(route_id);

    // Assert
    auto route = trade_system->GetRoute(route_id);
    EXPECT_FALSE(route.has_value()) << "Route should be removed";
}

TEST_F(TradeSystemTest, GetRoutesFromProvince_MultipleRoutes_ReturnsAll) {
    // Arrange
    EntityID source = static_cast<EntityID>(province_1.id);
    EntityID dest1 = static_cast<EntityID>(province_2.id);
    EntityID dest2 = static_cast<EntityID>(province_3.id);

    trade_system->EstablishTradeRoute(source, dest1, ResourceType::FOOD);
    trade_system->EstablishTradeRoute(source, dest2, ResourceType::WOOD);

    // Act
    auto routes = trade_system->GetRoutesFromProvince(source);

    // Assert
    EXPECT_EQ(routes.size(), 2);
}

// ============================================================================
// Trade Route Viability Tests
// ============================================================================

TEST_F(TradeSystemTest, TradeRoute_IsViable_ProfitableRoute_ReturnsTrue) {
    // Arrange
    TradeRoute route("test_route", 1, 2, ResourceType::FOOD);
    route.status = TradeStatus::ACTIVE;
    route.profitability = 0.10;  // 10% profit
    route.safety_rating = 0.8;
    route.current_volume = 100.0;

    // Act & Assert
    EXPECT_TRUE(route.IsViable());
}

TEST_F(TradeSystemTest, TradeRoute_IsViable_UnprofitableRoute_ReturnsFalse) {
    // Arrange
    TradeRoute route("test_route", 1, 2, ResourceType::FOOD);
    route.status = TradeStatus::ACTIVE;
    route.profitability = 0.02;  // 2% profit (below 5% threshold)
    route.safety_rating = 0.8;
    route.current_volume = 100.0;

    // Act & Assert
    EXPECT_FALSE(route.IsViable());
}

TEST_F(TradeSystemTest, TradeRoute_GetEffectiveVolume_ActiveRoute_AppliesModifiers) {
    // Arrange
    TradeRoute route("test_route", 1, 2, ResourceType::FOOD);
    route.status = TradeStatus::ACTIVE;
    route.current_volume = 100.0;
    route.efficiency_rating = 1.2;
    route.safety_rating = 0.9;
    route.seasonal_modifier = 1.1;

    // Act
    double effective_volume = route.GetEffectiveVolume();

    // Assert
    double expected = 100.0 * 1.2 * 0.9 * 1.1;
    EXPECT_NEAR(effective_volume, expected, 0.01);
}

TEST_F(TradeSystemTest, TradeRoute_GetEffectiveVolume_DisruptedRoute_ReturnsZero) {
    // Arrange
    TradeRoute route("test_route", 1, 2, ResourceType::FOOD);
    route.status = TradeStatus::DISRUPTED;
    route.current_volume = 100.0;

    // Act
    double effective_volume = route.GetEffectiveVolume();

    // Assert
    EXPECT_EQ(effective_volume, 0.0);
}

// ============================================================================
// Market Dynamics Tests
// ============================================================================

TEST_F(TradeSystemTest, CalculateMarketPrice_BalancedSupplyDemand_ReturnsBasePrice) {
    // Arrange
    double base_price = 10.0;
    double supply = 1.0;  // Balanced
    double demand = 1.0;  // Balanced

    // Act
    double price = TradeCalculator::CalculateMarketPrice(base_price, supply, demand);

    // Assert
    EXPECT_NEAR(price, base_price, 0.1);
}

TEST_F(TradeSystemTest, CalculateMarketPrice_HighDemand_IncreasesPrice) {
    // Arrange
    double base_price = 10.0;
    double supply = 1.0;
    double demand = 2.0;  // High demand

    // Act
    double price = TradeCalculator::CalculateMarketPrice(base_price, supply, demand);

    // Assert
    EXPECT_GT(price, base_price) << "High demand should increase price";
}

TEST_F(TradeSystemTest, CalculateMarketPrice_HighSupply_DecreasesPrice) {
    // Arrange
    double base_price = 10.0;
    double supply = 2.0;  // High supply
    double demand = 1.0;

    // Act
    double price = TradeCalculator::CalculateMarketPrice(base_price, supply, demand);

    // Assert
    EXPECT_LT(price, base_price) << "High supply should decrease price";
}

// ============================================================================
// Hub Management Tests
// ============================================================================

TEST_F(TradeSystemTest, CreateTradeHub_ValidProvince_CreatesSuccessfully) {
    // Arrange
    EntityID province = static_cast<EntityID>(province_1.id);

    // Act
    trade_system->CreateTradeHub(province, "Test Hub", HubType::LOCAL_MARKET);

    // Assert
    auto hub = trade_system->GetTradeHub(province);
    ASSERT_TRUE(hub.has_value());
    EXPECT_EQ(hub->hub_name, "Test Hub");
    EXPECT_EQ(hub->hub_type, HubType::LOCAL_MARKET);
}

TEST_F(TradeSystemTest, TradeHub_CanHandleVolume_WithinCapacity_ReturnsTrue) {
    // Arrange
    TradeHub hub(1, "Test Hub");
    hub.max_throughput_capacity = 100.0;
    hub.current_utilization = 0.5;  // 50% utilized

    // Act
    bool can_handle = hub.CanHandleVolume(30.0);  // 30% more = 80% total

    // Assert
    EXPECT_TRUE(can_handle);
}

TEST_F(TradeSystemTest, TradeHub_CanHandleVolume_ExceedsCapacity_ReturnsFalse) {
    // Arrange
    TradeHub hub(1, "Test Hub");
    hub.max_throughput_capacity = 100.0;
    hub.current_utilization = 0.8;  // 80% utilized

    // Act
    bool can_handle = hub.CanHandleVolume(30.0);  // 30% more = 110% total

    // Assert
    EXPECT_FALSE(can_handle);
}

TEST_F(TradeSystemTest, TradeHub_AddRoute_NewRoute_AddsSuccessfully) {
    // Arrange
    TradeHub hub(1, "Test Hub");

    // Act
    hub.AddRoute("route_1", true);  // Incoming route

    // Assert
    EXPECT_EQ(hub.incoming_route_ids.size(), 1);
    EXPECT_EQ(hub.incoming_route_ids[0], "route_1");
}

TEST_F(TradeSystemTest, TradeHub_AddRoute_DuplicateRoute_DoesNotDuplicate) {
    // Arrange
    TradeHub hub(1, "Test Hub");

    // Act
    hub.AddRoute("route_1", true);
    hub.AddRoute("route_1", true);  // Duplicate

    // Assert
    EXPECT_EQ(hub.incoming_route_ids.size(), 1) << "Should not add duplicate route";
}

// ============================================================================
// Calculator Function Tests
// ============================================================================

TEST_F(TradeSystemTest, TradeCalculator_CalculateSupplyLevel_DeterministicSeed_ConsistentResults) {
    // Arrange
    EntityID province = 1;
    ResourceType resource = ResourceType::FOOD;
    uint64_t game_tick = 100;

    // Act
    double supply1 = TradeCalculator::CalculateSupplyLevel(province, resource, game_tick);
    double supply2 = TradeCalculator::CalculateSupplyLevel(province, resource, game_tick);

    // Assert
    EXPECT_EQ(supply1, supply2) << "Deterministic RNG should produce consistent results";
}

TEST_F(TradeSystemTest, TradeCalculator_ClampPrice_ExceedsMax_ReturnsMax) {
    // Arrange
    double price = 150.0;
    double min_price = 0.1;
    double max_price = 100.0;

    // Act
    double clamped = TradeCalculator::ClampPrice(price, min_price, max_price);

    // Assert
    EXPECT_EQ(clamped, max_price);
}

TEST_F(TradeSystemTest, TradeCalculator_ClampPrice_BelowMin_ReturnsMin) {
    // Arrange
    double price = 0.05;
    double min_price = 0.1;
    double max_price = 100.0;

    // Act
    double clamped = TradeCalculator::ClampPrice(price, min_price, max_price);

    // Assert
    EXPECT_EQ(clamped, min_price);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(TradeSystemTest, TradeSystemConfig_DefaultValues_AreValid) {
    // Arrange & Act
    TradeSystemConfig config;
    std::string error_message;

    // Assert
    EXPECT_TRUE(config.Validate(error_message)) << error_message;
}

TEST_F(TradeSystemTest, TradeSystemConfig_GetConfig_ReturnsValidConfig) {
    // Act
    const auto& config = trade_system->GetConfig();

    // Assert
    EXPECT_GT(config.min_viable_profitability, 0.0);
    EXPECT_GT(config.min_viable_safety, 0.0);
    EXPECT_GT(config.performance.max_routes_per_frame, 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(TradeSystemTest, Integration_ComplexTradeNetwork_BalancesCorrectly) {
    // Arrange
    EntityID p1 = static_cast<EntityID>(province_1.id);
    EntityID p2 = static_cast<EntityID>(province_2.id);
    EntityID p3 = static_cast<EntityID>(province_3.id);

    // Create hub network
    trade_system->CreateTradeHub(p1, "Hub 1", HubType::REGIONAL_HUB);
    trade_system->CreateTradeHub(p2, "Hub 2", HubType::REGIONAL_HUB);
    trade_system->CreateTradeHub(p3, "Hub 3", HubType::LOCAL_MARKET);

    // Establish routes
    auto route1 = trade_system->EstablishTradeRoute(p1, p2, ResourceType::FOOD);
    auto route2 = trade_system->EstablishTradeRoute(p2, p3, ResourceType::FOOD);
    auto route3 = trade_system->EstablishTradeRoute(p1, p3, ResourceType::WOOD);

    // Act - Update system
    trade_system->Update(1.0f);

    // Assert
    EXPECT_FALSE(route1.empty());
    EXPECT_FALSE(route2.empty());
    EXPECT_FALSE(route3.empty());

    auto all_routes = trade_system->GetAllTradeRoutes();
    EXPECT_EQ(all_routes.size(), 3);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
