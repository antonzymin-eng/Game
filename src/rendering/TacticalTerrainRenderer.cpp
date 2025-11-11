// ============================================================================
// TacticalTerrainRenderer.cpp - LOD 4 Terrain Renderer Implementation
// Created: November 1, 2025
// ============================================================================

#include "map/render/TacticalTerrainRenderer.h"
#include "map/render/BuildingRenderer.h"
#include "map/render/UnitRenderer.h"
#include "map/render/EnvironmentalEffectRenderer.h"
#include "utils/PlatformCompat.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include "core/logging/Logger.h"

namespace game::map {

    // ========================================================================
    // Constructor / Destructor
    // ========================================================================
    TacticalTerrainRenderer::TacticalTerrainRenderer(::core::ecs::EntityManager& entity_manager)
        : entity_manager_(entity_manager)
    {
    }

    // ========================================================================
    // Initialization
    // ========================================================================
    bool TacticalTerrainRenderer::Initialize() {
        CORE_STREAM_INFO("TacticalTerrainRenderer") << "TacticalTerrainRenderer: Initializing...";
        terrain_data_.clear();

        // Initialize building renderer
        building_renderer_ = std::make_unique<BuildingRenderer>(entity_manager_);
        if (!building_renderer_->Initialize()) {
            CORE_STREAM_ERROR("TacticalTerrainRenderer") << "TacticalTerrainRenderer: Failed to initialize BuildingRenderer";
            return false;
        }

        // Initialize unit renderer
        unit_renderer_ = std::make_unique<UnitRenderer>(entity_manager_);
        if (!unit_renderer_->Initialize()) {
            CORE_STREAM_ERROR("TacticalTerrainRenderer") << "TacticalTerrainRenderer: Failed to initialize UnitRenderer";
            return false;
        }

        // Initialize environmental effect renderer
        environmental_effect_renderer_ = std::make_unique<EnvironmentalEffectRenderer>(entity_manager_);
        if (!environmental_effect_renderer_->Initialize()) {
            CORE_STREAM_ERROR("TacticalTerrainRenderer") << "TacticalTerrainRenderer: Failed to initialize EnvironmentalEffectRenderer";
            return false;
        }

        CORE_STREAM_INFO("TacticalTerrainRenderer") << "TacticalTerrainRenderer: Initialized successfully";
        return true;
    }

    // ========================================================================
    // Main Rendering
    // ========================================================================
    void TacticalTerrainRenderer::RenderProvinceTerrain(
        const ProvinceRenderComponent& province,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        // Get or generate terrain data for this province
        auto it = terrain_data_.find(province.province_id);
        if (it == terrain_data_.end()) {
            // Generate terrain data
            terrain_data_[province.province_id] = GenerateDefaultTerrain(province);
            it = terrain_data_.find(province.province_id);
        }

        const ProvinceTerrainData& terrain = it->second;
        if (!terrain.has_heightmap) {
            return; // No heightmap data
        }

        // Render the terrain grid
        RenderTerrainGrid(terrain.grid, camera, draw_list);

        // Render buildings on top of terrain
        if (building_renderer_) {
            building_renderer_->RenderProvinceBuildings(province, camera, draw_list);
        }
    }

    void TacticalTerrainRenderer::RenderAllTerrain(
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        rendered_cell_count_ = 0;
        rendered_grid_count_ = 0;

        // Get all provinces with render components
        auto entities = entity_manager_.GetEntitiesWithComponent<ProvinceRenderComponent>();

        for (const auto& entity_id : entities) {
            auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(entity_id);
            if (!render) continue;

            RenderProvinceTerrain(*render, camera, draw_list);
        }
    }

    // ========================================================================
    // Terrain Grid Rendering
    // ========================================================================
    void TacticalTerrainRenderer::RenderTerrainGrid(
        const TerrainGrid& grid,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        if (grid.cells.empty()) return;

        rendered_grid_count_++;

        // Render each cell
        for (uint32_t y = 0; y < grid.height; ++y) {
            for (uint32_t x = 0; x < grid.width; ++x) {
                const TerrainCell* cell = grid.GetCell(x, y);
                if (!cell) continue;

                // Get world position of cell
                Vector2 world_pos = grid.GetCellWorldPosition(x, y);

                // Viewport culling - skip cells outside viewport
                if (!IsCellVisible(world_pos, grid.cell_size, camera)) {
                    continue;
                }

                // Render the cell
                RenderTerrainCell(*cell, world_pos, grid.cell_size, camera, draw_list);
                rendered_cell_count_++;
            }
        }
    }

    void TacticalTerrainRenderer::RenderTerrainCell(
        const TerrainCell& cell,
        const Vector2& world_pos,
        float cell_size,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        // Calculate screen coordinates for cell corners
        Vector2 top_left = camera.WorldToScreen(world_pos.x, world_pos.y);
        Vector2 bottom_right = camera.WorldToScreen(world_pos.x + cell_size, world_pos.y + cell_size);

        // Calculate cell color based on terrain type and elevation
        Color cell_color = CalculateCellColor(cell);
        uint32_t color = IM_COL32(cell_color.r, cell_color.g, cell_color.b, cell_color.a);

        // Draw filled rectangle for the cell
        draw_list->AddRectFilled(
            ImVec2(top_left.x, top_left.y),
            ImVec2(bottom_right.x, bottom_right.y),
            color
        );

        // Optional: Draw cell borders at very high zoom for detail
        if (camera.zoom > 5.0f) {
            uint32_t border_color = IM_COL32(0, 0, 0, 30);
            draw_list->AddRect(
                ImVec2(top_left.x, top_left.y),
                ImVec2(bottom_right.x, bottom_right.y),
                border_color,
                0.0f,
                0,
                0.5f
            );
        }
    }

    // ========================================================================
    // Color Calculation
    // ========================================================================
    Color TacticalTerrainRenderer::CalculateCellColor(const TerrainCell& cell) const {
        if (show_terrain_types_) {
            // Use terrain type with elevation shading
            return cell.GetColor();
        }
        else if (show_elevation_) {
            // Pure elevation-based coloring (grayscale heightmap)
            float normalized_elevation = std::clamp(cell.elevation / 1000.0f, 0.0f, 1.0f);
            uint8_t gray = static_cast<uint8_t>(normalized_elevation * 255.0f * elevation_scale_);
            return Color(gray, gray, gray, 255);
        }
        else {
            // Default: plain color
            return Color(180, 180, 180, 255);
        }
    }

    // ========================================================================
    // Viewport Culling
    // ========================================================================
    bool TacticalTerrainRenderer::IsCellVisible(
        const Vector2& world_pos,
        float cell_size,
        const Camera2D& camera) const
    {
        // Simple bounding box check
        // Calculate world space viewport bounds
        Vector2 top_left_world = camera.ScreenToWorld(0, 0);
        Vector2 bottom_right_world = camera.ScreenToWorld(camera.viewport_width, camera.viewport_height);

        // Check if cell intersects viewport
        bool x_overlap = world_pos.x + cell_size >= top_left_world.x &&
                        world_pos.x <= bottom_right_world.x;
        bool y_overlap = world_pos.y + cell_size >= top_left_world.y &&
                        world_pos.y <= bottom_right_world.y;

        return x_overlap && y_overlap;
    }

    // ========================================================================
    // Terrain Generation
    // ========================================================================
    void TacticalTerrainRenderer::GenerateTerrainForProvince(EntityID province_id) {
        auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(province_id);
        if (!render) return;

        terrain_data_[render->province_id] = GenerateDefaultTerrain(*render);
    }

    ProvinceTerrainData TacticalTerrainRenderer::GenerateDefaultTerrain(
        const ProvinceRenderComponent& province)
    {
        ProvinceTerrainData terrain(province.province_id);

        // Calculate grid dimensions based on province bounding box
        Rect bounds = province.bounding_box;
        float width_world = bounds.GetWidth();
        float height_world = bounds.GetHeight();

        // Calculate grid size (cells)
        // At default_cell_size_ = 1.0, this gives us 1 cell per world unit
        uint32_t grid_width = static_cast<uint32_t>(std::ceil(width_world / default_cell_size_));
        uint32_t grid_height = static_cast<uint32_t>(std::ceil(height_world / default_cell_size_));

        // Cap grid size to prevent excessive memory usage
        const uint32_t MAX_GRID_SIZE = 500;
        grid_width = std::min(grid_width, MAX_GRID_SIZE);
        grid_height = std::min(grid_height, MAX_GRID_SIZE);

        // Create grid
        terrain.grid = TerrainGrid(
            grid_width,
            grid_height,
            default_cell_size_,
            Vector2(bounds.min_x, bounds.min_y)
        );

        // Generate heightmap
        GenerateHeightmap(terrain.grid, province);

        // Assign terrain types based on elevation and province terrain
        AssignTerrainTypes(terrain.grid, province);

        terrain.has_heightmap = true;

        CORE_STREAM_INFO("TacticalTerrainRenderer") << "Generated terrain grid for province " << province.province_id
                  << " (" << province.name << "): "
                  << grid_width << "x" << grid_height << " cells";

        return terrain;
    }

    void TacticalTerrainRenderer::GenerateHeightmap(
        TerrainGrid& grid,
        const ProvinceRenderComponent& province)
    {
        // Base elevation from province terrain type
        float base_elevation = 0.0f;
        switch (province.terrain_type) {
            case TerrainType::PLAINS:
                base_elevation = 50.0f;
                break;
            case TerrainType::HILLS:
                base_elevation = 200.0f;
                break;
            case TerrainType::MOUNTAINS:
                base_elevation = 600.0f;
                break;
            case TerrainType::HIGHLANDS:
                base_elevation = 400.0f;
                break;
            case TerrainType::COAST:
                base_elevation = 10.0f;
                break;
            case TerrainType::WETLAND:
                base_elevation = 5.0f;
                break;
            case TerrainType::DESERT:
                base_elevation = 100.0f;
                break;
            default:
                base_elevation = 50.0f;
        }

        // Generate elevation using simple Perlin-like noise
        for (uint32_t y = 0; y < grid.height; ++y) {
            for (uint32_t x = 0; x < grid.width; ++x) {
                Vector2 world_pos = grid.GetCellWorldPosition(x, y);

                // Multiple octaves of noise for realistic terrain
                float elevation = base_elevation;
                elevation += GetPerlinNoise(world_pos.x, world_pos.y, 0.1f) * 100.0f;   // Large features
                elevation += GetPerlinNoise(world_pos.x, world_pos.y, 0.5f) * 30.0f;    // Medium features
                elevation += GetPerlinNoise(world_pos.x, world_pos.y, 2.0f) * 10.0f;    // Small details

                // Clamp elevation to valid range
                elevation = std::clamp(elevation, 0.0f, 1000.0f);

                TerrainCell* cell = grid.GetCell(x, y);
                if (cell) {
                    cell->elevation = elevation;
                }
            }
        }
    }

    void TacticalTerrainRenderer::AssignTerrainTypes(
        TerrainGrid& grid,
        const ProvinceRenderComponent& province)
    {
        // Base terrain type from province
        TerrainCellType base_type = TerrainCellType::PLAINS;

        switch (province.terrain_type) {
            case TerrainType::PLAINS:
                base_type = TerrainCellType::PLAINS;
                break;
            case TerrainType::HILLS:
                base_type = TerrainCellType::HILLS;
                break;
            case TerrainType::MOUNTAINS:
                base_type = TerrainCellType::MOUNTAIN;
                break;
            case TerrainType::FOREST:
                base_type = TerrainCellType::FOREST;
                break;
            case TerrainType::DESERT:
                base_type = TerrainCellType::DESERT;
                break;
            case TerrainType::COAST:
                base_type = TerrainCellType::BEACH;
                break;
            case TerrainType::WETLAND:
                base_type = TerrainCellType::MARSH;
                break;
            case TerrainType::HIGHLANDS:
                base_type = TerrainCellType::HILLS;
                break;
            default:
                base_type = TerrainCellType::PLAINS;
        }

        // Assign terrain types with variation based on elevation
        for (uint32_t y = 0; y < grid.height; ++y) {
            for (uint32_t x = 0; x < grid.width; ++x) {
                TerrainCell* cell = grid.GetCell(x, y);
                if (!cell) continue;

                // Start with base type
                cell->type = base_type;

                // Modify based on elevation
                if (cell->elevation < 5.0f) {
                    cell->type = TerrainCellType::WATER;
                }
                else if (cell->elevation < 15.0f && base_type == TerrainCellType::BEACH) {
                    cell->type = TerrainCellType::BEACH;
                }
                else if (cell->elevation > 500.0f) {
                    // High elevations become mountains or snow
                    if (cell->elevation > 800.0f) {
                        cell->type = TerrainCellType::SNOW;
                    } else {
                        cell->type = TerrainCellType::MOUNTAIN;
                    }
                }
                else if (cell->elevation > 200.0f && base_type == TerrainCellType::PLAINS) {
                    // Plains at moderate elevation become hills
                    cell->type = TerrainCellType::HILLS;
                }

                // Add some moisture variation (for future use)
                Vector2 world_pos = grid.GetCellWorldPosition(x, y);
                float moisture = GetPerlinNoise(world_pos.x * 2.0f, world_pos.y * 2.0f, 1.0f);
                cell->moisture = static_cast<uint8_t>((moisture + 1.0f) * 127.5f);  // Convert -1..1 to 0..255
            }
        }
    }

    // ========================================================================
    // Perlin Noise Implementation (Simple)
    // ========================================================================
    float TacticalTerrainRenderer::GetPerlinNoise(float x, float y, float scale) const {
        // Simple pseudo-Perlin noise using trigonometric functions
        // This is a placeholder - a real implementation would use proper Perlin/Simplex noise
        x *= scale;
        y *= scale;

        float noise = 0.0f;
        noise += std::sin(x * 0.7f + y * 0.3f) * 0.5f;
        noise += std::sin(x * 1.3f - y * 0.7f) * 0.3f;
        noise += std::sin(x * 2.1f + y * 1.7f) * 0.2f;

        return noise; // Range approximately -1.0 to 1.0
    }

} // namespace game::map
