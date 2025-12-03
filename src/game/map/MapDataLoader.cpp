// ============================================================================
// MapDataLoader.cpp - JSON Province Data Loader with ECS Integration
// Created: October 21, 2025
// Description: Loads province data from JSON files and creates ECS entities
//              with ProvinceRenderComponent and ProvinceComponent
// ============================================================================

#include "map/MapDataLoader.h"
#include "map/ProvinceRenderComponent.h"
// NOTE: AI::ProvinceComponent (game/components/ProvinceComponent.h) is deprecated
// Use game::province::ProvinceDataComponent instead
#include "core/ECS/EntityManager.h"
#include "core/ECS/ComponentAccessManager.h"
#include "utils/PlatformCompat.h"
#include "map/MapData.h"
#include "map/GeographicUtils.h"
#include "map/loaders/ProvinceBuilder.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include "core/logging/Logger.h"

namespace game::map {

    // ========================================================================
    // Helper: Generate deterministic color from string (for new format)
    // ========================================================================
    static Color GenerateColorFromString(const std::string& name) {
        // Use FNV-1a hash for deterministic color generation
        uint32_t hash = 2166136261u;
        for (char c : name) {
            hash ^= static_cast<uint32_t>(c);
            hash *= 16777619u;
        }

        // Generate distinct, pleasing colors
        // Avoid pure grey by ensuring RGB values are different
        // Keep colors vibrant (values 80-220 range)
        uint8_t r = 80 + ((hash >> 0) & 0xFF) % 140;
        uint8_t g = 80 + ((hash >> 8) & 0xFF) % 140;
        uint8_t b = 80 + ((hash >> 16) & 0xFF) % 140;

        // Boost one channel to ensure color isn't too grey
        uint8_t max_channel = hash % 3;
        if (max_channel == 0) r = std::min(255, r + 50);
        else if (max_channel == 1) g = std::min(255, g + 50);
        else b = std::min(255, b + 50);

        return Color(r, g, b, 255);
    }

    // ========================================================================
    // Helper: Calculate realm colors from realm ID and optional name
    // ========================================================================
    static Color GetRealmColor(uint32_t realm_id, const Json::Value& realms_data, const std::string& realm_name = "") {
        // Try lookup in realms array first (legacy format with numeric IDs)
        for (const auto& realm : realms_data) {
            if (realm["id"].asUInt() == realm_id) {
                auto color_obj = realm["color"];
                return Color(
                    color_obj["r"].asUInt(),
                    color_obj["g"].asUInt(),
                    color_obj["b"].asUInt(),
                    255
                );
            }
        }

        // If no realms array (new format), generate color from realm name
        if (!realm_name.empty()) {
            return GenerateColorFromString(realm_name);
        }

        // Final fallback: generate color from numeric ID
        // This handles case where realm_id is hashed but name not provided
        return Color(
            100 + ((realm_id * 73) % 156),
            100 + ((realm_id * 151) % 156),
            100 + ((realm_id * 211) % 156),
            255
        );
    }

    // ========================================================================
    // Helper: Simplify polygon using Douglas-Peucker algorithm
    // ========================================================================
    static float PerpendicularDistance(const Vector2& point, const Vector2& line_start, const Vector2& line_end) {
        float dx = line_end.x - line_start.x;
        float dy = line_end.y - line_start.y;
        
        if (dx == 0 && dy == 0) {
            dx = point.x - line_start.x;
            dy = point.y - line_start.y;
            return std::sqrt(dx * dx + dy * dy);
        }
        
        float numerator = std::abs(dy * point.x - dx * point.y + line_end.x * line_start.y - line_end.y * line_start.x);
        float denominator = std::sqrt(dx * dx + dy * dy);
        return numerator / denominator;
    }

    static void SimplifyPolylineRecursive(
        const std::vector<Vector2>& points,
        int start_idx, int end_idx,
        float epsilon,
        std::vector<bool>& keep_mask
    ) {
        if (end_idx <= start_idx + 1) return;
        
        // Find the point with maximum distance
        float max_distance = 0.0f;
        int max_index = start_idx;
        
        for (int i = start_idx + 1; i < end_idx; ++i) {
            float distance = PerpendicularDistance(points[i], points[start_idx], points[end_idx]);
            if (distance > max_distance) {
                max_distance = distance;
                max_index = i;
            }
        }
        
        // If max distance is greater than epsilon, keep that point and recurse
        if (max_distance > epsilon) {
            keep_mask[max_index] = true;
            SimplifyPolylineRecursive(points, start_idx, max_index, epsilon, keep_mask);
            SimplifyPolylineRecursive(points, max_index, end_idx, epsilon, keep_mask);
        }
    }

    static std::vector<Vector2> SimplifyBoundary(const std::vector<Vector2>& points, float epsilon) {
        if (points.size() < 3) return points;
        
        std::vector<bool> keep_mask(points.size(), false);
        keep_mask[0] = true;
        keep_mask[points.size() - 1] = true;
        
        SimplifyPolylineRecursive(points, 0, points.size() - 1, epsilon, keep_mask);
        
        std::vector<Vector2> simplified;
        for (size_t i = 0; i < points.size(); ++i) {
            if (keep_mask[i]) {
                simplified.push_back(points[i]);
            }
        }
        
        return simplified;
    }

    // ========================================================================
    // MapDataLoader::LoadProvinces - Main loading function
    // ========================================================================
    bool MapDataLoader::LoadProvinces(
        const std::string& file_path,
        std::vector<SimpleProvince>& provinces
    ) {
        // Legacy function - kept for compatibility
        // The new ECS-based loading is done via LoadProvincesECS
        
        std::ifstream file(file_path);
        if (!file.is_open()) {
            CORE_STREAM_ERROR("MapDataLoader") << "Failed to open province data file: " << file_path;
            return false;
        }
        
        try {
            Json::Value data;
            Json::Reader reader;
            
            if (!reader.parse(file, data)) {
                CORE_STREAM_ERROR("MapDataLoader") << "JSON parsing error: " << reader.getFormattedErrorMessages();
                return false;
            }
            
            if (!data.isMember("provinces") || !data["provinces"].isArray()) {
                CORE_STREAM_ERROR("MapDataLoader") << "Invalid province data format: missing 'provinces' array";
                return false;
            }
            
            for (const auto& province_json : data["provinces"]) {
                SimpleProvince province;
                province.name = province_json["name"].asString();
                
                // Load boundary points
                for (const auto& point : province_json["boundary"]) {
                    double x = point["x"].asDouble();
                    double y = point["y"].asDouble();
                    province.boundary_points.emplace_back(x, y);
                }
                
                // Load center position
                auto center = province_json["center"];
                province.center_x = center["x"].asDouble();
                province.center_y = center["y"].asDouble();
                
                provinces.push_back(province);
            }
            
            CORE_STREAM_INFO("MapDataLoader") << "Loaded " << provinces.size() << " provinces from " << file_path;
            return true;
            
        } catch (const std::exception& e) {
            CORE_STREAM_ERROR("MapDataLoader") << "JSON parsing error: " << e.what();
            return false;
        }
    }

    // ========================================================================
    // MapDataLoader::LoadProvincesECS - ECS-based province loading
    // ========================================================================
    bool MapDataLoader::LoadProvincesECS(
        const std::string& file_path,
        ::core::ecs::EntityManager& entity_manager
    ) {
        CORE_STREAM_INFO("MapDataLoader") << "Loading provinces from " << file_path << "...";
        
        std::ifstream file(file_path);
        if (!file.is_open()) {
            CORE_STREAM_ERROR("MapDataLoader") << "ERROR: Failed to open province data file: " << file_path;
            return false;
        }
        
        try {
            Json::Value data;
            Json::Reader reader;
            
            if (!reader.parse(file, data)) {
                CORE_STREAM_ERROR("MapDataLoader") << "ERROR: JSON parsing failed: " << reader.getFormattedErrorMessages();
                return false;
            }

            // Support both formats: direct provinces array or nested under map_region
            Json::Value provinces_data;
            Json::Value realms_data;

            if (data.isMember("map_region")) {
                // New format: {"map_region": {"provinces": [...], ...}}
                CORE_STREAM_INFO("MapDataLoader") << "Detected map_region format";
                const auto& map_region = data["map_region"];

                if (!map_region.isMember("provinces") || !map_region["provinces"].isArray()) {
                    CORE_STREAM_ERROR("MapDataLoader") << "ERROR: Invalid map_region format: missing 'provinces' array";
                    return false;
                }

                provinces_data = map_region["provinces"];
                realms_data = map_region.isMember("realms") ? map_region["realms"] : Json::Value(Json::arrayValue);
            } else if (data.isMember("provinces") && data["provinces"].isArray()) {
                // Old format: {"provinces": [...], "realms": [...]}
                CORE_STREAM_INFO("MapDataLoader") << "Detected legacy provinces format";
                provinces_data = data["provinces"];
                realms_data = data.isMember("realms") ? data["realms"] : Json::Value(Json::arrayValue);
            } else {
                CORE_STREAM_ERROR("MapDataLoader") << "ERROR: Invalid province data format: missing 'provinces' array";
                return false;
            }

            if (!realms_data.isArray() || realms_data.empty()) {
                CORE_STREAM_INFO("MapDataLoader") << "WARNING: No realms data found in JSON, using default colors";
            }

            const auto& provinces_json = provinces_data;
            const auto& realms_json = realms_data;

            int loaded_count = 0;

            // Storage for province data and entity mappings
            std::vector<ProvinceData> province_data_list;
            province_data_list.reserve(provinces_json.size());

            // Map province ID -> entity ID for efficient neighbor storage
            std::unordered_map<uint32_t, ::core::ecs::EntityID> province_id_to_entity;

            // Track seen province IDs to detect duplicates
            std::unordered_set<uint32_t> seen_province_ids;

            for (const auto& province_json : provinces_json) {
                // Validate province ID uniqueness
                uint32_t province_id = province_json["id"].asUInt();
                if (seen_province_ids.count(province_id)) {
                    CORE_STREAM_ERROR("MapDataLoader") << "ERROR: Duplicate province ID: " << province_id;
                    return false;
                }
                seen_province_ids.insert(province_id);

                // Create new entity for this province
                ::core::ecs::EntityID entity_id = entity_manager.CreateEntity();

                // Create and configure ProvinceRenderComponent
                auto render_component = std::make_unique<ProvinceRenderComponent>();

                // Basic province info
                render_component->province_id = province_id;
                render_component->name = province_json["name"].asString();

                // Handle owner_realm: can be either number (legacy) or string (new format)
                std::string realm_name_str;  // Store realm name for color generation
                if (province_json["owner_realm"].isUInt()) {
                    render_component->owner_realm_id = province_json["owner_realm"].asUInt();
                } else if (province_json["owner_realm"].isString()) {
                    // Store realm name and hash to generate a unique ID
                    realm_name_str = province_json["owner_realm"].asString();
                    std::hash<std::string> hasher;
                    render_component->owner_realm_id = static_cast<uint32_t>(hasher(realm_name_str) % 10000);
                } else {
                    render_component->owner_realm_id = 0; // Default/neutral
                }

                // Terrain type
                std::string terrain_str = province_json["terrain_type"].asString();
                render_component->terrain_type = ProvinceRenderComponent::StringToTerrainType(terrain_str);

                // Load boundary points
                for (const auto& point : province_json["boundary"]) {
                    float x = point["x"].asFloat();
                    float y = point["y"].asFloat();
                    render_component->boundary_points.emplace_back(x, y);
                }

                // Edge case validation: check for empty or invalid boundaries
                if (render_component->boundary_points.size() < 3) {
                    CORE_STREAM_ERROR("MapDataLoader") << "ERROR: Province '" << render_component->name
                                                       << "' (ID: " << province_id << ") has invalid boundary ("
                                                       << render_component->boundary_points.size() << " points, need >= 3)";
                    return false;
                }

                // Load center position
                auto center = province_json["center"];
                render_component->center_position.x = center["x"].asFloat();
                render_component->center_position.y = center["y"].asFloat();

                // Calculate bounding box
                render_component->CalculateBoundingBox();

                // Build ProvinceData for adjacency computation
                ProvinceData province_data;
                province_data.id = province_id;
                province_data.name = render_component->name;
                province_data.owner_id = render_component->owner_realm_id;
                province_data.center.x = render_component->center_position.x;
                province_data.center.y = render_component->center_position.y;

                // Convert boundary points from Vector2 to Coordinate
                province_data.boundary.reserve(render_component->boundary_points.size());
                for (const auto& point : render_component->boundary_points) {
                    province_data.boundary.emplace_back(point.x, point.y);
                }

                // Calculate bounding box for ProvinceData
                province_data.bounds = GeoUtils::CalculateBoundingBox(province_data.boundary);

                province_data_list.push_back(province_data);

                // Store entity ID mapping for later neighbor storage
                province_id_to_entity[province_id] = entity_id;

                // Set colors based on realm (pass realm name for new format color generation)
                render_component->fill_color = GetRealmColor(render_component->owner_realm_id, realms_json, realm_name_str);
                render_component->border_color = Color(50, 50, 50); // Dark grey borders

                // Generate LOD-simplified boundaries
                // LOD 0: Very simplified (5-10 points max for state boundaries)
                render_component->boundary_lod0 = SimplifyBoundary(render_component->boundary_points, 30.0f);

                // LOD 1: Simplified (20-30% of original points)
                render_component->boundary_lod1 = SimplifyBoundary(render_component->boundary_points, 10.0f);

                // LOD 2: Medium detail (50-60% of original points)
                render_component->boundary_lod2 = SimplifyBoundary(render_component->boundary_points, 5.0f);

                // Load features (cities, terrain features, etc.)
                if (province_json.isMember("features") && province_json["features"].isArray()) {
                    for (const auto& feature_json : province_json["features"]) {
                        FeatureRenderData feature;

                        std::string type_str = feature_json["type"].asString();
                        feature.type = ProvinceRenderComponent::StringToFeatureType(type_str);
                        feature.name = feature_json["name"].asString();

                        auto pos = feature_json["position"];
                        feature.position.x = pos["x"].asFloat();
                        feature.position.y = pos["y"].asFloat();

                        feature.lod_min = feature_json["lod_min"].asInt();

                        // Population for cities
                        if (feature_json.isMember("population")) {
                            feature.population = feature_json["population"].asUInt();
                            // Scale city icon size by population
                            feature.size = 1.0f + (feature.population / 50000.0f);
                        }

                        render_component->features.push_back(feature);
                    }
                }

                // Store center position and owner before moving render_component
                float center_x = render_component->center_position.x;
                float center_y = render_component->center_position.y;
                uint32_t owner_id = render_component->owner_realm_id;

                // Add ProvinceRenderComponent to entity (copy construct)
                auto added_render = entity_manager.AddComponent<ProvinceRenderComponent>(entity_id, *render_component);

                // NOTE: AI::ProvinceComponent has been deprecated and removed.
                // AI systems should now use game::province::ProvinceDataComponent
                // which is created by the ProvinceSystem when provinces are loaded.

                loaded_count++;

                CORE_STREAM_INFO("MapDataLoader") << "  Loaded province: " << render_component->name
                          << " (ID: " << render_component->province_id << ")"
                          << " - " << render_component->boundary_points.size() << " boundary points, "
                          << render_component->features.size() << " features";
            }

            // ====================================================================
            // Compute province adjacency (neighbors)
            // ====================================================================
            size_t total_neighbors = 0;  // Declare outside for success message

            if (!province_data_list.empty()) {
                CORE_STREAM_INFO("MapDataLoader") << "\nComputing province adjacency...";

                // Create ProvinceBuilder to compute neighbors
                auto component_access = entity_manager.GetComponentAccessManager();
                loaders::ProvinceBuilder province_builder(*component_access);

                // Compute neighbors using proper geometry with configurable tolerance
                // Tolerance of 1.0 works well for the current map coordinate system
                province_builder.LinkProvinces(province_data_list, 1.0);

                if (!province_builder.GetLastError().empty()) {
                    CORE_STREAM_WARN("MapDataLoader") << "Warning during adjacency computation: "
                                                      << province_builder.GetLastError();
                }

                CORE_STREAM_INFO("MapDataLoader") << "Province adjacency computation complete!";

                // Log neighbor statistics
                size_t provinces_with_neighbors = 0;
                for (const auto& prov_data : province_data_list) {
                    if (!prov_data.neighbors.empty()) {
                        ++provinces_with_neighbors;
                        total_neighbors += prov_data.neighbors.size();
                    }
                }

                double avg_neighbors = provinces_with_neighbors > 0 ?
                    static_cast<double>(total_neighbors) / provinces_with_neighbors : 0.0;

                CORE_STREAM_INFO("MapDataLoader") << "  Total adjacencies: " << total_neighbors / 2
                                                  << " (each counted bidirectionally)";
                CORE_STREAM_INFO("MapDataLoader") << "  Provinces with neighbors: " << provinces_with_neighbors
                                                  << " / " << province_data_list.size();
                CORE_STREAM_INFO("MapDataLoader") << "  Average neighbors per province: " << avg_neighbors;

                // ====================================================================
                // Store computed neighbors back into ProvinceRenderComponent
                // ====================================================================
                CORE_STREAM_INFO("MapDataLoader") << "Storing neighbor data in ECS components...";

                // Use saved entity ID mappings for O(1) lookup (no entity search needed!)
                size_t updated_count = 0;
                size_t invalid_neighbor_count = 0;

                for (const auto& prov_data : province_data_list) {
                    // Find entity for this province using saved mapping
                    auto entity_it = province_id_to_entity.find(prov_data.id);
                    if (entity_it == province_id_to_entity.end()) {
                        CORE_STREAM_ERROR("MapDataLoader") << "ERROR: No entity found for province ID " << prov_data.id;
                        continue;
                    }

                    auto render_comp = entity_manager.GetComponent<ProvinceRenderComponent>(entity_it->second);
                    if (!render_comp) {
                        CORE_STREAM_ERROR("MapDataLoader") << "ERROR: No ProvinceRenderComponent for entity";
                        continue;
                    }

                    // Validate all neighbor IDs exist before storing
                    for (uint32_t neighbor_id : prov_data.neighbors) {
                        if (province_id_to_entity.find(neighbor_id) == province_id_to_entity.end()) {
                            CORE_STREAM_WARN("MapDataLoader") << "Warning: Province '" << prov_data.name
                                                              << "' has invalid neighbor ID: " << neighbor_id;
                            ++invalid_neighbor_count;
                        }
                    }

                    // Copy neighbors from ProvinceData to ProvinceRenderComponent
                    render_comp->neighbor_province_ids = prov_data.neighbors;
                    ++updated_count;
                }

                CORE_STREAM_INFO("MapDataLoader") << "Successfully stored neighbor data for "
                                                  << updated_count << " provinces.";

                if (invalid_neighbor_count > 0) {
                    CORE_STREAM_WARN("MapDataLoader") << "Warning: Found " << invalid_neighbor_count
                                                      << " invalid neighbor references";
                }

                // ====================================================================
                // Validate neighbor relationships (bidirectional check)
                // ====================================================================
                CORE_STREAM_INFO("MapDataLoader") << "Validating neighbor relationships...";

                size_t missing_bidirectional = 0;
                for (const auto& prov_data : province_data_list) {
                    for (uint32_t neighbor_id : prov_data.neighbors) {
                        // Find neighbor's data
                        auto neighbor_it = std::find_if(province_data_list.begin(), province_data_list.end(),
                            [neighbor_id](const ProvinceData& p) { return p.id == neighbor_id; });

                        if (neighbor_it != province_data_list.end()) {
                            // Check if neighbor has this province in its neighbor list
                            auto& neighbor_neighbors = neighbor_it->neighbors;
                            if (std::find(neighbor_neighbors.begin(), neighbor_neighbors.end(), prov_data.id)
                                == neighbor_neighbors.end()) {
                                CORE_STREAM_WARN("MapDataLoader") << "Warning: Non-bidirectional adjacency: "
                                                                  << prov_data.name << " -> " << neighbor_it->name
                                                                  << " but not reverse";
                                ++missing_bidirectional;
                            }
                        }
                    }
                }

                if (missing_bidirectional == 0) {
                    CORE_STREAM_INFO("MapDataLoader") << "✓ All neighbor relationships are bidirectional";
                } else {
                    CORE_STREAM_ERROR("MapDataLoader") << "ERROR: Found " << missing_bidirectional
                                                       << " non-bidirectional neighbor relationships!";
                }
            }
            
            // Print LOD statistics
            if (loaded_count > 0) {
                CORE_STREAM_INFO("MapDataLoader") << "\nLOD Simplification Statistics:";
                auto entities = entity_manager.GetEntitiesWithComponent<ProvinceRenderComponent>();
                if (!entities.empty()) {
                    auto render = entity_manager.GetComponent<ProvinceRenderComponent>(entities[0]);
                    CORE_STREAM_INFO("MapDataLoader") << "  LOD 0 (Strategic): " << render->boundary_lod0.size() << " points";
                    CORE_STREAM_INFO("MapDataLoader") << "  LOD 1 (Regional):  " << render->boundary_lod1.size() << " points";
                    CORE_STREAM_INFO("MapDataLoader") << "  LOD 2 (Province):  " << render->boundary_lod2.size() << " points";
                    CORE_STREAM_INFO("MapDataLoader") << "  LOD 3-4 (Detail):  " << render->boundary_points.size() << " points";
                }
            }

            // Success message AFTER all processing is complete
            CORE_STREAM_INFO("MapDataLoader") << "\n✓ SUCCESS: Map loading complete!";
            CORE_STREAM_INFO("MapDataLoader") << "  - Loaded " << loaded_count << " provinces";
            CORE_STREAM_INFO("MapDataLoader") << "  - Computed " << (total_neighbors / 2) << " adjacency relationships";
            CORE_STREAM_INFO("MapDataLoader") << "  - All data validated and stored in ECS";

            return true;
            
        } catch (const std::exception& e) {
            CORE_STREAM_ERROR("MapDataLoader") << "ERROR: Exception during province loading: " << e.what();
            return false;
        }
    }

    // ========================================================================
    // MapDataLoader::CalculateCenter - Helper function
    // ========================================================================
    std::pair<double, double> MapDataLoader::CalculateCenter(
        const std::vector<std::pair<double, double>>& points
    ) {
        if (points.empty()) return {0.0, 0.0};
        
        double sum_x = 0.0, sum_y = 0.0;
        for (const auto& [x, y] : points) {
            sum_x += x;
            sum_y += y;
        }
        
        return {sum_x / points.size(), sum_y / points.size()};
    }

} // namespace game::map
