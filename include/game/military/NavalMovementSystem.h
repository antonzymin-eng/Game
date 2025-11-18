// ============================================================================
// NavalMovementSystem.h - Naval Movement and Pathfinding
// Created: 2025-11-18 - Naval Movement System Implementation
// Location: include/game/military/NavalMovementSystem.h
// ============================================================================

#pragma once

#include "game/military/MilitaryComponents.h"
#include "map/TerrainData.h"
#include "map/MapData.h"
#include "map/WeatherData.h"
#include "core/types/game_types.h"
#include <vector>
#include <unordered_set>

namespace game::military {

    /// Naval movement restrictions
    struct NavalMovementRestrictions {
        bool can_enter_rivers = false;           // Most ships can't enter rivers
        bool requires_deep_water = false;        // Ships of the line need deep water
        bool can_enter_coastal_waters = true;    // Most ships can enter coastal waters
        double min_water_depth = 0.0;            // Minimum depth in meters
        double draft = 0.0;                      // Ship's draft (how deep it sits)
    };

    /// Naval movement result
    struct NavalMovementResult {
        bool can_move = false;
        std::string failure_reason;
        double movement_cost = 1.0;               // Movement point cost
        double attrition_risk = 0.0;              // Risk of damage/attrition
        std::vector<game::types::EntityID> path;  // Calculated path if successful
    };

    /// Naval terrain analyzer
    class NavalMovementSystem {
    public:
        // ========================================================================
        // Water Tile Detection
        // ========================================================================

        /// Check if a province is a water province
        static bool IsWaterProvince(
            const game::map::ProvinceData& province
        );

        /// Check if terrain cell is water
        static bool IsWaterTerrain(
            game::map::TerrainCellType terrain_type
        );

        /// Check if province is deep ocean (suitable for large ships)
        static bool IsDeepOcean(
            const game::map::ProvinceData& province
        );

        /// Check if province is coastal waters
        static bool IsCoastalWaters(
            const game::map::ProvinceData& province
        );

        /// Check if province is a river
        static bool IsRiver(
            const game::map::ProvinceData& province
        );

        /// Get water depth at province
        static double GetWaterDepth(
            const game::map::ProvinceData& province
        );

        // ========================================================================
        // Naval Movement Validation
        // ========================================================================

        /// Check if a naval unit can move to a province
        static NavalMovementResult CanNavalUnitMoveTo(
            const ArmyComponent& fleet,
            const game::map::ProvinceData& current_province,
            const game::map::ProvinceData& target_province,
            const NavalMovementRestrictions& restrictions
        );

        /// Check if fleet can enter province (all ships must be able to enter)
        static bool CanFleetEnterProvince(
            const ArmyComponent& fleet,
            const game::map::ProvinceData& province
        );

        /// Get movement restrictions for a ship type
        static NavalMovementRestrictions GetMovementRestrictions(
            UnitType ship_type
        );

        /// Calculate movement cost for naval travel
        static double CalculateNavalMovementCost(
            const game::map::ProvinceData& from_province,
            const game::map::ProvinceData& to_province,
            UnitType ship_type
        );

        // ========================================================================
        // Naval Pathfinding
        // ========================================================================

        /// Find naval path between two provinces
        static std::vector<game::types::EntityID> FindNavalPath(
            const game::map::ProvinceData& start_province,
            const game::map::ProvinceData& goal_province,
            const ArmyComponent& fleet,
            const std::vector<game::map::ProvinceData>& all_provinces
        );

        /// Check if two provinces are connected by water
        static bool AreConnectedByWater(
            const game::map::ProvinceData& province_a,
            const game::map::ProvinceData& province_b,
            const std::vector<game::map::ProvinceData>& all_provinces
        );

        /// Get all water neighbors of a province
        static std::vector<game::types::EntityID> GetWaterNeighbors(
            const game::map::ProvinceData& province,
            const std::vector<game::map::ProvinceData>& all_provinces
        );

        // ========================================================================
        // Naval Attrition and Hazards
        // ========================================================================

        /// Calculate attrition risk for fleet in current location
        static double CalculateNavalAttrition(
            const ArmyComponent& fleet,
            const game::map::ProvinceData& current_province,
            const game::map::WeatherState& weather
        );

        /// Check if fleet is in dangerous waters (storms, ice, etc.)
        static bool IsInDangerousWaters(
            const game::map::ProvinceData& province,
            const game::map::WeatherState& weather
        );

        /// Calculate storm damage to fleet
        static uint32_t CalculateStormDamage(
            const ArmyComponent& fleet,
            double storm_intensity
        );

        // ========================================================================
        // Port and Harbor Mechanics
        // ========================================================================

        /// Check if province has a port
        static bool HasPort(
            const game::map::ProvinceData& province
        );

        /// Check if fleet can resupply at province
        static bool CanResupplyAtProvince(
            const ArmyComponent& fleet,
            const game::map::ProvinceData& province
        );

        /// Get port capacity (how many ships can dock)
        static uint32_t GetPortCapacity(
            const game::map::ProvinceData& province
        );

        /// Check if province is a naval base (advanced port)
        static bool IsNavalBase(
            const game::map::ProvinceData& province
        );

        // ========================================================================
        // Land Unit Restrictions
        // ========================================================================

        /// Check if army contains any non-naval units
        static bool HasLandUnits(
            const ArmyComponent& army
        );

        /// Prevent land units from moving on water
        static bool CanArmyMoveOnWater(
            const ArmyComponent& army
        );

        /// Check if naval units are trying to move on land
        static bool CanNavalUnitsMoveOnLand(
            const ArmyComponent& fleet
        );

        // ========================================================================
        // Strategic Naval Zones
        // ========================================================================

        /// Check if province is in a strategic sea zone
        static bool IsStrategicSeaZone(
            const game::map::ProvinceData& province
        );

        /// Get controlling nation of sea zone
        static game::types::EntityID GetSeaZoneController(
            const game::map::ProvinceData& province
        );

        /// Check if fleet has passage rights in sea zone
        static bool HasPassageRights(
            const ArmyComponent& fleet,
            const game::map::ProvinceData& province,
            game::types::EntityID zone_controller
        );

        // ========================================================================
        // Utility Functions
        // ========================================================================

        /// Get ship draft based on ship type
        static double GetShipDraft(UnitType ship_type);

        /// Check if ship type is ocean-going
        static bool IsOceanGoingVessel(UnitType ship_type);

        /// Check if ship type is coastal vessel
        static bool IsCoastalVessel(UnitType ship_type);

        /// Get movement speed modifier based on ship type and conditions
        static double GetMovementSpeedModifier(
            UnitType ship_type,
            const game::map::ProvinceData& province,
            const game::map::WeatherState& weather
        );
    };

} // namespace game::military
