// ============================================================================
// NavalCombatConfig.cpp - Naval Combat Configuration Implementation
// Created: 2025-11-18 - Naval Combat Configuration
// Location: src/game/military/NavalCombatConfig.cpp
// ============================================================================

#include "game/military/NavalCombatConfig.h"
#include <sstream>
#include <iomanip>
#include <fstream>

namespace game::military {

    // Global configuration instance
    NavalCombatConfiguration g_naval_config;

    NavalCombatConfiguration NavalCombatConfiguration::LoadFromFile(const std::string& filepath) {
        // TODO: Implement JSON parsing when JSON library is available
        // For now, return default configuration
        (void)filepath;  // Suppress unused parameter warning
        return GetDefault();
    }

    void NavalCombatConfiguration::SaveToFile(const std::string& filepath) const {
        // TODO: Implement JSON serialization when JSON library is available
        (void)filepath;  // Suppress unused parameter warning
    }

    NavalCombatConfiguration NavalCombatConfiguration::GetDefault() {
        return NavalCombatConfiguration{};  // Uses default initializers
    }

    bool NavalCombatConfiguration::Validate() const {
        // Validate that all values are within reasonable ranges
        if (broadside_damage_multiplier < 0.0 || broadside_damage_multiplier > 10.0) return false;
        if (ramming_damage_multiplier < 0.0 || ramming_damage_multiplier > 10.0) return false;
        if (boarding_success_threshold < 0.0 || boarding_success_threshold > 1.0) return false;
        if (ship_capture_chance < 0.0 || ship_capture_chance > 1.0) return false;
        if (sinking_threshold < 0.0 || sinking_threshold > 1.0) return false;

        // Validate blockade thresholds are in ascending order
        if (blockade_partial_threshold >= blockade_moderate_threshold) return false;
        if (blockade_moderate_threshold >= blockade_strong_threshold) return false;
        if (blockade_strong_threshold >= blockade_total_threshold) return false;

        // Validate water depths are positive
        if (galley_min_depth < 0.0) return false;
        if (cog_min_depth < galley_min_depth) return false;
        if (carrack_min_depth < cog_min_depth) return false;
        if (galleon_min_depth < carrack_min_depth) return false;
        if (ship_of_line_min_depth < galleon_min_depth) return false;

        return true;
    }

    std::string NavalCombatConfiguration::ToString() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);

        oss << "=== Naval Combat Configuration ===\n\n";

        oss << "Combat Damage:\n";
        oss << "  Broadside Multiplier: " << broadside_damage_multiplier << "\n";
        oss << "  Ramming Multiplier: " << ramming_damage_multiplier << "\n";
        oss << "  Boarding Multiplier: " << boarding_damage_multiplier << "\n";
        oss << "  Fire Multiplier: " << fire_damage_multiplier << "\n\n";

        oss << "Ship Bonuses:\n";
        oss << "  Ship of Line Broadside: +" << (ship_of_line_broadside_bonus * 100) << "%\n";
        oss << "  Galleon Boarding: +" << (galleon_boarding_bonus * 100) << "%\n";
        oss << "  Galley Ramming: +" << (galley_ramming_bonus * 100) << "%\n";
        oss << "  Carrack Ocean: +" << (carrack_ocean_bonus * 100) << "%\n\n";

        oss << "Combat Mechanics:\n";
        oss << "  Boarding Success Threshold: " << (boarding_success_threshold * 100) << "%\n";
        oss << "  Ship Capture Chance: " << (ship_capture_chance * 100) << "%\n";
        oss << "  Fire Chance per Broadside: " << (fire_chance_per_broadside * 100) << "%\n";
        oss << "  Sinking Threshold: " << (sinking_threshold * 100) << "%\n\n";

        oss << "Environmental:\n";
        oss << "  Wind Impact: " << wind_impact_factor << "\n";
        oss << "  Wave Penalty: " << wave_penalty_factor << "\n";
        oss << "  Visibility Impact: " << visibility_impact << "\n\n";

        oss << "Blockade Disruption:\n";
        oss << "  Partial: " << (blockade_partial_disruption * 100) << "%\n";
        oss << "  Moderate: " << (blockade_moderate_disruption * 100) << "%\n";
        oss << "  Strong: " << (blockade_strong_disruption * 100) << "%\n";
        oss << "  Total: " << (blockade_total_disruption * 100) << "%\n\n";

        oss << "Performance:\n";
        oss << "  Max Pathfinding Iterations: " << max_pathfinding_iterations << "\n";
        oss << "  Composition Cache Size: " << fleet_composition_cache_size << "\n";
        oss << "  Early Termination: " << (enable_pathfinding_early_termination ? "ON" : "OFF") << "\n";
        oss << "  Composition Caching: " << (enable_composition_caching ? "ON" : "OFF") << "\n\n";

        oss << "Debug Settings:\n";
        oss << "  Combat Logging: " << (enable_combat_logging ? "ON" : "OFF") << "\n";
        oss << "  Pathfinding Debug: " << (enable_pathfinding_debug ? "ON" : "OFF") << "\n";
        oss << "  Blockade Logging: " << (enable_blockade_logging ? "ON" : "OFF") << "\n";
        oss << "  Performance Metrics: " << (enable_performance_metrics ? "ON" : "OFF") << "\n";

        return oss.str();
    }

} // namespace game::military
