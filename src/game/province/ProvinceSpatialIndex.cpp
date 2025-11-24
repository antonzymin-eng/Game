// ============================================================================
// ProvinceSpatialIndex.cpp - Spatial Partitioning Implementation
// Created: November 19, 2025
// ============================================================================

#include "game/province/ProvinceSpatialIndex.h"
#include "core/logging/Logger.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace game::province {

ProvinceSpatialIndex::ProvinceSpatialIndex(
    double cell_size,
    double world_min_x,
    double world_min_y,
    double world_max_x,
    double world_max_y
)
    : m_cell_size(cell_size)
    , m_world_min_x(world_min_x)
    , m_world_min_y(world_min_y)
    , m_world_max_x(world_max_x)
    , m_world_max_y(world_max_y)
{
    CORE_LOG_INFO("ProvinceSpatialIndex",
        "Initialized spatial index with cell size " + std::to_string(cell_size));
}

void ProvinceSpatialIndex::InsertProvince(types::EntityID province_id, double x, double y) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    GridCell cell = GetCell(x, y);

    // Add to grid
    m_grid[cell].push_back(province_id);

    // Cache province location
    m_province_cells[province_id] = cell;
}

void ProvinceSpatialIndex::RemoveProvince(types::EntityID province_id) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    // Find province's cell
    auto cell_it = m_province_cells.find(province_id);
    if (cell_it == m_province_cells.end()) {
        return;  // Province not in index
    }

    GridCell cell = cell_it->second;

    // Remove from grid
    auto grid_it = m_grid.find(cell);
    if (grid_it != m_grid.end()) {
        auto& provinces = grid_it->second;
        provinces.erase(
            std::remove(provinces.begin(), provinces.end(), province_id),
            provinces.end()
        );

        // Remove empty cells
        if (provinces.empty()) {
            m_grid.erase(grid_it);
        }
    }

    // Remove from cache
    m_province_cells.erase(cell_it);
}

void ProvinceSpatialIndex::UpdateProvincePosition(
    types::EntityID province_id, double new_x, double new_y
) {
    RemoveProvince(province_id);
    InsertProvince(province_id, new_x, new_y);
}

std::vector<types::EntityID> ProvinceSpatialIndex::FindProvincesInRadius(
    double x, double y, double radius
) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::unordered_set<types::EntityID> results;
    auto cells = GetCellsInRadius(x, y, radius);

    // Check all provinces in affected cells
    for (const auto& cell : cells) {
        auto it = m_grid.find(cell);
        if (it != m_grid.end()) {
            for (auto province_id : it->second) {
                results.insert(province_id);
            }
        }
    }

    return std::vector<types::EntityID>(results.begin(), results.end());
}

std::vector<types::EntityID> ProvinceSpatialIndex::FindProvincesInRegion(
    double min_x, double min_y, double max_x, double max_y
) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::unordered_set<types::EntityID> results;
    auto cells = GetCellsInRegion(min_x, min_y, max_x, max_y);

    for (const auto& cell : cells) {
        auto it = m_grid.find(cell);
        if (it != m_grid.end()) {
            for (auto province_id : it->second) {
                results.insert(province_id);
            }
        }
    }

    return std::vector<types::EntityID>(results.begin(), results.end());
}

std::vector<types::EntityID> ProvinceSpatialIndex::FindNearestProvinces(
    double x, double y, int count
) const {
    // Start with a small radius and expand until we find enough provinces
    double radius = m_cell_size;
    std::vector<types::EntityID> candidates;

    while (static_cast<int>(candidates.size()) < count && radius < 10000.0) {
        candidates = FindProvincesInRadius(x, y, radius);
        radius *= 2.0;
    }

    // Sort by distance (Note: would need province positions for this)
    // For now, just return first N
    if (static_cast<int>(candidates.size()) > count) {
        candidates.resize(count);
    }

    return candidates;
}

ProvinceSpatialIndex::GridCell ProvinceSpatialIndex::GetCell(double x, double y) const {
    GridCell cell;
    cell.x = static_cast<int>(std::floor(x / m_cell_size));
    cell.y = static_cast<int>(std::floor(y / m_cell_size));
    return cell;
}

std::vector<types::EntityID> ProvinceSpatialIndex::GetProvincesInCell(
    const GridCell& cell
) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    auto it = m_grid.find(cell);
    if (it != m_grid.end()) {
        return it->second;
    }
    return {};
}

void ProvinceSpatialIndex::Clear() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_grid.clear();
    m_province_cells.clear();
}

ProvinceSpatialIndex::Stats ProvinceSpatialIndex::GetStats() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    Stats stats;
    stats.total_provinces = static_cast<int>(m_province_cells.size());
    stats.total_cells_used = static_cast<int>(m_grid.size());
    stats.max_provinces_per_cell = 0;
    stats.avg_provinces_per_cell = 0.0;

    int total_province_count = 0;
    for (const auto& [cell, provinces] : m_grid) {
        int count = static_cast<int>(provinces.size());
        stats.max_provinces_per_cell = std::max(stats.max_provinces_per_cell, count);
        total_province_count += count;
    }

    if (stats.total_cells_used > 0) {
        stats.avg_provinces_per_cell =
            static_cast<double>(total_province_count) / stats.total_cells_used;
    }

    // Calculate theoretical max cells
    int grid_width = static_cast<int>(std::ceil((m_world_max_x - m_world_min_x) / m_cell_size));
    int grid_height = static_cast<int>(std::ceil((m_world_max_y - m_world_min_y) / m_cell_size));
    int max_cells = grid_width * grid_height;

    stats.load_factor = max_cells > 0
        ? static_cast<double>(stats.total_cells_used) / max_cells
        : 0.0;

    return stats;
}

std::vector<ProvinceSpatialIndex::GridCell> ProvinceSpatialIndex::GetCellsInRadius(
    double x, double y, double radius
) const {
    std::vector<GridCell> cells;

    // Get bounding box of circle
    double min_x = x - radius;
    double min_y = y - radius;
    double max_x = x + radius;
    double max_y = y + radius;

    GridCell min_cell = GetCell(min_x, min_y);
    GridCell max_cell = GetCell(max_x, max_y);

    // Add all cells in bounding box (conservative estimate)
    for (int cx = min_cell.x; cx <= max_cell.x; ++cx) {
        for (int cy = min_cell.y; cy <= max_cell.y; ++cy) {
            cells.push_back({cx, cy});
        }
    }

    return cells;
}

std::vector<ProvinceSpatialIndex::GridCell> ProvinceSpatialIndex::GetCellsInRegion(
    double min_x, double min_y, double max_x, double max_y
) const {
    std::vector<GridCell> cells;

    GridCell min_cell = GetCell(min_x, min_y);
    GridCell max_cell = GetCell(max_x, max_y);

    for (int cx = min_cell.x; cx <= max_cell.x; ++cx) {
        for (int cy = min_cell.y; cy <= max_cell.y; ++cy) {
            cells.push_back({cx, cy});
        }
    }

    return cells;
}

} // namespace game::province
