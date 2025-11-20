// ============================================================================
// ProvinceSpatialIndex.h - Spatial Partitioning for Province System
// Created: November 19, 2025
// Purpose: Optimize province lookups and updates for 1000+ provinces
// ============================================================================

#pragma once

#include "core/types/game_types.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <shared_mutex>

namespace game::province {

// ============================================================================
// Spatial Grid Configuration
// ============================================================================

/**
 * Grid-based spatial index for efficient province queries
 * Divides the world into a grid of cells for O(1) spatial lookups
 */
class ProvinceSpatialIndex {
public:
    struct GridCell {
        int x;
        int y;

        bool operator==(const GridCell& other) const {
            return x == other.x && y == other.y;
        }
    };

    struct GridCellHash {
        std::size_t operator()(const GridCell& cell) const {
            // Cantor pairing function for unique hash
            return ((cell.x + cell.y) * (cell.x + cell.y + 1)) / 2 + cell.y;
        }
    };

private:
    // Spatial grid parameters
    double m_cell_size;              // Size of each grid cell (e.g., 100.0 units)
    double m_world_min_x;            // World bounds
    double m_world_min_y;
    double m_world_max_x;
    double m_world_max_y;

    // Grid storage: cell -> list of province IDs in that cell
    std::unordered_map<GridCell, std::vector<types::EntityID>, GridCellHash> m_grid;

    // Province locations cache: province ID -> grid cell
    std::unordered_map<types::EntityID, GridCell> m_province_cells;

    // Thread safety
    mutable std::shared_mutex m_mutex;

public:
    /**
     * Constructor
     * @param cell_size Size of each grid cell (default 100.0)
     * @param world_bounds {min_x, min_y, max_x, max_y}
     */
    explicit ProvinceSpatialIndex(
        double cell_size = 100.0,
        double world_min_x = 0.0,
        double world_min_y = 0.0,
        double world_max_x = 10000.0,
        double world_max_y = 10000.0
    );

    /**
     * Insert a province into the spatial index
     */
    void InsertProvince(types::EntityID province_id, double x, double y);

    /**
     * Remove a province from the spatial index
     */
    void RemoveProvince(types::EntityID province_id);

    /**
     * Update a province's position (remove + insert)
     */
    void UpdateProvincePosition(types::EntityID province_id, double new_x, double new_y);

    /**
     * Find all provinces within a radius of a point
     * @param x Center x coordinate
     * @param y Center y coordinate
     * @param radius Search radius
     * @return Vector of province IDs within radius
     */
    std::vector<types::EntityID> FindProvincesInRadius(double x, double y, double radius) const;

    /**
     * Find all provinces in a rectangular region
     * @param min_x, min_y, max_x, max_y Bounding box
     * @return Vector of province IDs in region
     */
    std::vector<types::EntityID> FindProvincesInRegion(
        double min_x, double min_y, double max_x, double max_y) const;

    /**
     * Find the nearest N provinces to a point
     * @param x, y Point coordinates
     * @param count Number of provinces to find
     * @return Vector of nearest province IDs (sorted by distance)
     */
    std::vector<types::EntityID> FindNearestProvinces(
        double x, double y, int count) const;

    /**
     * Get the grid cell containing a point
     */
    GridCell GetCell(double x, double y) const;

    /**
     * Get all provinces in a specific grid cell
     */
    std::vector<types::EntityID> GetProvincesInCell(const GridCell& cell) const;

    /**
     * Clear all data from the spatial index
     */
    void Clear();

    /**
     * Get statistics about the spatial index
     */
    struct Stats {
        int total_provinces;
        int total_cells_used;
        int max_provinces_per_cell;
        double avg_provinces_per_cell;
        double load_factor;
    };

    Stats GetStats() const;

private:
    /**
     * Get all grid cells that intersect with a circle
     */
    std::vector<GridCell> GetCellsInRadius(double x, double y, double radius) const;

    /**
     * Get all grid cells that intersect with a rectangle
     */
    std::vector<GridCell> GetCellsInRegion(
        double min_x, double min_y, double max_x, double max_y) const;

    /**
     * Calculate distance between two points
     */
    static double Distance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }
};

} // namespace game::province
