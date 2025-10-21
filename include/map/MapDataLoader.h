#pragma once

#include <string>
#include <vector>
#include <memory>

// Forward declarations
namespace core::ecs {
    class EntityManager;
    class ComponentAccessManager;
}

namespace game {
    namespace map {

        struct SimpleProvince {
            std::string name;
            std::vector<std::pair<double, double>> boundary_points;
            double center_x, center_y;
        };

        class MapDataLoader {
        public:
            // Legacy loading (compatibility)
            static bool LoadCountries(const std::string& file_path, std::vector<SimpleProvince>& provinces);
            static bool LoadProvinces(const std::string& file_path, std::vector<SimpleProvince>& provinces);
            
            // ECS-based loading (NEW - recommended for MVP)
            static bool LoadProvincesECS(
                const std::string& file_path,
                core::ecs::EntityManager& entity_manager,
                core::ecs::ComponentAccessManager& access_manager
            );

        private:
            static std::pair<double, double> CalculateCenter(const std::vector<std::pair<double, double>>& points);
        };

    } // namespace map
} // namespace game
