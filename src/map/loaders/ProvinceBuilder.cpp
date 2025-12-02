// ============================================================================
// ProvinceBuilder.cpp - Province Builder Implementation
// Mechanica Imperii
// ============================================================================

#include "map/loaders/ProvinceBuilder.h"
#include "map/ProvinceRenderComponent.h"
#include "map/ProvinceGeometry.h"
#include "map/GeographicUtils.h"
#include <iostream>

namespace game::map::loaders {

    ProvinceBuilder::ProvinceBuilder(::core::ecs::ComponentAccessManager& access_manager)
        : m_access_manager(access_manager)
    {
    }

    ProvinceBuilder::~ProvinceBuilder() {}

    ::core::ecs::EntityID ProvinceBuilder::BuildProvince(const ProvinceData& data) {
        // Stub: Create province entity with components
        // This would typically create an entity and add ProvinceRenderComponent
        // and other relevant components
        return ::core::ecs::EntityID{0, 0};
    }

    std::vector<::core::ecs::EntityID> ProvinceBuilder::BuildProvinces(const std::vector<ProvinceData>& provinces) {
        std::vector<::core::ecs::EntityID> entities;
        for (const auto& province : provinces) {
            entities.push_back(BuildProvince(province));
        }
        return entities;
    }

    void ProvinceBuilder::LinkProvinces(std::vector<ProvinceData>& provinces) {
        if (provinces.empty()) {
            m_last_error = "No provinces to link";
            return;
        }

        std::cout << "LinkProvinces: Computing adjacency for " << provinces.size() << " provinces..." << std::endl;

        // Clear existing neighbor data
        for (auto& province : provinces) {
            province.neighbors.clear();
        }

        // Tolerance for considering provinces as neighbors (in map coordinates)
        const double tolerance = 1.0;  // Adjust based on your map scale

        // Track progress for large province sets
        size_t total_comparisons = (provinces.size() * (provinces.size() - 1)) / 2;
        size_t completed_comparisons = 0;
        size_t adjacencies_found = 0;
        size_t last_reported_percent = 0;

        // Compare each pair of provinces
        for (size_t i = 0; i < provinces.size(); ++i) {
            ProvinceData& province1 = provinces[i];

            for (size_t j = i + 1; j < provinces.size(); ++j) {
                ProvinceData& province2 = provinces[j];

                // Quick bounding box check before detailed geometry check
                if (!province1.bounds.Intersects(province2.bounds)) {
                    ++completed_comparisons;
                    continue;
                }

                // Check if provinces share a border using proper geometry
                if (ProvinceGeometry::AreNeighbors(province1.boundary, province2.boundary, tolerance)) {
                    // Add bidirectional neighbor relationship
                    province1.neighbors.push_back(province2.id);
                    province2.neighbors.push_back(province1.id);
                    ++adjacencies_found;

                    // Optional: Calculate border length for future use
                    auto shared_border = ProvinceGeometry::GetSharedBorder(
                        province1.boundary, province2.boundary, tolerance);

                    // Debug output for first few adjacencies
                    if (adjacencies_found <= 5) {
                        std::cout << "  Found adjacency: " << province1.name
                                  << " (" << province1.id << ") <-> "
                                  << province2.name << " (" << province2.id << ")"
                                  << " [" << shared_border.size() << " shared points]" << std::endl;
                    }
                }

                ++completed_comparisons;

                // Report progress for large datasets (every 10%)
                if (total_comparisons > 1000) {
                    size_t percent = (completed_comparisons * 100) / total_comparisons;
                    if (percent >= last_reported_percent + 10) {
                        std::cout << "  Progress: " << percent << "% ("
                                  << completed_comparisons << "/" << total_comparisons
                                  << " comparisons, " << adjacencies_found << " adjacencies found)" << std::endl;
                        last_reported_percent = percent;
                    }
                }
            }
        }

        std::cout << "LinkProvinces: Complete! Found " << adjacencies_found
                  << " adjacencies across " << provinces.size() << " provinces." << std::endl;

        // Validate results
        size_t provinces_with_no_neighbors = 0;
        size_t max_neighbors = 0;
        for (const auto& province : provinces) {
            if (province.neighbors.empty()) {
                ++provinces_with_no_neighbors;
            }
            if (province.neighbors.size() > max_neighbors) {
                max_neighbors = province.neighbors.size();
            }
        }

        if (provinces_with_no_neighbors > 0) {
            std::cout << "  Warning: " << provinces_with_no_neighbors
                      << " province(s) have no neighbors (isolated)" << std::endl;
        }

        std::cout << "  Max neighbors for any province: " << max_neighbors << std::endl;

        m_last_error.clear();
    }

} // namespace game::map::loaders
