// ============================================================================
// BuildingData.h - LOD 4 Building and Structure Data
// Created: November 1, 2025
// Description: Building types, structures, and urban development data for
//              tactical zoom level rendering
// ============================================================================

#pragma once

#include "map/ProvinceRenderComponent.h"
#include <vector>
#include <string>
#include <cstdint>

namespace game::map {

    // ========================================================================
    // BuildingType - Classification of buildings and structures
    // ========================================================================
    enum class BuildingType : uint8_t {
        // Residential
        HOUSE = 0,
        APARTMENT,
        MANSION,
        PALACE,

        // Commercial
        SHOP,
        MARKET,
        WAREHOUSE,
        WORKSHOP,
        FACTORY,

        // Agricultural
        FARM,
        MILL,
        GRANARY,
        BARN,

        // Military
        BARRACKS,
        ARMORY,
        TRAINING_GROUND,
        MILITARY_DEPOT,

        // Fortifications
        CASTLE,
        FORTRESS,
        TOWER,
        WALL_SECTION,
        GATE,
        WATCHTOWER,

        // Economic
        MINE,
        QUARRY,
        LUMBER_MILL,
        FISHING_HUT,

        // Religious
        CHURCH,
        CATHEDRAL,
        MONASTERY,
        TEMPLE,
        SHRINE,

        // Administrative
        TOWN_HALL,
        COURTHOUSE,
        TAX_OFFICE,
        CUSTOMS_HOUSE,

        // Infrastructure
        ROAD_SEGMENT,
        BRIDGE,
        AQUEDUCT,
        PORT_FACILITY,
        HARBOR,

        // Special
        MONUMENT,
        UNIVERSITY,
        LIBRARY,
        HOSPITAL,

        UNKNOWN
    };

    // ========================================================================
    // BuildingCategory - High-level grouping
    // ========================================================================
    enum class BuildingCategory : uint8_t {
        RESIDENTIAL,
        COMMERCIAL,
        AGRICULTURAL,
        MILITARY,
        FORTIFICATION,
        ECONOMIC,
        RELIGIOUS,
        ADMINISTRATIVE,
        INFRASTRUCTURE,
        SPECIAL
    };

    // ========================================================================
    // Building - Individual building instance
    // ========================================================================
    struct Building {
        BuildingType type = BuildingType::HOUSE;
        Vector2 position;              // World position
        float rotation = 0.0f;         // Rotation in radians
        float size = 1.0f;             // Size multiplier
        uint32_t level = 1;            // Building level/tier
        bool is_damaged = false;       // Damage state
        uint8_t health = 100;          // Health percentage

        Building() = default;
        Building(BuildingType t, Vector2 pos) : type(t), position(pos) {}

        // Get category from type
        BuildingCategory GetCategory() const {
            switch (type) {
                case BuildingType::HOUSE:
                case BuildingType::APARTMENT:
                case BuildingType::MANSION:
                case BuildingType::PALACE:
                    return BuildingCategory::RESIDENTIAL;

                case BuildingType::SHOP:
                case BuildingType::MARKET:
                case BuildingType::WAREHOUSE:
                case BuildingType::WORKSHOP:
                case BuildingType::FACTORY:
                    return BuildingCategory::COMMERCIAL;

                case BuildingType::FARM:
                case BuildingType::MILL:
                case BuildingType::GRANARY:
                case BuildingType::BARN:
                    return BuildingCategory::AGRICULTURAL;

                case BuildingType::BARRACKS:
                case BuildingType::ARMORY:
                case BuildingType::TRAINING_GROUND:
                case BuildingType::MILITARY_DEPOT:
                    return BuildingCategory::MILITARY;

                case BuildingType::CASTLE:
                case BuildingType::FORTRESS:
                case BuildingType::TOWER:
                case BuildingType::WALL_SECTION:
                case BuildingType::GATE:
                case BuildingType::WATCHTOWER:
                    return BuildingCategory::FORTIFICATION;

                case BuildingType::MINE:
                case BuildingType::QUARRY:
                case BuildingType::LUMBER_MILL:
                case BuildingType::FISHING_HUT:
                    return BuildingCategory::ECONOMIC;

                case BuildingType::CHURCH:
                case BuildingType::CATHEDRAL:
                case BuildingType::MONASTERY:
                case BuildingType::TEMPLE:
                case BuildingType::SHRINE:
                    return BuildingCategory::RELIGIOUS;

                case BuildingType::TOWN_HALL:
                case BuildingType::COURTHOUSE:
                case BuildingType::TAX_OFFICE:
                case BuildingType::CUSTOMS_HOUSE:
                    return BuildingCategory::ADMINISTRATIVE;

                case BuildingType::ROAD_SEGMENT:
                case BuildingType::BRIDGE:
                case BuildingType::AQUEDUCT:
                case BuildingType::PORT_FACILITY:
                case BuildingType::HARBOR:
                    return BuildingCategory::INFRASTRUCTURE;

                case BuildingType::MONUMENT:
                case BuildingType::UNIVERSITY:
                case BuildingType::LIBRARY:
                case BuildingType::HOSPITAL:
                    return BuildingCategory::SPECIAL;

                default:
                    return BuildingCategory::RESIDENTIAL;
            }
        }

        // Get color for rendering
        Color GetColor() const {
            switch (GetCategory()) {
                case BuildingCategory::RESIDENTIAL:
                    return Color(150, 100, 50);  // Brown
                case BuildingCategory::COMMERCIAL:
                    return Color(100, 100, 150); // Blue-grey
                case BuildingCategory::AGRICULTURAL:
                    return Color(180, 140, 80);  // Tan
                case BuildingCategory::MILITARY:
                    return Color(150, 50, 50);   // Dark red
                case BuildingCategory::FORTIFICATION:
                    return Color(100, 100, 100); // Grey
                case BuildingCategory::ECONOMIC:
                    return Color(140, 120, 60);  // Gold-brown
                case BuildingCategory::RELIGIOUS:
                    return Color(200, 200, 220); // Light grey
                case BuildingCategory::ADMINISTRATIVE:
                    return Color(120, 120, 180); // Purple-grey
                case BuildingCategory::INFRASTRUCTURE:
                    return Color(80, 80, 80);    // Dark grey
                case BuildingCategory::SPECIAL:
                    return Color(180, 150, 200); // Light purple
                default:
                    return Color(128, 128, 128);
            }
        }

        // Get display size based on type and level
        float GetDisplaySize() const {
            float base_size = size;

            // Larger buildings
            switch (type) {
                case BuildingType::PALACE:
                case BuildingType::CATHEDRAL:
                case BuildingType::CASTLE:
                case BuildingType::FORTRESS:
                    base_size *= 3.0f;
                    break;
                case BuildingType::MANSION:
                case BuildingType::CHURCH:
                case BuildingType::TOWN_HALL:
                case BuildingType::FACTORY:
                    base_size *= 2.0f;
                    break;
                case BuildingType::TOWER:
                case BuildingType::WATCHTOWER:
                case BuildingType::MONUMENT:
                    base_size *= 1.5f;
                    break;
                default:
                    break;
            }

            // Level scaling
            base_size *= (1.0f + (level - 1) * 0.2f);

            return base_size;
        }
    };

    // ========================================================================
    // UrbanDistrict - Cluster of buildings forming a city district
    // ========================================================================
    struct UrbanDistrict {
        Vector2 center;                    // District center
        float radius = 10.0f;              // District radius
        BuildingCategory primary_category; // Main district type
        std::vector<Building> buildings;   // Buildings in this district
        uint32_t population = 0;           // District population

        UrbanDistrict() : primary_category(BuildingCategory::RESIDENTIAL) {}
        UrbanDistrict(Vector2 pos, BuildingCategory cat)
            : center(pos), primary_category(cat) {}
    };

    // ========================================================================
    // CityLayout - Complete city structure with districts
    // ========================================================================
    struct CityLayout {
        uint32_t city_id = 0;
        std::string name;
        Vector2 center;                      // City center
        uint32_t population = 0;
        std::vector<UrbanDistrict> districts;
        std::vector<Building> fortifications; // City walls, gates, towers
        bool has_walls = false;

        CityLayout() = default;
        CityLayout(uint32_t id, const std::string& n, Vector2 pos, uint32_t pop)
            : city_id(id), name(n), center(pos), population(pop) {}

        // Get total building count
        size_t GetBuildingCount() const {
            size_t count = fortifications.size();
            for (const auto& district : districts) {
                count += district.buildings.size();
            }
            return count;
        }
    };

    // ========================================================================
    // ProvinceBuildingData - Building data for a province
    // ========================================================================
    struct ProvinceBuildingData {
        uint32_t province_id = 0;
        std::vector<CityLayout> cities;           // Major cities
        std::vector<Building> rural_buildings;    // Farms, mills, etc.
        std::vector<Building> infrastructure;     // Roads, bridges, etc.
        std::vector<Building> military_buildings; // Forts, barracks
        bool has_buildings = false;

        ProvinceBuildingData() = default;
        ProvinceBuildingData(uint32_t id) : province_id(id) {}

        // Get total building count
        size_t GetTotalBuildingCount() const {
            size_t count = rural_buildings.size() + infrastructure.size() + military_buildings.size();
            for (const auto& city : cities) {
                count += city.GetBuildingCount();
            }
            return count;
        }
    };

    // ========================================================================
    // Utility Functions
    // ========================================================================

    // Convert string to BuildingType
    inline BuildingType StringToBuildingType(const std::string& str) {
        if (str == "house") return BuildingType::HOUSE;
        if (str == "apartment") return BuildingType::APARTMENT;
        if (str == "mansion") return BuildingType::MANSION;
        if (str == "palace") return BuildingType::PALACE;
        if (str == "shop") return BuildingType::SHOP;
        if (str == "market") return BuildingType::MARKET;
        if (str == "warehouse") return BuildingType::WAREHOUSE;
        if (str == "workshop") return BuildingType::WORKSHOP;
        if (str == "factory") return BuildingType::FACTORY;
        if (str == "farm") return BuildingType::FARM;
        if (str == "mill") return BuildingType::MILL;
        if (str == "granary") return BuildingType::GRANARY;
        if (str == "barn") return BuildingType::BARN;
        if (str == "barracks") return BuildingType::BARRACKS;
        if (str == "armory") return BuildingType::ARMORY;
        if (str == "training_ground") return BuildingType::TRAINING_GROUND;
        if (str == "military_depot") return BuildingType::MILITARY_DEPOT;
        if (str == "castle") return BuildingType::CASTLE;
        if (str == "fortress") return BuildingType::FORTRESS;
        if (str == "tower") return BuildingType::TOWER;
        if (str == "wall") return BuildingType::WALL_SECTION;
        if (str == "gate") return BuildingType::GATE;
        if (str == "watchtower") return BuildingType::WATCHTOWER;
        if (str == "mine") return BuildingType::MINE;
        if (str == "quarry") return BuildingType::QUARRY;
        if (str == "lumber_mill") return BuildingType::LUMBER_MILL;
        if (str == "fishing_hut") return BuildingType::FISHING_HUT;
        if (str == "church") return BuildingType::CHURCH;
        if (str == "cathedral") return BuildingType::CATHEDRAL;
        if (str == "monastery") return BuildingType::MONASTERY;
        if (str == "temple") return BuildingType::TEMPLE;
        if (str == "shrine") return BuildingType::SHRINE;
        if (str == "town_hall") return BuildingType::TOWN_HALL;
        if (str == "courthouse") return BuildingType::COURTHOUSE;
        if (str == "tax_office") return BuildingType::TAX_OFFICE;
        if (str == "customs_house") return BuildingType::CUSTOMS_HOUSE;
        if (str == "road") return BuildingType::ROAD_SEGMENT;
        if (str == "bridge") return BuildingType::BRIDGE;
        if (str == "aqueduct") return BuildingType::AQUEDUCT;
        if (str == "port") return BuildingType::PORT_FACILITY;
        if (str == "harbor") return BuildingType::HARBOR;
        if (str == "monument") return BuildingType::MONUMENT;
        if (str == "university") return BuildingType::UNIVERSITY;
        if (str == "library") return BuildingType::LIBRARY;
        if (str == "hospital") return BuildingType::HOSPITAL;
        return BuildingType::UNKNOWN;
    }

    // Convert BuildingType to string
    inline const char* BuildingTypeToString(BuildingType type) {
        switch (type) {
            case BuildingType::HOUSE: return "house";
            case BuildingType::APARTMENT: return "apartment";
            case BuildingType::MANSION: return "mansion";
            case BuildingType::PALACE: return "palace";
            case BuildingType::SHOP: return "shop";
            case BuildingType::MARKET: return "market";
            case BuildingType::WAREHOUSE: return "warehouse";
            case BuildingType::WORKSHOP: return "workshop";
            case BuildingType::FACTORY: return "factory";
            case BuildingType::FARM: return "farm";
            case BuildingType::MILL: return "mill";
            case BuildingType::GRANARY: return "granary";
            case BuildingType::BARN: return "barn";
            case BuildingType::BARRACKS: return "barracks";
            case BuildingType::ARMORY: return "armory";
            case BuildingType::TRAINING_GROUND: return "training_ground";
            case BuildingType::MILITARY_DEPOT: return "military_depot";
            case BuildingType::CASTLE: return "castle";
            case BuildingType::FORTRESS: return "fortress";
            case BuildingType::TOWER: return "tower";
            case BuildingType::WALL_SECTION: return "wall";
            case BuildingType::GATE: return "gate";
            case BuildingType::WATCHTOWER: return "watchtower";
            case BuildingType::MINE: return "mine";
            case BuildingType::QUARRY: return "quarry";
            case BuildingType::LUMBER_MILL: return "lumber_mill";
            case BuildingType::FISHING_HUT: return "fishing_hut";
            case BuildingType::CHURCH: return "church";
            case BuildingType::CATHEDRAL: return "cathedral";
            case BuildingType::MONASTERY: return "monastery";
            case BuildingType::TEMPLE: return "temple";
            case BuildingType::SHRINE: return "shrine";
            case BuildingType::TOWN_HALL: return "town_hall";
            case BuildingType::COURTHOUSE: return "courthouse";
            case BuildingType::TAX_OFFICE: return "tax_office";
            case BuildingType::CUSTOMS_HOUSE: return "customs_house";
            case BuildingType::ROAD_SEGMENT: return "road";
            case BuildingType::BRIDGE: return "bridge";
            case BuildingType::AQUEDUCT: return "aqueduct";
            case BuildingType::PORT_FACILITY: return "port";
            case BuildingType::HARBOR: return "harbor";
            case BuildingType::MONUMENT: return "monument";
            case BuildingType::UNIVERSITY: return "university";
            case BuildingType::LIBRARY: return "library";
            case BuildingType::HOSPITAL: return "hospital";
            default: return "unknown";
        }
    }

} // namespace game::map
