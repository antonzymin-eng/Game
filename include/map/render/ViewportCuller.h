 // ============================================================================
// ViewportCuller.h - Efficient Viewport Culling for Map Rendering
// Created: October 21, 2025
// Description: Frustum culling and spatial optimization for rendering only
//              visible provinces and features within the camera viewport
// ============================================================================

#pragma once

#include "map/ProvinceRenderComponent.h"
#include "core/ECS/EntityManager.h"
#include <vector>
#include <memory>

namespace game::map {
    
    // Use core::ecs::EntityID instead of types::EntityID
    using ::core::ecs::EntityID;

    // ========================================================================
    // Camera2D - Simple 2D camera for map navigation
    // ========================================================================
    struct Camera2D {
        Vector2 position;      // Camera center position
        float zoom = 1.0f;     // Zoom level (0.1 = 10x zoom out, 10.0 = 10x zoom in)
        float viewport_width = 1920.0f;
        float viewport_height = 1080.0f;
        
        // Get the viewport bounds in world space
        Rect GetViewportBounds() const {
            float half_width = (viewport_width / zoom) / 2.0f;
            float half_height = (viewport_height / zoom) / 2.0f;
            
            return Rect(
                position.x - half_width,
                position.y - half_height,
                position.x + half_width,
                position.y + half_height
            );
        }
        
        // Convert screen space to world space
        Vector2 ScreenToWorld(float screen_x, float screen_y) const {
            float half_width = (viewport_width / zoom) / 2.0f;
            float half_height = (viewport_height / zoom) / 2.0f;
            
            float normalized_x = (screen_x / viewport_width) * 2.0f - 1.0f;
            float normalized_y = (screen_y / viewport_height) * 2.0f - 1.0f;
            
            return Vector2(
                position.x + normalized_x * half_width,
                position.y + normalized_y * half_height
            );
        }
        
        // Convert world space to screen space
        Vector2 WorldToScreen(float world_x, float world_y) const {
            float half_width = (viewport_width / zoom) / 2.0f;
            float half_height = (viewport_height / zoom) / 2.0f;
            
            float normalized_x = (world_x - position.x) / half_width;
            float normalized_y = (world_y - position.y) / half_height;
            
            return Vector2(
                (normalized_x + 1.0f) * viewport_width / 2.0f,
                (normalized_y + 1.0f) * viewport_height / 2.0f
            );
        }
        
        // Pan camera by delta
        void Pan(float dx, float dy) {
            position.x += dx / zoom;
            position.y += dy / zoom;
        }
        
        // Zoom in/out around a point
        void ZoomAt(float world_x, float world_y, float zoom_delta) {
            float new_zoom = zoom * zoom_delta;
            new_zoom = std::max(0.1f, std::min(10.0f, new_zoom)); // Clamp zoom
            
            // Adjust position to zoom towards point
            float dx = world_x - position.x;
            float dy = world_y - position.y;
            
            position.x += dx * (1.0f - zoom / new_zoom);
            position.y += dy * (1.0f - zoom / new_zoom);
            
            zoom = new_zoom;
        }
    };

    // ========================================================================
    // ViewportCuller - Spatial culling and visibility management
    // ========================================================================
    class ViewportCuller {
    public:
        ViewportCuller();
        ~ViewportCuller() = default;
        
        // Update viewport bounds from camera
        void UpdateViewport(const Camera2D& camera);
        
        // Get current viewport bounds
        const Rect& GetViewportBounds() const { return viewport_bounds_; }
        
        // Test if a province is visible
        bool IsProvinceVisible(const ProvinceRenderComponent& province) const;
        
        // Test if a feature is visible at current LOD
        bool IsFeatureVisible(const FeatureRenderData& feature, int current_lod) const;
        
        // Test if a point is visible
        bool IsPointVisible(float x, float y) const;
        
        // Get all visible province entities (fast culling)
        std::vector<EntityID> GetVisibleProvinces(
            ::core::ecs::EntityManager& entity_manager
        ) const;
        
        // Get visible provinces with expanded frustum (for smooth transitions)
        std::vector<EntityID> GetVisibleProvincesExpanded(
            ::core::ecs::EntityManager& entity_manager,
            float expansion_factor = 1.2f  // 20% larger viewport for pre-loading
        ) const;
        
        // Update visibility flags on all provinces
        void UpdateProvinceVisibility(::core::ecs::EntityManager& entity_manager);
        
        // Statistics
        int GetVisibleProvinceCount() const { return visible_province_count_; }
        int GetTotalProvinceCount() const { return total_province_count_; }
        float GetCullingEfficiency() const {
            if (total_province_count_ == 0) return 0.0f;
            return 1.0f - (float)visible_province_count_ / (float)total_province_count_;
        }
        
    private:
        Rect viewport_bounds_;
        Rect expanded_viewport_bounds_;
        
        // Statistics
        int visible_province_count_ = 0;
        int total_province_count_ = 0;
        
        // Helper: Expand viewport for pre-loading
        Rect ExpandViewport(const Rect& viewport, float factor) const {
            float center_x = (viewport.min_x + viewport.max_x) / 2.0f;
            float center_y = (viewport.min_y + viewport.max_y) / 2.0f;
            float half_width = (viewport.max_x - viewport.min_x) / 2.0f * factor;
            float half_height = (viewport.max_y - viewport.min_y) / 2.0f * factor;
            
            return Rect(
                center_x - half_width,
                center_y - half_height,
                center_x + half_width,
                center_y + half_height
            );
        }
    };

} // namespace game::map
