// ============================================================================
// MilitaryConfig.h - Centralized Military System Configuration
// Created: 2025-11-20
// Description: Loads and manages all military system configuration from JSON
// ============================================================================

#pragma once

#include <string>
#include <unordered_map>
#include "game/military/MilitaryComponents.h"

namespace game::military {

    /// Military system configuration loaded from JSON files
    struct MilitaryConfiguration {
        // Default initialization values
        double default_military_budget = 1000.0;
        uint32_t default_recruitment_capacity = 100;
        double default_training_facilities = 0.3;
        double default_supply_infrastructure = 0.5;
        double default_barracks_level = 1.0;

        // Recruitment costs
        double unit_base_cost = 50.0;
        double disbandment_refund_ratio = 0.2;

        // Military development costs
        double training_facility_cost_divisor = 1000.0;
        double equipment_upgrade_cost_divisor = 500.0;

        // Fortification construction
        double fortification_siege_resistance_bonus = 0.1;
        double fortification_artillery_bonus = 0.1;
        double fortification_integrity_bonus = 0.05;
        uint32_t fortification_garrison_capacity_bonus = 100;

        // Siege damage
        double siege_success_damage = 0.3;

        // Update intervals (in seconds)
        float monthly_update_interval = 30.0f;

        // Performance settings
        uint32_t army_registry_cleanup_interval = 100; // Clean every N updates
        uint32_t max_disbanded_armies_before_cleanup = 50;

        // Combat settings
        double base_casualty_rate = 0.15;
        double morale_casualty_multiplier = 0.5;
        double strength_ratio_impact = 1.5;

        /// Load configuration from JSON file
        bool LoadFromFile(const std::string& config_path);

        /// Save configuration to JSON file
        bool SaveToFile(const std::string& config_path) const;

        /// Get default configuration
        static MilitaryConfiguration GetDefault();
    };

    /// Configuration loader/manager
    class MilitaryConfigManager {
    public:
        MilitaryConfigManager() = default;

        /// Load configuration from file, or use defaults if file doesn't exist
        bool Initialize(const std::string& config_path = "data/config/military_config.json");

        /// Get current configuration
        const MilitaryConfiguration& GetConfig() const { return config_; }

        /// Reload configuration from disk
        bool Reload();

        /// Check if configuration is loaded
        bool IsLoaded() const { return loaded_; }

    private:
        MilitaryConfiguration config_;
        std::string config_file_path_;
        bool loaded_ = false;
    };

} // namespace game::military
