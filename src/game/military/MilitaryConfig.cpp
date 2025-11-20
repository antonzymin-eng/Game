// ============================================================================
// MilitaryConfig.cpp - Military Configuration Implementation
// Created: 2025-11-20
// ============================================================================

#include "game/military/MilitaryConfig.h"
#include "core/logging/Logger.h"
#include <json/json.h>
#include <fstream>

namespace game::military {

    bool MilitaryConfiguration::LoadFromFile(const std::string& config_path) {
        std::ifstream config_file(config_path);

        if (!config_file.is_open()) {
            CORE_LOG_WARN("MilitaryConfig", "Could not open config file: " + config_path);
            return false;
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        std::string errors;

        if (!Json::parseFromStream(builder, config_file, &root, &errors)) {
            CORE_LOG_ERROR("MilitaryConfig", "Failed to parse config: " + errors);
            return false;
        }

        config_file.close();

        // Load all configuration values
        if (root.isMember("default_military_budget"))
            default_military_budget = root["default_military_budget"].asDouble();
        if (root.isMember("default_recruitment_capacity"))
            default_recruitment_capacity = root["default_recruitment_capacity"].asUInt();
        if (root.isMember("default_training_facilities"))
            default_training_facilities = root["default_training_facilities"].asDouble();
        if (root.isMember("default_supply_infrastructure"))
            default_supply_infrastructure = root["default_supply_infrastructure"].asDouble();
        if (root.isMember("default_barracks_level"))
            default_barracks_level = root["default_barracks_level"].asDouble();

        if (root.isMember("unit_base_cost"))
            unit_base_cost = root["unit_base_cost"].asDouble();
        if (root.isMember("disbandment_refund_ratio"))
            disbandment_refund_ratio = root["disbandment_refund_ratio"].asDouble();

        if (root.isMember("training_facility_cost_divisor"))
            training_facility_cost_divisor = root["training_facility_cost_divisor"].asDouble();
        if (root.isMember("equipment_upgrade_cost_divisor"))
            equipment_upgrade_cost_divisor = root["equipment_upgrade_cost_divisor"].asDouble();

        if (root.isMember("fortification_siege_resistance_bonus"))
            fortification_siege_resistance_bonus = root["fortification_siege_resistance_bonus"].asDouble();
        if (root.isMember("fortification_artillery_bonus"))
            fortification_artillery_bonus = root["fortification_artillery_bonus"].asDouble();
        if (root.isMember("fortification_integrity_bonus"))
            fortification_integrity_bonus = root["fortification_integrity_bonus"].asDouble();
        if (root.isMember("fortification_garrison_capacity_bonus"))
            fortification_garrison_capacity_bonus = root["fortification_garrison_capacity_bonus"].asUInt();

        if (root.isMember("siege_success_damage"))
            siege_success_damage = root["siege_success_damage"].asDouble();
        if (root.isMember("monthly_update_interval"))
            monthly_update_interval = root["monthly_update_interval"].asFloat();

        if (root.isMember("army_registry_cleanup_interval"))
            army_registry_cleanup_interval = root["army_registry_cleanup_interval"].asUInt();
        if (root.isMember("max_disbanded_armies_before_cleanup"))
            max_disbanded_armies_before_cleanup = root["max_disbanded_armies_before_cleanup"].asUInt();

        if (root.isMember("base_casualty_rate"))
            base_casualty_rate = root["base_casualty_rate"].asDouble();
        if (root.isMember("morale_casualty_multiplier"))
            morale_casualty_multiplier = root["morale_casualty_multiplier"].asDouble();
        if (root.isMember("strength_ratio_impact"))
            strength_ratio_impact = root["strength_ratio_impact"].asDouble();

        CORE_LOG_INFO("MilitaryConfig", "Configuration loaded from: " + config_path);
        return true;
    }

    bool MilitaryConfiguration::SaveToFile(const std::string& config_path) const {
        Json::Value root;

        // Save all configuration values
        root["default_military_budget"] = default_military_budget;
        root["default_recruitment_capacity"] = default_recruitment_capacity;
        root["default_training_facilities"] = default_training_facilities;
        root["default_supply_infrastructure"] = default_supply_infrastructure;
        root["default_barracks_level"] = default_barracks_level;

        root["unit_base_cost"] = unit_base_cost;
        root["disbandment_refund_ratio"] = disbandment_refund_ratio;

        root["training_facility_cost_divisor"] = training_facility_cost_divisor;
        root["equipment_upgrade_cost_divisor"] = equipment_upgrade_cost_divisor;

        root["fortification_siege_resistance_bonus"] = fortification_siege_resistance_bonus;
        root["fortification_artillery_bonus"] = fortification_artillery_bonus;
        root["fortification_integrity_bonus"] = fortification_integrity_bonus;
        root["fortification_garrison_capacity_bonus"] = fortification_garrison_capacity_bonus;

        root["siege_success_damage"] = siege_success_damage;
        root["monthly_update_interval"] = monthly_update_interval;

        root["army_registry_cleanup_interval"] = army_registry_cleanup_interval;
        root["max_disbanded_armies_before_cleanup"] = max_disbanded_armies_before_cleanup;

        root["base_casualty_rate"] = base_casualty_rate;
        root["morale_casualty_multiplier"] = morale_casualty_multiplier;
        root["strength_ratio_impact"] = strength_ratio_impact;

        std::ofstream config_file(config_path);
        if (!config_file.is_open()) {
            CORE_LOG_ERROR("MilitaryConfig", "Failed to open file for writing: " + config_path);
            return false;
        }

        Json::StreamWriterBuilder builder;
        builder["indentation"] = "  ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        writer->write(root, &config_file);
        config_file.close();

        CORE_LOG_INFO("MilitaryConfig", "Configuration saved to: " + config_path);
        return true;
    }

    MilitaryConfiguration MilitaryConfiguration::GetDefault() {
        MilitaryConfiguration config;
        // Default values are already set in struct definition
        return config;
    }

    // ========================================================================
    // MilitaryConfigManager Implementation
    // ========================================================================

    bool MilitaryConfigManager::Initialize(const std::string& config_path) {
        config_file_path_ = config_path;

        // Try to load from file
        if (config_.LoadFromFile(config_path)) {
            loaded_ = true;
            CORE_LOG_INFO("MilitaryConfigManager", "Initialized with config from: " + config_path);
            return true;
        }

        // Fall back to defaults
        CORE_LOG_WARN("MilitaryConfigManager", "Using default configuration");
        config_ = MilitaryConfiguration::GetDefault();
        loaded_ = true;
        return true;
    }

    bool MilitaryConfigManager::Reload() {
        if (config_file_path_.empty()) {
            CORE_LOG_WARN("MilitaryConfigManager", "No config file path set");
            return false;
        }

        MilitaryConfiguration new_config;
        if (new_config.LoadFromFile(config_file_path_)) {
            config_ = new_config;
            CORE_LOG_INFO("MilitaryConfigManager", "Configuration reloaded successfully");
            return true;
        }

        CORE_LOG_ERROR("MilitaryConfigManager", "Failed to reload configuration");
        return false;
    }

} // namespace game::military
