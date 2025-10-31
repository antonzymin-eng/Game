// ============================================================================
// BattleResolutionCalculator.h - Battle Resolution Logic
// Created: 2025-10-31 - Battle Resolution System Implementation
// Location: include/game/military/BattleResolutionCalculator.h
// ============================================================================

#pragma once

#include "game/military/MilitaryComponents.h"
#include "core/types/game_types.h"
#include <string>
#include <random>

namespace game::military {

    /// Battle outcome enumeration
    enum class BattleOutcome {
        ATTACKER_DECISIVE_VICTORY,  // Attacker wins with minimal losses
        ATTACKER_VICTORY,            // Attacker wins
        ATTACKER_PYRRHIC_VICTORY,    // Attacker wins with heavy losses
        STALEMATE,                   // No clear winner, both sides withdraw
        DEFENDER_PYRRHIC_VICTORY,    // Defender wins with heavy losses
        DEFENDER_VICTORY,            // Defender wins
        DEFENDER_DECISIVE_VICTORY    // Defender wins with minimal losses
    };

    /// Battle result structure containing all battle outcome data
    struct BattleResult {
        BattleOutcome outcome;

        // Casualties
        uint32_t attacker_casualties;
        uint32_t defender_casualties;

        // Morale impact
        double attacker_morale_change;
        double defender_morale_change;

        // Experience gains
        double attacker_experience_gain;
        double defender_experience_gain;

        // Battle metrics
        double battle_intensity;
        double casualty_ratio;  // attacker_casualties / defender_casualties

        // War score and prestige
        double war_score_change;
        double prestige_change;

        std::string battle_summary;
    };

    /// Battle configuration parameters
    struct BattleConfig {
        // Combat multipliers
        double base_casualty_rate = 0.15;
        double morale_casualty_multiplier = 0.5;
        double strength_ratio_impact = 1.5;
        double experience_multiplier = 0.2;
        double equipment_multiplier = 0.3;

        // Commander bonuses
        double commander_skill_impact = 0.25;
        double command_limit_penalty_threshold = 1.2;

        // Morale thresholds
        double routing_threshold = 0.3;
        double wavering_threshold = 0.5;
        double confident_threshold = 0.8;

        // Terrain and fortification
        double terrain_modifier_max = 0.3;
        double fortification_defense_multiplier = 1.5;

        // Battle duration
        double base_battle_duration = 1.0;
        double max_battle_duration = 5.0;

        // Experience and prestige
        double experience_per_casualty_dealt = 0.01;
        double prestige_per_strength_defeated = 0.001;
    };

    /// Pure calculation functions for battle resolution
    class BattleResolutionCalculator {
    public:
        // ========================================================================
        // Core Battle Resolution
        // ========================================================================

        /// Resolve a complete battle and return the result
        static BattleResult ResolveBattle(
            const ArmyComponent& attacker,
            const ArmyComponent& defender,
            const CombatComponent& combat,
            const Commander* attacker_commander,
            const Commander* defender_commander,
            const FortificationComponent* fortification,
            const BattleConfig& config
        );

        // ========================================================================
        // Combat Strength Calculations
        // ========================================================================

        /// Calculate total combat strength of an army
        static double CalculateCombatStrength(
            const ArmyComponent& army,
            const Commander* commander,
            double terrain_modifier,
            double fortification_bonus,
            const BattleConfig& config
        );

        /// Calculate base unit strength considering all factors
        static double CalculateUnitStrength(
            const MilitaryUnit& unit,
            const BattleConfig& config
        );

        /// Calculate commander's contribution to combat strength
        static double CalculateCommanderBonus(
            const Commander* commander,
            uint32_t army_size,
            const BattleConfig& config
        );

        /// Calculate morale multiplier for combat effectiveness
        static double CalculateMoraleMultiplier(
            double army_morale,
            const BattleConfig& config
        );

        // ========================================================================
        // Casualty Calculations
        // ========================================================================

        /// Calculate casualties for both sides based on combat
        static void CalculateCasualties(
            uint32_t attacker_strength,
            uint32_t defender_strength,
            double attacker_combat_power,
            double defender_combat_power,
            double battle_duration,
            const BattleConfig& config,
            uint32_t& out_attacker_casualties,
            uint32_t& out_defender_casualties
        );

        /// Calculate casualty rate based on strength differential
        static double CalculateCasualtyRate(
            double strength_ratio,
            double combat_power_ratio,
            double morale,
            const BattleConfig& config
        );

        // ========================================================================
        // Morale Calculations
        // ========================================================================

        /// Calculate morale change after battle
        static double CalculateMoraleChange(
            uint32_t initial_strength,
            uint32_t casualties,
            BattleOutcome outcome,
            bool is_attacker,
            const BattleConfig& config
        );

        /// Determine morale state from morale value
        static MoraleState DetermineMoraleState(double morale);

        /// Check if army breaks and routs based on casualties and morale
        static bool CheckRouting(
            double morale,
            double casualty_percentage,
            const BattleConfig& config
        );

        // ========================================================================
        // Battle Outcome Determination
        // ========================================================================

        /// Determine battle outcome based on casualties and morale
        static BattleOutcome DetermineBattleOutcome(
            uint32_t attacker_strength,
            uint32_t defender_strength,
            uint32_t attacker_casualties,
            uint32_t defender_casualties,
            double attacker_morale,
            double defender_morale,
            const BattleConfig& config
        );

        /// Calculate war score change from battle outcome
        static double CalculateWarScoreChange(
            BattleOutcome outcome,
            uint32_t total_casualties,
            const BattleConfig& config
        );

        /// Calculate prestige change from battle outcome
        static double CalculatePrestigeChange(
            BattleOutcome outcome,
            uint32_t enemy_strength_defeated,
            const BattleConfig& config
        );

        // ========================================================================
        // Experience Calculations
        // ========================================================================

        /// Calculate experience gained from battle
        static double CalculateExperienceGain(
            uint32_t casualties_inflicted,
            uint32_t casualties_received,
            BattleOutcome outcome,
            bool is_winner,
            const BattleConfig& config
        );

        // ========================================================================
        // Terrain and Environmental Modifiers
        // ========================================================================

        /// Calculate terrain modifier based on terrain type and unit composition
        static double CalculateTerrainModifier(
            const std::string& terrain_type,
            const ArmyComponent& army,
            const BattleConfig& config
        );

        /// Calculate weather impact on combat effectiveness
        static double CalculateWeatherModifier(
            double weather_value,
            const ArmyComponent& army
        );

        /// Calculate fortification defensive bonus
        static double CalculateFortificationBonus(
            const FortificationComponent* fortification,
            const BattleConfig& config
        );

        // ========================================================================
        // Utility Functions
        // ========================================================================

        /// Convert battle outcome to string
        static std::string OutcomeToString(BattleOutcome outcome);

        /// Generate battle summary text
        static std::string GenerateBattleSummary(
            const BattleResult& result,
            const std::string& attacker_name,
            const std::string& defender_name,
            const std::string& location_name
        );

        /// Get dominant unit class in an army
        static UnitClass GetDominantUnitClass(const ArmyComponent& army);

        /// Calculate total effective strength considering all factors
        static uint32_t CalculateEffectiveStrength(
            const ArmyComponent& army,
            double combat_multiplier
        );

        // ========================================================================
        // Configuration Management
        // ========================================================================

        /// Load battle configuration from game config
        static BattleConfig LoadConfigFromGameConfig(const std::string& config_json);

        /// Get default battle configuration
        static BattleConfig GetDefaultConfig();
    };

} // namespace game::military
