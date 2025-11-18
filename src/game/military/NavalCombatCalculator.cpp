// ============================================================================
// NavalCombatCalculator.cpp - Naval Combat Resolution Implementation
// Created: 2025-11-18 - Naval Combat System Implementation
// Location: src/game/military/NavalCombatCalculator.cpp
// ============================================================================

#include "game/military/NavalCombatCalculator.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace game::military {

    // ========================================================================
    // Core Naval Battle Resolution
    // ========================================================================

    NavalBattleResult NavalCombatCalculator::ResolveNavalBattle(
        const ArmyComponent& attacker_fleet,
        const ArmyComponent& defender_fleet,
        const Commander* attacker_admiral,
        const Commander* defender_admiral,
        const NavalCombatModifiers& modifiers,
        const NavalCombatConfig& config
    ) {
        NavalBattleResult result{};

        // Determine type of naval combat
        NavalCombatType combat_type = DetermineNavalCombatType(
            attacker_fleet, defender_fleet, modifiers
        );

        // Calculate naval combat strength for both fleets
        double attacker_strength = CalculateNavalCombatStrength(
            attacker_fleet, attacker_admiral, modifiers, config
        );
        double defender_strength = CalculateNavalCombatStrength(
            defender_fleet, defender_admiral, modifiers, config
        );

        // Apply weather advantages
        double attacker_wind_modifier = CalculateWindAdvantage(modifiers, true);
        double defender_wind_modifier = CalculateWindAdvantage(modifiers, false);
        attacker_strength *= (1.0 + attacker_wind_modifier);
        defender_strength *= (1.0 + defender_wind_modifier);

        // Resolve different combat phases based on combat type
        switch (combat_type) {
            case NavalCombatType::LINE_BATTLE:
                ResolveBroadsideCombat(attacker_fleet, defender_fleet, modifiers, config, result);
                break;

            case NavalCombatType::BOARDING_ACTION:
                ResolveBoardingActions(attacker_fleet, defender_fleet, config, result);
                break;

            case NavalCombatType::RAMMING_ATTACK:
                ResolveRammingAttacks(attacker_fleet, defender_fleet, config, result);
                break;

            case NavalCombatType::CHASE:
            case NavalCombatType::BLOCKADE_ENGAGEMENT:
                // Mixed combat - apply all types with reduced effectiveness
                ResolveBroadsideCombat(attacker_fleet, defender_fleet, modifiers, config, result);
                result.casualties_from_broadsides = static_cast<uint32_t>(
                    result.casualties_from_broadsides * 0.7
                );
                ResolveBoardingActions(attacker_fleet, defender_fleet, config, result);
                break;
        }

        // Calculate total casualties
        result.attacker_casualties = result.casualties_from_broadsides +
                                      result.casualties_from_boarding +
                                      result.casualties_from_ramming +
                                      result.casualties_from_fire;
        result.defender_casualties = result.attacker_casualties; // Simplified for now

        // Calculate ship losses (sunk and captured)
        CalculateShipLosses(
            attacker_fleet, result.attacker_casualties, config,
            result.ships_sunk_attacker, result.ships_captured_by_defender
        );
        CalculateShipLosses(
            defender_fleet, result.defender_casualties, config,
            result.ships_sunk_defender, result.ships_captured_by_attacker
        );

        // Use standard battle resolution for overall outcome
        CombatComponent combat_context{};
        combat_context.terrain_modifier = modifiers.is_coastal ? -0.1 : 0.0;
        combat_context.weather_modifier = 1.0 - (modifiers.wave_height * 0.3);

        BattleResult base_result = BattleResolutionCalculator::ResolveBattle(
            attacker_fleet, defender_fleet, combat_context,
            attacker_admiral, defender_admiral, nullptr, config
        );

        // Copy base results
        result.outcome = base_result.outcome;
        result.attacker_morale_change = base_result.attacker_morale_change;
        result.defender_morale_change = base_result.defender_morale_change;
        result.attacker_experience_gain = base_result.attacker_experience_gain;
        result.defender_experience_gain = base_result.defender_experience_gain;
        result.battle_intensity = base_result.battle_intensity;
        result.war_score_change = base_result.war_score_change;
        result.prestige_change = base_result.prestige_change;

        // Calculate naval tradition
        result.naval_tradition_gained = CalculateNavalTradition(result, config);

        // Generate famous engagement name for decisive battles
        if (result.outcome == BattleOutcome::ATTACKER_DECISIVE_VICTORY ||
            result.outcome == BattleOutcome::DEFENDER_DECISIVE_VICTORY) {
            if (result.ships_sunk_attacker + result.ships_sunk_defender >= 5) {
                result.famous_engagement_name = "Battle of the " +
                    (modifiers.is_coastal ? "Coastal Straits" : "Open Seas");
            }
        }

        return result;
    }

    // ========================================================================
    // Naval Combat Types
    // ========================================================================

    NavalCombatType NavalCombatCalculator::DetermineNavalCombatType(
        const ArmyComponent& attacker_fleet,
        const ArmyComponent& defender_fleet,
        const NavalCombatModifiers& modifiers
    ) {
        // Count ship types
        uint32_t attacker_galleys = CountShipsOfType(attacker_fleet, UnitType::GALLEYS);
        uint32_t defender_galleys = CountShipsOfType(defender_fleet, UnitType::GALLEYS);
        uint32_t attacker_ships_of_line = CountShipsOfType(attacker_fleet, UnitType::SHIPS_OF_THE_LINE);
        uint32_t defender_ships_of_line = CountShipsOfType(defender_fleet, UnitType::SHIPS_OF_THE_LINE);

        // Ships of the line prefer line battles
        if (attacker_ships_of_line > 0 || defender_ships_of_line > 0) {
            if (modifiers.wind_strength > 0.3 && modifiers.visibility > 0.5) {
                return NavalCombatType::LINE_BATTLE;
            }
        }

        // Galleys prefer ramming
        if (attacker_galleys > static_cast<uint32_t>(attacker_fleet.units.size() * 0.5)) {
            return NavalCombatType::RAMMING_ATTACK;
        }

        // Coastal waters favor boarding
        if (modifiers.is_coastal) {
            return NavalCombatType::BOARDING_ACTION;
        }

        // Default to line battle for gunned ships
        return NavalCombatType::LINE_BATTLE;
    }

    void NavalCombatCalculator::ResolveBroadsideCombat(
        const ArmyComponent& attacker_fleet,
        const ArmyComponent& defender_fleet,
        const NavalCombatModifiers& modifiers,
        const NavalCombatConfig& config,
        NavalBattleResult& result
    ) {
        // Calculate broadside power
        double attacker_broadside = CalculateBroadsidePower(attacker_fleet, modifiers, config);
        double defender_broadside = CalculateBroadsidePower(defender_fleet, modifiers, config);

        // Apply visibility modifier (affects gunnery accuracy)
        double visibility_mod = CalculateVisibilityModifier(modifiers);
        attacker_broadside *= visibility_mod;
        defender_broadside *= visibility_mod;

        // Calculate casualties from broadsides
        uint32_t attacker_casualties = static_cast<uint32_t>(
            defender_broadside * config.broadside_damage_multiplier * 0.1
        );
        uint32_t defender_casualties = static_cast<uint32_t>(
            attacker_broadside * config.broadside_damage_multiplier * 0.1
        );

        result.casualties_from_broadsides = attacker_casualties + defender_casualties;

        // Calculate fire damage
        result.casualties_from_fire = CalculateFireDamage(attacker_fleet, defender_broadside, config);
        result.casualties_from_fire += CalculateFireDamage(defender_fleet, attacker_broadside, config);
    }

    void NavalCombatCalculator::ResolveBoardingActions(
        const ArmyComponent& attacker_fleet,
        const ArmyComponent& defender_fleet,
        const NavalCombatConfig& config,
        NavalBattleResult& result
    ) {
        // Calculate boarding strength
        double attacker_boarding = CalculateBoardingStrength(attacker_fleet, config);
        double defender_boarding = CalculateBoardingStrength(defender_fleet, config);

        // Determine boarding success
        double boarding_ratio = attacker_boarding / std::max(defender_boarding, 1.0);

        if (boarding_ratio > config.boarding_success_threshold) {
            // Successful boarding - higher casualties, chance of capture
            uint32_t boarding_casualties = static_cast<uint32_t>(
                defender_fleet.total_strength * 0.15
            );
            result.casualties_from_boarding = boarding_casualties;

            // Chance to capture ships
            if (boarding_ratio > 1.5) {
                result.ships_captured_by_attacker = static_cast<uint32_t>(
                    defender_fleet.units.size() * config.ship_capture_chance
                );
            }
        } else {
            // Failed boarding - moderate casualties
            result.casualties_from_boarding = static_cast<uint32_t>(
                attacker_fleet.total_strength * 0.1
            );
        }
    }

    void NavalCombatCalculator::ResolveRammingAttacks(
        const ArmyComponent& attacker_fleet,
        const ArmyComponent& defender_fleet,
        const NavalCombatConfig& config,
        NavalBattleResult& result
    ) {
        // Calculate ramming effectiveness
        NavalCombatModifiers modifiers{};  // Use default modifiers for ramming
        double attacker_ramming = CalculateRammingEffectiveness(attacker_fleet, modifiers, config);
        double defender_ramming = CalculateRammingEffectiveness(defender_fleet, modifiers, config);

        // Ramming causes heavy damage to both sides
        uint32_t attacker_casualties = static_cast<uint32_t>(
            defender_ramming * config.ramming_damage_multiplier * 50
        );
        uint32_t defender_casualties = static_cast<uint32_t>(
            attacker_ramming * config.ramming_damage_multiplier * 50
        );

        result.casualties_from_ramming = attacker_casualties + defender_casualties;

        // Ramming can sink ships directly
        if (attacker_ramming > defender_ramming * 1.5) {
            result.ships_sunk_defender += static_cast<uint32_t>(defender_fleet.units.size() * 0.1);
        }
    }

    // ========================================================================
    // Naval Combat Strength Calculations
    // ========================================================================

    double NavalCombatCalculator::CalculateNavalCombatStrength(
        const ArmyComponent& fleet,
        const Commander* admiral,
        const NavalCombatModifiers& modifiers,
        const NavalCombatConfig& config
    ) {
        double base_strength = 0.0;

        // Sum up strength from all ships
        for (const auto& unit : fleet.units) {
            if (IsNavalUnit(unit.type)) {
                double unit_strength = BattleResolutionCalculator::CalculateUnitStrength(unit, config);

                // Apply ship-specific weather bonuses
                double weather_bonus = GetShipWeatherBonus(unit.type, modifiers);
                unit_strength *= (1.0 + weather_bonus);

                base_strength += unit_strength;
            }
        }

        // Apply admiral bonuses
        if (admiral) {
            double admiral_bonus = CalculateAdmiralBonus(admiral, fleet, config);
            base_strength *= (1.0 + admiral_bonus);
        }

        // Apply morale multiplier
        double morale_multiplier = BattleResolutionCalculator::CalculateMoraleMultiplier(
            fleet.army_morale, config
        );
        base_strength *= morale_multiplier;

        // Apply organization
        base_strength *= fleet.organization;

        return base_strength;
    }

    double NavalCombatCalculator::CalculateBroadsidePower(
        const ArmyComponent& fleet,
        const NavalCombatModifiers& modifiers,
        const NavalCombatConfig& config
    ) {
        double broadside_power = 0.0;

        for (const auto& unit : fleet.units) {
            double unit_broadside = 0.0;

            switch (unit.type) {
                case UnitType::SHIPS_OF_THE_LINE:
                    unit_broadside = unit.current_strength * 2.0 *
                                     (1.0 + config.ship_of_line_broadside_bonus);
                    break;
                case UnitType::GALLEONS:
                case UnitType::WAR_GALLEONS:
                    unit_broadside = unit.current_strength * 1.5;
                    break;
                case UnitType::CARRACKS:
                    unit_broadside = unit.current_strength * 1.0;
                    break;
                case UnitType::COGS:
                    unit_broadside = unit.current_strength * 0.5;
                    break;
                case UnitType::GALLEYS:
                    unit_broadside = unit.current_strength * 0.3;  // Galleys have few guns
                    break;
                default:
                    unit_broadside = 0.0;
                    break;
            }

            // Apply equipment quality
            unit_broadside *= (0.7 + unit.equipment_quality * 0.3);

            broadside_power += unit_broadside;
        }

        return broadside_power;
    }

    double NavalCombatCalculator::CalculateBoardingStrength(
        const ArmyComponent& fleet,
        const NavalCombatConfig& config
    ) {
        double boarding_strength = 0.0;

        for (const auto& unit : fleet.units) {
            double unit_boarding = unit.current_strength;

            // Galleons are good at boarding
            if (unit.type == UnitType::GALLEONS || unit.type == UnitType::WAR_GALLEONS) {
                unit_boarding *= (1.0 + config.galleon_boarding_bonus);
            }

            // Galleys excel at boarding
            if (unit.type == UnitType::GALLEYS) {
                unit_boarding *= 1.5;
            }

            // Apply training and morale
            unit_boarding *= (0.5 + unit.training * 0.5);
            boarding_strength += unit_boarding;
        }

        return boarding_strength;
    }

    double NavalCombatCalculator::CalculateRammingEffectiveness(
        const ArmyComponent& fleet,
        const NavalCombatModifiers& modifiers,
        const NavalCombatConfig& config
    ) {
        double ramming_power = 0.0;

        for (const auto& unit : fleet.units) {
            double unit_ramming = 0.0;

            // Galleys are designed for ramming
            if (unit.type == UnitType::GALLEYS) {
                unit_ramming = unit.current_strength * (1.0 + config.galley_ramming_bonus);
            }
            // Heavy ships can ram but less effectively
            else if (unit.type == UnitType::SHIPS_OF_THE_LINE ||
                     unit.type == UnitType::CARRACKS) {
                unit_ramming = unit.current_strength * 0.5;
            }

            // Calm seas favor ramming
            if (modifiers.wave_height < 0.3) {
                unit_ramming *= 1.3;
            }

            ramming_power += unit_ramming;
        }

        return ramming_power;
    }

    // ========================================================================
    // Environmental Effects
    // ========================================================================

    double NavalCombatCalculator::CalculateWindAdvantage(
        const NavalCombatModifiers& modifiers,
        bool is_attacker
    ) {
        // Attacker with tailwind gets advantage
        double wind_bonus = modifiers.wind_direction * modifiers.wind_strength;

        if (is_attacker) {
            return wind_bonus * 0.4;
        } else {
            return -wind_bonus * 0.4;
        }
    }

    double NavalCombatCalculator::CalculateWavePenalty(
        const NavalCombatModifiers& modifiers,
        UnitType ship_type
    ) {
        double base_penalty = modifiers.wave_height * 0.3;

        // Smaller ships suffer more in rough seas
        if (ship_type == UnitType::GALLEYS || ship_type == UnitType::COGS) {
            base_penalty *= 1.5;
        }
        // Large ships handle waves better
        else if (ship_type == UnitType::SHIPS_OF_THE_LINE || ship_type == UnitType::CARRACKS) {
            base_penalty *= 0.7;
        }

        return base_penalty;
    }

    double NavalCombatCalculator::CalculateVisibilityModifier(
        const NavalCombatModifiers& modifiers
    ) {
        // Poor visibility reduces gunnery effectiveness
        return 0.5 + (modifiers.visibility * 0.5);
    }

    double NavalCombatCalculator::GetShipWeatherBonus(
        UnitType ship_type,
        const NavalCombatModifiers& modifiers
    ) {
        double bonus = 0.0;

        // Carracks excel in ocean conditions
        if (ship_type == UnitType::CARRACKS && modifiers.is_deep_ocean) {
            bonus += 0.3;
        }

        // Galleys prefer calm coastal waters
        if (ship_type == UnitType::GALLEYS && modifiers.is_coastal &&
            modifiers.wave_height < 0.3) {
            bonus += 0.4;
        }

        // Ships of the line perform well in moderate conditions
        if (ship_type == UnitType::SHIPS_OF_THE_LINE &&
            modifiers.wind_strength > 0.3 && modifiers.wave_height < 0.6) {
            bonus += 0.2;
        }

        return bonus;
    }

    // ========================================================================
    // Ship-Specific Mechanics
    // ========================================================================

    void NavalCombatCalculator::CalculateShipLosses(
        const ArmyComponent& fleet,
        uint32_t casualties,
        const NavalCombatConfig& config,
        uint32_t& ships_sunk,
        uint32_t& ships_captured
    ) {
        ships_sunk = 0;
        ships_captured = 0;

        double casualty_percentage = static_cast<double>(casualties) /
                                      std::max(fleet.total_strength, 1u);

        // Ships sink if casualties are catastrophic
        if (casualty_percentage > config.sinking_threshold) {
            ships_sunk = static_cast<uint32_t>(fleet.units.size() *
                                                (casualty_percentage - config.sinking_threshold) * 2.0);
        }

        // Some damaged ships may be captured instead of sunk
        if (casualty_percentage > 0.5 && casualty_percentage < config.sinking_threshold) {
            ships_captured = static_cast<uint32_t>(fleet.units.size() *
                                                     config.ship_capture_chance *
                                                     casualty_percentage);
        }

        // Cap at actual fleet size
        ships_sunk = std::min(ships_sunk, static_cast<uint32_t>(fleet.units.size()));
        ships_captured = std::min(ships_captured,
                                   static_cast<uint32_t>(fleet.units.size()) - ships_sunk);
    }

    uint32_t NavalCombatCalculator::CalculateFireDamage(
        const ArmyComponent& fleet,
        double broadside_power,
        const NavalCombatConfig& config
    ) {
        // Calculate chance of ships catching fire from broadsides
        double fire_chance = broadside_power * config.fire_chance_per_broadside;

        // Fire spreads and causes casualties
        uint32_t fire_casualties = 0;
        if (fire_chance > 0.1) {
            fire_casualties = static_cast<uint32_t>(
                fleet.total_strength * fire_chance * 0.05
            );
        }

        return fire_casualties;
    }

    double NavalCombatCalculator::GetShipTypeEffectiveness(
        UnitType ship_type,
        NavalCombatType combat_type,
        const NavalCombatConfig& config
    ) {
        switch (combat_type) {
            case NavalCombatType::LINE_BATTLE:
                if (ship_type == UnitType::SHIPS_OF_THE_LINE) return 2.0;
                if (ship_type == UnitType::GALLEONS || ship_type == UnitType::WAR_GALLEONS) return 1.5;
                if (ship_type == UnitType::CARRACKS) return 1.2;
                return 1.0;

            case NavalCombatType::BOARDING_ACTION:
                if (ship_type == UnitType::GALLEYS) return 1.8;
                if (ship_type == UnitType::GALLEONS) return 1.5;
                return 1.0;

            case NavalCombatType::RAMMING_ATTACK:
                if (ship_type == UnitType::GALLEYS) return 2.0;
                if (ship_type == UnitType::CARRACKS) return 1.2;
                return 0.8;

            default:
                return 1.0;
        }
    }

    // ========================================================================
    // Admiral Bonuses
    // ========================================================================

    double NavalCombatCalculator::CalculateAdmiralBonus(
        const Commander* admiral,
        const ArmyComponent& fleet,
        const NavalCombatConfig& config
    ) {
        if (!admiral) return 0.0;

        // Base naval command bonus
        double bonus = admiral->martial_skill * 0.3 + admiral->tactical_skill * 0.4;

        // Naval specialty bonus
        if (admiral->specialty == UnitClass::NAVAL) {
            bonus *= 1.5;
        }

        // Check for naval traits
        if (HasNavalTrait(admiral, "Master and Commander")) {
            bonus += 0.3;
        }
        if (HasNavalTrait(admiral, "Sea Dog")) {
            bonus += 0.2;
        }

        return bonus * config.commander_skill_impact;
    }

    bool NavalCombatCalculator::HasNavalTrait(
        const Commander* admiral,
        const std::string& trait_name
    ) {
        if (!admiral) return false;

        for (const auto& trait : admiral->traits) {
            if (trait == trait_name) {
                return true;
            }
        }
        return false;
    }

    // ========================================================================
    // Naval Tradition and Prestige
    // ========================================================================

    double NavalCombatCalculator::CalculateNavalTradition(
        const NavalBattleResult& result,
        const NavalCombatConfig& config
    ) {
        double tradition = 0.0;

        // Tradition from sinking enemy ships
        tradition += result.ships_sunk_defender * config.tradition_per_ship_sunk;

        // Extra tradition from capturing ships
        tradition += result.ships_captured_by_attacker * config.tradition_per_ship_captured;

        // Bonus for decisive victories
        if (result.outcome == BattleOutcome::ATTACKER_DECISIVE_VICTORY) {
            tradition *= 1.5;
        }

        return tradition;
    }

    std::string NavalCombatCalculator::GenerateFamousEngagementName(
        const NavalBattleResult& result,
        const std::string& location_name
    ) {
        if (result.ships_sunk_attacker + result.ships_sunk_defender >= 10) {
            return "Great Battle of " + location_name;
        } else if (result.ships_captured_by_attacker > 5) {
            return "Capture at " + location_name;
        }
        return "";
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    bool NavalCombatCalculator::IsNavalUnit(UnitType unit_type) {
        return unit_type == UnitType::GALLEYS ||
               unit_type == UnitType::COGS ||
               unit_type == UnitType::CARRACKS ||
               unit_type == UnitType::GALLEONS ||
               unit_type == UnitType::WAR_GALLEONS ||
               unit_type == UnitType::SHIPS_OF_THE_LINE;
    }

    std::string NavalCombatCalculator::NavalCombatTypeToString(NavalCombatType type) {
        switch (type) {
            case NavalCombatType::LINE_BATTLE: return "Line Battle";
            case NavalCombatType::BOARDING_ACTION: return "Boarding Action";
            case NavalCombatType::RAMMING_ATTACK: return "Ramming Attack";
            case NavalCombatType::CHASE: return "Chase";
            case NavalCombatType::BLOCKADE_ENGAGEMENT: return "Blockade Engagement";
            default: return "Unknown";
        }
    }

    std::string NavalCombatCalculator::GenerateNavalBattleSummary(
        const NavalBattleResult& result,
        const std::string& attacker_name,
        const std::string& defender_name,
        const std::string& location_name
    ) {
        std::ostringstream summary;
        summary << std::fixed << std::setprecision(1);

        summary << "Naval Battle of " << location_name << "\n";
        summary << "Outcome: " << BattleResolutionCalculator::OutcomeToString(result.outcome) << "\n\n";

        summary << attacker_name << " Ships Sunk: " << result.ships_sunk_attacker << "\n";
        summary << defender_name << " Ships Sunk: " << result.ships_sunk_defender << "\n";
        summary << "Ships Captured by " << attacker_name << ": " << result.ships_captured_by_attacker << "\n\n";

        summary << "Casualties from Broadsides: " << result.casualties_from_broadsides << "\n";
        summary << "Casualties from Boarding: " << result.casualties_from_boarding << "\n";
        summary << "Casualties from Ramming: " << result.casualties_from_ramming << "\n";
        summary << "Casualties from Fire: " << result.casualties_from_fire << "\n\n";

        summary << "Naval Tradition Gained: " << result.naval_tradition_gained << "\n";

        if (!result.famous_engagement_name.empty()) {
            summary << "Famous Engagement: " << result.famous_engagement_name;
        }

        return summary.str();
    }

    uint32_t NavalCombatCalculator::CountShipsOfType(
        const ArmyComponent& fleet,
        UnitType ship_type
    ) {
        uint32_t count = 0;
        for (const auto& unit : fleet.units) {
            if (unit.type == ship_type) {
                count++;
            }
        }
        return count;
    }

    NavalCombatConfig NavalCombatCalculator::GetDefaultNavalConfig() {
        return NavalCombatConfig{};
    }

} // namespace game::military
