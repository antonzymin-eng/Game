// ============================================================================
// ProvinceBuilder.cpp - Province Builder Implementation
// Mechanica Imperii
// ============================================================================

#include "map/loaders/ProvinceBuilder.h"
#include "map/ProvinceRenderComponent.h"
#include "map/ProvinceGeometry.h"
#include "map/GeographicUtils.h"
#include "core/logging/Logger.h"
#include <cmath>
#include <algorithm>

namespace game::map::loaders {

    // Adaptive tolerance percentage: 0.1% of average province diagonal
    constexpr double ADAPTIVE_TOLERANCE_PERCENTAGE = 0.001;

    ProvinceBuilder::~ProvinceBuilder() {}

    ::core::ecs::EntityID ProvinceBuilder::BuildProvince(
        const ProvinceData& data,
        ::core::ecs::EntityManager& entity_manager) {

        // Validate input data
        if (data.boundary.size() < 3) {
            m_last_error = "Province '" + data.name + "' has invalid boundary (< 3 points)";
            CORE_STREAM_ERROR("ProvinceBuilder") << m_last_error;
            return ::core::ecs::EntityID{0, 0};
        }

        // EXCEPTION SAFETY: Create and populate component BEFORE creating entity
        // This ensures we don't leak entities if component creation/population throws
        auto render_component = std::make_unique<ProvinceRenderComponent>();

        // Basic province info
        render_component->province_id = data.id;
        render_component->name = data.name;
        render_component->owner_realm_id = data.owner_id;
        render_component->terrain_type = data.terrain;

        // Convert boundary from Coordinate (double) to Vector2 (float)
        // Note: Precision loss is acceptable - GPU rendering uses float32
        render_component->boundary_points.reserve(data.boundary.size());
        for (const auto& coord : data.boundary) {
            render_component->boundary_points.emplace_back(
                static_cast<float>(coord.x),
                static_cast<float>(coord.y));
        }

        // Set center position
        render_component->center_position.x = static_cast<float>(data.center.x);
        render_component->center_position.y = static_cast<float>(data.center.y);

        // Copy bounding box from ProvinceData (already calculated upstream)
        // This avoids redundant O(n) recalculation
        render_component->bounding_box.min_x = static_cast<float>(data.bounds.min_x);
        render_component->bounding_box.min_y = static_cast<float>(data.bounds.min_y);
        render_component->bounding_box.max_x = static_cast<float>(data.bounds.max_x);
        render_component->bounding_box.max_y = static_cast<float>(data.bounds.max_y);

        // Set default colors (grey for neutral, can be overridden later)
        render_component->fill_color = Color(180, 180, 180, 255);
        render_component->border_color = Color(50, 50, 50, 255);

        // Add neighbor data if available
        render_component->neighbor_province_ids = data.neighbors;
        for (const auto& neighbor : data.detailed_neighbors) {
            render_component->detailed_neighbors.emplace_back(
                neighbor.neighbor_id,
                neighbor.border_length);
        }

        // Now create entity - if this succeeds, we're committed
        // Component is fully populated, so AddComponent should succeed
        std::string entity_name = data.name.empty()
            ? "Province_" + std::to_string(data.id)
            : "Province_" + data.name;
        ::core::ecs::EntityID entity_id = entity_manager.CreateEntity(entity_name);

        // Add component to entity
        entity_manager.AddComponent(entity_id, std::move(render_component));

        m_last_error.clear();  // Success
        return entity_id;
    }

    std::vector<::core::ecs::EntityID> ProvinceBuilder::BuildProvinces(
        const std::vector<ProvinceData>& provinces,
        ::core::ecs::EntityManager& entity_manager) {

        std::vector<::core::ecs::EntityID> entities;
        entities.reserve(provinces.size());

        size_t success_count = 0;
        size_t failure_count = 0;

        for (const auto& province : provinces) {
            auto entity_id = BuildProvince(province, entity_manager);
            if (entity_id.IsValid()) {
                entities.push_back(entity_id);
                ++success_count;
            } else {
                ++failure_count;
                CORE_STREAM_WARN("ProvinceBuilder") << "Failed to build province: "
                                                    << province.name << " (ID: " << province.id << ")";
            }
        }

        // Update error state consistently with BuildProvince
        if (failure_count > 0) {
            m_last_error = "Batch build: " + std::to_string(success_count) + " succeeded, "
                         + std::to_string(failure_count) + " failed";
        } else {
            m_last_error.clear();  // All succeeded
        }

        CORE_STREAM_INFO("ProvinceBuilder") << "Built " << success_count << " provinces"
                                            << (failure_count > 0 ?
                                                " (" + std::to_string(failure_count) + " failed)" : "");

        return entities;
    }

    void ProvinceBuilder::LinkProvinces(std::vector<ProvinceData>& provinces, double tolerance) {
        if (provinces.empty()) {
            m_last_error = "No provinces to link";
            return;
        }

        // Adaptive tolerance calculation based on median province size
        double adaptive_tolerance = tolerance;
        if (tolerance <= 0.0) {
            // Calculate bounding box diagonal for all provinces
            std::vector<double> diagonals;
            diagonals.reserve(provinces.size());

            for (const auto& province : provinces) {
                double width = province.bounds.max_x - province.bounds.min_x;
                double height = province.bounds.max_y - province.bounds.min_y;
                double diagonal = std::sqrt(width * width + height * height);
                diagonals.push_back(diagonal);
            }

            // Use median instead of average for robustness against outliers
            size_t n = diagonals.size();
            double median_diagonal;

            if (n % 2 == 0) {
                // Even number: average of two middle elements for true median
                std::nth_element(diagonals.begin(),
                               diagonals.begin() + n / 2,
                               diagonals.end());
                double upper = diagonals[n / 2];

                std::nth_element(diagonals.begin(),
                               diagonals.begin() + n / 2 - 1,
                               diagonals.begin() + n / 2);
                double lower = diagonals[n / 2 - 1];

                median_diagonal = (lower + upper) / 2.0;
            } else {
                // Odd number: single middle element
                std::nth_element(diagonals.begin(),
                               diagonals.begin() + n / 2,
                               diagonals.end());
                median_diagonal = diagonals[n / 2];
            }

            // Use configurable percentage of median province diagonal as tolerance
            adaptive_tolerance = median_diagonal * ADAPTIVE_TOLERANCE_PERCENTAGE;

            CORE_STREAM_INFO("ProvinceBuilder") << "Adaptive tolerance calculated: " << adaptive_tolerance
                                               << " (based on median province diagonal: " << median_diagonal << ")";
        }

        CORE_STREAM_INFO("ProvinceBuilder") << "Computing adjacency for " << provinces.size()
                                           << " provinces (tolerance: " << adaptive_tolerance << ")...";

        // Clear existing neighbor data
        for (auto& province : provinces) {
            province.neighbors.clear();
        }

        // Track progress for large province sets
        size_t total_comparisons = (provinces.size() * (provinces.size() - 1)) / 2;
        size_t completed_comparisons = 0;
        size_t adjacencies_found = 0;
        size_t last_reported_percent = 0;

        // Compare each pair of provinces with error handling
        for (size_t i = 0; i < provinces.size(); ++i) {
            ProvinceData& province1 = provinces[i];

            for (size_t j = i + 1; j < provinces.size(); ++j) {
                ProvinceData& province2 = provinces[j];

                // Quick bounding box check before detailed geometry check
                if (!province1.bounds.Intersects(province2.bounds)) {
                    ++completed_comparisons;
                    continue;
                }

                try {
                    // Check adjacency and compute border length in a single pass (2x faster!)
                    auto result = ProvinceGeometry::CheckAdjacency(
                        province1.boundary, province2.boundary, adaptive_tolerance);

                    if (result.are_neighbors) {
                        // Add bidirectional neighbor relationship (simple list)
                        province1.neighbors.push_back(province2.id);
                        province2.neighbors.push_back(province1.id);

                        // Add detailed neighbor data with border length
                        province1.detailed_neighbors.push_back({province2.id, result.border_length});
                        province2.detailed_neighbors.push_back({province1.id, result.border_length});

                        ++adjacencies_found;

                        // Debug output for first few adjacencies
                        if (adjacencies_found <= 5) {
                            CORE_STREAM_INFO("ProvinceBuilder") << "  Found adjacency: " << province1.name
                                                              << " (" << province1.id << ") <-> "
                                                              << province2.name << " (" << province2.id << ")"
                                                              << " [border length: " << result.border_length << "]";
                        }
                    }
                } catch (const std::exception& e) {
                    CORE_STREAM_WARN("ProvinceBuilder") << "Error checking adjacency between "
                                                        << province1.name << " and " << province2.name
                                                        << ": " << e.what();
                }

                ++completed_comparisons;

                // Report progress for large datasets (every 10%)
                if (total_comparisons > 100) {  // Lowered threshold from 1000 to 100
                    size_t percent = (completed_comparisons * 100) / total_comparisons;
                    if (percent >= last_reported_percent + 10) {
                        CORE_STREAM_INFO("ProvinceBuilder") << "  Progress: " << percent << "% ("
                                                           << completed_comparisons << "/" << total_comparisons
                                                           << " comparisons, " << adjacencies_found << " adjacencies found)";
                        last_reported_percent = percent;
                    }
                }
            }
        }

        CORE_STREAM_INFO("ProvinceBuilder") << "Complete! Found " << adjacencies_found
                                           << " adjacencies across " << provinces.size() << " provinces.";

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
            CORE_STREAM_WARN("ProvinceBuilder") << "Warning: " << provinces_with_no_neighbors
                                                << " province(s) have no neighbors (isolated)";
        }

        CORE_STREAM_INFO("ProvinceBuilder") << "  Max neighbors for any province: " << max_neighbors;

        m_last_error.clear();
    }

} // namespace game::map::loaders
