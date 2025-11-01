// ============================================================================
// BuildingRenderer.h - LOD 4 Building and Structure Renderer
// Created: November 1, 2025
// Description: Renders buildings, cities, and structures at tactical zoom
//              Handles individual building sprites, city layouts, and
//              fortifications
// ============================================================================

#pragma once

#include "map/BuildingData.h"
#include "map/ProvinceRenderComponent.h"
#include "core/ECS/EntityManager.h"
#include <memory>
#include <unordered_map>

// Forward declarations
struct ImDrawList;

namespace game::map {

    // Use core::ecs::EntityID
    using ::core::ecs::EntityID;

    // ========================================================================
    // Camera2D - Camera structure (matching MapRenderer)
    // ========================================================================
    struct Camera2D;  // Forward declaration (defined in TacticalTerrainRenderer.h)

    // ========================================================================
    // BuildingRenderer - Renders buildings at tactical zoom
    // ========================================================================
    class BuildingRenderer {
    public:
        BuildingRenderer(::core::ecs::EntityManager& entity_manager);
        ~BuildingRenderer() = default;

        // Initialize renderer
        bool Initialize();

        // Render buildings for a province
        void RenderProvinceBuildings(
            const ProvinceRenderComponent& province,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Render all visible buildings
        void RenderAllBuildings(
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Generate building data for a province if it doesn't exist
        void GenerateBuildingsForProvince(EntityID province_id);

        // Settings
        void SetShowBuildings(bool show) { show_buildings_ = show; }
        void SetShowCities(bool show) { show_cities_ = show; }
        void SetShowFortifications(bool show) { show_fortifications_ = show; }
        void SetMinZoomForBuildings(float zoom) { min_zoom_for_buildings_ = zoom; }

        // Getters
        bool IsShowingBuildings() const { return show_buildings_; }
        bool IsShowingCities() const { return show_cities_; }
        bool IsShowingFortifications() const { return show_fortifications_; }

        // Statistics
        int GetRenderedBuildingCount() const { return rendered_building_count_; }
        int GetRenderedCityCount() const { return rendered_city_count_; }

    private:
        // Core systems
        ::core::ecs::EntityManager& entity_manager_;

        // Building data storage (province_id -> building data)
        std::unordered_map<uint32_t, ProvinceBuildingData> building_data_;

        // Rendering settings
        bool show_buildings_ = true;
        bool show_cities_ = true;
        bool show_fortifications_ = true;
        float min_zoom_for_buildings_ = 2.5f;  // Only show at tactical zoom

        // Statistics
        mutable int rendered_building_count_ = 0;
        mutable int rendered_city_count_ = 0;

        // Rendering methods
        void RenderBuilding(
            const Building& building,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        void RenderCity(
            const CityLayout& city,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        void RenderDistrict(
            const UrbanDistrict& district,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        void RenderFortification(
            const Building& fortification,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Building shape rendering
        void DrawBuildingRect(
            const Vector2& screen_pos,
            float size,
            float rotation,
            const Color& color,
            ImDrawList* draw_list
        );

        void DrawBuildingCircle(
            const Vector2& screen_pos,
            float radius,
            const Color& color,
            ImDrawList* draw_list
        );

        void DrawCastle(
            const Vector2& screen_pos,
            float size,
            const Color& color,
            ImDrawList* draw_list
        );

        void DrawChurch(
            const Vector2& screen_pos,
            float size,
            const Color& color,
            ImDrawList* draw_list
        );

        void DrawTower(
            const Vector2& screen_pos,
            float size,
            const Color& color,
            ImDrawList* draw_list
        );

        // Viewport culling
        bool IsBuildingVisible(
            const Vector2& world_pos,
            const Camera2D& camera
        ) const;

        // Building generation
        ProvinceBuildingData GenerateDefaultBuildings(
            const ProvinceRenderComponent& province
        );

        void GenerateCityLayout(
            CityLayout& city,
            const ProvinceRenderComponent& province
        );

        void GenerateUrbanDistrict(
            UrbanDistrict& district,
            BuildingCategory category,
            uint32_t building_count
        );

        void GenerateRuralBuildings(
            std::vector<Building>& buildings,
            const ProvinceRenderComponent& province
        );

        void GenerateFortifications(
            std::vector<Building>& fortifications,
            const Vector2& city_center,
            float city_radius,
            bool has_walls
        );

        // Helper methods
        Vector2 GetRandomPositionInCircle(const Vector2& center, float radius);
        Vector2 GetRandomPositionInRect(const Rect& bounds);
        float RandomFloat(float min, float max);
    };

} // namespace game::map
