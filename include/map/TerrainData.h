// ============================================================================
// TerrainData.h - LOD 4 Terrain Grid Data Structures
// Created: November 1, 2025
// Description: Fine-grained terrain grid system for tactical zoom level
//              Provides heightmap and detailed terrain cell data
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "ProvinceRenderComponent.h"
#include <vector>
#include <cstdint>

namespace game::map {

    // ========================================================================
    // TerrainCellType - Fine-grained terrain classification for LOD 4
    // ========================================================================
    enum class TerrainCellType : uint8_t {
        PLAINS = 0,
        FOREST,
        MOUNTAIN,
        WATER,
        HILLS,
        MARSH,
        DESERT,
        TUNDRA,
        BEACH,
        SNOW,
        UNKNOWN
    };

    // ========================================================================
    // TerrainCell - Individual grid cell for heightmap rendering
    // ========================================================================
    struct TerrainCell {
        TerrainCellType type = TerrainCellType::PLAINS;
        float elevation = 0.0f;      // Height in meters (0-1000)
        uint8_t moisture = 128;      // Moisture level (0-255)
        uint8_t temperature = 128;   // Temperature (0-255)

        TerrainCell() = default;
        TerrainCell(TerrainCellType t, float e) : type(t), elevation(e) {}

        // Get color based on elevation and type
        Color GetColor() const {
            // Base color from terrain type
            Color base;
            switch (type) {
                case TerrainCellType::WATER:
                    base = Color(50, 100, 200);
                    break;
                case TerrainCellType::BEACH:
                    base = Color(230, 220, 170);
                    break;
                case TerrainCellType::PLAINS:
                    base = Color(120, 180, 100);
                    break;
                case TerrainCellType::FOREST:
                    base = Color(40, 100, 40);
                    break;
                case TerrainCellType::HILLS:
                    base = Color(140, 160, 90);
                    break;
                case TerrainCellType::MOUNTAIN:
                    base = Color(140, 140, 140);
                    break;
                case TerrainCellType::MARSH:
                    base = Color(80, 120, 100);
                    break;
                case TerrainCellType::DESERT:
                    base = Color(220, 200, 140);
                    break;
                case TerrainCellType::TUNDRA:
                    base = Color(180, 200, 200);
                    break;
                case TerrainCellType::SNOW:
                    base = Color(240, 250, 255);
                    break;
                default:
                    base = Color(100, 100, 100);
            }

            // Modify color based on elevation (shading)
            // Higher elevation = lighter, lower = darker
            float elevation_factor = (elevation / 1000.0f); // Normalize to 0-1
            float shade = 0.7f + (elevation_factor * 0.3f); // 0.7 to 1.0

            base.r = static_cast<uint8_t>(base.r * shade);
            base.g = static_cast<uint8_t>(base.g * shade);
            base.b = static_cast<uint8_t>(base.b * shade);

            return base;
        }
    };

    // ========================================================================
    // TerrainGrid - Grid of terrain cells for a region
    // ========================================================================
    struct TerrainGrid {
        uint32_t width = 0;          // Grid width in cells
        uint32_t height = 0;         // Grid height in cells
        float cell_size = 1.0f;      // Size of each cell in world units
        Vector2 origin;              // World position of grid origin (top-left)
        std::vector<TerrainCell> cells; // Row-major grid data

        TerrainGrid() = default;
        TerrainGrid(uint32_t w, uint32_t h, float size, Vector2 orig)
            : width(w), height(h), cell_size(size), origin(orig) {
            cells.resize(w * h);
        }

        // Get cell at grid coordinates
        TerrainCell* GetCell(uint32_t x, uint32_t y) {
            if (x >= width || y >= height) return nullptr;
            return &cells[y * width + x];
        }

        const TerrainCell* GetCell(uint32_t x, uint32_t y) const {
            if (x >= width || y >= height) return nullptr;
            return &cells[y * width + x];
        }

        // Get cell at world position
        TerrainCell* GetCellAtPosition(float world_x, float world_y) {
            float local_x = world_x - origin.x;
            float local_y = world_y - origin.y;

            if (local_x < 0 || local_y < 0) return nullptr;

            uint32_t grid_x = static_cast<uint32_t>(local_x / cell_size);
            uint32_t grid_y = static_cast<uint32_t>(local_y / cell_size);

            return GetCell(grid_x, grid_y);
        }

        const TerrainCell* GetCellAtPosition(float world_x, float world_y) const {
            float local_x = world_x - origin.x;
            float local_y = world_y - origin.y;

            if (local_x < 0 || local_y < 0) return nullptr;

            uint32_t grid_x = static_cast<uint32_t>(local_x / cell_size);
            uint32_t grid_y = static_cast<uint32_t>(local_y / cell_size);

            return GetCell(grid_x, grid_y);
        }

        // Set cell data
        void SetCell(uint32_t x, uint32_t y, const TerrainCell& cell) {
            if (x >= width || y >= height) return;
            cells[y * width + x] = cell;
        }

        // Get world position of cell
        Vector2 GetCellWorldPosition(uint32_t x, uint32_t y) const {
            return Vector2(
                origin.x + (x * cell_size),
                origin.y + (y * cell_size)
            );
        }

        // Get bounding box of grid in world coordinates
        Rect GetBounds() const {
            return Rect(
                origin.x,
                origin.y,
                origin.x + (width * cell_size),
                origin.y + (height * cell_size)
            );
        }
    };

    // ========================================================================
    // ProvinceTerrainData - Terrain grid component for provinces
    // ========================================================================
    struct ProvinceTerrainData {
        uint32_t province_id = 0;
        TerrainGrid grid;
        bool has_heightmap = false;

        ProvinceTerrainData() = default;
        ProvinceTerrainData(uint32_t id) : province_id(id) {}
    };

    // ========================================================================
    // Utility Functions
    // ========================================================================

    // Convert string to TerrainCellType
    inline TerrainCellType StringToTerrainCellType(const std::string& str) {
        if (str == "plains") return TerrainCellType::PLAINS;
        if (str == "forest") return TerrainCellType::FOREST;
        if (str == "mountain" || str == "mountains") return TerrainCellType::MOUNTAIN;
        if (str == "water") return TerrainCellType::WATER;
        if (str == "hills") return TerrainCellType::HILLS;
        if (str == "marsh" || str == "wetland") return TerrainCellType::MARSH;
        if (str == "desert") return TerrainCellType::DESERT;
        if (str == "tundra") return TerrainCellType::TUNDRA;
        if (str == "beach" || str == "coast") return TerrainCellType::BEACH;
        if (str == "snow") return TerrainCellType::SNOW;
        return TerrainCellType::UNKNOWN;
    }

    // Convert TerrainCellType to string
    inline const char* TerrainCellTypeToString(TerrainCellType type) {
        switch (type) {
            case TerrainCellType::PLAINS: return "plains";
            case TerrainCellType::FOREST: return "forest";
            case TerrainCellType::MOUNTAIN: return "mountain";
            case TerrainCellType::WATER: return "water";
            case TerrainCellType::HILLS: return "hills";
            case TerrainCellType::MARSH: return "marsh";
            case TerrainCellType::DESERT: return "desert";
            case TerrainCellType::TUNDRA: return "tundra";
            case TerrainCellType::BEACH: return "beach";
            case TerrainCellType::SNOW: return "snow";
            default: return "unknown";
        }
    }

} // namespace game::map
