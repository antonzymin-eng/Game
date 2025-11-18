// ============================================================================
// FleetManagementSystem.cpp - Fleet Organization and Management Implementation
// Created: 2025-11-18 - Fleet Management System Implementation
// Location: src/game/military/FleetManagementSystem.cpp
// ============================================================================

#include "game/military/FleetManagementSystem.h"
#include "game/military/NavalCombatCalculator.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace game::military {

    // ========================================================================
    // Fleet Creation and Organization
    // ========================================================================

    ArmyComponent FleetManagementSystem::CreateFleet(
        const std::string& fleet_name,
        const std::vector<MilitaryUnit>& ships,
        FleetRole role
    ) {
        ArmyComponent fleet(fleet_name);
        fleet.dominant_unit_class = UnitClass::NAVAL;

        for (const auto& ship : ships) {
            fleet.AddUnit(ship);
        }

        fleet.RecalculateStrength();
        return fleet;
    }

    std::pair<ArmyComponent, ArmyComponent> FleetManagementSystem::SplitFleet(
        const ArmyComponent& fleet,
        const std::vector<size_t>& units_to_split
    ) {
        ArmyComponent fleet_a(fleet.army_name + " A");
        ArmyComponent fleet_b(fleet.army_name + " B");

        fleet_a.dominant_unit_class = UnitClass::NAVAL;
        fleet_b.dominant_unit_class = UnitClass::NAVAL;

        for (size_t i = 0; i < fleet.units.size(); ++i) {
            if (std::find(units_to_split.begin(), units_to_split.end(), i) != units_to_split.end()) {
                fleet_b.AddUnit(fleet.units[i]);
            } else {
                fleet_a.AddUnit(fleet.units[i]);
            }
        }

        fleet_a.RecalculateStrength();
        fleet_b.RecalculateStrength();

        return {fleet_a, fleet_b};
    }

    ArmyComponent FleetManagementSystem::MergeFleets(
        const ArmyComponent& fleet_a,
        const ArmyComponent& fleet_b,
        const std::string& new_fleet_name
    ) {
        ArmyComponent merged_fleet(new_fleet_name);
        merged_fleet.dominant_unit_class = UnitClass::NAVAL;

        for (const auto& unit : fleet_a.units) {
            merged_fleet.AddUnit(unit);
        }

        for (const auto& unit : fleet_b.units) {
            merged_fleet.AddUnit(unit);
        }

        merged_fleet.RecalculateStrength();
        return merged_fleet;
    }

    void FleetManagementSystem::ReorganizeFleet(
        ArmyComponent& fleet,
        bool place_strongest_as_flagship
    ) {
        if (place_strongest_as_flagship && !fleet.units.empty()) {
            // Sort by combat strength, strongest first
            std::sort(fleet.units.begin(), fleet.units.end(),
                [](const MilitaryUnit& a, const MilitaryUnit& b) {
                    return a.current_strength > b.current_strength;
                });
        }
    }

    // ========================================================================
    // Fleet Analysis and Statistics
    // ========================================================================

    FleetComposition FleetManagementSystem::AnalyzeFleetComposition(
        const ArmyComponent& fleet
    ) {
        FleetComposition composition{};

        for (const auto& unit : fleet.units) {
            composition.total_ships++;

            switch (unit.type) {
                case UnitType::SHIPS_OF_THE_LINE:
                    composition.ships_of_the_line++;
                    composition.total_firepower += 2000;
                    break;
                case UnitType::WAR_GALLEONS:
                    composition.frigates++;
                    composition.total_firepower += 1500;
                    break;
                case UnitType::GALLEONS:
                    composition.corvettes++;
                    composition.total_firepower += 1000;
                    break;
                case UnitType::CARRACKS:
                case UnitType::COGS:
                    composition.light_ships++;
                    composition.total_firepower += 500;
                    break;
                case UnitType::GALLEYS:
                    composition.galleys++;
                    composition.total_firepower += 300;
                    break;
                default:
                    break;
            }
        }

        return composition;
    }

    FleetStatus FleetManagementSystem::GetFleetStatus(
        const ArmyComponent& fleet
    ) {
        FleetStatus status{};

        status.is_at_sea = fleet.is_active && !fleet.is_in_battle;
        status.is_in_port = !fleet.is_active;
        status.is_blockading = fleet.is_besieging;
        status.is_engaged = fleet.is_in_battle;

        status.readiness = fleet.organization * fleet.army_morale;
        status.crew_quality = CalculateAverageCrewQuality(fleet);
        status.supply_status = fleet.supply_level;

        return status;
    }

    uint32_t FleetManagementSystem::CalculateTotalFirepower(
        const ArmyComponent& fleet
    ) {
        return AnalyzeFleetComposition(fleet).total_firepower;
    }

    const MilitaryUnit* FleetManagementSystem::GetFlagship(
        const ArmyComponent& fleet
    ) {
        if (fleet.units.empty()) return nullptr;

        const MilitaryUnit* flagship = &fleet.units[0];
        for (const auto& unit : fleet.units) {
            if (unit.current_strength > flagship->current_strength) {
                flagship = &unit;
            }
        }

        return flagship;
    }

    double FleetManagementSystem::CalculateAverageCrewQuality(
        const ArmyComponent& fleet
    ) {
        if (fleet.units.empty()) return 0.0;

        double total_quality = 0.0;
        for (const auto& unit : fleet.units) {
            total_quality += (unit.training + unit.experience + unit.cohesion) / 3.0;
        }

        return total_quality / fleet.units.size();
    }

    FleetRole FleetManagementSystem::DetermineOptimalRole(
        const ArmyComponent& fleet
    ) {
        FleetComposition comp = AnalyzeFleetComposition(fleet);

        // Battle fleet if lots of heavy ships
        if (comp.ships_of_the_line >= 3 || comp.frigates >= 5) {
            return FleetRole::BATTLE_FLEET;
        }

        // Raiding if fast light ships
        if (comp.light_ships > comp.total_ships / 2) {
            return FleetRole::RAIDING_SQUADRON;
        }

        // Galley fleet for coastal operations
        if (comp.galleys > comp.total_ships / 2) {
            return FleetRole::BLOCKADE_FORCE;
        }

        // Default to escort
        return FleetRole::ESCORT_GROUP;
    }

    // ========================================================================
    // Fleet Missions and Orders
    // ========================================================================

    void FleetManagementSystem::AssignMission(
        ArmyComponent& fleet,
        const FleetMission& mission
    ) {
        // Store mission data in fleet's battle history as a workaround
        // In a full implementation, you'd add a mission field to ArmyComponent
        if (mission.mission_type == "blockade") {
            fleet.is_besieging = true;
            fleet.siege_target = mission.target_province;
        }
    }

    FleetMission FleetManagementSystem::CreatePatrolMission(
        const std::vector<game::types::EntityID>& patrol_route,
        uint32_t duration_days
    ) {
        FleetMission mission{};
        mission.mission_type = "patrol";
        mission.patrol_route = patrol_route;
        mission.mission_duration_days = duration_days;
        mission.is_active = true;
        return mission;
    }

    FleetMission FleetManagementSystem::CreateBlockadeMission(
        game::types::EntityID target_port,
        uint32_t duration_days
    ) {
        FleetMission mission{};
        mission.mission_type = "blockade";
        mission.target_province = target_port;
        mission.mission_duration_days = duration_days;
        mission.is_active = true;
        return mission;
    }

    FleetMission FleetManagementSystem::CreateEscortMission(
        game::types::EntityID convoy_to_escort,
        game::types::EntityID destination
    ) {
        FleetMission mission{};
        mission.mission_type = "escort";
        mission.escort_target = convoy_to_escort;
        mission.target_province = destination;
        mission.is_active = true;
        return mission;
    }

    void FleetManagementSystem::UpdateMissionProgress(
        ArmyComponent& fleet,
        FleetMission& mission,
        uint32_t days_elapsed
    ) {
        if (!mission.is_active) return;

        mission.mission_progress += static_cast<double>(days_elapsed) / mission.mission_duration_days;

        if (mission.mission_progress >= 1.0) {
            mission.is_active = false;
            mission.mission_progress = 1.0;
        }
    }

    // ========================================================================
    // Fleet Maintenance and Supply
    // ========================================================================

    void FleetManagementSystem::RepairFleet(
        ArmyComponent& fleet,
        double repair_amount
    ) {
        for (auto& unit : fleet.units) {
            uint32_t repair_points = static_cast<uint32_t>(
                (unit.max_strength - unit.current_strength) * repair_amount
            );
            unit.RestoreStrength(repair_points);
        }

        fleet.RecalculateStrength();
    }

    void FleetManagementSystem::ResupplyFleet(
        ArmyComponent& fleet,
        double supply_amount
    ) {
        fleet.supply_level = std::min(1.0, fleet.supply_level + supply_amount);

        for (auto& unit : fleet.units) {
            unit.supply_level = fleet.supply_level;
            unit.ammunition = std::min(1.0, unit.ammunition + supply_amount);
        }
    }

    double FleetManagementSystem::CalculateFleetMaintenanceCost(
        const ArmyComponent& fleet
    ) {
        double total_cost = 0.0;

        for (const auto& unit : fleet.units) {
            total_cost += unit.monthly_maintenance;
        }

        return total_cost;
    }

    double FleetManagementSystem::CalculateResupplyCost(
        const ArmyComponent& fleet,
        double supply_amount
    ) {
        // Cost is based on fleet size and supply amount
        double base_cost = fleet.total_strength * 0.01 * supply_amount;
        return base_cost;
    }

    uint32_t FleetManagementSystem::CalculateRepairTime(
        const ArmyComponent& fleet,
        bool has_naval_base
    ) {
        uint32_t total_damage = 0;
        for (const auto& unit : fleet.units) {
            total_damage += (unit.max_strength - unit.current_strength);
        }

        // Base repair rate: 100 damage per day with naval base, 50 without
        uint32_t repair_rate = has_naval_base ? 100 : 50;
        return (total_damage + repair_rate - 1) / repair_rate;  // Ceiling division
    }

    // ========================================================================
    // Fleet Combat Coordination
    // ========================================================================

    double FleetManagementSystem::CalculateFleetCoordination(
        const ArmyComponent& fleet,
        const Commander* admiral
    ) {
        double coordination = fleet.organization * 0.5;

        if (admiral) {
            coordination += admiral->tactical_skill * 0.3;
        }

        // Penalty for mixed ship types
        FleetComposition comp = AnalyzeFleetComposition(fleet);
        bool is_uniform = (comp.ships_of_the_line == comp.total_ships) ||
                          (comp.frigates == comp.total_ships) ||
                          (comp.galleys == comp.total_ships);

        if (!is_uniform) {
            coordination *= 0.9;  // 10% penalty for mixed fleet
        }

        return coordination;
    }

    std::unordered_map<std::string, std::vector<size_t>> FleetManagementSystem::AssignShipsToBattleLines(
        const ArmyComponent& fleet
    ) {
        std::unordered_map<std::string, std::vector<size_t>> battle_lines;

        // Sort ships by strength
        std::vector<size_t> ship_indices;
        for (size_t i = 0; i < fleet.units.size(); ++i) {
            ship_indices.push_back(i);
        }

        std::sort(ship_indices.begin(), ship_indices.end(),
            [&fleet](size_t a, size_t b) {
                return fleet.units[a].current_strength > fleet.units[b].current_strength;
            });

        // Divide into three battle lines
        size_t ships_per_line = fleet.units.size() / 3;
        size_t remainder = fleet.units.size() % 3;

        battle_lines["center"] = std::vector<size_t>(
            ship_indices.begin(),
            ship_indices.begin() + ships_per_line + (remainder > 0 ? 1 : 0)
        );
        battle_lines["van"] = std::vector<size_t>(
            ship_indices.begin() + ships_per_line + (remainder > 0 ? 1 : 0),
            ship_indices.begin() + 2 * ships_per_line + (remainder > 1 ? 2 : 1)
        );
        battle_lines["rear"] = std::vector<size_t>(
            ship_indices.begin() + 2 * ships_per_line + (remainder > 1 ? 2 : 1),
            ship_indices.end()
        );

        return battle_lines;
    }

    std::string FleetManagementSystem::GetOptimalFormation(
        const ArmyComponent& fleet,
        const std::string& combat_type
    ) {
        if (combat_type == "broadside" || combat_type == "line_battle") {
            return "NAVAL_LINE";
        } else if (combat_type == "boarding" || combat_type == "ramming") {
            return "NAVAL_CRESCENT";
        }

        return "NAVAL_LINE";  // Default
    }

    double FleetManagementSystem::CalculateFleetMorale(
        const ArmyComponent& fleet,
        const Commander* admiral
    ) {
        double morale = fleet.army_morale;

        if (admiral) {
            morale += admiral->charisma * 0.1;
        }

        // Recent victories boost morale
        if (!fleet.battle_history.empty()) {
            morale += 0.05;
        }

        return std::min(1.0, morale);
    }

    // ========================================================================
    // Fleet Admirals and Leadership
    // ========================================================================

    bool FleetManagementSystem::HasAdmiral(const ArmyComponent& fleet) {
        return fleet.commander_id != 0;
    }

    Commander* FleetManagementSystem::GetFleetAdmiral(
        const ArmyComponent& fleet,
        std::vector<Commander>& all_commanders
    ) {
        for (auto& commander : all_commanders) {
            if (commander.current_army == fleet.commander_id) {
                return &commander;
            }
        }
        return nullptr;
    }

    void FleetManagementSystem::AssignAdmiral(
        ArmyComponent& fleet,
        game::types::EntityID admiral_id
    ) {
        fleet.commander_id = admiral_id;
    }

    double FleetManagementSystem::CalculateAdmiralEffectiveness(
        const Commander* admiral,
        const ArmyComponent& fleet
    ) {
        if (!admiral) return 0.0;

        double effectiveness = admiral->martial_skill * 0.4 + admiral->tactical_skill * 0.6;

        // Bonus for naval specialty
        if (admiral->specialty == UnitClass::NAVAL) {
            effectiveness *= 1.3;
        }

        return effectiveness;
    }

    // ========================================================================
    // Fleet Movement and Positioning
    // ========================================================================

    double FleetManagementSystem::CalculateFleetSpeed(
        const ArmyComponent& fleet
    ) {
        if (fleet.units.empty()) return 0.0;

        const MilitaryUnit* slowest = GetSlowestShip(fleet);
        return slowest ? slowest->movement_speed : 1.0;
    }

    bool FleetManagementSystem::CanPerformOceanVoyage(
        const ArmyComponent& fleet
    ) {
        FleetComposition comp = AnalyzeFleetComposition(fleet);

        // Need ocean-going vessels
        return comp.ships_of_the_line > 0 ||
               comp.frigates > 0 ||
               comp.corvettes > 0 ||
               (comp.light_ships > 0 && comp.galleys == 0);
    }

    double FleetManagementSystem::CalculateMaximumRange(
        const ArmyComponent& fleet
    ) {
        // Range based on supply level and ship types
        FleetComposition comp = AnalyzeFleetComposition(fleet);

        double base_range = 100.0;  // 100 provinces base

        if (comp.ships_of_the_line > 0) {
            base_range *= 1.5;  // Long-range ships
        }

        return base_range * fleet.supply_level;
    }

    const MilitaryUnit* FleetManagementSystem::GetSlowestShip(
        const ArmyComponent& fleet
    ) {
        if (fleet.units.empty()) return nullptr;

        const MilitaryUnit* slowest = &fleet.units[0];
        for (const auto& unit : fleet.units) {
            if (unit.movement_speed < slowest->movement_speed) {
                slowest = &unit;
            }
        }

        return slowest;
    }

    // ========================================================================
    // Fleet Groups and Task Forces
    // ========================================================================

    ArmyComponent FleetManagementSystem::CreateTaskForce(
        const ArmyComponent& main_fleet,
        const std::vector<size_t>& ship_indices,
        const std::string& task_force_name
    ) {
        ArmyComponent task_force(task_force_name);
        task_force.dominant_unit_class = UnitClass::NAVAL;

        for (size_t idx : ship_indices) {
            if (idx < main_fleet.units.size()) {
                task_force.AddUnit(main_fleet.units[idx]);
            }
        }

        task_force.RecalculateStrength();
        return task_force;
    }

    bool FleetManagementSystem::CanOperateTogether(
        const std::vector<MilitaryUnit>& ships
    ) {
        // Check if ships have compatible speeds and capabilities
        if (ships.empty()) return true;

        double avg_speed = 0.0;
        for (const auto& ship : ships) {
            avg_speed += ship.movement_speed;
        }
        avg_speed /= ships.size();

        // Ships should have similar speeds
        for (const auto& ship : ships) {
            if (std::abs(ship.movement_speed - avg_speed) > 0.5) {
                return false;
            }
        }

        return true;
    }

    double FleetManagementSystem::CalculateFleetCoherence(
        const ArmyComponent& fleet
    ) {
        if (fleet.units.empty()) return 0.0;

        double coherence = fleet.cohesion * 0.5;

        // Check if ships can operate together
        if (CanOperateTogether(fleet.units)) {
            coherence += 0.3;
        }

        // Admiral improves coherence
        if (HasAdmiral(fleet)) {
            coherence += 0.2;
        }

        return std::min(1.0, coherence);
    }

    // ========================================================================
    // Fleet Upgrades and Modernization
    // ========================================================================

    void FleetManagementSystem::UpgradeFleetShips(
        ArmyComponent& fleet,
        UnitType new_ship_type,
        double upgrade_percentage
    ) {
        size_t ships_to_upgrade = static_cast<size_t>(
            fleet.units.size() * upgrade_percentage
        );

        for (size_t i = 0; i < ships_to_upgrade && i < fleet.units.size(); ++i) {
            MilitaryUnit new_ship(new_ship_type);
            new_ship.current_strength = fleet.units[i].current_strength;
            new_ship.experience = fleet.units[i].experience;
            new_ship.training = fleet.units[i].training;
            fleet.units[i] = new_ship;
        }

        fleet.RecalculateStrength();
    }

    void FleetManagementSystem::RefitFleet(
        ArmyComponent& fleet,
        double equipment_quality_increase
    ) {
        for (auto& unit : fleet.units) {
            unit.equipment_quality = std::min(1.0,
                unit.equipment_quality + equipment_quality_increase);
        }
    }

    double FleetManagementSystem::CalculateUpgradeCost(
        const ArmyComponent& fleet,
        UnitType new_ship_type
    ) {
        MilitaryUnit new_ship(new_ship_type);
        double cost_per_ship = new_ship.recruitment_cost * 0.5;  // 50% of new ship cost

        return fleet.units.size() * cost_per_ship;
    }

    // ========================================================================
    // Fleet Information and Reporting
    // ========================================================================

    std::string FleetManagementSystem::GenerateFleetReport(
        const ArmyComponent& fleet,
        const Commander* admiral
    ) {
        std::ostringstream report;
        report << std::fixed << std::setprecision(1);

        report << "=== Fleet Report: " << fleet.army_name << " ===\n\n";

        FleetComposition comp = AnalyzeFleetComposition(fleet);
        report << "Total Ships: " << comp.total_ships << "\n";
        report << "  Ships of the Line: " << comp.ships_of_the_line << "\n";
        report << "  Frigates: " << comp.frigates << "\n";
        report << "  Corvettes: " << comp.corvettes << "\n";
        report << "  Light Ships: " << comp.light_ships << "\n";
        report << "  Galleys: " << comp.galleys << "\n\n";

        report << "Total Firepower: " << comp.total_firepower << "\n";
        report << "Fleet Strength: " << fleet.total_strength << "\n\n";

        FleetStatus status = GetFleetStatus(fleet);
        report << "Status: " << (status.is_at_sea ? "At Sea" : "In Port") << "\n";
        report << "Readiness: " << (status.readiness * 100.0) << "%\n";
        report << "Crew Quality: " << (status.crew_quality * 100.0) << "%\n";
        report << "Supply Status: " << (status.supply_status * 100.0) << "%\n\n";

        if (admiral) {
            report << "Admiral: " << admiral->name << "\n";
            report << "  Martial: " << (admiral->martial_skill * 100.0) << "\n";
            report << "  Tactical: " << (admiral->tactical_skill * 100.0) << "\n";
        }

        return report.str();
    }

    std::string FleetManagementSystem::FleetRoleToString(FleetRole role) {
        switch (role) {
            case FleetRole::BATTLE_FLEET: return "Battle Fleet";
            case FleetRole::RAIDING_SQUADRON: return "Raiding Squadron";
            case FleetRole::ESCORT_GROUP: return "Escort Group";
            case FleetRole::BLOCKADE_FORCE: return "Blockade Force";
            case FleetRole::EXPLORATION_FLEET: return "Exploration Fleet";
            case FleetRole::TRANSPORT_CONVOY: return "Transport Convoy";
            default: return "Unknown";
        }
    }

    std::string FleetManagementSystem::GetFleetStrengthRating(
        const ArmyComponent& fleet
    ) {
        uint32_t power_score = CalculateFleetPowerScore(fleet);

        if (power_score >= 10000) return "Overwhelming";
        if (power_score >= 5000) return "Very Strong";
        if (power_score >= 2000) return "Strong";
        if (power_score >= 1000) return "Moderate";
        if (power_score >= 500) return "Weak";
        return "Very Weak";
    }

    uint32_t FleetManagementSystem::CalculateFleetPowerScore(
        const ArmyComponent& fleet
    ) {
        FleetComposition comp = AnalyzeFleetComposition(fleet);

        uint32_t power_score = comp.total_firepower;
        power_score += fleet.total_strength / 10;
        power_score = static_cast<uint32_t>(power_score * fleet.army_morale);

        return power_score;
    }

} // namespace game::military
