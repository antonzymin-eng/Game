#include "map/MapDataLoader.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include "core/logging/Logger.h"

namespace game {
    namespace map {

        bool MapDataLoader::LoadCountries(const std::string& file_path, std::vector<SimpleProvince>& provinces) {
            std::ifstream file(file_path);
            if (!file.is_open()) {
                CORE_STREAM_INFO("MapDataLoader") << "Failed to open file: " << file_path;
                return false;
            }

            // For now, just read the file and count features
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

            // Simple test: count how many "Feature" entries we have
            size_t feature_count = 0;
            size_t pos = 0;
            while ((pos = content.find("\"type\": \"Feature\"", pos)) != std::string::npos) {
                feature_count++;
                pos += 18;
            }

            CORE_STREAM_INFO("MapDataLoader") << "Found " << feature_count << " countries in GeoJSON file";

            // Create dummy provinces for testing
            for (size_t i = 0; i < std::min(feature_count, size_t(10)); ++i) {
                SimpleProvince province;
                province.name = "Country_" + std::to_string(i);
                province.center_x = -100.0 + i * 20.0;
                province.center_y = 100.0 + i * 10.0;
                provinces.push_back(province);
            }

            return true;
        }

        bool MapDataLoader::LoadProvinces(const std::string& file_path, std::vector<SimpleProvince>& provinces) {
            return LoadCountries(file_path, provinces); // Same logic for now
        }

        std::pair<double, double> MapDataLoader::CalculateCenter(const std::vector<std::pair<double, double>>& points) {
            if (points.empty()) return { 0.0, 0.0 };

            double sum_x = 0.0, sum_y = 0.0;
            for (const auto& point : points) {
                sum_x += point.first;
                sum_y += point.second;
            }

            return { sum_x / points.size(), sum_y / points.size() };
        }

    } // namespace map
} // namespace game
