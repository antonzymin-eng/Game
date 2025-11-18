// ============================================================================
// NavalCombatCalculator.h - Naval Combat Resolution Logic
// Created: 2025-11-18 - Naval Combat System Implementation
// Location: include/game/military/NavalCombatCalculator.h
// ============================================================================

#pragma once

#include "game/military/BattleResolutionCalculator.h"
#include "game/military/MilitaryComponents.h"
#include "core/types/game_types.h"
#include <string>

namespace game::military {

    /// Naval combat types
    enum class NavalCombatType {
        LINE_BATTLE,        // Traditional line of battle (broadside exchanges)
        BOARDING_ACTION,    // Close-quarters boarding combat
        RAMMING_ATTACK,     // Direct ramming assault
        CHASE,              // Pursuit of fleeing vessels
        BLOCKADE_ENGAGEMENT // Combat during blockade operations
    };

    /// Naval combat modifiers
    struct NavalCombatModifiers {
        double wind_direction = 0.0;    // -1.0 (headwind) to 1.0 (tailwind)
        double wind_strength = 0.5;     // 0.0 (calm) to 1.0 (storm)
        double wave_height = 0.3;       // 0.0 (calm) to 1.0 (very rough)
        double visibility = 1.0;        // 0.0 (fog) to 1.0 (clear)
        bool is_coastal = false;        // Fighting in coastal waters
        bool is_deep_ocean = false;     // Fighting in deep ocean
    };

    /// Naval battle result (extends BattleResult)
    struct NavalBattleResult : public BattleResult {
        // Naval-specific outcomes
        uint32_t ships_sunk_attacker = 0;
        uint32_t ships_sunk_defender = 0;
        uint32_t ships_captured_by_attacker = 0;
        uint32_t ships_captured_by_defender = 0;

        // Combat type breakdown
        uint32_t casualties_from_broadsides = 0;
        uint32_t casualties_from_boarding = 0;
        uint32_t casualties_from_ramming = 0;
        uint32_t casualties_from_fire = 0;

        // Naval prestige
        double naval_tradition_gained = 0.0;
        std::string famous_engagement_name;
    };

    /// Naval combat configuration
    struct NavalCombatConfig : public BattleConfig {
        // Naval-specific modifiers
        double broadside_damage_multiplier = 1.5;
        double ramming_damage_multiplier = 2.0;
        double boarding_success_threshold = 0.6;
        double ship_capture_chance = 0.3;

        // Environmental effects
        double wind_impact_factor = 0.4;
        double wave_penalty_factor = 0.3;
        double visibility_impact = 0.2;

        // Ship-specific bonuses
        double ship_of_line_broadside_bonus = 0.8;
        double galleon_boarding_bonus = 0.3;
        double galley_ramming_bonus = 0.5;
        double carrack_ocean_bonus = 0.4;

        // Fire and sinking
        double fire_chance_per_broadside = 0.05;
        double sinking_threshold = 0.8;  // 80% casualties = ship sinks

        // Naval tradition
        double tradition_per_ship_sunk = 2.0;
        double tradition_per_ship_captured = 3.0;
    };

    /// Naval Combat Calculator - specialized naval battle resolution
    class NavalCombatCalculator {
    public:
        // ========================================================================
        // Core Naval Battle Resolution
        // ========================================================================

        /// Resolve a naval battle between two fleets
        static NavalBattleResult ResolveNavalBattle(
            const ArmyComponent& attacker_fleet,
            const ArmyComponent& defender_fleet,
            const Commander* attacker_admiral,
            const Commander* defender_admiral,
            const NavalCombatModifiers& modifiers,
            const NavalCombatConfig& config
        );

        // ========================================================================
        // Naval Combat Types
        // ========================================================================

        /// Determine the type of naval combat based on fleets and conditions
        static NavalCombatType DetermineNavalCombatType(
            const ArmyComponent& attacker_fleet,
            const ArmyComponent& defender_fleet,
            const NavalCombatModifiers& modifiers
        );

        /// Resolve broadside combat (line of battle)
        static void ResolveBroadsideCombat(
            const ArmyComponent& attacker_fleet,
            const ArmyComponent& defender_fleet,
            const NavalCombatModifiers& modifiers,
            const NavalCombatConfig& config,
            NavalBattleResult& result
        );

        /// Resolve boarding actions
        static void ResolveBoardingActions(
            const ArmyComponent& attacker_fleet,
            const ArmyComponent& defender_fleet,
            const NavalCombatConfig& config,
            NavalBattleResult& result
        );

        /// Resolve ramming attacks
        static void ResolveRammingAttacks(
            const ArmyComponent& attacker_fleet,
            const ArmyComponent& defender_fleet,
            const NavalCombatConfig& config,
            NavalBattleResult& result
        );

        // ========================================================================
        // Naval Combat Strength Calculations
        // ========================================================================

        /// Calculate naval combat strength considering ship types and weather
        static double CalculateNavalCombatStrength(
            const ArmyComponent& fleet,
            const Commander* admiral,
            const NavalCombatModifiers& modifiers,
            const NavalCombatConfig& config
        );

        /// Calculate broadside power of a fleet
        static double CalculateBroadsidePower(
            const ArmyComponent& fleet,
            const NavalCombatModifiers& modifiers,
            const NavalCombatConfig& config
        );

        /// Calculate boarding strength
        static double CalculateBoardingStrength(
            const ArmyComponent& fleet,
            const NavalCombatConfig& config
        );

        /// Calculate ramming effectiveness
        static double CalculateRammingEffectiveness(
            const ArmyComponent& fleet,
            const NavalCombatModifiers& modifiers,
            const NavalCombatConfig& config
        );

        // ========================================================================
        // Environmental Effects
        // ========================================================================

        /// Calculate wind advantage modifier
        static double CalculateWindAdvantage(
            const NavalCombatModifiers& modifiers,
            bool is_attacker
        );

        /// Calculate wave penalty on ship handling
        static double CalculateWavePenalty(
            const NavalCombatModifiers& modifiers,
            UnitType ship_type
        );

        /// Calculate visibility impact on gunnery
        static double CalculateVisibilityModifier(
            const NavalCombatModifiers& modifiers
        );

        /// Check if weather favors certain ship types
        static double GetShipWeatherBonus(
            UnitType ship_type,
            const NavalCombatModifiers& modifiers
        );

        // ========================================================================
        // Ship-Specific Mechanics
        // ========================================================================

        /// Calculate ship sinking and capture
        static void CalculateShipLosses(
            const ArmyComponent& fleet,
            uint32_t casualties,
            const NavalCombatConfig& config,
            uint32_t& ships_sunk,
            uint32_t& ships_captured
        );

        /// Check for ship fires and spreading
        static uint32_t CalculateFireDamage(
            const ArmyComponent& fleet,
            double broadside_power,
            const NavalCombatConfig& config
        );

        /// Get ship type effectiveness in naval combat
        static double GetShipTypeEffectiveness(
            UnitType ship_type,
            NavalCombatType combat_type,
            const NavalCombatConfig& config
        );

        // ========================================================================
        // Admiral Bonuses
        // ========================================================================

        /// Calculate admiral's naval command bonus
        static double CalculateAdmiralBonus(
            const Commander* admiral,
            const ArmyComponent& fleet,
            const NavalCombatConfig& config
        );

        /// Check if admiral has naval-specific traits
        static bool HasNavalTrait(
            const Commander* admiral,
            const std::string& trait_name
        );

        // ========================================================================
        // Naval Tradition and Prestige
        // ========================================================================

        /// Calculate naval tradition gained from battle
        static double CalculateNavalTradition(
            const NavalBattleResult& result,
            const NavalCombatConfig& config
        );

        /// Generate famous engagement name for significant battles
        static std::string GenerateFamousEngagementName(
            const NavalBattleResult& result,
            const std::string& location_name
        );

        // ========================================================================
        // Utility Functions
        // ========================================================================

        /// Check if unit is a naval unit
        static bool IsNavalUnit(UnitType unit_type);

        /// Get naval combat type string
        static std::string NavalCombatTypeToString(NavalCombatType type);

        /// Generate naval battle summary
        static std::string GenerateNavalBattleSummary(
            const NavalBattleResult& result,
            const std::string& attacker_name,
            const std::string& defender_name,
            const std::string& location_name
        );

        /// Count ships of specific type in fleet
        static uint32_t CountShipsOfType(
            const ArmyComponent& fleet,
            UnitType ship_type
        );

        /// Get default naval combat configuration
        static NavalCombatConfig GetDefaultNavalConfig();
    };

} // namespace game::military
