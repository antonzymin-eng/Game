// ============================================================================
// FogOfWarRenderer.cpp - Fog of War Renderer Implementation
// Created: November 18, 2025
// ============================================================================

#include "map/render/FogOfWarRenderer.h"
#include "imgui.h"
#include "core/logging/Logger.h"
#include <algorithm>

namespace game::map {

    // ========================================================================
    // FogOfWarRenderer Implementation
    // ========================================================================

    FogOfWarRenderer::FogOfWarRenderer() {
        CORE_STREAM_INFO("FogOfWarRenderer") << "FogOfWarRenderer initialized";
    }

    bool FogOfWarRenderer::Initialize() {
        CORE_STREAM_INFO("FogOfWarRenderer") << "FogOfWarRenderer: Initialization complete";
        return true;
    }

    void FogOfWarRenderer::RenderFogOfWar(
        const VisibilityGrid& visibility_grid,
        const Camera2D& camera,
        ImDrawList* draw_list,
        game::types::EntityID player_id
    ) {
        if (!enabled_ || !draw_list) return;

        rendered_cell_count_ = 0;

        RenderFogCells(visibility_grid, camera, draw_list);
    }

    void FogOfWarRenderer::RenderFogCells(
        const VisibilityGrid& visibility_grid,
        const Camera2D& camera,
        ImDrawList* draw_list
    ) {
        if (!draw_list) return;

        // Calculate visible grid bounds
        float viewport_left = camera.position.x - (camera.viewport_width / (2.0f * camera.zoom));
        float viewport_right = camera.position.x + (camera.viewport_width / (2.0f * camera.zoom));
        float viewport_top = camera.position.y - (camera.viewport_height / (2.0f * camera.zoom));
        float viewport_bottom = camera.position.y + (camera.viewport_height / (2.0f * camera.zoom));

        // Convert to grid coordinates
        int min_x = static_cast<int>((viewport_left - visibility_grid.origin.x) / visibility_grid.cell_size);
        int max_x = static_cast<int>((viewport_right - visibility_grid.origin.x) / visibility_grid.cell_size);
        int min_y = static_cast<int>((viewport_top - visibility_grid.origin.y) / visibility_grid.cell_size);
        int max_y = static_cast<int>((viewport_bottom - visibility_grid.origin.y) / visibility_grid.cell_size);

        // Clamp to grid bounds
        min_x = std::max(0, min_x);
        max_x = std::min(static_cast<int>(visibility_grid.width) - 1, max_x);
        min_y = std::max(0, min_y);
        max_y = std::min(static_cast<int>(visibility_grid.height) - 1, max_y);

        // Render each visible cell
        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                const VisibilityCell* cell = visibility_grid.GetCell(
                    static_cast<uint32_t>(x),
                    static_cast<uint32_t>(y)
                );

                if (!cell) continue;

                // Skip visible cells (no fog)
                if (cell->state == VisibilityState::VISIBLE) {
                    continue;
                }

                // Get world position of cell
                Vector2 world_pos = visibility_grid.GetCellWorldPosition(
                    static_cast<uint32_t>(x),
                    static_cast<uint32_t>(y)
                );

                // Render fog based on state
                if (cell->state == VisibilityState::UNEXPLORED) {
                    RenderUnexploredCell(world_pos, visibility_grid.cell_size, camera, draw_list);
                }
                else if (cell->state == VisibilityState::EXPLORED) {
                    RenderExploredCell(world_pos, visibility_grid.cell_size, camera, draw_list);
                }

                rendered_cell_count_++;
            }
        }
    }

    void FogOfWarRenderer::RenderUnexploredCell(
        const Vector2& world_pos,
        float cell_size,
        const Camera2D& camera,
        ImDrawList* draw_list
    ) const {
        // Convert world coordinates to screen coordinates
        Vector2 screen_pos_tl = camera.WorldToScreen(world_pos.x, world_pos.y);
        Vector2 screen_pos_br = camera.WorldToScreen(
            world_pos.x + cell_size,
            world_pos.y + cell_size
        );

        // Get fog color
        uint32_t fog_color = GetFogColor(VisibilityState::UNEXPLORED, unexplored_opacity_);

        // Draw filled rectangle for unexplored area (black)
        draw_list->AddRectFilled(
            ImVec2(screen_pos_tl.x, screen_pos_tl.y),
            ImVec2(screen_pos_br.x, screen_pos_br.y),
            fog_color
        );
    }

    void FogOfWarRenderer::RenderExploredCell(
        const Vector2& world_pos,
        float cell_size,
        const Camera2D& camera,
        ImDrawList* draw_list
    ) const {
        // Convert world coordinates to screen coordinates
        Vector2 screen_pos_tl = camera.WorldToScreen(world_pos.x, world_pos.y);
        Vector2 screen_pos_br = camera.WorldToScreen(
            world_pos.x + cell_size,
            world_pos.y + cell_size
        );

        // Get fog color
        uint32_t fog_color = GetFogColor(VisibilityState::EXPLORED, explored_opacity_);

        // Draw semi-transparent grey for explored area
        draw_list->AddRectFilled(
            ImVec2(screen_pos_tl.x, screen_pos_tl.y),
            ImVec2(screen_pos_br.x, screen_pos_br.y),
            fog_color
        );
    }

    uint32_t FogOfWarRenderer::GetFogColor(VisibilityState state, float opacity) const {
        uint8_t alpha = static_cast<uint8_t>(opacity * 255.0f);

        switch (render_mode_) {
            case FogRenderMode::STANDARD:
                if (state == VisibilityState::UNEXPLORED) {
                    // Black shroud
                    return IM_COL32(0, 0, 0, alpha);
                }
                else if (state == VisibilityState::EXPLORED) {
                    // Grey semi-transparent
                    return IM_COL32(40, 40, 40, static_cast<uint8_t>(alpha * 0.7f));
                }
                break;

            case FogRenderMode::GRAYSCALE:
                if (state == VisibilityState::UNEXPLORED) {
                    return IM_COL32(0, 0, 0, alpha);
                }
                else if (state == VisibilityState::EXPLORED) {
                    // Darker grey for grayscale mode
                    return IM_COL32(60, 60, 60, static_cast<uint8_t>(alpha * 0.6f));
                }
                break;

            case FogRenderMode::TINTED:
                if (state == VisibilityState::UNEXPLORED) {
                    return IM_COL32(10, 10, 20, alpha);
                }
                else if (state == VisibilityState::EXPLORED) {
                    // Blue tint
                    return IM_COL32(30, 30, 60, static_cast<uint8_t>(alpha * 0.5f));
                }
                break;

            case FogRenderMode::MINIMAL:
                if (state == VisibilityState::UNEXPLORED) {
                    return IM_COL32(0, 0, 0, static_cast<uint8_t>(alpha * 0.8f));
                }
                else if (state == VisibilityState::EXPLORED) {
                    // Very light overlay
                    return IM_COL32(20, 20, 20, static_cast<uint8_t>(alpha * 0.3f));
                }
                break;

            default:
                break;
        }

        return IM_COL32(0, 0, 0, alpha);
    }

    bool FogOfWarRenderer::IsCellInViewport(
        const Vector2& world_pos,
        float cell_size,
        const Camera2D& camera
    ) const {
        Vector2 screen_pos = camera.WorldToScreen(world_pos.x, world_pos.y);

        float margin = cell_size * camera.zoom;
        return (screen_pos.x >= -margin && screen_pos.x <= camera.viewport_width + margin &&
                screen_pos.y >= -margin && screen_pos.y <= camera.viewport_height + margin);
    }

} // namespace game::map
