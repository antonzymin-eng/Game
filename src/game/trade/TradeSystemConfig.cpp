// ============================================================================
// Mechanica Imperii - Trade System Configuration Implementation
// JSON Load/Save and Validation
// Created: 2025-11-22
// Location: src/game/trade/TradeSystemConfig.cpp
// ============================================================================

#include "game/trade/TradeSystemConfig.h"
#include "core/logging/Logger.h"
#include <json/json.h>
#include <fstream>
#include <sstream>

namespace game::trade {

    // ========================================================================
    // Configuration File I/O
    // ========================================================================

    bool TradeSystemConfig::LoadFromFile(const std::string& config_file) {
        try {
            std::ifstream file(config_file);
            if (!file.is_open()) {
                CORE_STREAM_ERROR("TradeSystemConfig") << "Failed to open config file: " << config_file;
                return false;
            }

            Json::Value root;
            Json::CharReaderBuilder reader;
            std::string errors;

            if (!Json::parseFromStream(reader, file, &root, &errors)) {
                CORE_STREAM_ERROR("TradeSystemConfig") << "Failed to parse JSON: " << errors;
                return false;
            }

            // Load route viability thresholds
            if (root.isMember("route_viability")) {
                const auto& rv = root["route_viability"];
                min_viable_profitability = rv.get("min_profitability", 0.05).asDouble();
                min_viable_safety = rv.get("min_safety", 0.3).asDouble();
                min_viable_volume = rv.get("min_volume", 0.0).asDouble();
            }

            // Load market price thresholds
            if (root.isMember("market_prices")) {
                const auto& mp = root["market_prices"];
                price_above_average_threshold = mp.get("above_average_threshold", 1.1).asDouble();
                price_shock_threshold = mp.get("shock_threshold", 0.5).asDouble();
                price_volatility_threshold = mp.get("volatility_threshold", 0.3).asDouble();
                min_market_price = mp.get("min_price", 0.1).asDouble();
                max_market_price = mp.get("max_price", 100.0).asDouble();
            }

            // Load hub thresholds
            if (root.isMember("hub_thresholds")) {
                const auto& ht = root["hub_thresholds"];
                hub_thresholds.international_port_volume = ht.get("international_port_volume", 1000.0).asDouble();
                hub_thresholds.major_trading_center_volume = ht.get("major_trading_center_volume", 500.0).asDouble();
                hub_thresholds.regional_hub_volume = ht.get("regional_hub_volume", 100.0).asDouble();

                hub_thresholds.international_port_routes = ht.get("international_port_routes", 20).asInt();
                hub_thresholds.major_trading_center_routes = ht.get("major_trading_center_routes", 10).asInt();
                hub_thresholds.crossroads_routes = ht.get("crossroads_routes", 6).asInt();
                hub_thresholds.regional_hub_routes = ht.get("regional_hub_routes", 3).asInt();

                hub_thresholds.local_market_capacity = ht.get("local_market_capacity", 100.0).asDouble();
                hub_thresholds.regional_hub_capacity = ht.get("regional_hub_capacity", 250.0).asDouble();
                hub_thresholds.crossroads_capacity = ht.get("crossroads_capacity", 300.0).asDouble();
                hub_thresholds.major_trading_center_capacity = ht.get("major_trading_center_capacity", 500.0).asDouble();
                hub_thresholds.international_port_capacity = ht.get("international_port_capacity", 1000.0).asDouble();

                hub_thresholds.infrastructure_bonus_per_level = ht.get("infrastructure_bonus_per_level", 0.15).asDouble();
                hub_thresholds.capacity_bonus_per_level = ht.get("capacity_bonus_per_level", 0.25).asDouble();
                hub_thresholds.security_bonus_per_level = ht.get("security_bonus_per_level", 0.1).asDouble();
                hub_thresholds.max_upgrade_level = ht.get("max_upgrade_level", 5).asInt();
            }

            // Load performance settings
            if (root.isMember("performance")) {
                const auto& perf = root["performance"];
                performance.update_frequency = perf.get("update_frequency", 0.2).asDouble();
                performance.price_update_interval = perf.get("price_update_interval", 30.0).asFloat();
                performance.max_routes_per_frame = perf.get("max_routes_per_frame", 25).asUInt64();
                performance.pathfinder_cache_size = perf.get("pathfinder_cache_size", 1000).asUInt64();
                performance.max_trade_distance_km = perf.get("max_trade_distance_km", 2000.0).asDouble();
            }

            // Load economic parameters
            if (root.isMember("economic")) {
                const auto& econ = root["economic"];

                // Transport costs
                economic.base_transport_cost_per_km = econ.get("base_transport_cost_per_km", 0.01).asDouble();
                economic.land_distance_modifier = econ.get("land_distance_modifier", 1.0).asDouble();
                economic.river_distance_modifier = econ.get("river_distance_modifier", 0.8).asDouble();
                economic.coastal_distance_modifier = econ.get("coastal_distance_modifier", 0.9).asDouble();
                economic.sea_distance_modifier = econ.get("sea_distance_modifier", 1.2).asDouble();
                economic.overland_long_distance_modifier = econ.get("overland_long_distance_modifier", 2.0).asDouble();

                // Route efficiency
                economic.land_efficiency = econ.get("land_efficiency", 0.8).asDouble();
                economic.river_efficiency = econ.get("river_efficiency", 1.2).asDouble();
                economic.coastal_efficiency = econ.get("coastal_efficiency", 1.1).asDouble();
                economic.sea_efficiency = econ.get("sea_efficiency", 1.5).asDouble();
                economic.overland_long_efficiency = econ.get("overland_long_efficiency", 0.6).asDouble();

                // Travel speeds
                economic.land_speed = econ.get("land_speed", 50.0).asDouble();
                economic.river_speed = econ.get("river_speed", 70.0).asDouble();
                economic.coastal_speed = econ.get("coastal_speed", 80.0).asDouble();
                economic.sea_speed = econ.get("sea_speed", 100.0).asDouble();
                economic.overland_long_speed = econ.get("overland_long_speed", 30.0).asDouble();

                // Market dynamics
                economic.supply_demand_elasticity = econ.get("supply_demand_elasticity", 0.5).asDouble();
                economic.price_stabilization_factor = econ.get("price_stabilization_factor", 0.05).asDouble();
                economic.volatility_reduction_rate = econ.get("volatility_reduction_rate", 0.99).asDouble();
                economic.min_volatility = econ.get("min_volatility", 0.01).asDouble();

                // Seasonal impact
                economic.seasonal_demand_variation = econ.get("seasonal_demand_variation", 0.3).asDouble();
                economic.seasonal_supply_variation = econ.get("seasonal_supply_variation", 0.3).asDouble();

                // Hub specialization
                economic.specialization_threshold = econ.get("specialization_threshold", 0.2).asDouble();
                economic.specialization_efficiency_bonus = econ.get("specialization_efficiency_bonus", 0.3).asDouble();
            }

            // Load safety parameters
            if (root.isMember("safety")) {
                const auto& saf = root["safety"];
                safety.min_route_safety = saf.get("min_route_safety", 0.1).asDouble();
                safety.max_route_efficiency = saf.get("max_route_efficiency", 2.0).asDouble();
                safety.base_safety = saf.get("base_safety", 0.9).asDouble();
                safety.safety_variation_range = saf.get("safety_variation_range", 0.2).asDouble();
                safety.distance_penalty_threshold = saf.get("distance_penalty_threshold", 2000.0).asDouble();
                safety.recovery_rate_per_month = saf.get("recovery_rate_per_month", 0.1).asDouble();
                safety.min_recovery_months = saf.get("min_recovery_months", 1.0).asDouble();
                safety.max_recovery_months = saf.get("max_recovery_months", 12.0).asDouble();
            }

            // Load debug settings
            if (root.isMember("debug")) {
                const auto& dbg = root["debug"];
                debug.enable_trade_logging = dbg.get("enable_trade_logging", false).asBool();
                debug.enable_price_logging = dbg.get("enable_price_logging", false).asBool();
                debug.enable_route_logging = dbg.get("enable_route_logging", false).asBool();
                debug.enable_hub_logging = dbg.get("enable_hub_logging", false).asBool();
                debug.enable_pathfinder_logging = dbg.get("enable_pathfinder_logging", false).asBool();
                debug.performance_warning_threshold_ms = dbg.get("performance_warning_threshold_ms", 16.0).asDouble();
            }

            CORE_STREAM_INFO("TradeSystemConfig") << "Successfully loaded config from: " << config_file;
            return true;

        } catch (const std::exception& e) {
            CORE_STREAM_ERROR("TradeSystemConfig") << "Exception loading config: " << e.what();
            return false;
        }
    }

    bool TradeSystemConfig::SaveToFile(const std::string& config_file) const {
        try {
            Json::Value root;

            // Save route viability thresholds
            Json::Value route_viability;
            route_viability["min_profitability"] = min_viable_profitability;
            route_viability["min_safety"] = min_viable_safety;
            route_viability["min_volume"] = min_viable_volume;
            root["route_viability"] = route_viability;

            // Save market price thresholds
            Json::Value market_prices;
            market_prices["above_average_threshold"] = price_above_average_threshold;
            market_prices["shock_threshold"] = price_shock_threshold;
            market_prices["volatility_threshold"] = price_volatility_threshold;
            market_prices["min_price"] = min_market_price;
            market_prices["max_price"] = max_market_price;
            root["market_prices"] = market_prices;

            // Save hub thresholds
            Json::Value hub_thresh;
            hub_thresh["international_port_volume"] = hub_thresholds.international_port_volume;
            hub_thresh["major_trading_center_volume"] = hub_thresholds.major_trading_center_volume;
            hub_thresh["regional_hub_volume"] = hub_thresholds.regional_hub_volume;
            hub_thresh["international_port_routes"] = hub_thresholds.international_port_routes;
            hub_thresh["major_trading_center_routes"] = hub_thresholds.major_trading_center_routes;
            hub_thresh["crossroads_routes"] = hub_thresholds.crossroads_routes;
            hub_thresh["regional_hub_routes"] = hub_thresholds.regional_hub_routes;
            hub_thresh["local_market_capacity"] = hub_thresholds.local_market_capacity;
            hub_thresh["regional_hub_capacity"] = hub_thresholds.regional_hub_capacity;
            hub_thresh["crossroads_capacity"] = hub_thresholds.crossroads_capacity;
            hub_thresh["major_trading_center_capacity"] = hub_thresholds.major_trading_center_capacity;
            hub_thresh["international_port_capacity"] = hub_thresholds.international_port_capacity;
            hub_thresh["infrastructure_bonus_per_level"] = hub_thresholds.infrastructure_bonus_per_level;
            hub_thresh["capacity_bonus_per_level"] = hub_thresholds.capacity_bonus_per_level;
            hub_thresh["security_bonus_per_level"] = hub_thresholds.security_bonus_per_level;
            hub_thresh["max_upgrade_level"] = hub_thresholds.max_upgrade_level;
            root["hub_thresholds"] = hub_thresh;

            // Save performance settings
            Json::Value perf;
            perf["update_frequency"] = performance.update_frequency;
            perf["price_update_interval"] = performance.price_update_interval;
            perf["max_routes_per_frame"] = static_cast<Json::UInt64>(performance.max_routes_per_frame);
            perf["pathfinder_cache_size"] = static_cast<Json::UInt64>(performance.pathfinder_cache_size);
            perf["max_trade_distance_km"] = performance.max_trade_distance_km;
            root["performance"] = perf;

            // Save economic parameters
            Json::Value econ;
            econ["base_transport_cost_per_km"] = economic.base_transport_cost_per_km;
            econ["land_distance_modifier"] = economic.land_distance_modifier;
            econ["river_distance_modifier"] = economic.river_distance_modifier;
            econ["coastal_distance_modifier"] = economic.coastal_distance_modifier;
            econ["sea_distance_modifier"] = economic.sea_distance_modifier;
            econ["overland_long_distance_modifier"] = economic.overland_long_distance_modifier;
            econ["land_efficiency"] = economic.land_efficiency;
            econ["river_efficiency"] = economic.river_efficiency;
            econ["coastal_efficiency"] = economic.coastal_efficiency;
            econ["sea_efficiency"] = economic.sea_efficiency;
            econ["overland_long_efficiency"] = economic.overland_long_efficiency;
            econ["land_speed"] = economic.land_speed;
            econ["river_speed"] = economic.river_speed;
            econ["coastal_speed"] = economic.coastal_speed;
            econ["sea_speed"] = economic.sea_speed;
            econ["overland_long_speed"] = economic.overland_long_speed;
            econ["supply_demand_elasticity"] = economic.supply_demand_elasticity;
            econ["price_stabilization_factor"] = economic.price_stabilization_factor;
            econ["volatility_reduction_rate"] = economic.volatility_reduction_rate;
            econ["min_volatility"] = economic.min_volatility;
            econ["seasonal_demand_variation"] = economic.seasonal_demand_variation;
            econ["seasonal_supply_variation"] = economic.seasonal_supply_variation;
            econ["specialization_threshold"] = economic.specialization_threshold;
            econ["specialization_efficiency_bonus"] = economic.specialization_efficiency_bonus;
            root["economic"] = econ;

            // Save safety parameters
            Json::Value saf;
            saf["min_route_safety"] = safety.min_route_safety;
            saf["max_route_efficiency"] = safety.max_route_efficiency;
            saf["base_safety"] = safety.base_safety;
            saf["safety_variation_range"] = safety.safety_variation_range;
            saf["distance_penalty_threshold"] = safety.distance_penalty_threshold;
            saf["recovery_rate_per_month"] = safety.recovery_rate_per_month;
            saf["min_recovery_months"] = safety.min_recovery_months;
            saf["max_recovery_months"] = safety.max_recovery_months;
            root["safety"] = saf;

            // Save debug settings
            Json::Value dbg;
            dbg["enable_trade_logging"] = debug.enable_trade_logging;
            dbg["enable_price_logging"] = debug.enable_price_logging;
            dbg["enable_route_logging"] = debug.enable_route_logging;
            dbg["enable_hub_logging"] = debug.enable_hub_logging;
            dbg["enable_pathfinder_logging"] = debug.enable_pathfinder_logging;
            dbg["performance_warning_threshold_ms"] = debug.performance_warning_threshold_ms;
            root["debug"] = dbg;

            // Write to file with pretty formatting
            std::ofstream file(config_file);
            if (!file.is_open()) {
                CORE_STREAM_ERROR("TradeSystemConfig") << "Failed to open config file for writing: " << config_file;
                return false;
            }

            Json::StreamWriterBuilder writer;
            writer["indentation"] = "  ";  // 2-space indentation
            std::unique_ptr<Json::StreamWriter> json_writer(writer.newStreamWriter());
            json_writer->write(root, &file);

            CORE_STREAM_INFO("TradeSystemConfig") << "Successfully saved config to: " << config_file;
            return true;

        } catch (const std::exception& e) {
            CORE_STREAM_ERROR("TradeSystemConfig") << "Exception saving config: " << e.what();
            return false;
        }
    }

    // ========================================================================
    // Configuration Management
    // ========================================================================

    void TradeSystemConfig::Reset() {
        *this = TradeSystemConfig();  // Reset to default values
        CORE_STREAM_INFO("TradeSystemConfig") << "Configuration reset to defaults";
    }

    bool TradeSystemConfig::Validate(std::string& error_message) const {
        std::ostringstream errors;

        // Validate route viability
        if (min_viable_profitability < 0.0 || min_viable_profitability > 1.0) {
            errors << "min_viable_profitability must be in [0.0, 1.0]; ";
        }
        if (min_viable_safety < 0.0 || min_viable_safety > 1.0) {
            errors << "min_viable_safety must be in [0.0, 1.0]; ";
        }
        if (min_viable_volume < 0.0) {
            errors << "min_viable_volume must be >= 0.0; ";
        }

        // Validate market prices
        if (min_market_price <= 0.0) {
            errors << "min_market_price must be > 0.0; ";
        }
        if (max_market_price <= min_market_price) {
            errors << "max_market_price must be > min_market_price; ";
        }
        if (price_above_average_threshold < 1.0) {
            errors << "price_above_average_threshold must be >= 1.0; ";
        }

        // Validate performance
        if (performance.update_frequency <= 0.0) {
            errors << "update_frequency must be > 0.0; ";
        }
        if (performance.max_routes_per_frame == 0) {
            errors << "max_routes_per_frame must be > 0; ";
        }
        if (performance.pathfinder_cache_size == 0) {
            errors << "pathfinder_cache_size must be > 0; ";
        }

        // Validate economic parameters
        if (economic.base_transport_cost_per_km < 0.0) {
            errors << "base_transport_cost_per_km must be >= 0.0; ";
        }
        if (economic.supply_demand_elasticity < 0.0) {
            errors << "supply_demand_elasticity must be >= 0.0; ";
        }

        // Validate safety
        if (safety.min_route_safety < 0.0 || safety.min_route_safety > 1.0) {
            errors << "min_route_safety must be in [0.0, 1.0]; ";
        }
        if (safety.max_route_efficiency <= 0.0) {
            errors << "max_route_efficiency must be > 0.0; ";
        }

        error_message = errors.str();
        return error_message.empty();
    }

    std::string TradeSystemConfig::ToString() const {
        std::ostringstream oss;
        oss << "TradeSystemConfig:\n";
        oss << "  Route Viability:\n";
        oss << "    min_viable_profitability: " << min_viable_profitability << "\n";
        oss << "    min_viable_safety: " << min_viable_safety << "\n";
        oss << "    min_viable_volume: " << min_viable_volume << "\n";
        oss << "  Market Prices:\n";
        oss << "    price_above_average_threshold: " << price_above_average_threshold << "\n";
        oss << "    min_market_price: " << min_market_price << "\n";
        oss << "    max_market_price: " << max_market_price << "\n";
        oss << "  Performance:\n";
        oss << "    update_frequency: " << performance.update_frequency << "\n";
        oss << "    max_routes_per_frame: " << performance.max_routes_per_frame << "\n";
        oss << "    pathfinder_cache_size: " << performance.pathfinder_cache_size << "\n";
        oss << "  Debug:\n";
        oss << "    enable_trade_logging: " << (debug.enable_trade_logging ? "true" : "false") << "\n";
        return oss.str();
    }

} // namespace game::trade
