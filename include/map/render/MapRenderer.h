 // ============================================================================
// MapRenderer.h - Main Map Rendering System with LOD Support
// Created: October 21, 2025
// Description: Renders the game map with multiple LOD levels, camera controls,
//              and efficient viewport culling
// ============================================================================

#pragma once

#include "map/ProvinceRenderComponent.h"
#include "map/render/ViewportCuller.h"
#include "core/ECS/EntityManager.h"

#include <memory>
#include <string>

// Forward declarations
struct ImDrawList;
struct ImFont;

namespace game::map {

    // Forward declaration for LOD 4 terrain renderer
    class TacticalTerrainRenderer;

    // Use core::ecs::EntityID
    using ::core::ecs::EntityID;

    // ========================================================================
    // LODLevel - Level of Detail enumeration
    // ========================================================================
    enum class LODLevel : int {
        STRATEGIC = 0,   // 0-20% zoom: States/realms only
        REGIONAL = 1,    // 20-40% zoom: States + Provinces
        PROVINCIAL = 2,  // 40-60% zoom: Provinces + Major cities/features (DEFAULT)
        LOCAL = 3,       // 60-80% zoom: All details + roads + armies
        TACTICAL = 4     // 80-100% zoom: Terrain rendering + buildings + units
    };

    // ========================================================================
    // MapRenderer - Main rendering class
    // ========================================================================
    class MapRenderer {
    public:
        MapRenderer(
            ::core::ecs::EntityManager& entity_manager
        );
        ~MapRenderer() = default;

        // Initialize renderer
        bool Initialize();
        
        // Main render function (call every frame)
        void Render();
        
        // Camera controls
        void PanCamera(float dx, float dy);
        void ZoomCamera(float delta);
        void ZoomCameraAt(float world_x, float world_y, float delta);
        void SetCameraPosition(float x, float y);
        void SetCameraZoom(float zoom);
        
        // Input handling
        void HandleInput();
        void HandleMouseClick(float screen_x, float screen_y);
        void HandleMouseMove(float screen_x, float screen_y);
        
        // Province selection
        EntityID GetProvinceAtPoint(float world_x, float world_y);
        void SelectProvince(EntityID province_id);
        void ClearSelection();
        EntityID GetSelectedProvince() const { return selected_province_; }
        
        // Getters
        Camera2D& GetCamera() { return camera_; }
        const Camera2D& GetCamera() const { return camera_; }
        LODLevel GetCurrentLOD() const { return current_lod_; }
        ViewportCuller& GetCuller() { return culler_; }
        
        // Settings
        void SetViewportSize(float width, float height);
        void SetRenderBorders(bool render) { render_borders_ = render; }
        void SetRenderNames(bool render) { render_names_ = render; }
        void SetRenderFeatures(bool render) { render_features_ = render; }

        // LOD 4 Terrain Renderer Access
        TacticalTerrainRenderer* GetTacticalTerrainRenderer() { return tactical_terrain_renderer_.get(); }
        const TacticalTerrainRenderer* GetTacticalTerrainRenderer() const { return tactical_terrain_renderer_.get(); }

        // Statistics
        int GetRenderedProvinceCount() const { return rendered_province_count_; }
        int GetRenderedFeatureCount() const { return rendered_feature_count_; }
        float GetLastRenderTime() const { return last_render_time_ms_; }
        
    private:
        // Core systems
        ::core::ecs::EntityManager& entity_manager_;
        
        // Rendering components
        Camera2D camera_;
        ViewportCuller culler_;
        LODLevel current_lod_;

        // LOD 4 Tactical terrain renderer
        std::unique_ptr<TacticalTerrainRenderer> tactical_terrain_renderer_;

        // Selection
        EntityID selected_province_;
        EntityID hovered_province_;
        
        // Rendering settings
        bool render_borders_ = true;
        bool render_names_ = true;
        bool render_features_ = true;
        bool show_debug_info_ = true;
        
        // Input state
        bool mouse_dragging_ = false;
        Vector2 last_mouse_pos_;
        
        // Statistics
        int rendered_province_count_ = 0;
        int rendered_feature_count_ = 0;
        float last_render_time_ms_ = 0.0f;
        
        // ImGui fonts (optional)
        ImFont* font_large_ = nullptr;
        ImFont* font_medium_ = nullptr;
        ImFont* font_small_ = nullptr;
        
        // Private rendering methods
        void UpdateLOD();
        void RenderProvinces();
        void RenderProvince(const ProvinceRenderComponent& province, ImDrawList* draw_list);
        void RenderProvinceBorder(const ProvinceRenderComponent& province, ImDrawList* draw_list);
        void RenderProvinceName(const ProvinceRenderComponent& province, ImDrawList* draw_list);
        void RenderFeatures(const ProvinceRenderComponent& province, ImDrawList* draw_list);
        void RenderFeature(const FeatureRenderData& feature, ImDrawList* draw_list);
        void RenderDebugInfo();
        void RenderSelection();
        
        // Helper methods
        const std::vector<Vector2>& GetBoundaryForLOD(const ProvinceRenderComponent& province) const;
        bool IsPointInPolygon(const std::vector<Vector2>& polygon, float x, float y) const;
        uint32_t ColorToImU32(const Color& color) const;
        uint32_t ColorToImU32(const Color& color, uint8_t alpha) const;
    };

} // namespace game::map
