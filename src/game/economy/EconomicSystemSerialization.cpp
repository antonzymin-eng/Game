// ============================================================================
// Date/Time Created: September 27, 2025 - 4:25 PM PST
// Updated: November 18, 2025 - Complete rewrite for ECS compatibility
// Intended Folder Location: src/game/economy/EconomicSystemSerialization.cpp
// EconomicSystem Serialization Implementation
// ============================================================================

#include "game/economy/EconomicSystem.h"
#include "game/economy/EconomicComponents.h"
#include "utils/PlatformCompat.h"
#include "core/logging/Logger.h"

namespace game::economy {

    Json::Value EconomicSystem::Serialize(int version) const {
        Json::Value root;
        root["version"] = version;
        root["system_name"] = "EconomicSystem";
        root["initialized"] = m_initialized;

        // Serialize configuration
        Json::Value config;
        config["monthly_update_interval"] = m_config.monthly_update_interval;
        config["base_tax_rate"] = m_config.base_tax_rate;
        config["trade_efficiency"] = m_config.trade_efficiency;
        config["inflation_rate"] = m_config.inflation_rate;
        config["min_treasury"] = m_config.min_treasury;
        config["starting_treasury"] = m_config.starting_treasury;
        config["event_chance_per_month"] = m_config.event_chance_per_month;
        root["config"] = config;

        // Serialize timing state
        root["accumulated_time"] = m_accumulated_time;
        root["monthly_timer"] = m_monthly_timer;

        // Note: Component data is serialized by the ECS ComponentManager,
        // not here. This just saves system-level state.

        CORE_LOG_INFO("EconomicSystem", "Serialization complete");
        return root;
    }

    bool EconomicSystem::Deserialize(const Json::Value& data, int version) {
        try {
            if (!data.isMember("system_name") || data["system_name"].asString() != "EconomicSystem") {
                CORE_LOG_ERROR("EconomicSystem", "Invalid system name in serialization data");
                return false;
            }

            m_initialized = data.get("initialized", false).asBool();

            // Deserialize configuration if present
            if (data.isMember("config")) {
                const Json::Value& config = data["config"];
                m_config.monthly_update_interval = config.get("monthly_update_interval", 30.0).asDouble();
                m_config.base_tax_rate = config.get("base_tax_rate", 0.10).asDouble();
                m_config.trade_efficiency = config.get("trade_efficiency", 0.85).asDouble();
                m_config.inflation_rate = config.get("inflation_rate", 0.02).asDouble();
                m_config.min_treasury = config.get("min_treasury", 0).asInt();
                m_config.starting_treasury = config.get("starting_treasury", 1000).asInt();
                m_config.event_chance_per_month = config.get("event_chance_per_month", 0.15).asDouble();
            }

            // Deserialize timing state
            m_accumulated_time = data.get("accumulated_time", 0.0f).asFloat();
            m_monthly_timer = data.get("monthly_timer", 0.0f).asFloat();

            CORE_LOG_INFO("EconomicSystem", "Deserialization complete");
            return true;

        } catch (const std::exception& e) {
            CORE_LOG_ERROR("EconomicSystem", std::string("Deserialization failed: ") + e.what());
            return false;
        }
    }

} // namespace game::economy
