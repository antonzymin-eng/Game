// ============================================================================
// NavalCombatConfig.h - Naval Combat Configuration System
// Created: 2025-11-18 - Naval Combat Configuration
// Location: include/game/military/NavalCombatConfig.h
// ============================================================================

#pragma once

#include <string>
#include <unordered_map>

namespace game::military {

    /// Centralized configuration for all naval combat parameters
    struct NavalCombatConfiguration {
        // ========================================================================
        // Combat Damage Multipliers
        // ========================================================================
        double broadside_damage_multiplier = 1.5;
        double ramming_damage_multiplier = 2.0;
        double boarding_damage_multiplier = 1.0;
        double fire_damage_multiplier = 0.8;

        // ========================================================================
        // Ship-Specific Bonuses
        // ========================================================================
        double ship_of_line_broadside_bonus = 0.8;
        double galleon_boarding_bonus = 0.3;
        double galley_ramming_bonus = 0.5;
        double carrack_ocean_bonus = 0.4;
        double frigate_speed_bonus = 0.2;

        // ========================================================================
        // Combat Mechanics
        // ========================================================================
        double boarding_success_threshold = 0.6;
        double ship_capture_chance = 0.3;
        double fire_chance_per_broadside = 0.05;
        double sinking_threshold = 0.8;  // 80% casualties = ship sinks
        double critical_hit_chance = 0.05;

        // ========================================================================
        // Environmental Effects
        // ========================================================================
        double wind_impact_factor = 0.4;
        double wave_penalty_factor = 0.3;
        double visibility_impact = 0.2;
        double storm_damage_multiplier = 2.0;
        double fog_accuracy_penalty = 0.3;

        // ========================================================================
        // Blockade Parameters
        // ========================================================================
        double blockade_partial_threshold = 0.5;      // 50% strength
        double blockade_moderate_threshold = 1.0;     // 100% strength
        double blockade_strong_threshold = 1.5;       // 150% strength
        double blockade_total_threshold = 2.0;        // 200% strength

        double blockade_partial_disruption = 0.35;
        double blockade_moderate_disruption = 0.60;
        double blockade_strong_disruption = 0.80;
        double blockade_total_disruption = 0.95;

        double blockade_base_attrition = 0.1;
        double blockade_attrition_scaling = 0.01;  // Per day

        // ========================================================================
        // Movement Parameters
        // ========================================================================
        double coastal_movement_penalty = 1.2;
        double river_movement_penalty = 1.5;
        double ocean_movement_bonus = 0.8;
        double base_naval_attrition = 0.01;
        double storm_attrition_multiplier = 10.0;

        // Water depth requirements (in meters)
        double galley_min_depth = 3.0;
        double cog_min_depth = 4.0;
        double carrack_min_depth = 10.0;
        double galleon_min_depth = 15.0;
        double ship_of_line_min_depth = 30.0;

        // ========================================================================
        // Naval Tradition & Prestige
        // ========================================================================
        double tradition_per_ship_sunk = 2.0;
        double tradition_per_ship_captured = 3.0;
        double tradition_per_victory = 5.0;
        double prestige_per_naval_victory = 10.0;

        // ========================================================================
        // Economic Parameters
        // ========================================================================
        double naval_maintenance_cost = 50.0;
        double naval_recruitment_cost = 400.0;
        double repair_cost_per_strength = 0.5;
        double resupply_cost_per_ship = 10.0;
        double upgrade_cost_multiplier = 0.5;

        // ========================================================================
        // Performance Tuning
        // ========================================================================
        int max_pathfinding_iterations = 1000;
        int fleet_composition_cache_size = 100;
        bool enable_pathfinding_early_termination = true;
        bool enable_composition_caching = true;

        // ========================================================================
        // Logging & Debugging
        // ========================================================================
        bool enable_combat_logging = false;
        bool enable_pathfinding_debug = false;
        bool enable_blockade_logging = false;
        bool enable_performance_metrics = false;

        // ========================================================================
        // Configuration Management
        // ========================================================================

        /// Load configuration from JSON file
        static NavalCombatConfiguration LoadFromFile(const std::string& filepath);

        /// Save configuration to JSON file
        void SaveToFile(const std::string& filepath) const;

        /// Get default configuration
        static NavalCombatConfiguration GetDefault();

        /// Validate configuration values
        bool Validate() const;

        /// Get configuration as string for debugging
        std::string ToString() const;
    };

    /// Global naval combat configuration instance
    extern NavalCombatConfiguration g_naval_config;

} // namespace game::military
