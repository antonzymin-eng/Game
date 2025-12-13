// ============================================================================
// ProvinceRenderComponent.h - ECS Component for Province Rendering Data
// Created: October 21, 2025
// Description: Contains all visual/rendering data for a province including
//              boundaries, colors, features, and LOD-specific cached data
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include "map/MapData.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <string_view>

namespace game::map {

    // ========================================================================
    // Vector2 - Simple 2D position
    // ========================================================================
    struct Vector2 {
        float x = 0.0f;
        float y = 0.0f;
        
        Vector2() = default;
        Vector2(float x_, float y_) : x(x_), y(y_) {}
    };

    // ========================================================================
    // Color - RGBA color for rendering
    // ========================================================================
    struct Color {
        uint8_t r = 255;
        uint8_t g = 255;
        uint8_t b = 255;
        uint8_t a = 255;

        constexpr Color() = default;
        constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
            : r(r_), g(g_), b(b_), a(a_) {}

        constexpr bool operator==(const Color& other) const {
            return r == other.r && g == other.g && b == other.b && a == other.a;
        }

        constexpr bool operator!=(const Color& other) const {
            return !(*this == other);
        }
    };

    // ========================================================================
    // Rect - Axis-aligned bounding box
    // ========================================================================
    struct Rect {
        float min_x = 0.0f;
        float min_y = 0.0f;
        float max_x = 0.0f;
        float max_y = 0.0f;
        
        Rect() = default;
        Rect(float min_x_, float min_y_, float max_x_, float max_y_)
            : min_x(min_x_), min_y(min_y_), max_x(max_x_), max_y(max_y_) {}
        
        float GetWidth() const { return max_x - min_x; }
        float GetHeight() const { return max_y - min_y; }
        Vector2 GetCenter() const { return Vector2((min_x + max_x) / 2.0f, (min_y + max_y) / 2.0f); }
        
        bool Contains(float x, float y) const {
            return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
        }
        
        bool Intersects(const Rect& other) const {
            return !(max_x < other.min_x || min_x > other.max_x ||
                     max_y < other.min_y || min_y > other.max_y);
        }
    };

    // TerrainType is now imported from MapData.h to avoid duplication

    // ========================================================================
    // FeatureType - Visual features on the map
    // ========================================================================
    enum class FeatureType : uint8_t {
        CITY = 0,
        TOWN,
        VILLAGE,
        FORTRESS,
        PORT,
        MOUNTAIN,
        FOREST,
        RIVER,
        LAKE,
        HILLS,
        WETLAND,
        COAST,
        ROAD,
        UNKNOWN
    };

    // ========================================================================
    // FeatureRenderData - Individual feature on a province
    // ========================================================================
    struct FeatureRenderData {
        FeatureType type = FeatureType::UNKNOWN;
        std::string name;
        Vector2 position;
        int lod_min = 2;        // Minimum LOD level to show this feature
        int lod_max = 4;        // Maximum LOD level to show this feature

        // Feature-specific data
        uint32_t population = 0;  // For cities/towns
        float size = 1.0f;        // For scaling icons

        FeatureRenderData() = default;
    };

    // ========================================================================
    // ProvinceNeighborData - Detailed neighbor information
    // ========================================================================
    struct ProvinceNeighborData {
        uint32_t neighbor_id = 0;
        double border_length = 0.0;  // Length of shared border (for influence weights)

        ProvinceNeighborData() = default;
        ProvinceNeighborData(uint32_t id, double length = 0.0)
            : neighbor_id(id), border_length(length) {}
    };

    // ========================================================================
    // ProvinceRenderComponent - ECS Component for Province Rendering
    // ========================================================================
    class ProvinceRenderComponent : public core::Component<ProvinceRenderComponent> {
    public:
        // Province identification
        uint32_t province_id = 0;
        std::string name;
        
        // Ownership and visual style
        uint32_t owner_realm_id = 0;
        Color fill_color;
        Color border_color;
        TerrainType terrain_type = TerrainType::PLAINS;
        
        // Geometry data
        std::vector<Vector2> boundary_points;  // Full resolution boundary
        Vector2 center_position;
        Rect bounding_box;
        
        // LOD-specific simplified boundaries (cached)
        std::vector<Vector2> boundary_lod0;  // Very simplified (state level)
        std::vector<Vector2> boundary_lod1;  // Simplified
        std::vector<Vector2> boundary_lod2;  // Medium detail (default)
        // boundary_points used for LOD 3-4
        
        // Features within this province
        std::vector<FeatureRenderData> features;

        // Adjacency data (neighboring provinces with border lengths)
        // Note: Use GetNeighborIds() to extract simple ID list if needed
        std::vector<ProvinceNeighborData> detailed_neighbors;

        // Rendering state
        bool is_visible = true;      // Is currently in viewport
        bool is_selected = false;    // Player has selected this province
        bool is_hovered = false;     // Mouse is over this province
        bool needs_update = false;   // Geometry/color needs refresh
        
        // Constructors
        ProvinceRenderComponent() = default;
        
        // Component interface - uses copy constructor to avoid field-by-field duplication
        std::unique_ptr<core::IComponent> Clone() const override {
            return std::make_unique<ProvinceRenderComponent>(*this);
        }
        
        // Utility methods
        void CalculateBoundingBox() {
            if (boundary_points.empty()) return;
            
            bounding_box.min_x = boundary_points[0].x;
            bounding_box.max_x = boundary_points[0].x;
            bounding_box.min_y = boundary_points[0].y;
            bounding_box.max_y = boundary_points[0].y;
            
            for (const auto& point : boundary_points) {
                if (point.x < bounding_box.min_x) bounding_box.min_x = point.x;
                if (point.x > bounding_box.max_x) bounding_box.max_x = point.x;
                if (point.y < bounding_box.min_y) bounding_box.min_y = point.y;
                if (point.y > bounding_box.max_y) bounding_box.max_y = point.y;
            }
        }
        
        void CalculateCenter() {
            if (boundary_points.empty()) return;
            
            float sum_x = 0.0f, sum_y = 0.0f;
            for (const auto& point : boundary_points) {
                sum_x += point.x;
                sum_y += point.y;
            }
            center_position.x = sum_x / boundary_points.size();
            center_position.y = sum_y / boundary_points.size();
        }
        
        // Check if a point is within the bounding box (fast check)
        bool ContainsPoint(float x, float y) const {
            return bounding_box.Contains(x, y);
        }

        // Extract neighbor IDs as simple vector (for compatibility)
        std::vector<uint32_t> GetNeighborIds() const {
            std::vector<uint32_t> ids;
            ids.reserve(detailed_neighbors.size());
            for (const auto& neighbor : detailed_neighbors) {
                ids.push_back(neighbor.neighbor_id);
            }
            return ids;
        }
        
        // Get terrain type from string - O(1) hash map lookup
        static TerrainType StringToTerrainType(std::string_view str) {
            static const std::unordered_map<std::string_view, TerrainType> terrain_map = {
                {"plains", TerrainType::PLAINS},
                {"hills", TerrainType::HILLS},
                {"mountains", TerrainType::MOUNTAINS},
                {"forest", TerrainType::FOREST},
                {"desert", TerrainType::DESERT},
                {"coast", TerrainType::COAST},
                {"wetland", TerrainType::WETLAND},
                {"highlands", TerrainType::HIGHLANDS}
            };

            auto it = terrain_map.find(str);
            return (it != terrain_map.end()) ? it->second : TerrainType::UNKNOWN;
        }

        // Get feature type from string - O(1) hash map lookup
        static FeatureType StringToFeatureType(std::string_view str) {
            static const std::unordered_map<std::string_view, FeatureType> feature_map = {
                {"city", FeatureType::CITY},
                {"town", FeatureType::TOWN},
                {"village", FeatureType::VILLAGE},
                {"fortress", FeatureType::FORTRESS},
                {"port", FeatureType::PORT},
                {"mountain", FeatureType::MOUNTAIN},
                {"forest", FeatureType::FOREST},
                {"river", FeatureType::RIVER},
                {"lake", FeatureType::LAKE},
                {"hills", FeatureType::HILLS},
                {"wetland", FeatureType::WETLAND},
                {"coast", FeatureType::COAST},
                {"road", FeatureType::ROAD}
            };

            auto it = feature_map.find(str);
            return (it != feature_map.end()) ? it->second : FeatureType::UNKNOWN;
        }
    };

} // namespace game::map
