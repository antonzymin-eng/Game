// ============================================================================
// FleetManagementSystem.h - Fleet Organization and Management
// Created: 2025-11-18 - Fleet Management System Implementation
// Location: include/game/military/FleetManagementSystem.h
// ============================================================================

#pragma once

#include "game/military/MilitaryComponents.h"
#include "core/types/game_types.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace game::military {

    /// Fleet designation and role
    enum class FleetRole {
        BATTLE_FLEET,        // Main line of battle ships
        RAIDING_SQUADRON,    // Fast ships for commerce raiding
        ESCORT_GROUP,        // Convoy protection
        BLOCKADE_FORCE,      // Blockading enemy ports
        EXPLORATION_FLEET,   // Discovery and exploration
        TRANSPORT_CONVOY     // Troop/supply transport
    };

    /// Fleet composition and organization
    struct FleetComposition {
        uint32_t ships_of_the_line = 0;
        uint32_t frigates = 0;          // War galleons
        uint32_t corvettes = 0;         // Galleons
        uint32_t light_ships = 0;       // Carracks, cogs
        uint32_t galleys = 0;
        uint32_t total_ships = 0;
        uint32_t total_firepower = 0;   // Combined broadside power
    };

    /// Fleet status and readiness
    struct FleetStatus {
        bool is_at_sea = false;
        bool is_in_port = false;
        bool is_blockading = false;
        bool is_escorting = false;
        bool is_engaged = false;         // In combat
        double readiness = 1.0;          // 0.0 to 1.0
        double crew_quality = 0.5;       // Average training/morale
        double supply_status = 1.0;      // Supply level
        uint32_t days_at_sea = 0;
    };

    /// Fleet orders and missions
    struct FleetMission {
        std::string mission_type;        // "patrol", "blockade", "escort", etc.
        game::types::EntityID target_province = 0;
        game::types::EntityID escort_target = 0;
        std::vector<game::types::EntityID> patrol_route;
        bool is_active = false;
        uint32_t mission_duration_days = 0;
        double mission_progress = 0.0;
    };

    /// Fleet management system
    class FleetManagementSystem {
    public:
        // ========================================================================
        // Fleet Creation and Organization
        // ========================================================================

        /// Create a new fleet from naval units
        static ArmyComponent CreateFleet(
            const std::string& fleet_name,
            const std::vector<MilitaryUnit>& ships,
            FleetRole role = FleetRole::BATTLE_FLEET
        );

        /// Split fleet into two separate fleets
        static std::pair<ArmyComponent, ArmyComponent> SplitFleet(
            const ArmyComponent& fleet,
            const std::vector<size_t>& units_to_split
        );

        /// Merge two fleets into one
        static ArmyComponent MergeFleets(
            const ArmyComponent& fleet_a,
            const ArmyComponent& fleet_b,
            const std::string& new_fleet_name
        );

        /// Reorganize fleet by ship type (flagship, line, support)
        static void ReorganizeFleet(
            ArmyComponent& fleet,
            bool place_strongest_as_flagship = true
        );

        // ========================================================================
        // Fleet Analysis and Statistics
        // ========================================================================

        /// Get fleet composition breakdown
        static FleetComposition AnalyzeFleetComposition(
            const ArmyComponent& fleet
        );

        /// Get fleet combat readiness status
        static FleetStatus GetFleetStatus(
            const ArmyComponent& fleet
        );

        /// Calculate fleet's total firepower
        static uint32_t CalculateTotalFirepower(
            const ArmyComponent& fleet
        );

        /// Get fleet's flagship (strongest ship)
        static const MilitaryUnit* GetFlagship(
            const ArmyComponent& fleet
        );

        /// Calculate average crew quality
        static double CalculateAverageCrewQuality(
            const ArmyComponent& fleet
        );

        /// Determine optimal fleet role based on composition
        static FleetRole DetermineOptimalRole(
            const ArmyComponent& fleet
        );

        // ========================================================================
        // Fleet Missions and Orders
        // ========================================================================

        /// Assign mission to fleet
        static void AssignMission(
            ArmyComponent& fleet,
            const FleetMission& mission
        );

        /// Create patrol mission
        static FleetMission CreatePatrolMission(
            const std::vector<game::types::EntityID>& patrol_route,
            uint32_t duration_days
        );

        /// Create blockade mission
        static FleetMission CreateBlockadeMission(
            game::types::EntityID target_port,
            uint32_t duration_days
        );

        /// Create escort mission
        static FleetMission CreateEscortMission(
            game::types::EntityID convoy_to_escort,
            game::types::EntityID destination
        );

        /// Update fleet mission progress
        static void UpdateMissionProgress(
            ArmyComponent& fleet,
            FleetMission& mission,
            uint32_t days_elapsed
        );

        // ========================================================================
        // Fleet Maintenance and Supply
        // ========================================================================

        /// Repair damaged ships in fleet
        static void RepairFleet(
            ArmyComponent& fleet,
            double repair_amount
        );

        /// Resupply fleet from port
        static void ResupplyFleet(
            ArmyComponent& fleet,
            double supply_amount
        );

        /// Calculate daily maintenance cost for fleet
        static double CalculateFleetMaintenanceCost(
            const ArmyComponent& fleet
        );

        /// Calculate resupply cost at port
        static double CalculateResupplyCost(
            const ArmyComponent& fleet,
            double supply_amount
        );

        /// Calculate time needed to repair fleet
        static uint32_t CalculateRepairTime(
            const ArmyComponent& fleet,
            bool has_naval_base
        );

        // ========================================================================
        // Fleet Combat Coordination
        // ========================================================================

        /// Calculate fleet battle coordination bonus
        static double CalculateFleetCoordination(
            const ArmyComponent& fleet,
            const Commander* admiral
        );

        /// Assign ships to battle lines (van, center, rear)
        static std::unordered_map<std::string, std::vector<size_t>> AssignShipsToBattleLines(
            const ArmyComponent& fleet
        );

        /// Calculate optimal fleet formation for combat type
        static std::string GetOptimalFormation(
            const ArmyComponent& fleet,
            const std::string& combat_type
        );

        /// Calculate fleet morale
        static double CalculateFleetMorale(
            const ArmyComponent& fleet,
            const Commander* admiral
        );

        // ========================================================================
        // Fleet Admirals and Leadership
        // ========================================================================

        /// Check if fleet has admiral
        static bool HasAdmiral(const ArmyComponent& fleet);

        /// Get fleet admiral
        static Commander* GetFleetAdmiral(
            const ArmyComponent& fleet,
            std::vector<Commander>& all_commanders
        );

        /// Assign admiral to fleet
        static void AssignAdmiral(
            ArmyComponent& fleet,
            game::types::EntityID admiral_id
        );

        /// Calculate admiral effectiveness with current fleet
        static double CalculateAdmiralEffectiveness(
            const Commander* admiral,
            const ArmyComponent& fleet
        );

        // ========================================================================
        // Fleet Movement and Positioning
        // ========================================================================

        /// Calculate fleet speed (based on slowest ship)
        static double CalculateFleetSpeed(
            const ArmyComponent& fleet
        );

        /// Check if fleet can perform extended ocean voyage
        static bool CanPerformOceanVoyage(
            const ArmyComponent& fleet
        );

        /// Calculate maximum range from port
        static double CalculateMaximumRange(
            const ArmyComponent& fleet
        );

        /// Get fleet's slowest ship
        static const MilitaryUnit* GetSlowestShip(
            const ArmyComponent& fleet
        );

        // ========================================================================
        // Fleet Groups and Task Forces
        // ========================================================================

        /// Create task force from fleet
        static ArmyComponent CreateTaskForce(
            const ArmyComponent& main_fleet,
            const std::vector<size_t>& ship_indices,
            const std::string& task_force_name
        );

        /// Check if ships can operate together effectively
        static bool CanOperateTogether(
            const std::vector<MilitaryUnit>& ships
        );

        /// Calculate fleet coherence (how well ships work together)
        static double CalculateFleetCoherence(
            const ArmyComponent& fleet
        );

        // ========================================================================
        // Fleet Upgrades and Modernization
        // ========================================================================

        /// Upgrade fleet ships to newer types
        static void UpgradeFleetShips(
            ArmyComponent& fleet,
            UnitType new_ship_type,
            double upgrade_percentage
        );

        /// Refit ships with new equipment
        static void RefitFleet(
            ArmyComponent& fleet,
            double equipment_quality_increase
        );

        /// Calculate upgrade cost
        static double CalculateUpgradeCost(
            const ArmyComponent& fleet,
            UnitType new_ship_type
        );

        // ========================================================================
        // Fleet Information and Reporting
        // ========================================================================

        /// Generate fleet status report
        static std::string GenerateFleetReport(
            const ArmyComponent& fleet,
            const Commander* admiral
        );

        /// Get fleet role as string
        static std::string FleetRoleToString(FleetRole role);

        /// Get fleet strength rating
        static std::string GetFleetStrengthRating(
            const ArmyComponent& fleet
        );

        /// Calculate fleet power score
        static uint32_t CalculateFleetPowerScore(
            const ArmyComponent& fleet
        );
    };

} // namespace game::military
