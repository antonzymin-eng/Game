// ============================================================================
// FogOfWar.h - Fog of War and Visibility System
// Created: November 18, 2025
// Description: Grid-based fog of war system for military campaigns
//              Tracks explored/unexplored territory and real-time visibility
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "map/TerrainData.h"
#include "core/types/game_types.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace game::map {

    // ========================================================================
    // VisibilityState - States for fog of war cells
    // ========================================================================
    enum class VisibilityState : uint8_t {
        UNEXPLORED = 0,  // Never seen - black shroud
        EXPLORED,        // Previously seen - grey/faded
        VISIBLE          // Currently visible - full color
    };

    // ========================================================================
    // VisibilityCell - Individual cell visibility data
    // ========================================================================
    struct VisibilityCell {
        VisibilityState state = VisibilityState::UNEXPLORED;
        float last_seen_time = 0.0f;      // Game time when last visible
        uint8_t explored_terrain_type = 0; // Cached terrain type when explored
        float explored_elevation = 0.0f;   // Cached elevation when explored

        VisibilityCell() = default;

        bool IsVisible() const { return state == VisibilityState::VISIBLE; }
        bool IsExplored() const { return state != VisibilityState::UNEXPLORED; }
        bool IsUnexplored() const { return state == VisibilityState::UNEXPLORED; }
    };

    // ========================================================================
    // VisibilityGrid - Grid-based visibility tracking
    // ========================================================================
    struct VisibilityGrid {
        uint32_t width = 0;
        uint32_t height = 0;
        float cell_size = 1.0f;          // World units per cell
        Vector2 origin;                   // World position of grid origin
        std::vector<VisibilityCell> cells; // Row-major grid data

        VisibilityGrid() = default;
        VisibilityGrid(uint32_t w, uint32_t h, float size, Vector2 orig)
            : width(w), height(h), cell_size(size), origin(orig) {
            cells.resize(w * h);
        }

        // Get cell at grid coordinates
        VisibilityCell* GetCell(uint32_t x, uint32_t y) {
            if (x >= width || y >= height) return nullptr;
            return &cells[y * width + x];
        }

        const VisibilityCell* GetCell(uint32_t x, uint32_t y) const {
            if (x >= width || y >= height) return nullptr;
            return &cells[y * width + x];
        }

        // Get cell at world position
        VisibilityCell* GetCellAtPosition(float world_x, float world_y) {
            float local_x = world_x - origin.x;
            float local_y = world_y - origin.y;

            if (local_x < 0 || local_y < 0) return nullptr;

            uint32_t grid_x = static_cast<uint32_t>(local_x / cell_size);
            uint32_t grid_y = static_cast<uint32_t>(local_y / cell_size);

            return GetCell(grid_x, grid_y);
        }

        const VisibilityCell* GetCellAtPosition(float world_x, float world_y) const {
            float local_x = world_x - origin.x;
            float local_y = world_y - origin.y;

            if (local_x < 0 || local_y < 0) return nullptr;

            uint32_t grid_x = static_cast<uint32_t>(local_x / cell_size);
            uint32_t grid_y = static_cast<uint32_t>(local_y / cell_size);

            return GetCell(grid_x, grid_y);
        }

        // Set cell visibility state
        void SetCellState(uint32_t x, uint32_t y, VisibilityState state, float game_time) {
            if (auto* cell = GetCell(x, y)) {
                cell->state = state;
                if (state == VisibilityState::VISIBLE) {
                    cell->last_seen_time = game_time;
                }
            }
        }

        // Reveal area around a point
        void RevealCircle(float world_x, float world_y, float radius, float game_time);

        // Update visibility to explored (when no longer visible)
        void UpdateToExplored(uint32_t x, uint32_t y);

        // Clear all visibility (reset to unexplored)
        void ClearAll() {
            for (auto& cell : cells) {
                cell.state = VisibilityState::UNEXPLORED;
                cell.last_seen_time = 0.0f;
            }
        }
    };

    // ========================================================================
    // FogOfWarManager - Manages fog of war for all players/factions
    // ========================================================================
    class FogOfWarManager {
    public:
        FogOfWarManager();
        ~FogOfWarManager() = default;

        // Initialize fog of war for a player/faction
        void InitializeForPlayer(game::types::EntityID player_id, uint32_t world_width, uint32_t world_height, float cell_size);

        // Update visibility based on unit positions
        void UpdateVisibility(game::types::EntityID player_id, float game_time);

        // Reveal area around a position (e.g., army, scout)
        void RevealArea(game::types::EntityID player_id, float world_x, float world_y, float radius, float game_time);

        // Check if a position is visible for a player
        bool IsPositionVisible(game::types::EntityID player_id, float world_x, float world_y) const;

        // Check if a position is explored for a player
        bool IsPositionExplored(game::types::EntityID player_id, float world_x, float world_y) const;

        // Get visibility state at position
        VisibilityState GetVisibilityState(game::types::EntityID player_id, float world_x, float world_y) const;

        // Get visibility grid for a player
        VisibilityGrid* GetVisibilityGrid(game::types::EntityID player_id);
        const VisibilityGrid* GetVisibilityGrid(game::types::EntityID player_id) const;

        // Cache terrain data when exploring
        void CacheTerrainData(game::types::EntityID player_id, uint32_t x, uint32_t y,
                            uint8_t terrain_type, float elevation);

        // Clear all fog of war (debug/cheat)
        void RevealAll(game::types::EntityID player_id, float game_time);

        // Reset fog of war for a player
        void ResetPlayer(game::types::EntityID player_id);

        // Settings
        void SetDefaultVisionRange(float range) { default_vision_range_ = range; }
        void SetScoutVisionRange(float range) { scout_vision_range_ = range; }
        void SetCavalryVisionRange(float range) { cavalry_vision_range_ = range; }

        float GetDefaultVisionRange() const { return default_vision_range_; }
        float GetScoutVisionRange() const { return scout_vision_range_; }
        float GetCavalryVisionRange() const { return cavalry_vision_range_; }

    private:
        // Player visibility grids (player_id -> grid)
        std::unordered_map<game::types::EntityID, VisibilityGrid> player_visibility_;

        // Vision range settings (in world units/meters)
        float default_vision_range_ = 50.0f;    // Default unit vision range
        float scout_vision_range_ = 100.0f;     // Scout units
        float cavalry_vision_range_ = 75.0f;    // Cavalry units
        float fortification_vision_range_ = 150.0f; // Towers/fortifications

        // Helper methods
        void InitializeGrid(VisibilityGrid& grid, uint32_t width, uint32_t height, float cell_size);
    };

    // ========================================================================
    // Inline implementations
    // ========================================================================

    inline void VisibilityGrid::RevealCircle(float world_x, float world_y, float radius, float game_time) {
        // Calculate grid bounds for the circle
        int min_x = static_cast<int>((world_x - radius - origin.x) / cell_size);
        int max_x = static_cast<int>((world_x + radius - origin.x) / cell_size);
        int min_y = static_cast<int>((world_y - radius - origin.y) / cell_size);
        int max_y = static_cast<int>((world_y + radius - origin.y) / cell_size);

        // Clamp to grid bounds
        min_x = std::max(0, min_x);
        max_x = std::min(static_cast<int>(width) - 1, max_x);
        min_y = std::max(0, min_y);
        max_y = std::min(static_cast<int>(height) - 1, max_y);

        float radius_sq = radius * radius;

        // Iterate through bounding box and check distance
        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                // Get world position of cell center
                float cell_world_x = origin.x + (x + 0.5f) * cell_size;
                float cell_world_y = origin.y + (y + 0.5f) * cell_size;

                // Check if within radius
                float dx = cell_world_x - world_x;
                float dy = cell_world_y - world_y;
                float dist_sq = dx * dx + dy * dy;

                if (dist_sq <= radius_sq) {
                    SetCellState(static_cast<uint32_t>(x), static_cast<uint32_t>(y),
                               VisibilityState::VISIBLE, game_time);
                }
            }
        }
    }

    inline void VisibilityGrid::UpdateToExplored(uint32_t x, uint32_t y) {
        if (auto* cell = GetCell(x, y)) {
            if (cell->state == VisibilityState::VISIBLE) {
                cell->state = VisibilityState::EXPLORED;
            }
        }
    }

} // namespace game::map
