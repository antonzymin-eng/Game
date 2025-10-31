// ============================================================================
// MapDataParser.h - Generic Map Data Parsing
// Mechanica Imperii - Parse various map data formats
// ============================================================================

#pragma once

#include "map/MapData.h"
#include <string>
#include <vector>
#include <json/json.h>

namespace game::map::loaders {

    // ============================================================================
    // Map Data Parser
    // ============================================================================

    class MapDataParser {
    public:
        MapDataParser();
        ~MapDataParser();

        // Parse map data from JSON
        bool ParseProvinces(const Json::Value& data, std::vector<ProvinceData>& provinces);
        bool ParseProvince(const Json::Value& data, ProvinceData& province);

        // Validation
        bool ValidateMapData(const Json::Value& data);

        // Error handling
        std::string GetLastError() const { return m_last_error; }

    private:
        std::string m_last_error;

        bool ParseCoordinateArray(const Json::Value& array, std::vector<Coordinate>& coords);
        TerrainType ParseTerrainType(const std::string& type_str);
        ClimateZone ParseClimateZone(const std::string& zone_str);
    };

} // namespace game::map::loaders
