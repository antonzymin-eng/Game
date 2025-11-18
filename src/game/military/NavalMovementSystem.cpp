// ============================================================================
// NavalMovementSystem.cpp - Naval Movement and Pathfinding Implementation
// Created: 2025-11-18 - Naval Movement System Implementation
// Location: src/game/military/NavalMovementSystem.cpp
// ============================================================================

#include "game/military/NavalMovementSystem.h"
#include "game/military/NavalCombatCalculator.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <cmath>

namespace game::military {

    // ========================================================================
    // Water Tile Detection
    // ========================================================================

    bool NavalMovementSystem::IsWaterProvince(const game::map::ProvinceData& province) {
        // Check if province terrain is coastal or if it's marked as coastal
        return province.terrain == game::map::TerrainType::COAST || province.is_coastal;
    }

    bool NavalMovementSystem::IsWaterTerrain(game::map::TerrainCellType terrain_type) {
        return terrain_type == game::map::TerrainCellType::WATER ||
               terrain_type == game::map::TerrainCellType::BEACH;
    }

    bool NavalMovementSystem::IsDeepOcean(const game::map::ProvinceData& province) {
        // Deep ocean provinces are coastal but not adjacent to land
        if (!IsWaterProvince(province)) return false;

        // If province has neighbors, check if all neighbors are also water
        // This is a simplified check - in a real implementation, you'd check actual neighbors
        return province.is_coastal && !province.has_river;
    }

    bool NavalMovementSystem::IsCoastalWaters(const game::map::ProvinceData& province) {
        // Coastal waters are water provinces adjacent to land
        return IsWaterProvince(province) && province.is_coastal;
    }

    bool NavalMovementSystem::IsRiver(const game::map::ProvinceData& province) {
        return province.has_river;
    }

    double NavalMovementSystem::GetWaterDepth(const game::map::ProvinceData& province) {
        if (!IsWaterProvince(province)) return 0.0;

        // Estimate depth based on province type
        if (IsRiver(province)) {
            return 5.0;  // Shallow rivers (5 meters)
        } else if (IsCoastalWaters(province)) {
            return 50.0;  // Coastal shelf (50 meters)
        } else if (IsDeepOcean(province)) {
            return 1000.0;  // Deep ocean (1000+ meters)
        }

        return 20.0;  // Default moderate depth
    }

    // ========================================================================
    // Naval Movement Validation
    // ========================================================================

    NavalMovementResult NavalMovementSystem::CanNavalUnitMoveTo(
        const ArmyComponent& fleet,
        const game::map::ProvinceData& current_province,
        const game::map::ProvinceData& target_province,
        const NavalMovementRestrictions& restrictions
    ) {
        NavalMovementResult result;
        result.can_move = false;

        // Check if fleet has any units
        if (fleet.units.empty()) {
            result.failure_reason = "Fleet has no ships";
            return result;
        }

        // Check if target is water
        if (!IsWaterProvince(target_province)) {
            result.failure_reason = "Target province is not water";
            return result;
        }

        // Check water depth requirements
        double target_depth = GetWaterDepth(target_province);
        if (target_depth < restrictions.min_water_depth) {
            result.failure_reason = "Water too shallow for this vessel";
            return result;
        }

        // Check if ship requires deep water
        if (restrictions.requires_deep_water && !IsDeepOcean(target_province)) {
            result.failure_reason = "Ship requires deep ocean waters";
            return result;
        }

        // Check river access
        if (IsRiver(target_province) && !restrictions.can_enter_rivers) {
            result.failure_reason = "Ship cannot enter rivers";
            return result;
        }

        // Check coastal access
        if (IsCoastalWaters(target_province) && !restrictions.can_enter_coastal_waters) {
            result.failure_reason = "Ship cannot enter coastal waters";
            return result;
        }

        // Movement is valid
        result.can_move = true;
        result.movement_cost = CalculateNavalMovementCost(
            current_province, target_province, fleet.units[0].type
        );

        // Calculate attrition risk
        game::map::WeatherState default_weather;
        result.attrition_risk = CalculateNavalAttrition(fleet, target_province, default_weather);

        return result;
    }

    bool NavalMovementSystem::CanFleetEnterProvince(
        const ArmyComponent& fleet,
        const game::map::ProvinceData& province
    ) {
        // All ships in fleet must be able to enter the province
        for (const auto& unit : fleet.units) {
            NavalMovementRestrictions restrictions = GetMovementRestrictions(unit.type);

            // Simplified check - just verify water depth
            double water_depth = GetWaterDepth(province);
            if (water_depth < restrictions.min_water_depth) {
                return false;
            }

            if (restrictions.requires_deep_water && !IsDeepOcean(province)) {
                return false;
            }
        }

        return true;
    }

    NavalMovementRestrictions NavalMovementSystem::GetMovementRestrictions(
        UnitType ship_type
    ) {
        NavalMovementRestrictions restrictions;

        switch (ship_type) {
            case UnitType::GALLEYS:
                restrictions.can_enter_rivers = true;
                restrictions.requires_deep_water = false;
                restrictions.can_enter_coastal_waters = true;
                restrictions.min_water_depth = 3.0;
                restrictions.draft = 2.0;
                break;

            case UnitType::COGS:
                restrictions.can_enter_rivers = true;
                restrictions.requires_deep_water = false;
                restrictions.can_enter_coastal_waters = true;
                restrictions.min_water_depth = 4.0;
                restrictions.draft = 3.0;
                break;

            case UnitType::CARRACKS:
                restrictions.can_enter_rivers = false;
                restrictions.requires_deep_water = false;
                restrictions.can_enter_coastal_waters = true;
                restrictions.min_water_depth = 10.0;
                restrictions.draft = 6.0;
                break;

            case UnitType::GALLEONS:
            case UnitType::WAR_GALLEONS:
                restrictions.can_enter_rivers = false;
                restrictions.requires_deep_water = false;
                restrictions.can_enter_coastal_waters = true;
                restrictions.min_water_depth = 15.0;
                restrictions.draft = 8.0;
                break;

            case UnitType::SHIPS_OF_THE_LINE:
                restrictions.can_enter_rivers = false;
                restrictions.requires_deep_water = true;
                restrictions.can_enter_coastal_waters = false;
                restrictions.min_water_depth = 30.0;
                restrictions.draft = 12.0;
                break;

            default:
                // Non-naval units
                restrictions.can_enter_rivers = false;
                restrictions.requires_deep_water = false;
                restrictions.can_enter_coastal_waters = false;
                restrictions.min_water_depth = 1000.0;  // Effectively can't move on water
                break;
        }

        return restrictions;
    }

    double NavalMovementSystem::CalculateNavalMovementCost(
        const game::map::ProvinceData& from_province,
        const game::map::ProvinceData& to_province,
        UnitType ship_type
    ) {
        double base_cost = 1.0;

        // Coastal waters are slower to navigate
        if (IsCoastalWaters(to_province)) {
            base_cost *= 1.2;
        }

        // Rivers are even slower
        if (IsRiver(to_province)) {
            base_cost *= 1.5;
        }

        // Deep ocean is fastest
        if (IsDeepOcean(to_province)) {
            base_cost *= 0.8;
        }

        // Ship-specific modifiers
        if (ship_type == UnitType::GALLEYS) {
            // Galleys are fast in coastal waters
            if (IsCoastalWaters(to_province)) {
                base_cost *= 0.8;
            }
        } else if (ship_type == UnitType::SHIPS_OF_THE_LINE) {
            // Ships of the line are slow in general
            base_cost *= 1.3;
        }

        return base_cost;
    }

    // ========================================================================
    // Naval Pathfinding
    // ========================================================================

    std::vector<game::types::EntityID> NavalMovementSystem::FindNavalPath(
        const game::map::ProvinceData& start_province,
        const game::map::ProvinceData& goal_province,
        const ArmyComponent& fleet,
        const std::vector<game::map::ProvinceData>& all_provinces
    ) {
        std::vector<game::types::EntityID> path;

        // Check if fleet has units
        if (fleet.units.empty()) {
            return path;  // Return empty path
        }

        // Simple A* pathfinding for naval routes
        std::unordered_map<uint32_t, double> g_score;
        std::unordered_map<uint32_t, double> f_score;
        std::unordered_map<uint32_t, uint32_t> came_from;

        auto heuristic = [](const game::map::ProvinceData& a, const game::map::ProvinceData& b) {
            double dx = a.center.x - b.center.x;
            double dy = a.center.y - b.center.y;
            return std::sqrt(dx * dx + dy * dy);
        };

        std::priority_queue<
            std::pair<double, uint32_t>,
            std::vector<std::pair<double, uint32_t>>,
            std::greater<std::pair<double, uint32_t>>
        > open_set;

        g_score[start_province.id] = 0.0;
        f_score[start_province.id] = heuristic(start_province, goal_province);
        open_set.push({f_score[start_province.id], start_province.id});

        while (!open_set.empty()) {
            uint32_t current_id = open_set.top().second;
            open_set.pop();

            if (current_id == goal_province.id) {
                // Reconstruct path
                uint32_t node = goal_province.id;
                while (came_from.find(node) != came_from.end()) {
                    path.push_back(node);
                    node = came_from[node];
                }
                path.push_back(start_province.id);
                std::reverse(path.begin(), path.end());
                return path;
            }

            // Find current province
            const game::map::ProvinceData* current_province = nullptr;
            for (const auto& prov : all_provinces) {
                if (prov.id == current_id) {
                    current_province = &prov;
                    break;
                }
            }

            if (!current_province) continue;

            // Check all neighbors
            for (uint32_t neighbor_id : current_province->neighbors) {
                // Find neighbor province
                const game::map::ProvinceData* neighbor = nullptr;
                for (const auto& prov : all_provinces) {
                    if (prov.id == neighbor_id) {
                        neighbor = &prov;
                        break;
                    }
                }

                if (!neighbor || !IsWaterProvince(*neighbor)) continue;

                // Check if fleet can enter
                if (!CanFleetEnterProvince(fleet, *neighbor)) continue;

                double tentative_g_score = g_score[current_id] +
                    CalculateNavalMovementCost(*current_province, *neighbor, fleet.units[0].type);

                if (g_score.find(neighbor_id) == g_score.end() ||
                    tentative_g_score < g_score[neighbor_id]) {
                    came_from[neighbor_id] = current_id;
                    g_score[neighbor_id] = tentative_g_score;
                    f_score[neighbor_id] = tentative_g_score + heuristic(*neighbor, goal_province);
                    open_set.push({f_score[neighbor_id], neighbor_id});
                }
            }
        }

        // No path found
        return path;
    }

    bool NavalMovementSystem::AreConnectedByWater(
        const game::map::ProvinceData& province_a,
        const game::map::ProvinceData& province_b,
        const std::vector<game::map::ProvinceData>& all_provinces
    ) {
        // Check if both provinces are water
        if (!IsWaterProvince(province_a) || !IsWaterProvince(province_b)) {
            return false;
        }

        // Check if they share a border
        for (uint32_t neighbor_id : province_a.neighbors) {
            if (neighbor_id == province_b.id) {
                return true;
            }
        }

        return false;
    }

    std::vector<game::types::EntityID> NavalMovementSystem::GetWaterNeighbors(
        const game::map::ProvinceData& province,
        const std::vector<game::map::ProvinceData>& all_provinces
    ) {
        std::vector<game::types::EntityID> water_neighbors;

        for (uint32_t neighbor_id : province.neighbors) {
            for (const auto& prov : all_provinces) {
                if (prov.id == neighbor_id && IsWaterProvince(prov)) {
                    water_neighbors.push_back(neighbor_id);
                    break;
                }
            }
        }

        return water_neighbors;
    }

    // ========================================================================
    // Naval Attrition and Hazards
    // ========================================================================

    double NavalMovementSystem::CalculateNavalAttrition(
        const ArmyComponent& fleet,
        const game::map::ProvinceData& current_province,
        const game::map::WeatherState& weather
    ) {
        double attrition_risk = 0.0;

        // Base attrition for being at sea
        attrition_risk += 0.01;

        // Storm damage
        if (weather.current_weather == game::map::WeatherType::STORMY) {
            attrition_risk += 0.1;
        } else if (weather.current_weather == game::map::WeatherType::HEAVY_RAIN) {
            attrition_risk += 0.05;
        }

        // High winds
        if (weather.wind.strength > 30.0f) {
            attrition_risk += 0.05;
        }

        // Deep ocean in bad weather is more dangerous
        if (IsDeepOcean(current_province) && weather.wind.strength > 20.0f) {
            attrition_risk += 0.03;
        }

        // Low supply increases attrition
        if (fleet.supply_level < 0.3) {
            attrition_risk += 0.05;
        }

        return attrition_risk;
    }

    bool NavalMovementSystem::IsInDangerousWaters(
        const game::map::ProvinceData& province,
        const game::map::WeatherState& weather
    ) {
        // Storms make waters dangerous
        if (weather.current_weather == game::map::WeatherType::STORMY ||
            weather.current_weather == game::map::WeatherType::BLIZZARD) {
            return true;
        }

        // Very high winds
        if (weather.wind.strength > 40.0f) {
            return true;
        }

        return false;
    }

    uint32_t NavalMovementSystem::CalculateStormDamage(
        const ArmyComponent& fleet,
        double storm_intensity
    ) {
        uint32_t total_damage = 0;

        for (const auto& unit : fleet.units) {
            double unit_damage = unit.current_strength * storm_intensity * 0.05;

            // Smaller ships take more damage
            if (unit.type == UnitType::GALLEYS || unit.type == UnitType::COGS) {
                unit_damage *= 1.5;
            }

            total_damage += static_cast<uint32_t>(unit_damage);
        }

        return total_damage;
    }

    // ========================================================================
    // Port and Harbor Mechanics
    // ========================================================================

    bool NavalMovementSystem::HasPort(const game::map::ProvinceData& province) {
        // Check if province is coastal (has both land and water)
        return province.is_coastal;
    }

    bool NavalMovementSystem::CanResupplyAtProvince(
        const ArmyComponent& fleet,
        const game::map::ProvinceData& province
    ) {
        // Can resupply at friendly ports
        return HasPort(province) && province.owner_id == fleet.home_province;
    }

    uint32_t NavalMovementSystem::GetPortCapacity(const game::map::ProvinceData& province) {
        if (!HasPort(province)) return 0;

        // Base capacity of 10 ships, more for larger provinces
        return 10;
    }

    bool NavalMovementSystem::IsNavalBase(const game::map::ProvinceData& province) {
        // Naval bases are major ports (simplified check)
        return HasPort(province) && province.is_coastal;
    }

    // ========================================================================
    // Land Unit Restrictions
    // ========================================================================

    bool NavalMovementSystem::HasLandUnits(const ArmyComponent& army) {
        for (const auto& unit : army.units) {
            if (!NavalCombatCalculator::IsNavalUnit(unit.type)) {
                return true;
            }
        }
        return false;
    }

    bool NavalMovementSystem::CanArmyMoveOnWater(const ArmyComponent& army) {
        // Army can only move on water if it's entirely naval units
        return !HasLandUnits(army);
    }

    bool NavalMovementSystem::CanNavalUnitsMoveOnLand(const ArmyComponent& fleet) {
        // Naval units cannot move on land
        for (const auto& unit : fleet.units) {
            if (NavalCombatCalculator::IsNavalUnit(unit.type)) {
                return false;
            }
        }
        return true;
    }

    // ========================================================================
    // Strategic Naval Zones
    // ========================================================================

    bool NavalMovementSystem::IsStrategicSeaZone(const game::map::ProvinceData& province) {
        // Strategic zones are key water provinces (simplified)
        return IsDeepOcean(province) || IsCoastalWaters(province);
    }

    game::types::EntityID NavalMovementSystem::GetSeaZoneController(
        const game::map::ProvinceData& province
    ) {
        // Sea zones can have controllers based on naval dominance
        return province.owner_id;
    }

    bool NavalMovementSystem::HasPassageRights(
        const ArmyComponent& fleet,
        const game::map::ProvinceData& province,
        game::types::EntityID zone_controller
    ) {
        // Fleet has passage rights if:
        // - Zone has no controller (international waters)
        // - Fleet belongs to zone controller
        // - Fleet has diplomatic access
        if (zone_controller == 0) return true;
        if (fleet.home_province == zone_controller) return true;

        // TODO: Check diplomatic relations
        return false;
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    double NavalMovementSystem::GetShipDraft(UnitType ship_type) {
        return GetMovementRestrictions(ship_type).draft;
    }

    bool NavalMovementSystem::IsOceanGoingVessel(UnitType ship_type) {
        return ship_type == UnitType::CARRACKS ||
               ship_type == UnitType::GALLEONS ||
               ship_type == UnitType::WAR_GALLEONS ||
               ship_type == UnitType::SHIPS_OF_THE_LINE;
    }

    bool NavalMovementSystem::IsCoastalVessel(UnitType ship_type) {
        return ship_type == UnitType::GALLEYS ||
               ship_type == UnitType::COGS;
    }

    double NavalMovementSystem::GetMovementSpeedModifier(
        UnitType ship_type,
        const game::map::ProvinceData& province,
        const game::map::WeatherState& weather
    ) {
        double modifier = 1.0;

        // Wind affects sailing ships
        double wind_effect = weather.wind.strength / 50.0;  // Normalize to 0-1
        modifier *= (0.7 + wind_effect * 0.6);  // 0.7 to 1.3 range

        // Storms slow movement
        if (weather.current_weather == game::map::WeatherType::STORMY) {
            modifier *= 0.5;
        }

        // Fog slows movement
        if (weather.atmosphere.fog_density > 0.5f) {
            modifier *= 0.8;
        }

        // Coastal vessels slower in open ocean
        if (IsCoastalVessel(ship_type) && IsDeepOcean(province)) {
            modifier *= 0.7;
        }

        // Ocean-going vessels slower in coastal waters
        if (IsOceanGoingVessel(ship_type) && IsCoastalWaters(province)) {
            modifier *= 0.9;
        }

        return modifier;
    }

} // namespace game::military
