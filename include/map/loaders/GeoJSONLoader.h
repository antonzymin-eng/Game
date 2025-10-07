// ============================================================================
// include/game/map/loaders/GeoJSONLoader.h - Simple GeoJSON Loading
// Start with your test_france.geojson (162KB) for initial implementation
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <json/json.h>
#include "game/map/MapData.h"

namespace game::map::loaders {

    struct Point {
        double longitude, latitude;
        Point(double lon = 0.0, double lat = 0.0) : longitude(lon), latitude(lat) {}
    };

    struct MapPolygon {
        std::vector<std::vector<Point>> rings; // First ring is outer boundary, rest are holes
        std::string name;
        std::string country_code;
        std::string admin_level;
    };

    struct SimpleMapFeature {
        enum Type { COUNTRY, PROVINCE, CITY, RIVER, LAKE };

        Type type;
        std::string name;
        std::string iso_code;
        std::vector<MapPolygon> polygons;
        Point center_point;

        // Properties from Natural Earth
        double area_km2 = 0.0;
        int population = 0;
        std::string continent;
        std::string region;
    };

    class GeoJSONLoader {
    public:
        // Load your converted GeoJSON files
        static bool LoadCountries(const std::string& filepath, std::vector<SimpleMapFeature>& features);
        static bool LoadProvinces(const std::string& filepath, std::vector<SimpleMapFeature>& features);
        static bool LoadCities(const std::string& filepath, std::vector<SimpleMapFeature>& features);

        // Test with your small France file first
        static bool LoadTestFrance(const std::string& filepath, SimpleMapFeature& france);

        // Convert to game coordinates (you can adjust these bounds)
        static Point LatLonToGame(double latitude, double longitude);
        static void GameToLatLon(const Point& game_pos, double& latitude, double& longitude);

    private:
        // JSON parsing helpers
        static SimpleMapFeature ParseFeature(const Json::Value& feature);
        static MapPolygon ParsePolygon(const Json::Value& geometry);
        static std::vector<Point> ParseCoordinateArray(const Json::Value& coordinates);
        static std::string ExtractProperty(const Json::Value& properties, const std::string& key, const std::string& default_val = "");
        static double ExtractNumericProperty(const Json::Value& properties, const std::string& key, double default_val = 0.0);

        // Coordinate conversion settings (adjust for your game world)
        static constexpr double EUROPE_MIN_LAT = 35.0;
        static constexpr double EUROPE_MAX_LAT = 72.0;
        static constexpr double EUROPE_MIN_LON = -15.0;
        static constexpr double EUROPE_MAX_LON = 45.0;
        static constexpr double GAME_WORLD_SIZE = 1000.0; // -500 to +500
    };


} // namespace game::map::loaders
