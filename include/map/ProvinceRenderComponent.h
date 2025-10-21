// ============================================================================
// ProvinceRenderComponent.h - ECS Component for Province Rendering Data
// Created: October 21, 2025
// Description: Contains all visual/rendering data for a province including
//              boundaries, colors, features, and LOD-specific cached data
// ============================================================================

#pragma once

#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <vector>
#include <string>
#include <memory>

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
        
        Color() = default;
        Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
            : r(r_), g(g_), b(b_), a(a_) {}
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

    // ========================================================================
    // TerrainType - Province terrain classification
    // ========================================================================
    enum class TerrainType : uint8_t {
        PLAINS = 0,
        HILLS,
        MOUNTAINS,
        FOREST,
        DESERT,
        COAST,
        WETLAND,
        HIGHLANDS,
        UNKNOWN
    };

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
        
        // Rendering state
        bool is_visible = true;      // Is currently in viewport
        bool is_selected = false;    // Player has selected this province
        bool is_hovered = false;     // Mouse is over this province
        bool needs_update = false;   // Geometry/color needs refresh
        
        // Constructors
        ProvinceRenderComponent() = default;
        
        // Component interface
        std::unique_ptr<core::IComponent> Clone() const override {
            auto clone = std::make_unique<ProvinceRenderComponent>();
            clone->province_id = province_id;
            clone->name = name;
            clone->owner_realm_id = owner_realm_id;
            clone->fill_color = fill_color;
            clone->border_color = border_color;
            clone->terrain_type = terrain_type;
            clone->boundary_points = boundary_points;
            clone->center_position = center_position;
            clone->bounding_box = bounding_box;
            clone->boundary_lod0 = boundary_lod0;
            clone->boundary_lod1 = boundary_lod1;
            clone->boundary_lod2 = boundary_lod2;
            clone->features = features;
            clone->is_visible = is_visible;
            clone->is_selected = is_selected;
            clone->is_hovered = is_hovered;
            clone->needs_update = needs_update;
            return clone;
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
        
        // Get terrain type from string
        static TerrainType StringToTerrainType(const std::string& str) {
            if (str == "plains") return TerrainType::PLAINS;
            if (str == "hills") return TerrainType::HILLS;
            if (str == "mountains") return TerrainType::MOUNTAINS;
            if (str == "forest") return TerrainType::FOREST;
            if (str == "desert") return TerrainType::DESERT;
            if (str == "coast") return TerrainType::COAST;
            if (str == "wetland") return TerrainType::WETLAND;
            if (str == "highlands") return TerrainType::HIGHLANDS;
            return TerrainType::UNKNOWN;
        }
        
        // Get feature type from string
        static FeatureType StringToFeatureType(const std::string& str) {
            if (str == "city") return FeatureType::CITY;
            if (str == "town") return FeatureType::TOWN;
            if (str == "village") return FeatureType::VILLAGE;
            if (str == "fortress") return FeatureType::FORTRESS;
            if (str == "port") return FeatureType::PORT;
            if (str == "mountain") return FeatureType::MOUNTAIN;
            if (str == "forest") return FeatureType::FOREST;
            if (str == "river") return FeatureType::RIVER;
            if (str == "lake") return FeatureType::LAKE;
            if (str == "hills") return FeatureType::HILLS;
            if (str == "wetland") return FeatureType::WETLAND;
            if (str == "coast") return FeatureType::COAST;
            if (str == "road") return FeatureType::ROAD;
            return FeatureType::UNKNOWN;
        }
    };

} // namespace game::map
