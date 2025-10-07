#pragma once

#include <string>
#include <vector>
#include <memory>

namespace game {
    namespace map {

        struct SimpleProvince {
            std::string name;
            std::vector<std::pair<double, double>> boundary_points;
            double center_x, center_y;
        };

        class MapDataLoader {
        public:
            static bool LoadCountries(const std::string& file_path, std::vector<SimpleProvince>& provinces);
            static bool LoadProvinces(const std::string& file_path, std::vector<SimpleProvince>& provinces);

        private:
            static std::pair<double, double> CalculateCenter(const std::vector<std::pair<double, double>>& points);
        };

    } // namespace map
} // namespace game#pragma once
