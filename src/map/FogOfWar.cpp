// ============================================================================
// FogOfWar.cpp - Fog of War System Implementation
// Created: November 18, 2025
// ============================================================================

#include "map/FogOfWar.h"
#include "core/logging/Logger.h"
#include <algorithm>

namespace game::map {

    // ========================================================================
    // FogOfWarManager Implementation
    // ========================================================================

    FogOfWarManager::FogOfWarManager() {
        CORE_STREAM_INFO("FogOfWar") << "FogOfWarManager initialized";
    }

    void FogOfWarManager::InitializeForPlayer(
        game::types::EntityID player_id,
        uint32_t world_width,
        uint32_t world_height,
        float cell_size
    ) {
        VisibilityGrid grid;
        InitializeGrid(grid, world_width, world_height, cell_size);
        player_visibility_[player_id] = std::move(grid);

        CORE_STREAM_INFO("FogOfWar") << "Initialized fog of war for player " << player_id
                                     << " (grid: " << world_width << "x" << world_height
                                     << ", cell size: " << cell_size << ")";
    }

    void FogOfWarManager::UpdateVisibility(game::types::EntityID player_id, float game_time) {
        auto it = player_visibility_.find(player_id);
        if (it == player_visibility_.end()) return;

        VisibilityGrid& grid = it->second;

        // First, transition all VISIBLE cells to EXPLORED
        for (uint32_t y = 0; y < grid.height; ++y) {
            for (uint32_t x = 0; x < grid.width; ++x) {
                auto* cell = grid.GetCell(x, y);
                if (cell && cell->state == VisibilityState::VISIBLE) {
                    cell->state = VisibilityState::EXPLORED;
                }
            }
        }

        // Note: Actual visibility updates should be called by the military system
        // when armies/units are updated. This method just transitions visible->explored.
    }

    void FogOfWarManager::RevealArea(
        game::types::EntityID player_id,
        float world_x,
        float world_y,
        float radius,
        float game_time
    ) {
        auto it = player_visibility_.find(player_id);
        if (it == player_visibility_.end()) return;

        VisibilityGrid& grid = it->second;
        grid.RevealCircle(world_x, world_y, radius, game_time);
    }

    bool FogOfWarManager::IsPositionVisible(
        game::types::EntityID player_id,
        float world_x,
        float world_y
    ) const {
        auto it = player_visibility_.find(player_id);
        if (it == player_visibility_.end()) return false;

        const VisibilityGrid& grid = it->second;
        const auto* cell = grid.GetCellAtPosition(world_x, world_y);
        return cell && cell->IsVisible();
    }

    bool FogOfWarManager::IsPositionExplored(
        game::types::EntityID player_id,
        float world_x,
        float world_y
    ) const {
        auto it = player_visibility_.find(player_id);
        if (it == player_visibility_.end()) return false;

        const VisibilityGrid& grid = it->second;
        const auto* cell = grid.GetCellAtPosition(world_x, world_y);
        return cell && cell->IsExplored();
    }

    VisibilityState FogOfWarManager::GetVisibilityState(
        game::types::EntityID player_id,
        float world_x,
        float world_y
    ) const {
        auto it = player_visibility_.find(player_id);
        if (it == player_visibility_.end()) return VisibilityState::UNEXPLORED;

        const VisibilityGrid& grid = it->second;
        const auto* cell = grid.GetCellAtPosition(world_x, world_y);
        return cell ? cell->state : VisibilityState::UNEXPLORED;
    }

    VisibilityGrid* FogOfWarManager::GetVisibilityGrid(game::types::EntityID player_id) {
        auto it = player_visibility_.find(player_id);
        return it != player_visibility_.end() ? &it->second : nullptr;
    }

    const VisibilityGrid* FogOfWarManager::GetVisibilityGrid(game::types::EntityID player_id) const {
        auto it = player_visibility_.find(player_id);
        return it != player_visibility_.end() ? &it->second : nullptr;
    }

    void FogOfWarManager::CacheTerrainData(
        game::types::EntityID player_id,
        uint32_t x,
        uint32_t y,
        uint8_t terrain_type,
        float elevation
    ) {
        auto it = player_visibility_.find(player_id);
        if (it == player_visibility_.end()) return;

        VisibilityGrid& grid = it->second;
        auto* cell = grid.GetCell(x, y);
        if (cell) {
            cell->explored_terrain_type = terrain_type;
            cell->explored_elevation = elevation;
        }
    }

    void FogOfWarManager::RevealAll(game::types::EntityID player_id, float game_time) {
        auto it = player_visibility_.find(player_id);
        if (it == player_visibility_.end()) return;

        VisibilityGrid& grid = it->second;
        for (uint32_t y = 0; y < grid.height; ++y) {
            for (uint32_t x = 0; x < grid.width; ++x) {
                grid.SetCellState(x, y, VisibilityState::VISIBLE, game_time);
            }
        }

        CORE_STREAM_INFO("FogOfWar") << "Revealed all fog of war for player " << player_id;
    }

    void FogOfWarManager::ResetPlayer(game::types::EntityID player_id) {
        auto it = player_visibility_.find(player_id);
        if (it == player_visibility_.end()) return;

        it->second.ClearAll();
        CORE_STREAM_INFO("FogOfWar") << "Reset fog of war for player " << player_id;
    }

    void FogOfWarManager::InitializeGrid(
        VisibilityGrid& grid,
        uint32_t width,
        uint32_t height,
        float cell_size
    ) {
        grid.width = width;
        grid.height = height;
        grid.cell_size = cell_size;
        grid.origin = Vector2(0.0f, 0.0f);
        grid.cells.resize(width * height);

        // Initialize all cells to UNEXPLORED
        for (auto& cell : grid.cells) {
            cell.state = VisibilityState::UNEXPLORED;
            cell.last_seen_time = 0.0f;
        }
    }

} // namespace game::map
