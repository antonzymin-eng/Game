// ============================================================================
// Mechanica Imperii - Comprehensive Trade System Tests
// Tests for TradeSystem, TradeCalculator, TradeRepository
// Created: 2025-11-19
// ============================================================================

#include "game/trade/TradeSystem.h"
#include "game/trade/TradeCalculator.h"
#include "game/trade/TradeRepository.h"
#include <cassert>
#include <iostream>
#include <cmath>
#include <vector>

using namespace game::trade;

// ============================================================================
// Test Utilities
// ============================================================================

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAILED: " << message << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running: " << #test_func << "..." << std::endl; \
        if (!test_func()) { \
            std::cerr << "❌ Test failed: " << #test_func << std::endl; \
            all_passed = false; \
        } else { \
            std::cout << "✅ Passed: " << #test_func << std::endl; \
        } \
    } while (0)

constexpr double EPSILON = 0.0001;

bool approximately_equal(double a, double b) {
    return std::abs(a - b) < EPSILON;
}

// ============================================================================
// TradeCalculator Tests
// ============================================================================

bool Test_CalculateMarketPrice_BalancedSupplyDemand() {
    double price = TradeCalculator::CalculateMarketPrice(1.0, 1.0, 1.0);
    TEST_ASSERT(approximately_equal(price, 1.0),
                "Balanced supply/demand should maintain base price");
    return true;
}

bool Test_CalculateMarketPrice_HighDemand() {
    double price = TradeCalculator::CalculateMarketPrice(1.0, 1.0, 2.0);
    TEST_ASSERT(price > 1.0,
                "High demand should increase price");
    return true;
}

bool Test_CalculateMarketPrice_HighSupply() {
    double price = TradeCalculator::CalculateMarketPrice(1.0, 2.0, 1.0);
    TEST_ASSERT(price < 1.0,
                "High supply should decrease price");
    return true;
}

bool Test_CalculateMarketPrice_ZeroSupply() {
    double price = TradeCalculator::CalculateMarketPrice(1.0, 0.0, 1.0);
    TEST_ASSERT(price > 0.0,
                "Zero supply should not cause division by zero");
    return true;
}

bool Test_CalculateMarketPrice_Bounds() {
    double price_high = TradeCalculator::CalculateMarketPrice(1.0, 0.1, 10.0);
    TEST_ASSERT(price_high <= 5.0,
                "Price should be clamped to reasonable bounds");
    return true;
}

bool Test_CalculateProfitPerUnit_Profitable() {
    double profit = TradeCalculator::CalculateProfitPerUnit(1.0, 2.0, 0.3);
    TEST_ASSERT(approximately_equal(profit, 0.7),
                "Profit calculation: 2.0 - 1.0 - 0.3 = 0.7");
    return true;
}

bool Test_CalculateProfitPerUnit_Unprofitable() {
    double profit = TradeCalculator::CalculateProfitPerUnit(1.0, 1.5, 1.0);
    TEST_ASSERT(profit < 0.0,
                "Should return negative profit when unprofitable");
    return true;
}

bool Test_CalculateProfitMargin_Valid() {
    double margin = TradeCalculator::CalculateProfitMargin(0.5, 1.0);
    TEST_ASSERT(approximately_equal(margin, 0.5),
                "Profit margin: 0.5 / 1.0 = 50%");
    return true;
}

bool Test_CalculateProfitMargin_ZeroSourcePrice() {
    double margin = TradeCalculator::CalculateProfitMargin(0.5, 0.0);
    TEST_ASSERT(approximately_equal(margin, 0.0),
                "Should handle zero source price gracefully");
    return true;
}

bool Test_CalculateTransportCost_BaseCost() {
    double cost = TradeCalculator::CalculateTransportCost(100.0, 1.0, 0.0, 1.0);
    TEST_ASSERT(cost > 0.0,
                "Transport cost should be positive");
    return true;
}

bool Test_CalculateTransportCost_HighBulk() {
    double normal_cost = TradeCalculator::CalculateTransportCost(100.0, 1.0, 0.0, 1.0);
    double bulk_cost = TradeCalculator::CalculateTransportCost(100.0, 2.0, 0.0, 1.0);
    TEST_ASSERT(bulk_cost > normal_cost,
                "Higher bulk factor should increase transport cost");
    return true;
}

bool Test_CalculateTransportCost_Perishability() {
    double non_perishable = TradeCalculator::CalculateTransportCost(100.0, 1.0, 0.0, 1.0);
    double perishable = TradeCalculator::CalculateTransportCost(100.0, 1.0, 0.5, 1.0);
    TEST_ASSERT(perishable > non_perishable,
                "Perishability should increase transport cost");
    return true;
}

bool Test_CalculateTransportCost_HighEfficiency() {
    double low_eff = TradeCalculator::CalculateTransportCost(100.0, 1.0, 0.0, 0.5);
    double high_eff = TradeCalculator::CalculateTransportCost(100.0, 1.0, 0.0, 1.5);
    TEST_ASSERT(low_eff > high_eff,
                "Higher efficiency should decrease transport cost");
    return true;
}

bool Test_CalculateDistance_Deterministic() {
    double dist1 = TradeCalculator::CalculateDistance(1, 5, 100);
    double dist2 = TradeCalculator::CalculateDistance(1, 5, 100);
    TEST_ASSERT(approximately_equal(dist1, dist2),
                "Distance calculation should be deterministic");
    return true;
}

bool Test_CalculateDistance_Symmetric() {
    double dist_forward = TradeCalculator::CalculateDistance(1, 5, 100);
    double dist_backward = TradeCalculator::CalculateDistance(5, 1, 100);
    // Note: Distance may not be perfectly symmetric due to ID ordering in seed
    TEST_ASSERT(std::abs(dist_forward - dist_backward) < dist_forward * 0.5,
                "Distance should be relatively symmetric");
    return true;
}

bool Test_CalculateDistance_SameProvince() {
    double dist = TradeCalculator::CalculateDistance(5, 5, 100);
    TEST_ASSERT(approximately_equal(dist, 0.0),
                "Distance to same province should be zero");
    return true;
}

bool Test_CalculateRouteEfficiency_NoInfrastructure() {
    double eff = TradeCalculator::CalculateRouteEfficiency(false, false, false);
    TEST_ASSERT(approximately_equal(eff, 1.0),
                "Base efficiency with no infrastructure should be 1.0");
    return true;
}

bool Test_CalculateRouteEfficiency_WithRiver() {
    double eff = TradeCalculator::CalculateRouteEfficiency(true, false, false);
    TEST_ASSERT(eff > 1.0,
                "River should increase efficiency");
    return true;
}

bool Test_CalculateRouteEfficiency_WithRoad() {
    double eff = TradeCalculator::CalculateRouteEfficiency(false, true, false);
    TEST_ASSERT(eff > 1.0,
                "Road should increase efficiency");
    return true;
}

bool Test_CalculateRouteEfficiency_WithSea() {
    double eff = TradeCalculator::CalculateRouteEfficiency(false, false, true);
    TEST_ASSERT(eff > 1.0,
                "Sea route should increase efficiency");
    return true;
}

bool Test_CalculateRouteEfficiency_AllInfrastructure() {
    double eff = TradeCalculator::CalculateRouteEfficiency(true, true, true);
    TEST_ASSERT(eff > 1.0,
                "All infrastructure should significantly increase efficiency");
    TEST_ASSERT(eff <= 2.0,
                "Efficiency should be capped at 200%");
    return true;
}

bool Test_CalculateRouteSafety_Deterministic() {
    double safety1 = TradeCalculator::CalculateRouteSafety(100.0, 1, 5, 100);
    double safety2 = TradeCalculator::CalculateRouteSafety(100.0, 1, 5, 100);
    TEST_ASSERT(approximately_equal(safety1, safety2),
                "Safety calculation should be deterministic");
    return true;
}

bool Test_CalculateRouteSafety_LongDistance() {
    double short_dist = TradeCalculator::CalculateRouteSafety(100.0, 1, 5, 100);
    double long_dist = TradeCalculator::CalculateRouteSafety(3000.0, 1, 5, 100);
    TEST_ASSERT(long_dist < short_dist,
                "Longer distances should reduce safety");
    return true;
}

bool Test_CalculateRouteSafety_BoundsCheck() {
    double safety = TradeCalculator::CalculateRouteSafety(10000.0, 1, 100, 100);
    TEST_ASSERT(safety >= 0.1 && safety <= 1.0,
                "Safety should be bounded between 0.1 and 1.0");
    return true;
}

bool Test_CalculateHubCapacity_LocalMarket() {
    double capacity = TradeCalculator::CalculateHubCapacity(1001, HubType::LOCAL_MARKET, 100);
    TEST_ASSERT(capacity > 0.0,
                "Local market should have positive capacity");
    return true;
}

bool Test_CalculateHubCapacity_InternationalPort() {
    double local = TradeCalculator::CalculateHubCapacity(1001, HubType::LOCAL_MARKET, 100);
    double port = TradeCalculator::CalculateHubCapacity(1001, HubType::INTERNATIONAL_PORT, 100);
    TEST_ASSERT(port > local,
                "International port should have higher capacity than local market");
    return true;
}

bool Test_CalculateEffectiveVolume_BaseCase() {
    double volume = TradeCalculator::CalculateEffectiveVolume(100.0, 1.0, 1.0, 1.0);
    TEST_ASSERT(approximately_equal(volume, 100.0),
                "Base case should return base volume");
    return true;
}

bool Test_CalculateEffectiveVolume_WithModifiers() {
    double volume = TradeCalculator::CalculateEffectiveVolume(100.0, 1.2, 0.9, 1.1);
    TEST_ASSERT(volume > 100.0,
                "Positive modifiers should increase effective volume");
    return true;
}

bool Test_CalculateEffectiveVolume_LowSafety() {
    double volume = TradeCalculator::CalculateEffectiveVolume(100.0, 1.0, 0.5, 1.0);
    TEST_ASSERT(approximately_equal(volume, 50.0),
                "50% safety should halve effective volume");
    return true;
}

bool Test_Clamp_WithinBounds() {
    double value = TradeCalculator::Clamp(5.0, 0.0, 10.0);
    TEST_ASSERT(approximately_equal(value, 5.0),
                "Value within bounds should be unchanged");
    return true;
}

bool Test_Clamp_AboveMax() {
    double value = TradeCalculator::Clamp(15.0, 0.0, 10.0);
    TEST_ASSERT(approximately_equal(value, 10.0),
                "Value above max should be clamped");
    return true;
}

bool Test_Clamp_BelowMin() {
    double value = TradeCalculator::Clamp(-5.0, 0.0, 10.0);
    TEST_ASSERT(approximately_equal(value, 0.0),
                "Value below min should be clamped");
    return true;
}

bool Test_CalculatePercentageChange_Increase() {
    double change = TradeCalculator::CalculatePercentageChange(100.0, 150.0);
    TEST_ASSERT(approximately_equal(change, 50.0),
                "100 to 150 should be 50% increase");
    return true;
}

bool Test_CalculatePercentageChange_Decrease() {
    double change = TradeCalculator::CalculatePercentageChange(100.0, 50.0);
    TEST_ASSERT(approximately_equal(change, -50.0),
                "100 to 50 should be -50% decrease");
    return true;
}

bool Test_CalculatePercentageChange_ZeroOldValue() {
    double change = TradeCalculator::CalculatePercentageChange(0.0, 100.0);
    TEST_ASSERT(approximately_equal(change, 0.0),
                "Should handle zero old value gracefully");
    return true;
}

// ============================================================================
// TradeRoute Tests
// ============================================================================

bool Test_TradeRoute_IsViable_ActiveProfitable() {
    TradeRoute route("test", 1, 2, types::ResourceType::FOOD);
    route.status = TradeStatus::ACTIVE;
    route.profitability = 0.1;
    route.safety_rating = 0.5;
    route.current_volume = 100.0;

    TEST_ASSERT(route.IsViable(),
                "Active, profitable route with volume should be viable");
    return true;
}

bool Test_TradeRoute_IsViable_Disrupted() {
    TradeRoute route("test", 1, 2, types::ResourceType::FOOD);
    route.status = TradeStatus::DISRUPTED;
    route.profitability = 0.1;
    route.safety_rating = 0.5;
    route.current_volume = 100.0;

    TEST_ASSERT(!route.IsViable(),
                "Disrupted route should not be viable");
    return true;
}

bool Test_TradeRoute_IsViable_Unprofitable() {
    TradeRoute route("test", 1, 2, types::ResourceType::FOOD);
    route.status = TradeStatus::ACTIVE;
    route.profitability = 0.01;
    route.safety_rating = 0.5;
    route.current_volume = 100.0;

    TEST_ASSERT(!route.IsViable(),
                "Unprofitable route should not be viable (< 5%)");
    return true;
}

bool Test_TradeRoute_IsViable_UnsafeRoute() {
    TradeRoute route("test", 1, 2, types::ResourceType::FOOD);
    route.status = TradeStatus::ACTIVE;
    route.profitability = 0.1;
    route.safety_rating = 0.2;
    route.current_volume = 100.0;

    TEST_ASSERT(!route.IsViable(),
                "Route with safety < 0.3 should not be viable");
    return true;
}

bool Test_TradeRoute_IsViable_NoVolume() {
    TradeRoute route("test", 1, 2, types::ResourceType::FOOD);
    route.status = TradeStatus::ACTIVE;
    route.profitability = 0.1;
    route.safety_rating = 0.5;
    route.current_volume = 0.0;

    TEST_ASSERT(!route.IsViable(),
                "Route with no volume should not be viable");
    return true;
}

bool Test_TradeRoute_GetEffectiveVolume_Active() {
    TradeRoute route("test", 1, 2, types::ResourceType::FOOD);
    route.status = TradeStatus::ACTIVE;
    route.current_volume = 100.0;
    route.efficiency_rating = 1.0;
    route.safety_rating = 1.0;
    route.seasonal_modifier = 1.0;

    double effective = route.GetEffectiveVolume();
    TEST_ASSERT(approximately_equal(effective, 100.0),
                "Base case should return current volume");
    return true;
}

bool Test_TradeRoute_GetEffectiveVolume_WithModifiers() {
    TradeRoute route("test", 1, 2, types::ResourceType::FOOD);
    route.status = TradeStatus::ACTIVE;
    route.current_volume = 100.0;
    route.efficiency_rating = 1.2;
    route.safety_rating = 0.8;
    route.seasonal_modifier = 1.1;

    double effective = route.GetEffectiveVolume();
    double expected = 100.0 * 1.2 * 0.8 * 1.1;
    TEST_ASSERT(approximately_equal(effective, expected),
                "Should apply all modifiers");
    return true;
}

bool Test_TradeRoute_GetEffectiveVolume_Disrupted() {
    TradeRoute route("test", 1, 2, types::ResourceType::FOOD);
    route.status = TradeStatus::DISRUPTED;
    route.current_volume = 100.0;

    double effective = route.GetEffectiveVolume();
    TEST_ASSERT(approximately_equal(effective, 0.0),
                "Disrupted route should have zero effective volume");
    return true;
}

// ============================================================================
// TradeHub Tests
// ============================================================================

bool Test_TradeHub_CanHandleVolume_BelowCapacity() {
    TradeHub hub(1001, "Test Market");
    hub.max_throughput_capacity = 1000.0;
    hub.current_utilization = 0.5;

    TEST_ASSERT(hub.CanHandleVolume(400.0),
                "Should handle volume below remaining capacity");
    return true;
}

bool Test_TradeHub_CanHandleVolume_ExceedsCapacity() {
    TradeHub hub(1001, "Test Market");
    hub.max_throughput_capacity = 1000.0;
    hub.current_utilization = 0.8;

    TEST_ASSERT(!hub.CanHandleVolume(300.0),
                "Should reject volume that exceeds capacity");
    return true;
}

bool Test_TradeHub_GetEffectiveCapacity_BaseCase() {
    TradeHub hub(1001, "Test Market");
    hub.max_throughput_capacity = 1000.0;
    hub.infrastructure_bonus = 1.0;
    hub.reputation_rating = 1.0;

    double effective = hub.GetEffectiveCapacity();
    TEST_ASSERT(effective == 1000.0,
                "Base case should return max capacity");
    return true;
}

bool Test_TradeHub_GetEffectiveCapacity_WithBonuses() {
    TradeHub hub(1001, "Test Market");
    hub.max_throughput_capacity = 1000.0;
    hub.infrastructure_bonus = 1.5;
    hub.reputation_rating = 2.0;

    double effective = hub.GetEffectiveCapacity();
    TEST_ASSERT(effective > 1000.0,
                "Bonuses should increase effective capacity");
    return true;
}

// ============================================================================
// MarketData Tests
// ============================================================================

bool Test_MarketData_IsPriceAboveAverage_True() {
    MarketData market(1001, types::ResourceType::FOOD);
    market.current_price = 1.2;
    market.avg_price_12_months = 1.0;

    TEST_ASSERT(market.IsPriceAboveAverage(),
                "Price 20% above average should be detected");
    return true;
}

bool Test_MarketData_IsPriceAboveAverage_False() {
    MarketData market(1001, types::ResourceType::FOOD);
    market.current_price = 1.05;
    market.avg_price_12_months = 1.0;

    TEST_ASSERT(!market.IsPriceAboveAverage(),
                "Price 5% above average should not be detected (threshold 10%)");
    return true;
}

bool Test_MarketData_IsExperiencingShock_ShockUp() {
    MarketData market(1001, types::ResourceType::FOOD);
    market.trend = PriceMovement::SHOCK_UP;

    TEST_ASSERT(market.IsExperiencingShock(),
                "SHOCK_UP should be detected");
    return true;
}

bool Test_MarketData_IsExperiencingShock_ShockDown() {
    MarketData market(1001, types::ResourceType::FOOD);
    market.trend = PriceMovement::SHOCK_DOWN;

    TEST_ASSERT(market.IsExperiencingShock(),
                "SHOCK_DOWN should be detected");
    return true;
}

bool Test_MarketData_IsExperiencingShock_Stable() {
    MarketData market(1001, types::ResourceType::FOOD);
    market.trend = PriceMovement::STABLE;

    TEST_ASSERT(!market.IsExperiencingShock(),
                "Stable market should not be in shock");
    return true;
}

bool Test_MarketData_GetPriceDeviation_Above() {
    MarketData market(1001, types::ResourceType::FOOD);
    market.current_price = 1.5;
    market.avg_price_12_months = 1.0;

    double deviation = market.GetPriceDeviation();
    TEST_ASSERT(approximately_equal(deviation, 0.5),
                "Deviation should be 50%");
    return true;
}

bool Test_MarketData_GetPriceDeviation_Below() {
    MarketData market(1001, types::ResourceType::FOOD);
    market.current_price = 0.8;
    market.avg_price_12_months = 1.0;

    double deviation = market.GetPriceDeviation();
    TEST_ASSERT(approximately_equal(deviation, -0.2),
                "Deviation should be -20%");
    return true;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "     COMPREHENSIVE TRADE SYSTEM TEST SUITE\n";
    std::cout << "========================================================\n";
    std::cout << "\n";

    bool all_passed = true;

    // TradeCalculator Tests - Price Calculations
    std::cout << "\n--- TradeCalculator: Price Calculations ---\n";
    RUN_TEST(Test_CalculateMarketPrice_BalancedSupplyDemand);
    RUN_TEST(Test_CalculateMarketPrice_HighDemand);
    RUN_TEST(Test_CalculateMarketPrice_HighSupply);
    RUN_TEST(Test_CalculateMarketPrice_ZeroSupply);
    RUN_TEST(Test_CalculateMarketPrice_Bounds);

    // TradeCalculator Tests - Profitability
    std::cout << "\n--- TradeCalculator: Profitability ---\n";
    RUN_TEST(Test_CalculateProfitPerUnit_Profitable);
    RUN_TEST(Test_CalculateProfitPerUnit_Unprofitable);
    RUN_TEST(Test_CalculateProfitMargin_Valid);
    RUN_TEST(Test_CalculateProfitMargin_ZeroSourcePrice);

    // TradeCalculator Tests - Transport Cost
    std::cout << "\n--- TradeCalculator: Transport Cost ---\n";
    RUN_TEST(Test_CalculateTransportCost_BaseCost);
    RUN_TEST(Test_CalculateTransportCost_HighBulk);
    RUN_TEST(Test_CalculateTransportCost_Perishability);
    RUN_TEST(Test_CalculateTransportCost_HighEfficiency);

    // TradeCalculator Tests - Distance
    std::cout << "\n--- TradeCalculator: Distance ---\n";
    RUN_TEST(Test_CalculateDistance_Deterministic);
    RUN_TEST(Test_CalculateDistance_Symmetric);
    RUN_TEST(Test_CalculateDistance_SameProvince);

    // TradeCalculator Tests - Route Efficiency
    std::cout << "\n--- TradeCalculator: Route Efficiency ---\n";
    RUN_TEST(Test_CalculateRouteEfficiency_NoInfrastructure);
    RUN_TEST(Test_CalculateRouteEfficiency_WithRiver);
    RUN_TEST(Test_CalculateRouteEfficiency_WithRoad);
    RUN_TEST(Test_CalculateRouteEfficiency_WithSea);
    RUN_TEST(Test_CalculateRouteEfficiency_AllInfrastructure);

    // TradeCalculator Tests - Route Safety
    std::cout << "\n--- TradeCalculator: Route Safety ---\n";
    RUN_TEST(Test_CalculateRouteSafety_Deterministic);
    RUN_TEST(Test_CalculateRouteSafety_LongDistance);
    RUN_TEST(Test_CalculateRouteSafety_BoundsCheck);

    // TradeCalculator Tests - Hub Capacity
    std::cout << "\n--- TradeCalculator: Hub Capacity ---\n";
    RUN_TEST(Test_CalculateHubCapacity_LocalMarket);
    RUN_TEST(Test_CalculateHubCapacity_InternationalPort);

    // TradeCalculator Tests - Effective Volume
    std::cout << "\n--- TradeCalculator: Effective Volume ---\n";
    RUN_TEST(Test_CalculateEffectiveVolume_BaseCase);
    RUN_TEST(Test_CalculateEffectiveVolume_WithModifiers);
    RUN_TEST(Test_CalculateEffectiveVolume_LowSafety);

    // TradeCalculator Tests - Utility Functions
    std::cout << "\n--- TradeCalculator: Utility Functions ---\n";
    RUN_TEST(Test_Clamp_WithinBounds);
    RUN_TEST(Test_Clamp_AboveMax);
    RUN_TEST(Test_Clamp_BelowMin);
    RUN_TEST(Test_CalculatePercentageChange_Increase);
    RUN_TEST(Test_CalculatePercentageChange_Decrease);
    RUN_TEST(Test_CalculatePercentageChange_ZeroOldValue);

    // TradeRoute Tests
    std::cout << "\n--- TradeRoute: Route Viability ---\n";
    RUN_TEST(Test_TradeRoute_IsViable_ActiveProfitable);
    RUN_TEST(Test_TradeRoute_IsViable_Disrupted);
    RUN_TEST(Test_TradeRoute_IsViable_Unprofitable);
    RUN_TEST(Test_TradeRoute_IsViable_UnsafeRoute);
    RUN_TEST(Test_TradeRoute_IsViable_NoVolume);

    std::cout << "\n--- TradeRoute: Effective Volume ---\n";
    RUN_TEST(Test_TradeRoute_GetEffectiveVolume_Active);
    RUN_TEST(Test_TradeRoute_GetEffectiveVolume_WithModifiers);
    RUN_TEST(Test_TradeRoute_GetEffectiveVolume_Disrupted);

    // TradeHub Tests
    std::cout << "\n--- TradeHub: Capacity Management ---\n";
    RUN_TEST(Test_TradeHub_CanHandleVolume_BelowCapacity);
    RUN_TEST(Test_TradeHub_CanHandleVolume_ExceedsCapacity);
    RUN_TEST(Test_TradeHub_GetEffectiveCapacity_BaseCase);
    RUN_TEST(Test_TradeHub_GetEffectiveCapacity_WithBonuses);

    // MarketData Tests
    std::cout << "\n--- MarketData: Price Analysis ---\n";
    RUN_TEST(Test_MarketData_IsPriceAboveAverage_True);
    RUN_TEST(Test_MarketData_IsPriceAboveAverage_False);
    RUN_TEST(Test_MarketData_IsExperiencingShock_ShockUp);
    RUN_TEST(Test_MarketData_IsExperiencingShock_ShockDown);
    RUN_TEST(Test_MarketData_IsExperiencingShock_Stable);
    RUN_TEST(Test_MarketData_GetPriceDeviation_Above);
    RUN_TEST(Test_MarketData_GetPriceDeviation_Below);

    // Print summary
    std::cout << "\n";
    std::cout << "========================================================\n";
    if (all_passed) {
        std::cout << "     ✅ ALL TESTS PASSED\n";
    } else {
        std::cout << "     ❌ SOME TESTS FAILED\n";
    }
    std::cout << "========================================================\n";
    std::cout << "\n";

    return all_passed ? 0 : 1;
}
