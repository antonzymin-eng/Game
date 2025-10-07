// ============================================================================
// src/game/map/loaders/GeoJSONLoader.cpp - Implementation
// ============================================================================

#include "GeoJSONLoader.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace game::map::loaders {

    bool GeoJSONLoader::LoadTestFrance(const std::string& filepath, SimpleMapFeature& france) {
        std::vector<SimpleMapFeature> features;
        if (!LoadCountries(filepath, features)) {
            return false;
        }

        if (features.empty()) {
            std::cerr << "No features found in " << filepath << std::endl;
            return false;
        }

        france = features[0]; // Should be France from your test file
        std::cout << "Loaded France: " << france.name << " with " << france.polygons.size() << " polygons" << std::endl;
        return true;
    }

    bool GeoJSONLoader::LoadCountries(const std::string& filepath, std::vector<SimpleMapFeature>& features) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filepath << std::endl;
            return false;
        }

        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(file, root)) {
            std::cerr << "Failed to parse JSON: " << reader.getFormattedErrorMessages() << std::endl;
            return false;
        }

        if (!root.isMember("features") || !root["features"].isArray()) {
            std::cerr << "Invalid GeoJSON format: missing features array" << std::endl;
            return false;
        }

        const Json::Value& json_features = root["features"];
        features.clear();
        features.reserve(json_features.size());

        for (const Json::Value& feature : json_features) {
            try {
                SimpleMapFeature map_feature = ParseFeature(feature);
                if (!map_feature.polygons.empty()) {
                    features.push_back(std::move(map_feature));
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error parsing feature: " << e.what() << std::endl;
                continue; // Skip this feature, continue with others
            }
        }

        std::cout << "Successfully loaded " << features.size() << " features from " << filepath << std::endl;
        return true;
    }

    bool GeoJSONLoader::LoadProvinces(const std::string& filepath, std::vector<SimpleMapFeature>& features) {
        // Same as LoadCountries but mark as PROVINCE type
        if (!LoadCountries(filepath, features)) {
            return false;
        }

        // Mark all as provinces
        for (auto& feature : features) {
            feature.type = SimpleMapFeature::PROVINCE;
        }

        return true;
    }

    bool GeoJSONLoader::LoadCities(const std::string& filepath, std::vector<SimpleMapFeature>& features) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open cities file: " << filepath << std::endl;
            return false;
        }

        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(file, root)) {
            std::cerr << "Failed to parse cities JSON: " << reader.getFormattedErrorMessages() << std::endl;
            return false;
        }

        const Json::Value& json_features = root["features"];
        features.clear();
        features.reserve(json_features.size());

        for (const Json::Value& feature : json_features) {
            if (!feature.isMember("geometry") || !feature.isMember("properties")) {
                continue;
            }

            const Json::Value& geometry = feature["geometry"];
            if (geometry["type"].asString() != "Point") {
                continue; // Skip non-point features for cities
            }

            SimpleMapFeature city_feature;
            city_feature.type = SimpleMapFeature::CITY;
            city_feature.name = ExtractProperty(feature["properties"], "NAME", "Unknown City");
            city_feature.population = static_cast<int>(ExtractNumericProperty(feature["properties"], "POP_MAX", 0));

            // Extract point coordinates
            const Json::Value& coordinates = geometry["coordinates"];
            if (coordinates.size() >= 2) {
                double longitude = coordinates[0].asDouble();
                double latitude = coordinates[1].asDouble();
                city_feature.center_point = LatLonToGame(latitude, longitude);
            }

            features.push_back(std::move(city_feature));
        }

        std::cout << "Successfully loaded " << features.size() << " cities from " << filepath << std::endl;
        return true;
    }

    SimpleMapFeature GeoJSONLoader::ParseFeature(const Json::Value& feature) {
        if (!feature.isMember("geometry") || !feature.isMember("properties")) {
            throw std::runtime_error("Feature missing geometry or properties");
        }

        SimpleMapFeature map_feature;
        map_feature.type = SimpleMapFeature::COUNTRY; // Default to country

        const Json::Value& properties = feature["properties"];
        const Json::Value& geometry = feature["geometry"];

        // Extract basic properties
        map_feature.name = ExtractProperty(properties, "NAME", "Unknown");
        map_feature.iso_code = ExtractProperty(properties, "ISO_A2", "");
        map_feature.continent = ExtractProperty(properties, "CONTINENT", "");
        map_feature.region = ExtractProperty(properties, "REGION_UN", "");
        map_feature.area_km2 = ExtractNumericProperty(properties, "POP_EST", 0);

        // Parse geometry
        std::string geom_type = geometry["type"].asString();
        if (geom_type == "Polygon" || geom_type == "MultiPolygon") {
            MapPolygon polygon = ParsePolygon(geometry);
            if (!polygon.rings.empty()) {
                polygon.name = map_feature.name;
                polygon.country_code = map_feature.iso_code;
                map_feature.polygons.push_back(std::move(polygon));

                // Calculate center point from first polygon
                if (!map_feature.polygons[0].rings.empty() && !map_feature.polygons[0].rings[0].empty()) {
                    map_feature.center_point = CalculateCentroid(map_feature.polygons[0].rings[0]);
                }
            }
        }

        return map_feature;
    }

    MapPolygon GeoJSONLoader::ParsePolygon(const Json::Value& geometry) {
        MapPolygon polygon;
        std::string geom_type = geometry["type"].asString();

        if (geom_type == "Polygon") {
            const Json::Value& coordinates = geometry["coordinates"];
            for (const Json::Value& ring : coordinates) {
                std::vector<Point> ring_points = ParseCoordinateArray(ring);
                if (!ring_points.empty()) {
                    polygon.rings.push_back(std::move(ring_points));
                }
            }
        }
        else if (geom_type == "MultiPolygon") {
            // Use the largest polygon for simplicity
            const Json::Value& polygons = geometry["coordinates"];
            size_t largest_polygon = 0;
            size_t max_points = 0;

            for (size_t i = 0; i < polygons.size(); ++i) {
                if (polygons[static_cast<int>(i)].size() > 0) {
                    size_t point_count = polygons[static_cast<int>(i)][0].size();
                    if (point_count > max_points) {
                        max_points = point_count;
                        largest_polygon = i;
                    }
                }
            }

            if (max_points > 0) {
                const Json::Value& largest_poly = polygons[static_cast<int>(largest_polygon)];
                for (const Json::Value& ring : largest_poly) {
                    std::vector<Point> ring_points = ParseCoordinateArray(ring);
                    if (!ring_points.empty()) {
                        polygon.rings.push_back(std::move(ring_points));
                    }
                }
            }
        }

        return polygon;
    }

    std::vector<Point> GeoJSONLoader::ParseCoordinateArray(const Json::Value& coordinates) {
        std::vector<Point> points;
        points.reserve(coordinates.size());

        for (const Json::Value& coord : coordinates) {
            if (coord.size() >= 2) {
                double longitude = coord[0].asDouble();
                double latitude = coord[1].asDouble();
                points.push_back(LatLonToGame(latitude, longitude));
            }
        }

        return points;
    }

    Point GeoJSONLoader::LatLonToGame(double latitude, double longitude) {
        // Normalize to 0-1 within European bounds
        double x_norm = (longitude - EUROPE_MIN_LON) / (EUROPE_MAX_LON - EUROPE_MIN_LON);
        double y_norm = (latitude - EUROPE_MIN_LAT) / (EUROPE_MAX_LAT - EUROPE_MIN_LAT);

        // Clamp to valid range
        x_norm = std::clamp(x_norm, 0.0, 1.0);
        y_norm = std::clamp(y_norm, 0.0, 1.0);

        // Scale to game world coordinates (-500 to +500)
        double game_x = -GAME_WORLD_SIZE / 2 + x_norm * GAME_WORLD_SIZE;
        double game_y = -GAME_WORLD_SIZE / 2 + y_norm * GAME_WORLD_SIZE;

        return Point(game_x, game_y);
    }

    void GeoJSONLoader::GameToLatLon(const Point& game_pos, double& latitude, double& longitude) {
        // Normalize game coordinates to 0-1
        double x_norm = (game_pos.longitude + GAME_WORLD_SIZE / 2) / GAME_WORLD_SIZE;
        double y_norm = (game_pos.latitude + GAME_WORLD_SIZE / 2) / GAME_WORLD_SIZE;

        // Scale to real world coordinates
        longitude = EUROPE_MIN_LON + x_norm * (EUROPE_MAX_LON - EUROPE_MIN_LON);
        latitude = EUROPE_MIN_LAT + y_norm * (EUROPE_MAX_LAT - EUROPE_MIN_LAT);
    }

    std::string GeoJSONLoader::ExtractProperty(const Json::Value& properties, const std::string& key, const std::string& default_val) {
        if (properties.isMember(key) && properties[key].isString()) {
            return properties[key].asString();
        }
        return default_val;
    }

    double GeoJSONLoader::ExtractNumericProperty(const Json::Value& properties, const std::string& key, double default_val) {
        if (properties.isMember(key) && properties[key].isNumeric()) {
            return properties[key].asDouble();
        }
        return default_val;
    }

    Point GeoJSONLoader::CalculateCentroid(const std::vector<Point>& points) {
        if (points.empty()) return Point(0, 0);

        double sum_x = 0, sum_y = 0;
        for (const auto& point : points) {
            sum_x += point.longitude;
            sum_y += point.latitude;
        }

        return Point(sum_x / points.size(), sum_y / points.size());
    }

} // namespace game::map::loaders