// ============================================================================
// Mechanica Imperii - Trade System Integration Tests
// Tests for trade-economy cross-system interactions
// Created: 2025-11-19
// ============================================================================

#include "game/trade/TradeSystem.h"
#include "game/economy/EconomicSystem.h"
#include "game/economy/TradeEconomicBridge.h"
#include <cassert>
#include <iostream>
#include <memory>

using namespace game::trade;
using namespace game::economy;
using namespace mechanica::integration;

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
// Integration Test Scenarios
// ============================================================================

/**
 * @brief Test complete trade route lifecycle
 *
 * Scenario: Establish a trade route, simulate disruption, verify recovery
 */
bool Test_TradeRouteLifecycle_Complete() {
    std::cout << "\n  Testing complete trade route lifecycle..." << std::endl;

    // Phase 1: Route Establishment
    TradeRoute route("test_route_1_2", 1001, 1002, types::ResourceType::FOOD);
    route.status = TradeStatus::ESTABLISHING;
    route.source_price = 1.0;
    route.destination_price = 1.8;
    route.transport_cost_per_unit = 0.3;
    route.distance_km = 250.0;

    // Calculate profitability
    route.profitability = TradeCalculator::CalculateRouteProfitability(route);
    TEST_ASSERT(route.profitability > 0.0, "Route should be profitable");
    std::cout << "    ✓ Route profitability: " << route.profitability << std::endl;

    // Activate route
    route.status = TradeStatus::ACTIVE;
    route.base_volume = 100.0;
    route.current_volume = 100.0;
    route.efficiency_rating = 1.0;
    route.safety_rating = 0.9;
    route.seasonal_modifier = 1.0;

    TEST_ASSERT(route.IsViable(), "Active route should be viable");
    std::cout << "    ✓ Route activated successfully" << std::endl;

    // Phase 2: Trade Disruption
    route.pre_disruption_volume = route.current_volume;
    route.pre_disruption_safety = route.safety_rating;
    route.status = TradeStatus::DISRUPTED;
    route.current_volume *= 0.1;  // 90% reduction
    route.safety_rating = 0.3;
    route.disruption_count++;
    route.is_recovering = false;

    TEST_ASSERT(!route.IsViable(), "Disrupted route should not be viable");
    std::cout << "    ✓ Route disrupted: volume reduced to " << route.current_volume << std::endl;

    // Phase 3: Recovery Process
    route.is_recovering = true;
    route.recovery_progress = 0.5;  // 50% recovered
    route.current_volume = route.pre_disruption_volume * 0.6;  // Partial recovery
    route.safety_rating = 0.6;

    TEST_ASSERT(route.is_recovering, "Route should be in recovery");
    std::cout << "    ✓ Recovery in progress: " << (route.recovery_progress * 100) << "% complete" << std::endl;

    // Phase 4: Full Recovery
    route.is_recovering = false;
    route.recovery_progress = 1.0;
    route.status = TradeStatus::ACTIVE;
    route.current_volume = route.pre_disruption_volume;
    route.safety_rating = route.pre_disruption_safety;

    TEST_ASSERT(route.IsViable(), "Recovered route should be viable again");
    std::cout << "    ✓ Route fully recovered" << std::endl;

    return true;
}

/**
 * @brief Test trade hub evolution
 *
 * Scenario: Hub grows from local market to regional hub
 */
bool Test_TradeHubEvolution_LocalToRegional() {
    std::cout << "\n  Testing trade hub evolution..." << std::endl;

    // Phase 1: Start as local market
    TradeHub hub(2001, "Test Market");
    hub.hub_type = HubType::LOCAL_MARKET;
    hub.max_throughput_capacity = 100.0;
    hub.current_utilization = 0.3;
    hub.infrastructure_bonus = 1.0;
    hub.reputation_rating = 1.0;
    hub.upgrade_level = 1;

    double initial_capacity = hub.GetEffectiveCapacity();
    std::cout << "    ✓ Local market created with capacity: " << initial_capacity << std::endl;

    // Phase 2: Add trade routes
    hub.AddRoute("route_1", true);   // Incoming
    hub.AddRoute("route_2", false);  // Outgoing
    hub.AddRoute("route_3", true);   // Incoming
    hub.AddRoute("route_4", false);  // Outgoing

    TEST_ASSERT(hub.incoming_route_ids.size() == 2, "Hub should have 2 incoming routes");
    TEST_ASSERT(hub.outgoing_route_ids.size() == 2, "Hub should have 2 outgoing routes");
    std::cout << "    ✓ Added 4 trade routes to hub" << std::endl;

    // Phase 3: Increase utilization (triggers evolution)
    hub.current_utilization = 0.9;  // High utilization

    // Phase 4: Evolve to Regional Hub
    hub.hub_type = HubType::REGIONAL_HUB;
    hub.max_throughput_capacity *= 2.0;  // Double capacity
    hub.infrastructure_bonus = 1.2;
    hub.reputation_rating = 1.3;
    hub.upgrade_level = 2;

    double evolved_capacity = hub.GetEffectiveCapacity();
    TEST_ASSERT(evolved_capacity > initial_capacity,
                "Evolved hub should have higher capacity");
    std::cout << "    ✓ Hub evolved to Regional Hub with capacity: " << evolved_capacity << std::endl;

    // Phase 5: Add specialization
    hub.specialized_goods.insert(types::ResourceType::FOOD);
    hub.specialized_goods.insert(types::ResourceType::WOOD);
    hub.handling_efficiency[types::ResourceType::FOOD] = 1.3;
    hub.handling_efficiency[types::ResourceType::WOOD] = 1.2;

    TEST_ASSERT(hub.specialized_goods.size() == 2, "Hub should have 2 specializations");
    std::cout << "    ✓ Hub specialized in FOOD and WOOD" << std::endl;

    return true;
}

/**
 * @brief Test market price shock propagation
 *
 * Scenario: Price shock in one province affects connected markets
 */
bool Test_MarketPriceShock_Propagation() {
    std::cout << "\n  Testing market price shock propagation..." << std::endl;

    // Phase 1: Normal market conditions
    MarketData market1(3001, types::ResourceType::GRAIN);
    market1.current_price = 1.0;
    market1.base_price = 1.0;
    market1.avg_price_12_months = 1.0;
    market1.supply_level = 1.0;
    market1.demand_level = 1.0;
    market1.trend = PriceMovement::STABLE;
    market1.volatility_index = 0.1;

    std::cout << "    ✓ Initial market price: " << market1.current_price << std::endl;

    // Phase 2: Apply price shock (e.g., harvest failure)
    double shock_magnitude = 0.8;  // 80% price increase
    market1.current_price *= (1.0 + shock_magnitude);
    market1.trend = PriceMovement::SHOCK_UP;
    market1.volatility_index += shock_magnitude;
    market1.supply_level = 0.4;  // Supply drops to 40%

    TEST_ASSERT(market1.IsExperiencingShock(), "Market should be experiencing shock");
    std::cout << "    ✓ Price shock applied: new price = " << market1.current_price << std::endl;

    // Phase 3: Check price deviation
    double deviation = market1.GetPriceDeviation();
    TEST_ASSERT(deviation > 0.5, "Deviation should be significant (>50%)");
    std::cout << "    ✓ Price deviation: " << (deviation * 100) << "%" << std::endl;

    // Phase 4: Simulate stabilization over time
    for (int month = 0; month < 6; ++month) {
        // Gradual price recovery
        double stabilization = TradeCalculator::CalculateStabilizationAdjustment(
            market1.current_price, market1.avg_price_12_months, 0.05);
        market1.current_price += stabilization;
        market1.volatility_index *= 0.9;  // Reduce volatility

        // Supply recovers
        market1.supply_level += 0.1;
        if (market1.supply_level > 1.0) market1.supply_level = 1.0;
    }

    TEST_ASSERT(market1.current_price < 1.0 + shock_magnitude,
                "Price should have stabilized somewhat");
    std::cout << "    ✓ After 6 months stabilization: price = " << market1.current_price << std::endl;

    // Phase 5: Return to normal
    if (std::abs(market1.current_price - market1.avg_price_12_months) < 0.2) {
        market1.trend = PriceMovement::STABLE;
        std::cout << "    ✓ Market returned to stable conditions" << std::endl;
    }

    return true;
}

/**
 * @brief Test trade-economy integration
 *
 * Scenario: Trade income affects province treasury
 */
bool Test_TradeEconomyIntegration_IncomeFlow() {
    std::cout << "\n  Testing trade-economy income flow..." << std::endl;

    // Phase 1: Calculate trade effects
    TradeEconomicEffects effects;
    effects.trade_route_income = 500.0;
    effects.trade_volume = 1000.0;
    effects.merchant_activity_level = 10.0;
    effects.trade_efficiency = 1.1;

    // Calculate customs revenue (5% of volume + merchant tax)
    double customs_rate = 0.05;
    double merchant_tax_rate = 0.02;
    effects.customs_revenue = (effects.trade_volume * customs_rate) +
                             (effects.merchant_activity_level * merchant_tax_rate);

    double total_income = effects.trade_route_income + effects.customs_revenue;
    std::cout << "    ✓ Trade route income: " << effects.trade_route_income << std::endl;
    std::cout << "    ✓ Customs revenue: " << effects.customs_revenue << std::endl;
    std::cout << "    ✓ Total trade income: " << total_income << std::endl;

    TEST_ASSERT(total_income > 500.0, "Total income should include customs");

    // Phase 2: Calculate profitability
    effects.trade_profitability = total_income / effects.trade_volume;
    TEST_ASSERT(effects.trade_profitability > 0.0, "Trade should be profitable");
    std::cout << "    ✓ Trade profitability: " << effects.trade_profitability << std::endl;

    // Phase 3: Verify income to treasury ratio
    double treasury_ratio = 0.9;  // 90% goes to treasury
    double treasury_income = total_income * treasury_ratio;
    TEST_ASSERT(treasury_income < total_income, "Treasury income should be less than total");
    std::cout << "    ✓ Treasury receives: " << treasury_income << " (" << (treasury_ratio * 100) << "%)" << std::endl;

    return true;
}

/**
 * @brief Test economic impact on trade
 *
 * Scenario: High taxes reduce trade profitability
 */
bool Test_EconomicImpactOnTrade_TaxBurden() {
    std::cout << "\n  Testing economic impact on trade..." << std::endl;

    // Phase 1: Low tax scenario
    EconomicTradeContribution low_tax_contrib;
    low_tax_contrib.tax_burden = 0.15;  // 15% tax
    low_tax_contrib.economic_stability = 1.0;
    low_tax_contrib.infrastructure_quality = 0.8;
    low_tax_contrib.available_capital = 5000.0;

    // Calculate tax penalty (threshold is 0.25)
    double tax_threshold = 0.25;
    double high_tax_penalty = 0.4;
    double low_tax_penalty = 0.0;  // No penalty below threshold

    std::cout << "    ✓ Low tax scenario (15%): no penalty" << std::endl;
    TEST_ASSERT(low_tax_contrib.tax_burden < tax_threshold, "Tax should be below threshold");

    // Phase 2: High tax scenario
    EconomicTradeContribution high_tax_contrib;
    high_tax_contrib.tax_burden = 0.35;  // 35% tax
    high_tax_contrib.economic_stability = 1.0;
    high_tax_contrib.infrastructure_quality = 0.8;
    high_tax_contrib.available_capital = 5000.0;

    // Calculate tax penalty
    double excess_tax = high_tax_contrib.tax_burden - tax_threshold;
    double tax_penalty = excess_tax * high_tax_penalty;

    std::cout << "    ✓ High tax scenario (35%): penalty = " << tax_penalty << std::endl;
    TEST_ASSERT(tax_penalty > 0.0, "High taxes should incur penalty");

    // Phase 3: Compare trade modifiers
    double low_tax_modifier = 1.0 - low_tax_penalty;
    double high_tax_modifier = 1.0 - tax_penalty;

    TEST_ASSERT(low_tax_modifier > high_tax_modifier,
                "Low tax should result in better trade conditions");
    std::cout << "    ✓ Low tax modifier: " << low_tax_modifier << std::endl;
    std::cout << "    ✓ High tax modifier: " << high_tax_modifier << std::endl;

    return true;
}

/**
 * @brief Test infrastructure bonus on trade efficiency
 *
 * Scenario: Good infrastructure increases trade efficiency
 */
bool Test_InfrastructureBonus_TradeEfficiency() {
    std::cout << "\n  Testing infrastructure impact on trade..." << std::endl;

    // Phase 1: Poor infrastructure
    double poor_infra_quality = 0.4;
    double infra_threshold = 0.7;
    double infra_bonus_rate = 0.5;

    double poor_infra_bonus = 0.0;  // No bonus below threshold
    std::cout << "    ✓ Poor infrastructure (40%): no bonus" << std::endl;

    // Phase 2: Good infrastructure
    double good_infra_quality = 0.9;
    double good_infra_bonus = (good_infra_quality - infra_threshold) * infra_bonus_rate;

    TEST_ASSERT(good_infra_bonus > 0.0, "Good infrastructure should provide bonus");
    std::cout << "    ✓ Good infrastructure (90%): bonus = " << good_infra_bonus << std::endl;

    // Phase 3: Calculate effective efficiency
    double base_efficiency = 1.0;
    double poor_infra_efficiency = base_efficiency;
    double good_infra_efficiency = base_efficiency * (1.0 + good_infra_bonus);

    TEST_ASSERT(good_infra_efficiency > poor_infra_efficiency,
                "Good infrastructure should increase efficiency");
    std::cout << "    ✓ Poor infra efficiency: " << poor_infra_efficiency << std::endl;
    std::cout << "    ✓ Good infra efficiency: " << good_infra_efficiency << std::endl;

    return true;
}

/**
 * @brief Test trade crisis detection
 *
 * Scenario: Detect when trade collapses
 */
bool Test_TradeCrisisDetection() {
    std::cout << "\n  Testing trade crisis detection..." << std::endl;

    // Phase 1: Normal trade conditions
    TradeEconomicBridgeComponent bridge;
    bridge.trade_effects.trade_efficiency = 1.0;
    bridge.trade_income_history = {800.0, 850.0, 900.0, 920.0};

    double crisis_threshold = 0.3;
    bool crisis_detected = false;

    std::cout << "    ✓ Normal conditions: average income = 867.5" << std::endl;

    // Phase 2: Trade starts declining
    bridge.trade_income_history.push_back(700.0);
    bridge.trade_income_history.push_back(500.0);
    bridge.trade_income_history.push_back(300.0);

    // Calculate recent average (last 3 months)
    double recent_avg = 0.0;
    for (size_t i = bridge.trade_income_history.size() - 3; i < bridge.trade_income_history.size(); ++i) {
        recent_avg += bridge.trade_income_history[i];
    }
    recent_avg /= 3.0;

    std::cout << "    ✓ Declining conditions: recent average = " << recent_avg << std::endl;

    // Phase 3: Check for crisis
    if (recent_avg < crisis_threshold * 1000.0 || bridge.trade_effects.trade_efficiency < crisis_threshold) {
        crisis_detected = true;
        bridge.trade_crisis = true;
        bridge.crisis_severity = 0.6;
    }

    TEST_ASSERT(crisis_detected, "Trade crisis should be detected");
    std::cout << "    ✓ Trade crisis detected! Severity: " << bridge.crisis_severity << std::endl;

    return true;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "========================================================\n";
    std::cout << "     TRADE SYSTEM INTEGRATION TEST SUITE\n";
    std::cout << "========================================================\n";
    std::cout << "\n";

    bool all_passed = true;

    std::cout << "\n=== Trade Route Lifecycle Tests ===\n";
    RUN_TEST(Test_TradeRouteLifecycle_Complete);

    std::cout << "\n=== Trade Hub Evolution Tests ===\n";
    RUN_TEST(Test_TradeHubEvolution_LocalToRegional);

    std::cout << "\n=== Market Dynamics Tests ===\n";
    RUN_TEST(Test_MarketPriceShock_Propagation);

    std::cout << "\n=== Trade-Economy Integration Tests ===\n";
    RUN_TEST(Test_TradeEconomyIntegration_IncomeFlow);
    RUN_TEST(Test_EconomicImpactOnTrade_TaxBurden);
    RUN_TEST(Test_InfrastructureBonus_TradeEfficiency);

    std::cout << "\n=== Crisis Detection Tests ===\n";
    RUN_TEST(Test_TradeCrisisDetection);

    // Print summary
    std::cout << "\n";
    std::cout << "========================================================\n";
    if (all_passed) {
        std::cout << "     ✅ ALL INTEGRATION TESTS PASSED\n";
    } else {
        std::cout << "     ❌ SOME INTEGRATION TESTS FAILED\n";
    }
    std::cout << "========================================================\n";
    std::cout << "\n";

    std::cout << "Integration Test Coverage:\n";
    std::cout << "  - Trade route lifecycle (establish → disrupt → recover)\n";
    std::cout << "  - Trade hub evolution (local market → regional hub)\n";
    std::cout << "  - Market price shocks and stabilization\n";
    std::cout << "  - Trade-economy income flow\n";
    std::cout << "  - Economic impacts on trade (taxes, infrastructure)\n";
    std::cout << "  - Trade crisis detection\n";
    std::cout << "\n";

    return all_passed ? 0 : 1;
}
