// ============================================================================
// FogOfWarRenderer.h - Fog of War Visual Overlay Renderer
// Created: November 18, 2025
// Description: Renders fog of war overlay with black shroud for unexplored,
//              grey/faded for explored, and full color for visible areas
// ============================================================================

#pragma once

#include "map/FogOfWar.h"
#include "map/render/ViewportCuller.h"
#include "core/types/game_types.h"

// Forward declarations
struct ImDrawList;

namespace game::map {

    // ========================================================================
    // FogRenderMode - Different visual styles for fog of war
    // ========================================================================
    enum class FogRenderMode {
        STANDARD = 0,   // Black unexplored, grey explored, clear visible
        GRAYSCALE,      // Grayscale for explored, color for visible
        TINTED,         // Tinted overlay
        MINIMAL,        // Minimal fog (thin overlay)
        COUNT
    };

    // ========================================================================
    // FogOfWarRenderer - Renders fog of war overlay
    // ========================================================================
    class FogOfWarRenderer {
    public:
        FogOfWarRenderer();
        ~FogOfWarRenderer() = default;

        // Initialize renderer
        bool Initialize();

        // Render fog of war for a player
        void RenderFogOfWar(
            const VisibilityGrid& visibility_grid,
            const Camera2D& camera,
            ImDrawList* draw_list,
            game::types::EntityID player_id
        );

        // Render fog of war cells (optimized with culling)
        void RenderFogCells(
            const VisibilityGrid& visibility_grid,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Settings
        void SetRenderMode(FogRenderMode mode) { render_mode_ = mode; }
        void SetUnexploredOpacity(float opacity) { unexplored_opacity_ = opacity; }
        void SetExploredOpacity(float opacity) { explored_opacity_ = opacity; }
        void SetFadeTime(float seconds) { fade_time_ = seconds; }
        void SetEnabled(bool enabled) { enabled_ = enabled; }

        FogRenderMode GetRenderMode() const { return render_mode_; }
        float GetUnexploredOpacity() const { return unexplored_opacity_; }
        float GetExploredOpacity() const { return explored_opacity_; }
        float GetFadeTime() const { return fade_time_; }
        bool IsEnabled() const { return enabled_; }

        // Statistics
        int GetRenderedCellCount() const { return rendered_cell_count_; }

    private:
        // Settings
        FogRenderMode render_mode_ = FogRenderMode::STANDARD;
        float unexplored_opacity_ = 1.0f;     // Full black (0-1)
        float explored_opacity_ = 0.5f;       // Semi-transparent grey (0-1)
        float fade_time_ = 1.0f;              // Fade animation time in seconds
        bool enabled_ = true;

        // Statistics
        mutable int rendered_cell_count_ = 0;

        // Helper methods
        void RenderUnexploredCell(
            const Vector2& world_pos,
            float cell_size,
            const Camera2D& camera,
            ImDrawList* draw_list
        ) const;

        void RenderExploredCell(
            const Vector2& world_pos,
            float cell_size,
            const Camera2D& camera,
            ImDrawList* draw_list
        ) const;

        // Get fog color based on visibility state
        uint32_t GetFogColor(VisibilityState state, float opacity) const;

        // Check if cell should be rendered (viewport culling)
        bool IsCellInViewport(
            const Vector2& world_pos,
            float cell_size,
            const Camera2D& camera
        ) const;
    };

} // namespace game::map
