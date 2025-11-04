// ============================================================================
// TacticalTerrainRenderer.h - LOD 4 Heightmap and Detailed Terrain Renderer
// Created: November 1, 2025
// Description: Renders fine-grained terrain grids at tactical zoom level
//              Provides heightmap visualization, terrain textures, and
//              detailed environmental features
// ============================================================================

#pragma once

#include "map/TerrainData.h"
#include "map/ProvinceRenderComponent.h"
#include "map/render/ViewportCuller.h"  // For Camera2D
#include "map/render/BuildingRenderer.h"
#include "map/render/UnitRenderer.h"
#include "map/render/EnvironmentalEffectRenderer.h"
#include "core/ECS/EntityManager.h"
#include <memory>
#include <unordered_map>

// Forward declarations
struct ImDrawList;

namespace game::map {

    // Use core::ecs::EntityID
    using ::core::ecs::EntityID;

    // ========================================================================
    // TacticalTerrainRenderer - Heightmap and terrain grid renderer
    // ========================================================================
    class TacticalTerrainRenderer {
    public:
        TacticalTerrainRenderer(::core::ecs::EntityManager& entity_manager);
        ~TacticalTerrainRenderer() = default;

        // Initialize renderer
        bool Initialize();

        // Render terrain grid for a province
        void RenderProvinceTerrain(
            const ProvinceRenderComponent& province,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Render all visible terrain grids
        void RenderAllTerrain(
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Generate terrain data for a province if it doesn't exist
        void GenerateTerrainForProvince(EntityID province_id);

        // Settings
        void SetCellSize(float size) { default_cell_size_ = size; }
        void SetShowElevation(bool show) { show_elevation_ = show; }
        void SetShowTerrainTypes(bool show) { show_terrain_types_ = show; }
        void SetElevationScale(float scale) { elevation_scale_ = scale; }

        // Getters
        float GetCellSize() const { return default_cell_size_; }
        bool IsShowingElevation() const { return show_elevation_; }
        bool IsShowingTerrainTypes() const { return show_terrain_types_; }

        // Statistics
        int GetRenderedCellCount() const { return rendered_cell_count_; }
        int GetRenderedGridCount() const { return rendered_grid_count_; }

        // Building Renderer Access
        BuildingRenderer* GetBuildingRenderer() { return building_renderer_.get(); }
        const BuildingRenderer* GetBuildingRenderer() const { return building_renderer_.get(); }

        // Unit Renderer Access
        UnitRenderer* GetUnitRenderer() { return unit_renderer_.get(); }
        const UnitRenderer* GetUnitRenderer() const { return unit_renderer_.get(); }

        // Environmental Effect Renderer Access
        EnvironmentalEffectRenderer* GetEnvironmentalEffectRenderer() { return environmental_effect_renderer_.get(); }
        const EnvironmentalEffectRenderer* GetEnvironmentalEffectRenderer() const { return environmental_effect_renderer_.get(); }

    private:
        // Core systems
        ::core::ecs::EntityManager& entity_manager_;

        // Terrain data storage (province_id -> terrain data)
        std::unordered_map<uint32_t, ProvinceTerrainData> terrain_data_;

        // Sub-renderers for LOD 4
        std::unique_ptr<BuildingRenderer> building_renderer_;
        std::unique_ptr<UnitRenderer> unit_renderer_;
        std::unique_ptr<EnvironmentalEffectRenderer> environmental_effect_renderer_;

        // Rendering settings
        float default_cell_size_ = 1.0f;       // World units per cell (1m at max zoom)
        bool show_elevation_ = true;           // Show elevation shading
        bool show_terrain_types_ = true;       // Show terrain type colors
        float elevation_scale_ = 1.0f;         // Elevation shading intensity

        // Statistics
        mutable int rendered_cell_count_ = 0;
        mutable int rendered_grid_count_ = 0;

        // Helper methods
        void RenderTerrainGrid(
            const TerrainGrid& grid,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        void RenderTerrainCell(
            const TerrainCell& cell,
            const Vector2& world_pos,
            float cell_size,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        Color CalculateCellColor(const TerrainCell& cell) const;

        // Check if a cell is visible in the viewport
        bool IsCellVisible(
            const Vector2& world_pos,
            float cell_size,
            const Camera2D& camera
        ) const;

        // Generate default terrain data for a province
        ProvinceTerrainData GenerateDefaultTerrain(
            const ProvinceRenderComponent& province
        );

        // Terrain generation helpers
        void GenerateHeightmap(TerrainGrid& grid, const ProvinceRenderComponent& province);
        void AssignTerrainTypes(TerrainGrid& grid, const ProvinceRenderComponent& province);
        float GetPerlinNoise(float x, float y, float scale) const;
    };

} // namespace game::map
