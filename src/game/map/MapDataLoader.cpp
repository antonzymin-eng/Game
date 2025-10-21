// ============================================================================
// MapDataLoader.cpp - JSON Province Data Loader with ECS Integration
// Created: October 21, 2025
// Description: Loads province data from JSON files and creates ECS entities
//              with ProvinceRenderComponent and ProvinceComponent
// ============================================================================

#include "map/MapDataLoader.h"
#include "map/ProvinceRenderComponent.h"
#include "game/components/ProvinceComponent.h"
#include "core/ECS/EntityManager.h"
#include "core/ECS/ComponentAccessManager.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// Simple JSON parsing (using nlohmann/json if available, or manual parsing)
// For now, we'll use a simple manual JSON parser for the MVP
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace game::map {

    // ========================================================================
    // Helper: Calculate realm colors from realm ID
    // ========================================================================
    static Color GetRealmColor(uint32_t realm_id, const json& realms_data) {
        for (const auto& realm : realms_data) {
            if (realm["id"].get<uint32_t>() == realm_id) {
                auto color_obj = realm["color"];
                return Color(
                    color_obj["r"].get<uint8_t>(),
                    color_obj["g"].get<uint8_t>(),
                    color_obj["b"].get<uint8_t>(),
                    255
                );
            }
        }
        // Default color if realm not found
        return Color(150, 150, 150);
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
            std::cerr << "Failed to open province data file: " << file_path << std::endl;
            return false;
        }
        
        try {
            json data;
            file >> data;
            
            if (!data.contains("provinces") || !data["provinces"].is_array()) {
                std::cerr << "Invalid province data format: missing 'provinces' array" << std::endl;
                return false;
            }
            
            for (const auto& province_json : data["provinces"]) {
                SimpleProvince province;
                province.name = province_json["name"].get<std::string>();
                
                // Load boundary points
                for (const auto& point : province_json["boundary"]) {
                    double x = point["x"].get<double>();
                    double y = point["y"].get<double>();
                    province.boundary_points.emplace_back(x, y);
                }
                
                // Load center position
                auto center = province_json["center"];
                province.center_x = center["x"].get<double>();
                province.center_y = center["y"].get<double>();
                
                provinces.push_back(province);
            }
            
            std::cout << "Loaded " << provinces.size() << " provinces from " << file_path << std::endl;
            return true;
            
        } catch (const json::exception& e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
            return false;
        }
    }

    // ========================================================================
    // MapDataLoader::LoadProvincesECS - ECS-based province loading
    // ========================================================================
    bool MapDataLoader::LoadProvincesECS(
        const std::string& file_path,
        core::ecs::EntityManager& entity_manager,
        core::ecs::ComponentAccessManager& access_manager
    ) {
        std::cout << "Loading provinces from " << file_path << "..." << std::endl;
        
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "ERROR: Failed to open province data file: " << file_path << std::endl;
            return false;
        }
        
        try {
            json data;
            file >> data;
            
            if (!data.contains("provinces") || !data["provinces"].is_array()) {
                std::cerr << "ERROR: Invalid province data format: missing 'provinces' array" << std::endl;
                return false;
            }
            
            if (!data.contains("realms") || !data["realms"].is_array()) {
                std::cerr << "WARNING: No realms data found in JSON" << std::endl;
            }
            
            const auto& provinces_json = data["provinces"];
            const auto& realms_json = data.contains("realms") ? data["realms"] : json::array();
            
            int loaded_count = 0;
            
            for (const auto& province_json : provinces_json) {
                // Create new entity for this province
                types::EntityID entity_id = entity_manager.CreateEntity();
                
                // Create and configure ProvinceRenderComponent
                auto render_component = std::make_unique<ProvinceRenderComponent>();
                
                // Basic province info
                render_component->province_id = province_json["id"].get<uint32_t>();
                render_component->name = province_json["name"].get<std::string>();
                render_component->owner_realm_id = province_json["owner_realm"].get<uint32_t>();
                
                // Terrain type
                std::string terrain_str = province_json["terrain_type"].get<std::string>();
                render_component->terrain_type = ProvinceRenderComponent::StringToTerrainType(terrain_str);
                
                // Load boundary points
                for (const auto& point : province_json["boundary"]) {
                    float x = point["x"].get<float>();
                    float y = point["y"].get<float>();
                    render_component->boundary_points.emplace_back(x, y);
                }
                
                // Load center position
                auto center = province_json["center"];
                render_component->center_position.x = center["x"].get<float>();
                render_component->center_position.y = center["y"].get<float>();
                
                // Calculate bounding box
                render_component->CalculateBoundingBox();
                
                // Set colors based on realm
                render_component->fill_color = GetRealmColor(render_component->owner_realm_id, realms_json);
                render_component->border_color = Color(50, 50, 50); // Dark grey borders
                
                // Generate LOD-simplified boundaries
                // LOD 0: Very simplified (5-10 points max for state boundaries)
                render_component->boundary_lod0 = SimplifyBoundary(render_component->boundary_points, 30.0f);
                
                // LOD 1: Simplified (20-30% of original points)
                render_component->boundary_lod1 = SimplifyBoundary(render_component->boundary_points, 10.0f);
                
                // LOD 2: Medium detail (50-60% of original points)
                render_component->boundary_lod2 = SimplifyBoundary(render_component->boundary_points, 5.0f);
                
                // Load features (cities, terrain features, etc.)
                if (province_json.contains("features") && province_json["features"].is_array()) {
                    for (const auto& feature_json : province_json["features"]) {
                        FeatureRenderData feature;
                        
                        std::string type_str = feature_json["type"].get<std::string>();
                        feature.type = ProvinceRenderComponent::StringToFeatureType(type_str);
                        feature.name = feature_json["name"].get<std::string>();
                        
                        auto pos = feature_json["position"];
                        feature.position.x = pos["x"].get<float>();
                        feature.position.y = pos["y"].get<float>();
                        
                        feature.lod_min = feature_json["lod_min"].get<int>();
                        
                        // Population for cities
                        if (feature_json.contains("population")) {
                            feature.population = feature_json["population"].get<uint32_t>();
                            // Scale city icon size by population
                            feature.size = 1.0f + (feature.population / 50000.0f);
                        }
                        
                        render_component->features.push_back(feature);
                    }
                }
                
                // Add ProvinceRenderComponent to entity
                entity_manager.AddComponent(entity_id, std::move(render_component));
                
                // Also create AI::ProvinceComponent for compatibility with existing systems
                auto ai_component = std::make_unique<AI::ProvinceComponent>();
                ai_component->SetPosition(render_component->center_position.x, render_component->center_position.y);
                ai_component->SetOwnerNationId(render_component->owner_realm_id);
                entity_manager.AddComponent(entity_id, std::move(ai_component));
                
                loaded_count++;
                
                std::cout << "  Loaded province: " << render_component->name 
                          << " (ID: " << render_component->province_id << ")"
                          << " - " << render_component->boundary_points.size() << " boundary points, "
                          << render_component->features.size() << " features" << std::endl;
            }
            
            std::cout << "SUCCESS: Loaded " << loaded_count << " provinces into ECS" << std::endl;
            
            // Print LOD statistics
            if (loaded_count > 0) {
                std::cout << "\nLOD Simplification Statistics:" << std::endl;
                auto entities = entity_manager.GetEntitiesWithComponent<ProvinceRenderComponent>();
                if (!entities.empty()) {
                    auto render = entity_manager.GetComponent<ProvinceRenderComponent>(entities[0]);
                    std::cout << "  LOD 0 (Strategic): " << render->boundary_lod0.size() << " points" << std::endl;
                    std::cout << "  LOD 1 (Regional):  " << render->boundary_lod1.size() << " points" << std::endl;
                    std::cout << "  LOD 2 (Province):  " << render->boundary_lod2.size() << " points" << std::endl;
                    std::cout << "  LOD 3-4 (Detail):  " << render->boundary_points.size() << " points" << std::endl;
                }
            }
            
            return true;
            
        } catch (const json::exception& e) {
            std::cerr << "ERROR: JSON parsing error: " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "ERROR: Exception during province loading: " << e.what() << std::endl;
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
