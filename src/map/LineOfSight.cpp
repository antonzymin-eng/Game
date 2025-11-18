// ============================================================================
// LineOfSight.cpp - Line of Sight Calculation Implementation
// Created: November 18, 2025
// ============================================================================

#include "map/LineOfSight.h"
#include "core/logging/Logger.h"
#include <cmath>
#include <algorithm>

namespace game::map {

    // ========================================================================
    // LineOfSightCalculator Implementation
    // ========================================================================

    LineOfSightCalculator::LineOfSightCalculator() {
        CORE_STREAM_INFO("LineOfSight") << "LineOfSightCalculator initialized";
    }

    bool LineOfSightCalculator::HasLineOfSight(
        const Vector2& from,
        const Vector2& to,
        const TerrainGrid* terrain,
        LOSModifier* out_modifier
    ) const {
        if (!terrain) return true; // No terrain = always visible

        float distance = std::sqrt(
            (to.x - from.x) * (to.x - from.x) +
            (to.y - from.y) * (to.y - from.y)
        );

        if (distance > maximum_range_) return false;

        // Calculate LOS modifier
        LOSModifier modifier = CalculateLOSModifier(from, to, terrain, distance, 1.0f);

        if (out_modifier) {
            *out_modifier = modifier;
        }

        // Check if distance is within effective range
        float effective_range = modifier.GetEffectiveRange();
        if (distance > effective_range) return false;

        // Ray-cast to check for obstructions
        float obstruction_factor = 0.0f;
        bool clear_path = RayCast(from, to, terrain, obstruction_factor);

        // If obstruction is too high, no LOS
        return clear_path && obstruction_factor < 0.8f;
    }

    float LineOfSightCalculator::CalculateVisionRange(
        const Vector2& position,
        const TerrainGrid* terrain,
        float base_range,
        float weather_modifier
    ) const {
        if (!terrain) return base_range * weather_modifier;

        // Get terrain at viewer position
        const TerrainCell* viewer_cell = terrain->GetCellAtPosition(position.x, position.y);
        if (!viewer_cell) return base_range * weather_modifier;

        float range = base_range;

        // Apply elevation bonus
        if (use_elevation_bonus_ && viewer_cell->elevation > 100.0f) {
            float elevation_bonus = (viewer_cell->elevation / 100.0f) *
                                   TerrainVisibilityModifiers::ELEVATION_BONUS_PER_100M;
            range += elevation_bonus;
        }

        // Apply terrain penalty for viewer
        if (use_terrain_concealment_) {
            float penalty = CalculateTerrainPenalty(viewer_cell->type);
            range -= penalty;
        }

        // Apply weather modifier
        range *= weather_modifier;

        // Clamp to reasonable range
        range = std::max(10.0f, std::min(range, maximum_range_));

        return range;
    }

    void LineOfSightCalculator::GetVisibleCells(
        const Vector2& position,
        float vision_range,
        const TerrainGrid* terrain,
        std::vector<std::pair<uint32_t, uint32_t>>& out_visible_cells
    ) const {
        if (!terrain) return;

        out_visible_cells.clear();

        // Calculate grid bounds for the vision circle
        int min_x = static_cast<int>((position.x - vision_range - terrain->origin.x) / terrain->cell_size);
        int max_x = static_cast<int>((position.x + vision_range - terrain->origin.x) / terrain->cell_size);
        int min_y = static_cast<int>((position.y - vision_range - terrain->origin.y) / terrain->cell_size);
        int max_y = static_cast<int>((position.y + vision_range - terrain->origin.y) / terrain->cell_size);

        // Clamp to grid bounds
        min_x = std::max(0, min_x);
        max_x = std::min(static_cast<int>(terrain->width) - 1, max_x);
        min_y = std::max(0, min_y);
        max_y = std::min(static_cast<int>(terrain->height) - 1, max_y);

        float range_sq = vision_range * vision_range;

        // Check each cell in range
        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                // Get world position of cell center
                Vector2 cell_pos = terrain->GetCellWorldPosition(
                    static_cast<uint32_t>(x),
                    static_cast<uint32_t>(y)
                );
                cell_pos.x += terrain->cell_size * 0.5f;
                cell_pos.y += terrain->cell_size * 0.5f;

                // Check if within vision range
                float dx = cell_pos.x - position.x;
                float dy = cell_pos.y - position.y;
                float dist_sq = dx * dx + dy * dy;

                if (dist_sq <= range_sq) {
                    // Check line of sight
                    if (HasLineOfSight(position, cell_pos, terrain, nullptr)) {
                        out_visible_cells.emplace_back(
                            static_cast<uint32_t>(x),
                            static_cast<uint32_t>(y)
                        );
                    }
                }
            }
        }
    }

    LOSModifier LineOfSightCalculator::CalculateLOSModifier(
        const Vector2& viewer_pos,
        const Vector2& target_pos,
        const TerrainGrid* terrain,
        float base_range,
        float weather_modifier
    ) const {
        LOSModifier modifier;
        modifier.base_range = base_range;
        modifier.weather_modifier = weather_modifier;

        if (!terrain) return modifier;

        // Get terrain cells
        const TerrainCell* viewer_cell = terrain->GetCellAtPosition(viewer_pos.x, viewer_pos.y);
        const TerrainCell* target_cell = terrain->GetCellAtPosition(target_pos.x, target_pos.y);

        if (!viewer_cell || !target_cell) return modifier;

        // Calculate elevation bonus
        modifier.elevation_bonus = CalculateElevationBonus(
            viewer_cell->elevation,
            target_cell->elevation
        );

        // Calculate terrain penalty for viewer
        modifier.terrain_penalty = CalculateTerrainPenalty(viewer_cell->type);

        // Calculate concealment of target
        modifier.forest_concealment = CalculateTerrainConcealment(target_cell->type);

        return modifier;
    }

    bool LineOfSightCalculator::RayCast(
        const Vector2& from,
        const Vector2& to,
        const TerrainGrid* terrain,
        float& out_obstruction_factor
    ) const {
        if (!terrain) {
            out_obstruction_factor = 0.0f;
            return true;
        }

        // Get line points using Bresenham-like algorithm
        std::vector<std::pair<int, int>> line_points;
        GetLinePoints(from, to, line_points);

        // Get viewer elevation
        const TerrainCell* viewer_cell = terrain->GetCellAtPosition(from.x, from.y);
        float viewer_elevation = viewer_cell ? viewer_cell->elevation : 0.0f;

        float total_obstruction = 0.0f;
        int sample_count = 0;

        // Check each point along the line
        for (const auto& point : line_points) {
            const TerrainCell* cell = terrain->GetCell(
                static_cast<uint32_t>(point.first),
                static_cast<uint32_t>(point.second)
            );

            if (cell) {
                float distance_from_viewer = std::sqrt(
                    (point.first - from.x) * (point.first - from.x) +
                    (point.second - from.y) * (point.second - from.y)
                );

                if (IsTerrainBlocking(cell, viewer_elevation, distance_from_viewer)) {
                    total_obstruction += 0.5f;
                }

                sample_count++;
            }
        }

        out_obstruction_factor = sample_count > 0 ?
                                total_obstruction / sample_count :
                                0.0f;

        return out_obstruction_factor < 0.8f;
    }

    void LineOfSightCalculator::GetLinePoints(
        const Vector2& from,
        const Vector2& to,
        std::vector<std::pair<int, int>>& out_points
    ) const {
        out_points.clear();

        int x0 = static_cast<int>(from.x);
        int y0 = static_cast<int>(from.y);
        int x1 = static_cast<int>(to.x);
        int y1 = static_cast<int>(to.y);

        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;

        while (true) {
            out_points.emplace_back(x0, y0);

            if (x0 == x1 && y0 == y1) break;

            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    bool LineOfSightCalculator::IsTerrainBlocking(
        const TerrainCell* cell,
        float viewer_elevation,
        float distance_from_viewer
    ) const {
        if (!cell) return false;

        // Mountains and hills can block LOS if viewer is at similar/lower elevation
        if (cell->type == TerrainCellType::MOUNTAIN) {
            return cell->elevation > viewer_elevation + 50.0f;
        }

        if (cell->type == TerrainCellType::HILLS) {
            return cell->elevation > viewer_elevation + 100.0f;
        }

        // Forest blocks LOS for nearby targets
        if (cell->type == TerrainCellType::FOREST && distance_from_viewer > 30.0f) {
            return true;
        }

        return false;
    }

} // namespace game::map
