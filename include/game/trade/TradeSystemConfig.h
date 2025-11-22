// ============================================================================
// Mechanica Imperii - Trade System Configuration
// Centralized configuration for trade system tuning
// Created: 2025-11-22
// Location: include/game/trade/TradeSystemConfig.h
// ============================================================================

#pragma once

#include <string>

namespace game::trade {

    /**
     * @brief Configuration structure for Trade System
     *
     * IMPROVEMENT (Issue #4): Extracted magic numbers to configuration
     * for easier gameplay tuning and testing.
     *
     * All economic thresholds, multipliers, and game balance parameters
     * are centralized here for easy modification without recompiling.
     */
    struct TradeSystemConfig {
        // ====================================================================
        // Route Viability Thresholds
        // ====================================================================
        double min_viable_profitability = 0.05;    // 5% minimum profit margin
        double min_viable_safety = 0.3;            // 30% minimum safety rating
        double min_viable_volume = 0.0;            // Minimum trade volume

        // ====================================================================
        // Market Price Thresholds
        // ====================================================================
        double price_above_average_threshold = 1.1;  // 10% above average
        double price_shock_threshold = 0.5;           // 50% change = shock
        double price_volatility_threshold = 0.3;      // 30% volatility
        double min_market_price = 0.1;                // Minimum price floor
        double max_market_price = 100.0;              // Maximum price ceiling

        // ====================================================================
        // Hub Evolution Thresholds
        // ====================================================================
        struct HubThresholds {
            // Trade volume thresholds for hub type evolution
            double international_port_volume = 1000.0;
            double major_trading_center_volume = 500.0;
            double regional_hub_volume = 100.0;

            // Route count thresholds
            int international_port_routes = 20;
            int major_trading_center_routes = 10;
            int crossroads_routes = 6;
            int regional_hub_routes = 3;

            // Capacity multipliers by hub type
            double local_market_capacity = 100.0;
            double regional_hub_capacity = 250.0;
            double crossroads_capacity = 300.0;
            double major_trading_center_capacity = 500.0;
            double international_port_capacity = 1000.0;

            // Infrastructure bonuses per upgrade level
            double infrastructure_bonus_per_level = 0.15;  // 15% per level
            double capacity_bonus_per_level = 0.25;        // 25% per level
            double security_bonus_per_level = 0.1;         // 10% per level
            int max_upgrade_level = 5;
        } hub_thresholds;

        // ====================================================================
        // Performance Configuration
        // ====================================================================
        struct Performance {
            double update_frequency = 0.2;              // 5 updates per second
            float price_update_interval = 30.0f;        // Update prices every 30 seconds
            size_t max_routes_per_frame = 25;           // Limit processing per frame
            size_t pathfinder_cache_size = 1000;        // Max cached paths
            double max_trade_distance_km = 2000.0;      // Maximum trade route distance
        } performance;

        // ====================================================================
        // Economic Balance Parameters
        // ====================================================================
        struct Economic {
            // Transport costs
            double base_transport_cost_per_km = 0.01;
            double land_distance_modifier = 1.0;
            double river_distance_modifier = 0.8;
            double coastal_distance_modifier = 0.9;
            double sea_distance_modifier = 1.2;
            double overland_long_distance_modifier = 2.0;

            // Route efficiency
            double land_efficiency = 0.8;
            double river_efficiency = 1.2;
            double coastal_efficiency = 1.1;
            double sea_efficiency = 1.5;
            double overland_long_efficiency = 0.6;

            // Travel speeds (km per day)
            double land_speed = 50.0;
            double river_speed = 70.0;
            double coastal_speed = 80.0;
            double sea_speed = 100.0;
            double overland_long_speed = 30.0;

            // Market dynamics
            double supply_demand_elasticity = 0.5;
            double price_stabilization_factor = 0.05;
            double volatility_reduction_rate = 0.99;     // 1% per update
            double min_volatility = 0.01;

            // Seasonal impact
            double seasonal_demand_variation = 0.3;      // ±30% seasonal variation
            double seasonal_supply_variation = 0.3;

            // Hub specialization
            double specialization_threshold = 0.2;       // 20% of capacity
            double specialization_efficiency_bonus = 0.3; // 30% efficiency bonus
        } economic;

        // ====================================================================
        // Safety and Risk Parameters
        // ====================================================================
        struct Safety {
            double min_route_safety = 0.1;               // 10% minimum safety
            double max_route_efficiency = 2.0;           // 200% maximum efficiency
            double base_safety = 0.9;                    // Default safety rating
            double safety_variation_range = 0.2;         // ±20% variation
            double distance_penalty_threshold = 2000.0;  // Distance penalty start

            // Recovery parameters
            double recovery_rate_per_month = 0.1;        // 10% recovery per month
            double min_recovery_months = 1.0;
            double max_recovery_months = 12.0;
        } safety;

        // ====================================================================
        // Debug and Logging
        // ====================================================================
        struct Debug {
            bool enable_trade_logging = false;
            bool enable_price_logging = false;
            bool enable_route_logging = false;
            bool enable_hub_logging = false;
            bool enable_pathfinder_logging = false;
            double performance_warning_threshold_ms = 16.0; // Warn if update > 16ms
        } debug;

        // ====================================================================
        // Configuration Management
        // ====================================================================

        /**
         * @brief Load configuration from JSON file
         * @param config_file Path to configuration file
         * @return true if loaded successfully
         */
        bool LoadFromFile(const std::string& config_file);

        /**
         * @brief Save configuration to JSON file
         * @param config_file Path to configuration file
         * @return true if saved successfully
         */
        bool SaveToFile(const std::string& config_file) const;

        /**
         * @brief Reset to default values
         */
        void Reset();

        /**
         * @brief Validate configuration values
         * @param error_message Output parameter for validation errors
         * @return true if configuration is valid
         */
        bool Validate(std::string& error_message) const;

        /**
         * @brief Get configuration as JSON string (for debugging)
         */
        std::string ToString() const;

        // Default constructor initializes with default values
        TradeSystemConfig() = default;
    };

} // namespace game::trade
