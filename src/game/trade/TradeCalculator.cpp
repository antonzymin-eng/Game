// ============================================================================
// Mechanica Imperii - Trade Calculator Implementation
// Pure Calculation Functions for Trade System
// ============================================================================

#include "game/trade/TradeCalculator.h"
#include "utils/RandomGenerator.h"
#include <algorithm>
#include <cmath>

namespace game::trade {

    // ========================================================================
    // Price Calculations
    // ========================================================================

    double TradeCalculator::CalculateMarketPrice(double base_price, double supply, double demand) {
        if (supply <= 0.0) supply = 0.01; // Prevent division by zero
        if (demand <= 0.0) demand = 0.01;

        double price_modifier = demand / supply;
        price_modifier = Clamp(price_modifier, 0.2, 5.0); // Reasonable bounds

        return base_price * price_modifier;
    }

    double TradeCalculator::ApplySeasonalPriceAdjustment(double base_price, int month,
                                                        const TradeGoodProperties& good_properties) {
        double seasonal_demand = good_properties.GetSeasonalDemandMultiplier(month);
        double seasonal_supply = good_properties.GetSeasonalSupplyMultiplier(month);

        double price_adjustment = seasonal_demand / seasonal_supply;
        return base_price * price_adjustment;
    }

    double TradeCalculator::CalculatePriceDeviation(double current_price, double avg_price) {
        if (avg_price > 0.0) {
            return (current_price - avg_price) / avg_price;
        }
        return 0.0;
    }

    double TradeCalculator::ClampPrice(double price, double min_price, double max_price) {
        return Clamp(price, min_price, max_price);
    }

    // ========================================================================
    // Supply and Demand Calculations
    // ========================================================================

    double TradeCalculator::CalculateSupplyLevel(types::EntityID province_id, types::ResourceType resource,
                                                 uint64_t game_tick) {
        // Simplified calculation - would integrate with production systems
        double base_supply = 1.0;
        
        // Use deterministic variation based on province_id, resource, and game_tick
        uint64_t seed = utils::RandomGenerator::createSeed(
            static_cast<uint64_t>(province_id),
            static_cast<uint64_t>(resource),
            game_tick,
            1ULL  // Category identifier for supply
        );
        double variation = utils::RandomGenerator::deterministicFloat(seed, 0.5f, 2.0f);
        
        return base_supply * variation;
    }

    double TradeCalculator::CalculateDemandLevel(types::EntityID province_id, types::ResourceType resource,
                                                 uint64_t game_tick) {
        // Simplified calculation - would integrate with population systems
        double base_demand = 1.0;
        
        // Use deterministic variation based on province_id, resource, and game_tick
        uint64_t seed = utils::RandomGenerator::createSeed(
            static_cast<uint64_t>(province_id),
            static_cast<uint64_t>(resource),
            game_tick,
            2ULL  // Category identifier for demand (different from supply)
        );
        double variation = utils::RandomGenerator::deterministicFloat(seed, 0.6f, 1.8f);
        
        return base_demand * variation;
    }

    double TradeCalculator::CalculateSupplyDemandRatio(double supply, double demand) {
        if (demand <= 0.0) return 0.0;
        return supply / demand;
    }

    // ========================================================================
    // Route Profitability Calculations
    // ========================================================================

    double TradeCalculator::CalculateRouteProfitability(const TradeRoute& route) {
        if (route.destination_price <= route.source_price) {
            return 0.0; // No profit if destination price is not higher
        }

        double profit_per_unit = route.destination_price - route.source_price - route.transport_cost_per_unit;
        double profit_margin = profit_per_unit / route.source_price;

        // Adjust for risk factors
        profit_margin *= route.safety_rating * route.efficiency_rating;

        return std::max(0.0, profit_margin);
    }

    double TradeCalculator::EstimateRouteProfitability(double source_price, double dest_price,
                                                      double transport_cost, double safety, double efficiency) {
        if (dest_price <= source_price) {
            return 0.0;
        }

        double profit_per_unit = dest_price - source_price - transport_cost;
        double profit_margin = profit_per_unit / source_price;

        // Adjust for estimated route factors
        profit_margin *= safety * efficiency;

        return std::max(0.0, profit_margin);
    }

    double TradeCalculator::CalculateProfitPerUnit(double source_price, double dest_price, double transport_cost) {
        return dest_price - source_price - transport_cost;
    }

    double TradeCalculator::CalculateProfitMargin(double profit_per_unit, double source_price) {
        if (source_price <= 0.0) return 0.0;
        return profit_per_unit / source_price;
    }

    // ========================================================================
    // Transport Cost Calculations
    // ========================================================================

    double TradeCalculator::CalculateTransportCost(double distance, double bulk_factor,
                                                  double perishability, double efficiency,
                                                  RouteType route_type,
                                                  types::ResourceType resource, uint64_t game_tick) {
        // Base transport cost per km with route type modifier
        double base_cost_per_km = GetBaseTransportCostPerKm(route_type);
        double transport_cost = distance * base_cost_per_km * bulk_factor;

        // Apply perishability penalty (spoilage risk)
        transport_cost *= (1.0 + perishability * distance / 1000.0);

        // Apply route efficiency modifier
        if (efficiency > 0.0) {
            transport_cost /= efficiency;
        }
        
        // Add small deterministic variation based on resource and conditions
        uint64_t seed = utils::RandomGenerator::createSeed(
            static_cast<uint64_t>(resource),
            game_tick,
            7ULL  // Category identifier for transport cost
        );
        double variation = utils::RandomGenerator::deterministicFloat(seed, 0.9f, 1.1f);

        return transport_cost * variation;
    }

    double TradeCalculator::GetBaseTransportCostPerKm(RouteType route_type) {
        switch (route_type) {
        case RouteType::LAND: return BASE_TRANSPORT_COST_PER_KM * 1.0;
        case RouteType::RIVER: return BASE_TRANSPORT_COST_PER_KM * 0.7;
        case RouteType::COASTAL: return BASE_TRANSPORT_COST_PER_KM * 0.5;
        case RouteType::SEA: return BASE_TRANSPORT_COST_PER_KM * 0.3;
        case RouteType::OVERLAND_LONG: return BASE_TRANSPORT_COST_PER_KM * 1.5;
        default: return BASE_TRANSPORT_COST_PER_KM;
        }
    }

    double TradeCalculator::ApplyRouteTypeModifier(double base_cost, RouteType route_type) {
        double modifier = GetBaseTransportCostPerKm(route_type) / BASE_TRANSPORT_COST_PER_KM;
        return base_cost * modifier;
    }

    // ========================================================================
    // Distance and Geography Calculations
    // ========================================================================

    double TradeCalculator::CalculateDistance(types::EntityID province1, types::EntityID province2,
                                             uint64_t game_tick) {
        if (province1 == province2) return 0.0;

        // Generate consistent distance based on ID difference
        int id_diff = std::abs(static_cast<int>(province2) - static_cast<int>(province1));
        double base_distance = id_diff * 25.0; // 25km per ID unit difference

        // Add deterministic variation to make it more realistic
        uint64_t seed = utils::RandomGenerator::createSeed(
            static_cast<uint64_t>(province1),
            static_cast<uint64_t>(province2),
            game_tick,
            3ULL  // Category identifier for distance
        );
        double variation = utils::RandomGenerator::deterministicFloat(seed, 0.8f, 1.2f);

        return base_distance * variation;
    }

    double TradeCalculator::CalculateRouteEfficiency(bool has_river, bool has_road, bool has_sea_route) {
        double efficiency = 1.0;

        if (has_river) efficiency *= 1.3;
        if (has_road) efficiency *= 1.2;
        if (has_sea_route) efficiency *= 1.5;

        return std::min(2.0, efficiency); // Cap at 200% efficiency
    }

    double TradeCalculator::CalculateRouteSafety(double distance, types::EntityID province1,
                                                 types::EntityID province2, uint64_t game_tick,
                                                 double base_safety) {
        double safety = base_safety;

        // Longer routes are generally less safe
        double distance_penalty = distance / DISTANCE_PENALTY_THRESHOLD;
        safety -= std::min(0.3, distance_penalty);

        // Deterministic variation for different route conditions
        uint64_t seed = utils::RandomGenerator::createSeed(
            static_cast<uint64_t>(province1),
            static_cast<uint64_t>(province2),
            game_tick,
            4ULL  // Category identifier for safety
        );
        double variation = utils::RandomGenerator::deterministicFloat(seed, 0.9f, 1.1f);

        return Clamp(safety * variation, 0.1, 1.0);
    }

    double TradeCalculator::CalculateTravelTime(double distance, RouteType route_type) {
        double speed_km_per_day = 50.0; // Default land speed

        switch (route_type) {
        case RouteType::LAND: speed_km_per_day = 50.0; break;
        case RouteType::RIVER: speed_km_per_day = 70.0; break;
        case RouteType::COASTAL: speed_km_per_day = 80.0; break;
        case RouteType::SEA: speed_km_per_day = 100.0; break;
        case RouteType::OVERLAND_LONG: speed_km_per_day = 30.0; break;
        }

        return distance / speed_km_per_day;
    }

    // ========================================================================
    // Hub Calculations
    // ========================================================================

    double TradeCalculator::CalculateHubCapacity(types::EntityID province_id, HubType hub_type,
                                                 uint64_t game_tick) {
        double base_capacity = 100.0;

        // Apply hub type multipliers
        switch (hub_type) {
        case HubType::LOCAL_MARKET: base_capacity *= 1.0; break;
        case HubType::REGIONAL_HUB: base_capacity *= 2.0; break;
        case HubType::MAJOR_TRADING_CENTER: base_capacity *= 5.0; break;
        case HubType::INTERNATIONAL_PORT: base_capacity *= 10.0; break;
        case HubType::CROSSROADS: base_capacity *= 3.0; break;
        }

        // Add deterministic variation based on province characteristics
        uint64_t seed = utils::RandomGenerator::createSeed(
            static_cast<uint64_t>(province_id),
            static_cast<uint64_t>(hub_type),
            game_tick,
            5ULL  // Category identifier for hub capacity
        );
        double variation = utils::RandomGenerator::deterministicFloat(seed, 0.5f, 2.0f);

        return base_capacity * variation;
    }

    double TradeCalculator::CalculateEffectiveHubCapacity(double base_capacity, double infrastructure_bonus,
                                                         double reputation_rating) {
        return base_capacity * infrastructure_bonus * (reputation_rating * 0.5 + 0.5);
    }

    double TradeCalculator::CalculateHubUtilization(double current_volume, double max_capacity) {
        if (max_capacity <= 0.0) return 0.0;
        return Clamp(current_volume / max_capacity, 0.0, 1.5); // Allow overutilization up to 150%
    }

    double TradeCalculator::DetermineInfrastructureBonus(types::EntityID province_id, uint64_t game_tick) {
        // Would check actual infrastructure systems
        // For now, return deterministic base bonus with variation
        uint64_t seed = utils::RandomGenerator::createSeed(
            static_cast<uint64_t>(province_id),
            game_tick,
            6ULL  // Category identifier for infrastructure
        );
        return utils::RandomGenerator::deterministicFloat(seed, 0.8f, 1.5f);
    }

    double TradeCalculator::CalculateHubReputation(double trade_volume, double safety, double efficiency) {
        double trade_volume_factor = std::min(2.0, trade_volume);
        double reputation = (trade_volume_factor + safety + efficiency) / 3.0;
        return Clamp(reputation, 0.5, 2.0);
    }

    // ========================================================================
    // Market Force Calculations
    // ========================================================================

    double TradeCalculator::CalculatePriceChangeFromMarketForces(double supply_change, double demand_change,
                                                                double price_elasticity) {
        double supply_effect = -supply_change * price_elasticity; // More supply = lower price
        double demand_effect = demand_change * price_elasticity;   // More demand = higher price

        return supply_effect + demand_effect;
    }

    double TradeCalculator::CalculateVolatilityImpact(double base_volatility, double market_shock) {
        return base_volatility + std::abs(market_shock);
    }

    double TradeCalculator::CalculateStabilizationAdjustment(double current_price, double avg_price,
                                                            double stabilization_factor) {
        if (avg_price <= 0.0) return 0.0;

        double price_deviation = current_price - avg_price;
        return -price_deviation * stabilization_factor;
    }

    // ========================================================================
    // Volume and Flow Calculations
    // ========================================================================

    double TradeCalculator::CalculateEffectiveVolume(double base_volume, double efficiency,
                                                    double safety, double seasonal_modifier) {
        return base_volume * efficiency * safety * seasonal_modifier;
    }

    double TradeCalculator::CalculateOptimalVolume(double supply, double demand, double capacity_limit) {
        double optimal = std::min(supply, demand) * 0.1; // 10% of available supply/demand
        return std::min(optimal, capacity_limit);
    }

    double TradeCalculator::CalculateTradeValue(double volume, double price) {
        return volume * price;
    }

    // ========================================================================
    // Economic Analysis
    // ========================================================================

    double TradeCalculator::CalculateMonthlyIncome(double volume, double dest_price,
                                                  double source_price, double transport_cost) {
        double profit_per_unit = dest_price - source_price - transport_cost;
        return std::max(0.0, volume * profit_per_unit);
    }

    double TradeCalculator::CalculateMonthlyExpenses(double volume, double source_price, double transport_cost) {
        return volume * (source_price + transport_cost);
    }

    double TradeCalculator::CalculateNetBalance(double income, double expenses) {
        return income - expenses;
    }

    double TradeCalculator::CalculateROI(double profit, double investment) {
        if (investment <= 0.0) return 0.0;
        return profit / investment;
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    double TradeCalculator::Clamp(double value, double min_val, double max_val) {
        return std::max(min_val, std::min(value, max_val));
    }

    double TradeCalculator::Lerp(double a, double b, double t) {
        return a + (b - a) * Clamp(t, 0.0, 1.0);
    }

    double TradeCalculator::CalculatePercentageChange(double old_value, double new_value) {
        if (old_value == 0.0) return 0.0;
        return ((new_value - old_value) / old_value) * 100.0;
    }

} // namespace game::trade
