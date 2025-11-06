// ============================================================================
// Mechanica Imperii - Trade Calculator Header
// Pure Calculation Functions for Trade System
// ============================================================================

#pragma once

#include "game/trade/TradeSystem.h"
#include "core/types/game_types.h"
#include <optional>
#include <unordered_map>

namespace game::trade {

    /**
     * @brief Pure calculation functions for trade system
     *
     * All functions are static and have no side effects.
     * This makes them:
     * - Easy to test
     * - Easy to reason about
     * - Thread-safe by design
     * - Reusable across systems
     */
    class TradeCalculator {
    public:
        // ====================================================================
        // Price Calculations
        // ====================================================================

        /**
         * @brief Calculate market price based on supply and demand
         * @param base_price Base value of the resource
         * @param supply Current supply level (1.0 = balanced)
         * @param demand Current demand level (1.0 = balanced)
         * @return Calculated market price
         */
        static double CalculateMarketPrice(double base_price, double supply, double demand);

        /**
         * @brief Calculate price with seasonal adjustments
         */
        static double ApplySeasonalPriceAdjustment(double base_price, int month,
                                                   const TradeGoodProperties& good_properties);

        /**
         * @brief Calculate price deviation from average
         */
        static double CalculatePriceDeviation(double current_price, double avg_price);

        /**
         * @brief Clamp price to reasonable bounds
         */
        static double ClampPrice(double price, double min_price = 0.1, double max_price = 100.0);

        // ====================================================================
        // Supply and Demand Calculations
        // ====================================================================

        /**
         * @brief Calculate supply level for a resource (deterministic)
         * @param province_id Province to calculate supply for
         * @param resource Resource type
         * @param game_tick Current game tick for deterministic variation
         * @note This is a simplified calculation; would integrate with production systems
         */
        static double CalculateSupplyLevel(types::EntityID province_id, types::ResourceType resource, 
                                          uint64_t game_tick = 0);

        /**
         * @brief Calculate demand level for a resource (deterministic)
         * @param province_id Province to calculate demand for
         * @param resource Resource type
         * @param game_tick Current game tick for deterministic variation
         * @note This is a simplified calculation; would integrate with population systems
         */
        static double CalculateDemandLevel(types::EntityID province_id, types::ResourceType resource,
                                          uint64_t game_tick = 0);

        /**
         * @brief Calculate supply/demand ratio
         */
        static double CalculateSupplyDemandRatio(double supply, double demand);

        // ====================================================================
        // Route Profitability Calculations
        // ====================================================================

        /**
         * @brief Calculate profitability of an existing trade route
         */
        static double CalculateRouteProfitability(const TradeRoute& route);

        /**
         * @brief Estimate profitability of a potential trade route
         */
        static double EstimateRouteProfitability(double source_price, double dest_price,
                                                double transport_cost, double safety, double efficiency);

        /**
         * @brief Calculate profit per unit
         */
        static double CalculateProfitPerUnit(double source_price, double dest_price, double transport_cost);

        /**
         * @brief Calculate profit margin (as percentage)
         */
        static double CalculateProfitMargin(double profit_per_unit, double source_price);

        // ====================================================================
        // Transport Cost Calculations
        // ====================================================================

        /**
         * @brief Calculate transport cost for a route (deterministic)
         * @param distance Route distance
         * @param bulk_factor Bulk factor of goods
         * @param perishability Perishability factor
         * @param efficiency Route efficiency
         * @param resource Resource type for seed
         * @param game_tick Current game tick for deterministic variation
         */
        static double CalculateTransportCost(double distance, double bulk_factor,
                                           double perishability, double efficiency,
                                           types::ResourceType resource = types::ResourceType::FOOD,
                                           uint64_t game_tick = 0);

        /**
         * @brief Calculate base transport cost per kilometer
         */
        static double GetBaseTransportCostPerKm(RouteType route_type);

        /**
         * @brief Calculate transport cost with route type modifiers
         */
        static double ApplyRouteTypeModifier(double base_cost, RouteType route_type);

        // ====================================================================
        // Distance and Geography Calculations
        // ====================================================================

        /**
         * @brief Calculate distance between provinces (deterministic)
         * @param province1 First province
         * @param province2 Second province
         * @param game_tick Current game tick for deterministic variation
         * @note Simplified calculation; would use actual coordinates in production
         */
        static double CalculateDistance(types::EntityID province1, types::EntityID province2,
                                       uint64_t game_tick = 0);

        /**
         * @brief Calculate route efficiency based on infrastructure
         */
        static double CalculateRouteEfficiency(bool has_river, bool has_road, bool has_sea_route);

        /**
         * @brief Calculate route safety rating (deterministic)
         * @param distance Route distance
         * @param province1 First province for seed
         * @param province2 Second province for seed
         * @param game_tick Current game tick for deterministic variation
         * @param base_safety Base safety value (default 0.9)
         */
        static double CalculateRouteSafety(double distance, types::EntityID province1,
                                          types::EntityID province2, uint64_t game_tick = 0,
                                          double base_safety = 0.9);

        /**
         * @brief Calculate travel time in days
         */
        static double CalculateTravelTime(double distance, RouteType route_type);

        // ====================================================================
        // Hub Calculations
        // ====================================================================

        /**
         * @brief Calculate hub capacity based on province characteristics (deterministic)
         * @param province_id Province ID
         * @param hub_type Type of hub
         * @param game_tick Current game tick for deterministic variation
         */
        static double CalculateHubCapacity(types::EntityID province_id, HubType hub_type,
                                          uint64_t game_tick = 0);

        /**
         * @brief Calculate effective hub capacity with bonuses
         */
        static double CalculateEffectiveHubCapacity(double base_capacity, double infrastructure_bonus,
                                                   double reputation_rating);

        /**
         * @brief Calculate hub utilization percentage
         */
        static double CalculateHubUtilization(double current_volume, double max_capacity);

        /**
         * @brief Determine infrastructure bonus for a province (deterministic)
         * @param province_id Province ID
         * @param game_tick Current game tick for deterministic variation
         */
        static double DetermineInfrastructureBonus(types::EntityID province_id, uint64_t game_tick = 0);

        /**
         * @brief Calculate hub reputation based on performance
         */
        static double CalculateHubReputation(double trade_volume, double safety, double efficiency);

        // ====================================================================
        // Market Force Calculations
        // ====================================================================

        /**
         * @brief Calculate price change due to supply/demand shifts
         */
        static double CalculatePriceChangeFromMarketForces(double supply_change, double demand_change,
                                                          double price_elasticity = 0.5);

        /**
         * @brief Calculate volatility impact on price
         */
        static double CalculateVolatilityImpact(double base_volatility, double market_shock);

        /**
         * @brief Calculate price stabilization adjustment
         */
        static double CalculateStabilizationAdjustment(double current_price, double avg_price,
                                                      double stabilization_factor = 0.05);

        // ====================================================================
        // Volume and Flow Calculations
        // ====================================================================

        /**
         * @brief Calculate effective trade volume with modifiers
         */
        static double CalculateEffectiveVolume(double base_volume, double efficiency,
                                              double safety, double seasonal_modifier);

        /**
         * @brief Calculate optimal trade volume
         */
        static double CalculateOptimalVolume(double supply, double demand, double capacity_limit);

        /**
         * @brief Calculate total trade value (volume * price)
         */
        static double CalculateTradeValue(double volume, double price);

        // ====================================================================
        // Economic Analysis
        // ====================================================================

        /**
         * @brief Calculate monthly income from a route
         */
        static double CalculateMonthlyIncome(double volume, double dest_price,
                                            double source_price, double transport_cost);

        /**
         * @brief Calculate monthly expenses for a route
         */
        static double CalculateMonthlyExpenses(double volume, double source_price, double transport_cost);

        /**
         * @brief Calculate net trade balance
         */
        static double CalculateNetBalance(double income, double expenses);

        /**
         * @brief Calculate return on investment (ROI)
         */
        static double CalculateROI(double profit, double investment);

        // ====================================================================
        // Utility Functions
        // ====================================================================

        /**
         * @brief Clamp value to range
         */
        static double Clamp(double value, double min_val, double max_val);

        /**
         * @brief Linear interpolation
         */
        static double Lerp(double a, double b, double t);

        /**
         * @brief Calculate percentage change
         */
        static double CalculatePercentageChange(double old_value, double new_value);

    private:
        // Constants for calculations
        static constexpr double BASE_TRANSPORT_COST_PER_KM = 0.01;
        static constexpr double MIN_PRICE = 0.1;
        static constexpr double MAX_PRICE = 100.0;
        static constexpr double BASE_SAFETY = 0.9;
        static constexpr double DISTANCE_PENALTY_THRESHOLD = 2000.0;
    };

} // namespace game::trade
