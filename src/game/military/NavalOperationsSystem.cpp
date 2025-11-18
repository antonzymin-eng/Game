// ============================================================================
// NavalOperationsSystem.cpp - Naval Blockades and Coastal Bombardment Implementation
// Created: 2025-11-18 - Naval Operations System Implementation
// Location: src/game/military/NavalOperationsSystem.cpp
// ============================================================================

#include "game/military/NavalOperationsSystem.h"
#include "game/military/NavalCombatCalculator.h"
#include "game/military/FleetManagementSystem.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace game::military {

    // ========================================================================
    // Naval Blockade Operations
    // ========================================================================

    BlockadeStatus NavalOperationsSystem::EstablishBlockade(
        const ArmyComponent& fleet,
        game::types::EntityID target_port,
        const game::map::ProvinceData& port_province
    ) {
        BlockadeStatus blockade{};

        // Check if fleet can establish blockade
        if (!CanMaintainBlockade(fleet, port_province)) {
            blockade.is_active = false;
            return blockade;
        }

        blockade.is_active = true;
        blockade.blockading_fleet = fleet.commander_id;  // Use commander ID as fleet ID
        blockade.target_port = target_port;
        blockade.effectiveness = CalculateBlockadeEffectiveness(fleet, port_province);
        blockade.trade_disruption_percent = CalculateTradeDisruption(blockade.effectiveness);
        blockade.days_active = 0;

        return blockade;
    }

    BlockadeEffectiveness NavalOperationsSystem::CalculateBlockadeEffectiveness(
        const ArmyComponent& blockading_fleet,
        const game::map::ProvinceData& target_port
    ) {
        uint32_t required_strength = CalculateRequiredBlockadeStrength(target_port);
        double effectiveness_ratio = static_cast<double>(blockading_fleet.total_strength) / required_strength;

        if (effectiveness_ratio >= 2.0) {
            return BlockadeEffectiveness::TOTAL;
        } else if (effectiveness_ratio >= 1.5) {
            return BlockadeEffectiveness::STRONG;
        } else if (effectiveness_ratio >= 1.0) {
            return BlockadeEffectiveness::MODERATE;
        } else if (effectiveness_ratio >= 0.5) {
            return BlockadeEffectiveness::PARTIAL;
        }

        return BlockadeEffectiveness::NONE;
    }

    void NavalOperationsSystem::UpdateBlockade(
        BlockadeStatus& blockade,
        const ArmyComponent& fleet
    ) {
        if (!blockade.is_active) return;

        blockade.days_active++;

        // Blockade effectiveness may increase over time
        if (blockade.days_active > 30) {
            // Long blockades become more effective
            blockade.trade_disruption_percent = std::min(1.0,
                blockade.trade_disruption_percent * 1.05);
        }

        // Calculate cumulative effects
        blockade.enemy_attrition_rate = CalculateBlockadeAttrition(
            blockade.effectiveness, blockade.days_active);
    }

    double NavalOperationsSystem::CalculateTradeDisruption(
        BlockadeEffectiveness effectiveness
    ) {
        switch (effectiveness) {
            case BlockadeEffectiveness::TOTAL: return 0.95;
            case BlockadeEffectiveness::STRONG: return 0.80;
            case BlockadeEffectiveness::MODERATE: return 0.60;
            case BlockadeEffectiveness::PARTIAL: return 0.35;
            default: return 0.0;
        }
    }

    double NavalOperationsSystem::CalculateBlockadeAttrition(
        BlockadeEffectiveness effectiveness,
        uint32_t days_blockaded
    ) {
        double base_attrition = CalculateTradeDisruption(effectiveness) * 0.1;

        // Attrition increases over time
        double time_multiplier = 1.0 + (days_blockaded / 100.0);

        return base_attrition * time_multiplier;
    }

    bool NavalOperationsSystem::CanMaintainBlockade(
        const ArmyComponent& fleet,
        const game::map::ProvinceData& target_port
    ) {
        // Fleet must have sufficient supply
        if (fleet.supply_level < 0.3) return false;

        // Fleet must be active
        if (!fleet.is_active) return false;

        // Fleet must have enough ships
        if (fleet.units.size() < 3) return false;

        return true;
    }

    bool NavalOperationsSystem::AttemptBlockadeBreak(
        const ArmyComponent& blockaded_fleet,
        const ArmyComponent& blockading_fleet,
        const game::map::ProvinceData& port_province
    ) {
        // Compare fleet strengths
        double strength_ratio = static_cast<double>(blockaded_fleet.total_strength) /
                                std::max(blockading_fleet.total_strength, 1u);

        // Breaking blockade requires significant force
        if (strength_ratio > 1.5) {
            return true;  // Successfully broke blockade
        } else if (strength_ratio > 1.0) {
            // 50% chance if evenly matched
            return (rand() % 100) < 50;
        }

        return false;  // Failed to break blockade
    }

    // ========================================================================
    // Coastal Bombardment
    // ========================================================================

    CoastalBombardmentResult NavalOperationsSystem::BombardCoastalFortifications(
        const ArmyComponent& fleet,
        const FortificationComponent& fortification,
        uint32_t bombardment_duration_hours
    ) {
        CoastalBombardmentResult result{};

        // Check if fleet can bombard
        if (!GetFleetBombardmentPower(fleet)) {
            result.was_successful = false;
            result.bombardment_summary = "Fleet lacks bombardment capability";
            return result;
        }

        // Calculate damage
        result.fortification_damage = CalculateFortificationDamage(fleet, bombardment_duration_hours);

        // Ammunition expenditure
        result.ammunition_expended = CalculateAmmunitionRequired(fleet, bombardment_duration_hours);

        // Siege progress
        result.siege_progress_contribution = result.fortification_damage / 1000.0;

        result.was_successful = true;
        result.bombardment_summary = GenerateBombardmentSummary(result);

        return result;
    }

    uint32_t NavalOperationsSystem::CalculateFortificationDamage(
        const ArmyComponent& fleet,
        uint32_t bombardment_duration_hours
    ) {
        uint32_t bombardment_power = GetFleetBombardmentPower(fleet);

        // Damage = power * duration
        uint32_t base_damage = bombardment_power * bombardment_duration_hours / 10;

        // Random variation (80-120%)
        double variation = 0.8 + (rand() % 40) / 100.0;

        return static_cast<uint32_t>(base_damage * variation);
    }

    uint32_t NavalOperationsSystem::CalculateGarrisonCasualties(
        const ArmyComponent& fleet,
        const MilitaryComponent& garrison,
        uint32_t bombardment_duration_hours
    ) {
        uint32_t bombardment_power = GetFleetBombardmentPower(fleet);

        // Garrison casualties are lower than fortification damage
        uint32_t base_casualties = bombardment_power * bombardment_duration_hours / 50;

        // Cap at 10% of garrison
        uint32_t max_casualties = garrison.GetTotalGarrisonStrength() / 10;

        return std::min(base_casualties, max_casualties);
    }

    bool NavalOperationsSystem::CanBombardProvince(
        const ArmyComponent& fleet,
        const game::map::ProvinceData& target_province
    ) {
        // Province must be coastal
        if (!target_province.is_coastal) return false;

        // Fleet must have bombardment-capable ships
        if (GetFleetBombardmentPower(fleet) == 0) return false;

        // Fleet must have ammunition
        bool has_ammunition = false;
        for (const auto& unit : fleet.units) {
            if (unit.ammunition > 0.2) {
                has_ammunition = true;
                break;
            }
        }

        return has_ammunition;
    }

    double NavalOperationsSystem::CalculateSiegeSupportBonus(
        const ArmyComponent& fleet,
        const FortificationComponent& fortification
    ) {
        if (!GetFleetBombardmentPower(fleet)) return 0.0;

        uint32_t bombardment_power = GetFleetBombardmentPower(fleet);

        // Bonus is based on bombardment power relative to fortification strength
        double fort_strength = fortification.walls_level * 100.0 +
                               fortification.towers_level * 50.0 +
                               fortification.citadel_level * 150.0;

        double bonus = bombardment_power / std::max(fort_strength, 100.0);

        return std::min(0.5, bonus);  // Max 50% bonus
    }

    // ========================================================================
    // Commerce Raiding
    // ========================================================================

    uint32_t NavalOperationsSystem::ConductCommerceRaiding(
        const ArmyComponent& raiding_fleet,
        const game::map::ProvinceData& target_sea_zone,
        uint32_t days_raiding
    ) {
        double raiding_effectiveness = CalculateRaidingEffectiveness(raiding_fleet);

        // Estimate trade volume (simplified)
        double trade_volume = 100.0;  // Base trade volume

        uint32_t ships_intercepted = InterceptTradeShips(
            raiding_fleet, trade_volume, days_raiding);

        return ships_intercepted;
    }

    double NavalOperationsSystem::CalculateRaidingEffectiveness(
        const ArmyComponent& raiding_fleet
    ) {
        // Fast ships are better at raiding
        double avg_speed = 0.0;
        for (const auto& unit : raiding_fleet.units) {
            avg_speed += unit.movement_speed;
        }
        avg_speed /= raiding_fleet.units.size();

        double effectiveness = avg_speed * 0.3 + raiding_fleet.organization * 0.7;

        return effectiveness;
    }

    uint32_t NavalOperationsSystem::InterceptTradeShips(
        const ArmyComponent& raiding_fleet,
        double trade_volume,
        uint32_t days_raiding
    ) {
        double effectiveness = CalculateRaidingEffectiveness(raiding_fleet);

        // Ships intercepted = effectiveness * trade volume * days * fleet size
        double interceptions = effectiveness * (trade_volume / 100.0) * days_raiding *
                                (raiding_fleet.units.size() / 5.0);

        return static_cast<uint32_t>(interceptions);
    }

    double NavalOperationsSystem::CalculateCapturedGoods(
        uint32_t ships_intercepted,
        double average_trade_value
    ) {
        return ships_intercepted * average_trade_value;
    }

    // ========================================================================
    // Naval Bombardment Effectiveness
    // ========================================================================

    uint32_t NavalOperationsSystem::GetFleetBombardmentPower(
        const ArmyComponent& fleet
    ) {
        uint32_t total_power = 0;

        for (const auto& unit : fleet.units) {
            if (HasBombardmentCapability(unit.type)) {
                uint32_t unit_power = 0;

                switch (unit.type) {
                    case UnitType::SHIPS_OF_THE_LINE:
                        unit_power = 500;
                        break;
                    case UnitType::WAR_GALLEONS:
                    case UnitType::GALLEONS:
                        unit_power = 300;
                        break;
                    case UnitType::CARRACKS:
                        unit_power = 150;
                        break;
                    default:
                        unit_power = 0;
                        break;
                }

                // Adjust for ammunition
                unit_power = static_cast<uint32_t>(unit_power * unit.ammunition);
                total_power += unit_power;
            }
        }

        return total_power;
    }

    double NavalOperationsSystem::GetBombardmentRange(
        const ArmyComponent& fleet
    ) {
        // Ships have limited bombardment range (in kilometers)
        double max_range = 0.0;

        for (const auto& unit : fleet.units) {
            if (unit.type == UnitType::SHIPS_OF_THE_LINE) {
                max_range = std::max(max_range, 4.0);  // 4km range
            } else if (unit.type == UnitType::GALLEONS || unit.type == UnitType::WAR_GALLEONS) {
                max_range = std::max(max_range, 2.5);  // 2.5km range
            } else if (unit.type == UnitType::CARRACKS) {
                max_range = std::max(max_range, 1.5);  // 1.5km range
            }
        }

        return max_range;
    }

    bool NavalOperationsSystem::HasBombardmentCapability(
        UnitType ship_type
    ) {
        return ship_type == UnitType::SHIPS_OF_THE_LINE ||
               ship_type == UnitType::WAR_GALLEONS ||
               ship_type == UnitType::GALLEONS ||
               ship_type == UnitType::CARRACKS;
    }

    uint32_t NavalOperationsSystem::CalculateAmmunitionRequired(
        const ArmyComponent& fleet,
        uint32_t bombardment_duration_hours
    ) {
        uint32_t total_ammunition = 0;

        for (const auto& unit : fleet.units) {
            if (HasBombardmentCapability(unit.type)) {
                // Each hour of bombardment consumes 5% of ammunition
                total_ammunition += bombardment_duration_hours * 5;
            }
        }

        return total_ammunition;
    }

    // ========================================================================
    // Amphibious Operations Support
    // ========================================================================

    double NavalOperationsSystem::CalculateLandingSupport(
        const ArmyComponent& fleet,
        const game::map::ProvinceData& landing_zone
    ) {
        // Landing support based on bombardment capability and transport capacity
        double bombardment_support = GetFleetBombardmentPower(fleet) / 1000.0;
        double transport_support = CalculateTransportCapacity(fleet) / 5000.0;

        return std::min(1.0, bombardment_support + transport_support);
    }

    bool NavalOperationsSystem::CanSupportLanding(
        const ArmyComponent& fleet,
        uint32_t troops_landing
    ) {
        uint32_t transport_capacity = CalculateTransportCapacity(fleet);
        return transport_capacity >= troops_landing;
    }

    uint32_t NavalOperationsSystem::CalculateTransportCapacity(
        const ArmyComponent& fleet
    ) {
        uint32_t total_capacity = 0;

        for (const auto& unit : fleet.units) {
            uint32_t unit_capacity = 0;

            switch (unit.type) {
                case UnitType::SHIPS_OF_THE_LINE:
                    unit_capacity = 200;
                    break;
                case UnitType::GALLEONS:
                case UnitType::WAR_GALLEONS:
                    unit_capacity = 300;
                    break;
                case UnitType::CARRACKS:
                    unit_capacity = 250;
                    break;
                case UnitType::COGS:
                    unit_capacity = 400;  // Cogs are good transports
                    break;
                case UnitType::GALLEYS:
                    unit_capacity = 150;
                    break;
                default:
                    unit_capacity = 100;
                    break;
            }

            total_capacity += unit_capacity;
        }

        return total_capacity;
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    std::string NavalOperationsSystem::BlockadeEffectivenessToString(
        BlockadeEffectiveness effectiveness
    ) {
        switch (effectiveness) {
            case BlockadeEffectiveness::TOTAL: return "Total Blockade";
            case BlockadeEffectiveness::STRONG: return "Strong Blockade";
            case BlockadeEffectiveness::MODERATE: return "Moderate Blockade";
            case BlockadeEffectiveness::PARTIAL: return "Partial Blockade";
            default: return "No Blockade";
        }
    }

    std::string NavalOperationsSystem::GenerateBlockadeReport(
        const BlockadeStatus& blockade,
        const ArmyComponent& fleet
    ) {
        std::ostringstream report;
        report << std::fixed << std::setprecision(1);

        report << "=== Blockade Report ===\n\n";
        report << "Status: " << (blockade.is_active ? "Active" : "Inactive") << "\n";

        if (blockade.is_active) {
            report << "Effectiveness: " << BlockadeEffectivenessToString(blockade.effectiveness) << "\n";
            report << "Trade Disruption: " << (blockade.trade_disruption_percent * 100.0) << "%\n";
            report << "Days Active: " << blockade.days_active << "\n";
            report << "Ships Intercepted: " << blockade.ships_intercepted << "\n";
            report << "Fleet Size: " << fleet.units.size() << " ships\n";
            report << "Fleet Strength: " << fleet.total_strength << "\n";
        }

        return report.str();
    }

    std::string NavalOperationsSystem::GenerateBombardmentSummary(
        const CoastalBombardmentResult& result
    ) {
        std::ostringstream summary;

        summary << "Coastal Bombardment Results:\n";
        summary << "  Fortification Damage: " << result.fortification_damage << "\n";
        summary << "  Garrison Casualties: " << result.garrison_casualties << "\n";
        summary << "  Ammunition Expended: " << result.ammunition_expended << "\n";
        summary << "  Siege Progress: +" << (result.siege_progress_contribution * 100.0) << "%\n";

        return summary.str();
    }

    uint32_t NavalOperationsSystem::CalculateRequiredBlockadeStrength(
        const game::map::ProvinceData& target_port
    ) {
        // Base requirement
        uint32_t required_strength = 2000;

        // Larger ports require more ships
        // (In a full implementation, this would check port size)

        return required_strength;
    }

    bool NavalOperationsSystem::IsProvinceBlockaded(
        game::types::EntityID province_id,
        const std::vector<BlockadeStatus>& active_blockades
    ) {
        for (const auto& blockade : active_blockades) {
            if (blockade.target_port == province_id && blockade.is_active) {
                return true;
            }
        }
        return false;
    }

} // namespace game::military
