// ============================================================================
// GeoJSONLoader.cpp - GeoJSON Loading Implementation
// Mechanica Imperii
// ============================================================================

#include "map/loaders/GeoJSONLoader.h"
#include "map/GeographicUtils.h"
#include <fstream>

#include "core/logging/Logger.h"

namespace game::map::loaders {

    // ========================================================================
    // GeoJSONLoader Implementation
    // ========================================================================

    bool GeoJSONLoader::LoadCountries(const std::string& filepath,
                                     std::vector<SimpleMapFeature>& features) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            CORE_STREAM_ERROR("GeoJSONLoader") << "Failed to open GeoJSON file: " << filepath << std::endl;
            return false;
        }

        try {
            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(file, root)) {
                CORE_STREAM_ERROR("GeoJSONLoader") << "JSON parse error: " << reader.getFormattedErrorMessages() << std::endl;
                return false;
            }

            if (!root.isMember("features") || !root["features"].isArray()) {
                CORE_STREAM_ERROR("GeoJSONLoader") << "Invalid GeoJSON: missing features array" << std::endl;
                return false;
            }

            for (const auto& feature_json : root["features"]) {
                SimpleMapFeature feature = ParseFeature(feature_json);
                feature.type = SimpleMapFeature::COUNTRY;
                features.push_back(feature);
            }

            return true;
        } catch (const std::exception& e) {
            CORE_STREAM_ERROR("GeoJSONLoader") << "Exception loading GeoJSON: " << e.what() << std::endl;
            return false;
        }
    }

    bool GeoJSONLoader::LoadProvinces(const std::string& filepath,
                                     std::vector<SimpleMapFeature>& features) {
        // Stub: Similar to LoadCountries but for provinces
        return LoadCountries(filepath, features);
    }

    bool GeoJSONLoader::LoadCities(const std::string& filepath,
                                  std::vector<SimpleMapFeature>& features) {
        // Stub: Load city point features
        return false;
    }

    bool GeoJSONLoader::LoadTestFrance(const std::string& filepath, SimpleMapFeature& france) {
        std::vector<SimpleMapFeature> features;
        if (LoadCountries(filepath, features) && !features.empty()) {
            france = features[0];
            return true;
        }
        return false;
    }

    Point GeoJSONLoader::LatLonToGame(double latitude, double longitude) {
        // Simple mercator-style projection
        double x = (longitude - EUROPE_MIN_LON) / (EUROPE_MAX_LON - EUROPE_MIN_LON) * GAME_WORLD_SIZE - GAME_WORLD_SIZE / 2.0;
        double y = (latitude - EUROPE_MIN_LAT) / (EUROPE_MAX_LAT - EUROPE_MIN_LAT) * GAME_WORLD_SIZE - GAME_WORLD_SIZE / 2.0;
        return Point(x, y);
    }

    void GeoJSONLoader::GameToLatLon(const Point& game_pos, double& latitude, double& longitude) {
        longitude = (game_pos.longitude + GAME_WORLD_SIZE / 2.0) / GAME_WORLD_SIZE * (EUROPE_MAX_LON - EUROPE_MIN_LON) + EUROPE_MIN_LON;
        latitude = (game_pos.latitude + GAME_WORLD_SIZE / 2.0) / GAME_WORLD_SIZE * (EUROPE_MAX_LAT - EUROPE_MIN_LAT) + EUROPE_MIN_LAT;
    }

    SimpleMapFeature GeoJSONLoader::ParseFeature(const Json::Value& feature) {
        SimpleMapFeature result;
        result.name = ExtractProperty(feature["properties"], "name");
        result.iso_code = ExtractProperty(feature["properties"], "iso_a3");
        result.continent = ExtractProperty(feature["properties"], "continent");
        result.region = ExtractProperty(feature["properties"], "region_un");
        result.area_km2 = ExtractNumericProperty(feature["properties"], "area_km2");
        result.population = static_cast<int>(ExtractNumericProperty(feature["properties"], "pop_est"));

        if (feature.isMember("geometry")) {
            result.polygons.push_back(ParsePolygon(feature["geometry"]));
        }

        return result;
    }

    MapPolygon GeoJSONLoader::ParsePolygon(const Json::Value& geometry) {
        MapPolygon polygon;

        if (!geometry.isMember("type") || !geometry.isMember("coordinates")) {
            return polygon;
        }

        std::string geom_type = geometry["type"].asString();
        const Json::Value& coords = geometry["coordinates"];

        if (geom_type == "Polygon") {
            if (coords.isArray() && !coords.empty()) {
                polygon.rings.push_back(ParseCoordinateArray(coords[0]));
            }
        } else if (geom_type == "MultiPolygon") {
            if (coords.isArray() && !coords.empty()) {
                polygon.rings.push_back(ParseCoordinateArray(coords[0][0]));
            }
        }

        return polygon;
    }

    std::vector<Point> GeoJSONLoader::ParseCoordinateArray(const Json::Value& coordinates) {
        std::vector<Point> points;

        if (!coordinates.isArray()) return points;

        for (const auto& coord : coordinates) {
            if (coord.isArray() && coord.size() >= 2) {
                double lon = coord[0].asDouble();
                double lat = coord[1].asDouble();
                points.push_back(LatLonToGame(lat, lon));
            }
        }

        return points;
    }

    std::string GeoJSONLoader::ExtractProperty(const Json::Value& properties,
                                              const std::string& key,
                                              const std::string& default_val) {
        if (properties.isMember(key) && properties[key].isString()) {
            return properties[key].asString();
        }
        return default_val;
    }

    double GeoJSONLoader::ExtractNumericProperty(const Json::Value& properties,
                                                const std::string& key,
                                                double default_val) {
        if (properties.isMember(key)) {
            if (properties[key].isDouble()) {
                return properties[key].asDouble();
            } else if (properties[key].isInt()) {
                return static_cast<double>(properties[key].asInt());
            }
        }
        return default_val;
    }

} // namespace game::map::loaders
