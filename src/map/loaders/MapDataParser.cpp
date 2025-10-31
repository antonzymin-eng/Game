// ============================================================================
// MapDataParser.cpp - Map Data Parser Implementation
// Mechanica Imperii
// ============================================================================

#include "map/loaders/MapDataParser.h"

namespace game::map::loaders {

    MapDataParser::MapDataParser() {}
    MapDataParser::~MapDataParser() {}

    bool MapDataParser::ParseProvinces(const Json::Value& data, std::vector<ProvinceData>& provinces) {
        if (!data.isMember("provinces") || !data["provinces"].isArray()) {
            m_last_error = "Missing or invalid 'provinces' array";
            return false;
        }

        for (const auto& prov_json : data["provinces"]) {
            ProvinceData province;
            if (ParseProvince(prov_json, province)) {
                provinces.push_back(province);
            }
        }

        return !provinces.empty();
    }

    bool MapDataParser::ParseProvince(const Json::Value& data, ProvinceData& province) {
        if (!data.isMember("id") || !data.isMember("name")) {
            m_last_error = "Province missing required fields (id, name)";
            return false;
        }

        province.id = data["id"].asUInt();
        province.name = data["name"].asString();

        if (data.isMember("owner_id")) {
            province.owner_id = data["owner_id"].asUInt();
        }

        if (data.isMember("terrain")) {
            province.terrain = ParseTerrainType(data["terrain"].asString());
        }

        if (data.isMember("climate")) {
            province.climate = ParseClimateZone(data["climate"].asString());
        }

        if (data.isMember("boundary") && data["boundary"].isArray()) {
            ParseCoordinateArray(data["boundary"], province.boundary);
        }

        if (data.isMember("center")) {
            const auto& center = data["center"];
            if (center.isArray() && center.size() >= 2) {
                province.center.x = center[0].asDouble();
                province.center.y = center[1].asDouble();
            }
        }

        return true;
    }

    bool MapDataParser::ValidateMapData(const Json::Value& data) {
        return data.isMember("provinces") && data["provinces"].isArray();
    }

    bool MapDataParser::ParseCoordinateArray(const Json::Value& array, std::vector<Coordinate>& coords) {
        if (!array.isArray()) return false;

        for (const auto& point : array) {
            if (point.isArray() && point.size() >= 2) {
                Coordinate coord;
                coord.x = point[0].asDouble();
                coord.y = point[1].asDouble();
                coords.push_back(coord);
            } else if (point.isObject() && point.isMember("x") && point.isMember("y")) {
                Coordinate coord;
                coord.x = point["x"].asDouble();
                coord.y = point["y"].asDouble();
                coords.push_back(coord);
            }
        }

        return !coords.empty();
    }

    TerrainType MapDataParser::ParseTerrainType(const std::string& type_str) {
        if (type_str == "plains") return TerrainType::PLAINS;
        if (type_str == "hills") return TerrainType::HILLS;
        if (type_str == "mountains") return TerrainType::MOUNTAINS;
        if (type_str == "forest") return TerrainType::FOREST;
        if (type_str == "desert") return TerrainType::DESERT;
        if (type_str == "coast") return TerrainType::COAST;
        if (type_str == "wetland") return TerrainType::WETLAND;
        if (type_str == "highlands") return TerrainType::HIGHLANDS;
        return TerrainType::UNKNOWN;
    }

    ClimateZone MapDataParser::ParseClimateZone(const std::string& zone_str) {
        if (zone_str == "arctic") return ClimateZone::ARCTIC;
        if (zone_str == "subarctic") return ClimateZone::SUBARCTIC;
        if (zone_str == "temperate") return ClimateZone::TEMPERATE;
        if (zone_str == "subtropical") return ClimateZone::SUBTROPICAL;
        if (zone_str == "tropical") return ClimateZone::TROPICAL;
        if (zone_str == "arid") return ClimateZone::ARID;
        if (zone_str == "semiarid") return ClimateZone::SEMIARID;
        if (zone_str == "mediterranean") return ClimateZone::MEDITERRANEAN;
        return ClimateZone::UNKNOWN;
    }

} // namespace game::map::loaders
