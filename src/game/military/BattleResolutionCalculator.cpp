// ============================================================================
// BattleResolutionCalculator.cpp - Battle Resolution Implementation
// Created: 2025-10-31 - Battle Resolution System Implementation
// Location: src/game/military/BattleResolutionCalculator.cpp
// ============================================================================

#include "game/military/BattleResolutionCalculator.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace game::military {

    // ========================================================================
    // Core Battle Resolution
    // ========================================================================

    BattleResult BattleResolutionCalculator::ResolveBattle(
        const ArmyComponent& attacker,
        const ArmyComponent& defender,
        const CombatComponent& combat,
        const Commander* attacker_commander,
        const Commander* defender_commander,
        const FortificationComponent* fortification,
        const BattleConfig& config
    ) {
        BattleResult result{};

        // Calculate terrain and fortification modifiers
        double attacker_terrain_modifier = combat.terrain_modifier;
        double defender_terrain_modifier = combat.terrain_modifier;
        double fortification_bonus = CalculateFortificationBonus(fortification, config);

        // Calculate combat strength for both sides
        double attacker_strength = CalculateCombatStrength(
            attacker, attacker_commander, attacker_terrain_modifier, 0.0, config
        );
        double defender_strength = CalculateCombatStrength(
            defender, defender_commander, defender_terrain_modifier, fortification_bonus, config
        );

        // Determine battle duration based on strength parity
        double strength_ratio = attacker_strength / std::max(defender_strength, 1.0);
        double battle_duration = config.base_battle_duration *
            (1.0 + std::min(std::abs(1.0 - strength_ratio), config.max_battle_duration - 1.0));

        // Calculate casualties
        CalculateCasualties(
            attacker.total_strength,
            defender.total_strength,
            attacker_strength,
            defender_strength,
            battle_duration,
            config,
            result.attacker_casualties,
            result.defender_casualties
        );

        // Check for routing
        double attacker_casualty_percentage =
            static_cast<double>(result.attacker_casualties) / std::max(attacker.total_strength, 1u);
        double defender_casualty_percentage =
            static_cast<double>(result.defender_casualties) / std::max(defender.total_strength, 1u);

        bool attacker_routed = CheckRouting(
            attacker.army_morale, attacker_casualty_percentage, config
        );
        bool defender_routed = CheckRouting(
            defender.army_morale, defender_casualty_percentage, config
        );

        // Determine battle outcome
        result.outcome = DetermineBattleOutcome(
            attacker.total_strength,
            defender.total_strength,
            result.attacker_casualties,
            result.defender_casualties,
            attacker.army_morale,
            defender.army_morale,
            config
        );

        // Calculate morale changes
        result.attacker_morale_change = CalculateMoraleChange(
            attacker.total_strength,
            result.attacker_casualties,
            result.outcome,
            true,
            config
        );
        result.defender_morale_change = CalculateMoraleChange(
            defender.total_strength,
            result.defender_casualties,
            result.outcome,
            false,
            config
        );

        // Calculate experience gains
        bool attacker_won = (result.outcome == BattleOutcome::ATTACKER_DECISIVE_VICTORY ||
                             result.outcome == BattleOutcome::ATTACKER_VICTORY ||
                             result.outcome == BattleOutcome::ATTACKER_PYRRHIC_VICTORY);

        result.attacker_experience_gain = CalculateExperienceGain(
            result.defender_casualties,
            result.attacker_casualties,
            result.outcome,
            attacker_won,
            config
        );
        result.defender_experience_gain = CalculateExperienceGain(
            result.attacker_casualties,
            result.defender_casualties,
            result.outcome,
            !attacker_won,
            config
        );

        // Calculate war score and prestige changes
        result.war_score_change = CalculateWarScoreChange(
            result.outcome,
            result.attacker_casualties + result.defender_casualties,
            config
        );

        uint32_t attacker_defeated = attacker_won ? result.defender_casualties : result.attacker_casualties;
        result.prestige_change = CalculatePrestigeChange(
            result.outcome,
            attacker_defeated,
            config
        );

        // Calculate battle metrics
        result.battle_intensity = battle_duration * (attacker_strength + defender_strength) / 2000.0;
        result.casualty_ratio = static_cast<double>(result.attacker_casualties) /
            std::max(static_cast<double>(result.defender_casualties), 1.0);

        return result;
    }

    // ========================================================================
    // Combat Strength Calculations
    // ========================================================================

    double BattleResolutionCalculator::CalculateCombatStrength(
        const ArmyComponent& army,
        const Commander* commander,
        double terrain_modifier,
        double fortification_bonus,
        const BattleConfig& config
    ) {
        double base_strength = 0.0;

        // Sum up strength of all units
        for (const auto& unit : army.units) {
            base_strength += CalculateUnitStrength(unit, config);
        }

        // Apply morale multiplier
        double morale_multiplier = CalculateMoraleMultiplier(army.army_morale, config);
        base_strength *= morale_multiplier;

        // Apply supply penalty
        double supply_multiplier = 0.5 + (army.supply_level * 0.5);  // 50-100% based on supply
        base_strength *= supply_multiplier;

        // Apply fatigue penalty
        double fatigue_multiplier = 1.0 - (army.fatigue * 0.3);  // Up to 30% penalty
        base_strength *= fatigue_multiplier;

        // Apply organization bonus
        base_strength *= army.organization;

        // Apply commander bonuses
        if (commander) {
            double commander_bonus = CalculateCommanderBonus(commander, army.total_strength, config);
            base_strength *= (1.0 + commander_bonus);
        }

        // Apply terrain modifier
        base_strength *= (1.0 + terrain_modifier);

        // Apply fortification bonus
        base_strength *= (1.0 + fortification_bonus);

        return base_strength;
    }

    double BattleResolutionCalculator::CalculateUnitStrength(
        const MilitaryUnit& unit,
        const BattleConfig& config
    ) {
        // Base strength from current manpower
        double strength = static_cast<double>(unit.current_strength);

        // Apply combat stats
        double combat_multiplier = (unit.attack_strength + unit.defense_strength) / 20.0;
        strength *= combat_multiplier;

        // Apply experience bonus
        double experience_bonus = 1.0 + (unit.experience * config.experience_multiplier);
        strength *= experience_bonus;

        // Apply equipment quality
        double equipment_bonus = 0.7 + (unit.equipment_quality * config.equipment_multiplier);
        strength *= equipment_bonus;

        // Apply training level
        strength *= (0.5 + unit.training * 0.5);

        // Apply cohesion
        strength *= unit.cohesion;

        return strength;
    }

    double BattleResolutionCalculator::CalculateCommanderBonus(
        const Commander* commander,
        uint32_t army_size,
        const BattleConfig& config
    ) {
        if (!commander) return 0.0;

        // Base bonus from commander skills
        double skill_bonus = (commander->martial_skill + commander->tactical_skill) * 0.5;
        skill_bonus *= config.commander_skill_impact;

        // Apply command limit penalty if army is too large
        double size_ratio = static_cast<double>(army_size) / commander->command_limit;
        if (size_ratio > config.command_limit_penalty_threshold) {
            double penalty = (size_ratio - config.command_limit_penalty_threshold) * 0.2;
            skill_bonus *= (1.0 - std::min(penalty, 0.5));  // Max 50% penalty
        }

        // Apply morale bonus
        double morale_bonus = commander->GetMoraleBonus() * 0.1;

        return skill_bonus + morale_bonus;
    }

    double BattleResolutionCalculator::CalculateMoraleMultiplier(
        double army_morale,
        const BattleConfig& config
    ) {
        // Morale ranges from 0.0 to 1.0, affects combat strength significantly
        if (army_morale < config.routing_threshold) {
            return 0.3;  // Routing units fight very poorly
        } else if (army_morale < config.wavering_threshold) {
            return 0.6;  // Wavering units fight at reduced effectiveness
        } else if (army_morale < config.confident_threshold) {
            return 1.0;  // Steady morale, normal effectiveness
        } else {
            return 1.3;  // High morale provides significant bonus
        }
    }

    // ========================================================================
    // Casualty Calculations
    // ========================================================================

    void BattleResolutionCalculator::CalculateCasualties(
        uint32_t attacker_strength,
        uint32_t defender_strength,
        double attacker_combat_power,
        double defender_combat_power,
        double battle_duration,
        const BattleConfig& config,
        uint32_t& out_attacker_casualties,
        uint32_t& out_defender_casualties
    ) {
        // Calculate strength ratio (higher = attacker advantage)
        double strength_ratio = attacker_combat_power / std::max(defender_combat_power, 1.0);

        // Calculate casualty rates for both sides
        double attacker_casualty_rate = CalculateCasualtyRate(
            1.0 / strength_ratio,  // Inverted for attacker
            defender_combat_power / std::max(attacker_combat_power, 1.0),
            1.0,
            config
        );

        double defender_casualty_rate = CalculateCasualtyRate(
            strength_ratio,
            attacker_combat_power / std::max(defender_combat_power, 1.0),
            1.0,
            config
        );

        // Apply battle duration multiplier
        attacker_casualty_rate *= battle_duration;
        defender_casualty_rate *= battle_duration;

        // Calculate absolute casualties
        out_attacker_casualties = static_cast<uint32_t>(
            attacker_strength * std::min(attacker_casualty_rate, 0.8)  // Max 80% casualties
        );
        out_defender_casualties = static_cast<uint32_t>(
            defender_strength * std::min(defender_casualty_rate, 0.8)  // Max 80% casualties
        );

        // Ensure minimum casualties in any battle
        out_attacker_casualties = std::max(out_attacker_casualties,
            static_cast<uint32_t>(attacker_strength * 0.05));
        out_defender_casualties = std::max(out_defender_casualties,
            static_cast<uint32_t>(defender_strength * 0.05));
    }

    double BattleResolutionCalculator::CalculateCasualtyRate(
        double strength_ratio,
        double combat_power_ratio,
        double morale,
        const BattleConfig& config
    ) {
        // Base casualty rate
        double casualty_rate = config.base_casualty_rate;

        // Strength ratio impact (being outnumbered increases casualties)
        if (strength_ratio < 1.0) {
            double penalty = (1.0 - strength_ratio) * config.strength_ratio_impact;
            casualty_rate += penalty * 0.15;
        }

        // Combat power ratio impact
        if (combat_power_ratio < 1.0) {
            double penalty = (1.0 - combat_power_ratio) * 0.1;
            casualty_rate += penalty;
        }

        // Morale impact (low morale = more casualties)
        double morale_impact = (1.0 - morale) * config.morale_casualty_multiplier;
        casualty_rate += morale_impact * 0.1;

        return casualty_rate;
    }

    // ========================================================================
    // Morale Calculations
    // ========================================================================

    double BattleResolutionCalculator::CalculateMoraleChange(
        uint32_t initial_strength,
        uint32_t casualties,
        BattleOutcome outcome,
        bool is_attacker,
        const BattleConfig& config
    ) {
        double morale_change = 0.0;

        // Base morale change from outcome
        switch (outcome) {
            case BattleOutcome::ATTACKER_DECISIVE_VICTORY:
                morale_change = is_attacker ? 0.15 : -0.30;
                break;
            case BattleOutcome::ATTACKER_VICTORY:
                morale_change = is_attacker ? 0.10 : -0.20;
                break;
            case BattleOutcome::ATTACKER_PYRRHIC_VICTORY:
                morale_change = is_attacker ? 0.03 : -0.15;
                break;
            case BattleOutcome::STALEMATE:
                morale_change = -0.08;
                break;
            case BattleOutcome::DEFENDER_PYRRHIC_VICTORY:
                morale_change = is_attacker ? -0.15 : 0.03;
                break;
            case BattleOutcome::DEFENDER_VICTORY:
                morale_change = is_attacker ? -0.20 : 0.10;
                break;
            case BattleOutcome::DEFENDER_DECISIVE_VICTORY:
                morale_change = is_attacker ? -0.30 : 0.15;
                break;
        }

        // Additional morale loss from casualties
        double casualty_percentage = static_cast<double>(casualties) /
            std::max(static_cast<uint32_t>(1), initial_strength);
        morale_change -= casualty_percentage * 0.2;

        // Clamp morale change
        return std::clamp(morale_change, -0.4, 0.2);
    }

    MoraleState BattleResolutionCalculator::DetermineMoraleState(double morale) {
        if (morale < 0.2) return MoraleState::ROUTING;
        if (morale < 0.4) return MoraleState::BROKEN;
        if (morale < 0.6) return MoraleState::WAVERING;
        if (morale < 0.75) return MoraleState::STEADY;
        if (morale < 0.9) return MoraleState::CONFIDENT;
        return MoraleState::FANATICAL;
    }

    bool BattleResolutionCalculator::CheckRouting(
        double morale,
        double casualty_percentage,
        const BattleConfig& config
    ) {
        // Routing occurs if morale is very low or casualties are catastrophic
        if (morale < config.routing_threshold) {
            return true;
        }

        // Heavy casualties can trigger routing even with decent morale
        if (casualty_percentage > 0.5 && morale < config.wavering_threshold) {
            return true;
        }

        // Catastrophic casualties always cause routing
        if (casualty_percentage > 0.7) {
            return true;
        }

        return false;
    }

    // ========================================================================
    // Battle Outcome Determination
    // ========================================================================

    BattleOutcome BattleResolutionCalculator::DetermineBattleOutcome(
        uint32_t attacker_strength,
        uint32_t defender_strength,
        uint32_t attacker_casualties,
        uint32_t defender_casualties,
        double attacker_morale,
        double defender_morale,
        const BattleConfig& config
    ) {
        // Calculate casualty ratios
        double attacker_casualty_ratio = static_cast<double>(attacker_casualties) /
            std::max(attacker_strength, 1u);
        double defender_casualty_ratio = static_cast<double>(defender_casualties) /
            std::max(defender_strength, 1u);

        // Calculate remaining strength ratios
        double attacker_remaining = 1.0 - attacker_casualty_ratio;
        double defender_remaining = 1.0 - defender_casualty_ratio;

        // Check for routing
        bool attacker_routed = CheckRouting(attacker_morale, attacker_casualty_ratio, config);
        bool defender_routed = CheckRouting(defender_morale, defender_casualty_ratio, config);

        // Routing determines outcome immediately
        if (attacker_routed && !defender_routed) {
            return BattleOutcome::DEFENDER_DECISIVE_VICTORY;
        }
        if (defender_routed && !attacker_routed) {
            return BattleOutcome::ATTACKER_DECISIVE_VICTORY;
        }
        if (attacker_routed && defender_routed) {
            return BattleOutcome::STALEMATE;
        }

        // Determine winner based on remaining strength
        double strength_difference = defender_casualty_ratio - attacker_casualty_ratio;

        // Attacker victories
        if (strength_difference > 0.3) {
            return BattleOutcome::ATTACKER_DECISIVE_VICTORY;
        } else if (strength_difference > 0.15) {
            return BattleOutcome::ATTACKER_VICTORY;
        } else if (strength_difference > 0.05) {
            // Check if it's pyrrhic (heavy attacker casualties)
            if (attacker_casualty_ratio > 0.3) {
                return BattleOutcome::ATTACKER_PYRRHIC_VICTORY;
            }
            return BattleOutcome::ATTACKER_VICTORY;
        }

        // Defender victories
        else if (strength_difference < -0.3) {
            return BattleOutcome::DEFENDER_DECISIVE_VICTORY;
        } else if (strength_difference < -0.15) {
            return BattleOutcome::DEFENDER_VICTORY;
        } else if (strength_difference < -0.05) {
            // Check if it's pyrrhic (heavy defender casualties)
            if (defender_casualty_ratio > 0.3) {
                return BattleOutcome::DEFENDER_PYRRHIC_VICTORY;
            }
            return BattleOutcome::DEFENDER_VICTORY;
        }

        // Stalemate (very close battle)
        return BattleOutcome::STALEMATE;
    }

    double BattleResolutionCalculator::CalculateWarScoreChange(
        BattleOutcome outcome,
        uint32_t total_casualties,
        const BattleConfig& config
    ) {
        double war_score = 0.0;

        // Base war score from outcome
        switch (outcome) {
            case BattleOutcome::ATTACKER_DECISIVE_VICTORY:
                war_score = 15.0;
                break;
            case BattleOutcome::ATTACKER_VICTORY:
                war_score = 10.0;
                break;
            case BattleOutcome::ATTACKER_PYRRHIC_VICTORY:
                war_score = 5.0;
                break;
            case BattleOutcome::STALEMATE:
                war_score = 0.0;
                break;
            case BattleOutcome::DEFENDER_PYRRHIC_VICTORY:
                war_score = -5.0;
                break;
            case BattleOutcome::DEFENDER_VICTORY:
                war_score = -10.0;
                break;
            case BattleOutcome::DEFENDER_DECISIVE_VICTORY:
                war_score = -15.0;
                break;
        }

        // Scale by battle size
        double scale = std::min(total_casualties / 1000.0, 3.0);
        war_score *= scale;

        return war_score;
    }

    double BattleResolutionCalculator::CalculatePrestigeChange(
        BattleOutcome outcome,
        uint32_t enemy_strength_defeated,
        const BattleConfig& config
    ) {
        double prestige = 0.0;

        // Base prestige from outcome
        switch (outcome) {
            case BattleOutcome::ATTACKER_DECISIVE_VICTORY:
            case BattleOutcome::DEFENDER_DECISIVE_VICTORY:
                prestige = 5.0;
                break;
            case BattleOutcome::ATTACKER_VICTORY:
            case BattleOutcome::DEFENDER_VICTORY:
                prestige = 3.0;
                break;
            case BattleOutcome::ATTACKER_PYRRHIC_VICTORY:
            case BattleOutcome::DEFENDER_PYRRHIC_VICTORY:
                prestige = 1.0;
                break;
            case BattleOutcome::STALEMATE:
                prestige = -1.0;
                break;
        }

        // Add prestige based on enemy strength defeated
        prestige += enemy_strength_defeated * config.prestige_per_strength_defeated;

        return prestige;
    }

    // ========================================================================
    // Experience Calculations
    // ========================================================================

    double BattleResolutionCalculator::CalculateExperienceGain(
        uint32_t casualties_inflicted,
        uint32_t casualties_received,
        BattleOutcome outcome,
        bool is_winner,
        const BattleConfig& config
    ) {
        double experience = 0.0;

        // Base experience from battle participation
        experience += 5.0;

        // Experience from casualties inflicted
        experience += casualties_inflicted * config.experience_per_casualty_dealt;

        // Bonus for winning
        if (is_winner) {
            experience += 5.0;
        }

        // Less experience from catastrophic defeat
        if (!is_winner && casualties_received > casualties_inflicted * 2) {
            experience *= 0.5;
        }

        return experience;
    }

    // ========================================================================
    // Terrain and Environmental Modifiers
    // ========================================================================

    double BattleResolutionCalculator::CalculateTerrainModifier(
        const std::string& terrain_type,
        const ArmyComponent& army,
        const BattleConfig& config
    ) {
        UnitClass dominant_class = GetDominantUnitClass(army);

        // Terrain advantages/disadvantages by unit type
        if (terrain_type == "plains" || terrain_type == "grassland") {
            if (dominant_class == UnitClass::CAVALRY) {
                return config.terrain_modifier_max * 0.5;  // Cavalry bonus
            }
        } else if (terrain_type == "forest" || terrain_type == "jungle") {
            if (dominant_class == UnitClass::INFANTRY) {
                return config.terrain_modifier_max * 0.3;  // Infantry bonus
            }
            if (dominant_class == UnitClass::CAVALRY) {
                return -config.terrain_modifier_max * 0.5;  // Cavalry penalty
            }
        } else if (terrain_type == "hills" || terrain_type == "mountains") {
            if (dominant_class == UnitClass::INFANTRY) {
                return config.terrain_modifier_max * 0.4;  // Infantry bonus
            }
            if (dominant_class == UnitClass::CAVALRY) {
                return -config.terrain_modifier_max * 0.6;  // Cavalry penalty
            }
        }

        return 0.0;
    }

    double BattleResolutionCalculator::CalculateWeatherModifier(
        double weather_value,
        const ArmyComponent& army
    ) {
        // Weather value: 0.0 = terrible, 0.5 = normal, 1.0 = ideal
        // Poor weather generally hurts all armies but especially cavalry and ranged units
        if (weather_value < 0.3) {
            return -0.2;  // Poor weather penalty
        } else if (weather_value > 0.7) {
            return 0.1;   // Good weather bonus
        }
        return 0.0;
    }

    double BattleResolutionCalculator::CalculateFortificationBonus(
        const FortificationComponent* fortification,
        const BattleConfig& config
    ) {
        if (!fortification) return 0.0;

        // Calculate fortification strength
        double fort_strength = fortification->walls_level * 0.2 +
                               fortification->towers_level * 0.15 +
                               fortification->citadel_level * 0.3 +
                               fortification->moat_level * 0.1;

        // Apply structural integrity
        fort_strength *= fortification->structural_integrity;

        // Apply siege resistance
        fort_strength *= fortification->siege_resistance;

        // Scale by config multiplier
        return fort_strength * config.fortification_defense_multiplier;
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    std::string BattleResolutionCalculator::OutcomeToString(BattleOutcome outcome) {
        switch (outcome) {
            case BattleOutcome::ATTACKER_DECISIVE_VICTORY:
                return "Attacker Decisive Victory";
            case BattleOutcome::ATTACKER_VICTORY:
                return "Attacker Victory";
            case BattleOutcome::ATTACKER_PYRRHIC_VICTORY:
                return "Attacker Pyrrhic Victory";
            case BattleOutcome::STALEMATE:
                return "Stalemate";
            case BattleOutcome::DEFENDER_PYRRHIC_VICTORY:
                return "Defender Pyrrhic Victory";
            case BattleOutcome::DEFENDER_VICTORY:
                return "Defender Victory";
            case BattleOutcome::DEFENDER_DECISIVE_VICTORY:
                return "Defender Decisive Victory";
            default:
                return "Unknown Outcome";
        }
    }

    std::string BattleResolutionCalculator::GenerateBattleSummary(
        const BattleResult& result,
        const std::string& attacker_name,
        const std::string& defender_name,
        const std::string& location_name
    ) {
        std::ostringstream summary;
        summary << std::fixed << std::setprecision(1);

        summary << "Battle of " << location_name << "\n";
        summary << "Outcome: " << OutcomeToString(result.outcome) << "\n\n";

        summary << attacker_name << " Casualties: " << result.attacker_casualties << "\n";
        summary << defender_name << " Casualties: " << result.defender_casualties << "\n\n";

        summary << "Battle Intensity: " << result.battle_intensity << "\n";
        summary << "Casualty Ratio: " << result.casualty_ratio << "\n";
        summary << "War Score Change: " << result.war_score_change << "\n";
        summary << "Prestige Change: " << result.prestige_change;

        return summary.str();
    }

    UnitClass BattleResolutionCalculator::GetDominantUnitClass(const ArmyComponent& army) {
        std::unordered_map<UnitClass, uint32_t> class_counts;

        for (const auto& unit : army.units) {
            class_counts[unit.unit_class] += unit.current_strength;
        }

        UnitClass dominant = UnitClass::INFANTRY;
        uint32_t max_count = 0;

        for (const auto& [unit_class, count] : class_counts) {
            if (count > max_count) {
                max_count = count;
                dominant = unit_class;
            }
        }

        return dominant;
    }

    uint32_t BattleResolutionCalculator::CalculateEffectiveStrength(
        const ArmyComponent& army,
        double combat_multiplier
    ) {
        double effective_strength = army.total_strength * combat_multiplier;
        return static_cast<uint32_t>(effective_strength);
    }

    // ========================================================================
    // Configuration Management
    // ========================================================================

    BattleConfig BattleResolutionCalculator::GetDefaultConfig() {
        return BattleConfig{};
    }

    BattleConfig BattleResolutionCalculator::LoadConfigFromGameConfig(
        const std::string& config_json
    ) {
        // TODO: Parse JSON configuration
        // For now, return default config
        return GetDefaultConfig();
    }

} // namespace game::military
