// ============================================================================
// MapData.h - Core Map Data Structures
// Mechanica Imperii - Basic geographic data structures
// ============================================================================

#pragma once

#include "core/types/game_types.h"
#include <vector>
#include <string>
#include <cstdint>

namespace game {
    namespace map {

        // ============================================================================
        // Basic Types
        // ============================================================================

        struct Coordinate {
            double x = 0.0;
            double y = 0.0;

            Coordinate() = default;
            Coordinate(double x_, double y_) : x(x_), y(y_) {}
        };

        struct BoundingBox {
            double min_x = 0.0;
            double min_y = 0.0;
            double max_x = 0.0;
            double max_y = 0.0;

            BoundingBox() = default;
            BoundingBox(double min_x_, double min_y_, double max_x_, double max_y_)
                : min_x(min_x_), min_y(min_y_), max_x(max_x_), max_y(max_y_) {}

            double GetWidth() const { return max_x - min_x; }
            double GetHeight() const { return max_y - min_y; }
            Coordinate GetCenter() const { return Coordinate((min_x + max_x) / 2.0, (min_y + max_y) / 2.0); }

            bool Contains(double x, double y) const {
                return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
            }

            bool Intersects(const BoundingBox& other) const {
                return !(max_x < other.min_x || min_x > other.max_x ||
                         max_y < other.min_y || min_y > other.max_y);
            }

            bool operator==(const BoundingBox& other) const {
                return min_x == other.min_x && min_y == other.min_y &&
                       max_x == other.max_x && max_y == other.max_y;
            }

            bool operator!=(const BoundingBox& other) const {
                return !(*this == other);
            }
        };

        // ============================================================================
        // Terrain and Climate Types
        // ============================================================================

        enum class TerrainType : uint8_t {
            PLAINS = 0,
            HILLS,
            MOUNTAINS,
            FOREST,
            DESERT,
            COAST,
            WETLAND,
            HIGHLANDS,
            UNKNOWN
        };

        enum class ClimateZone : uint8_t {
            ARCTIC = 0,
            SUBARCTIC,
            TEMPERATE,
            SUBTROPICAL,
            TROPICAL,
            ARID,
            SEMIARID,
            MEDITERRANEAN,
            UNKNOWN
        };

        // ============================================================================
        // Render Level
        // ============================================================================

        enum class RenderLevel : uint8_t {
            LOD0 = 0,  // Strategic view (0-20% zoom)
            LOD1 = 1,  // Operational view (20-40% zoom)
            LOD2 = 2,  // Tactical view (40-60% zoom)
            LOD3 = 3,  // Detailed view (60-80% zoom)
            LOD4 = 4   // Maximum detail (80-100% zoom)
        };

        // ============================================================================
        // Camera
        // ============================================================================

        struct Camera {
            Coordinate position;
            double zoom = 1.0;
            double rotation = 0.0;
            int viewport_width = 1280;
            int viewport_height = 720;

            Camera() = default;
        };

        // ============================================================================
        // Province Data
        // ============================================================================

        struct ProvinceData {
            uint32_t id = 0;
            std::string name;
            std::vector<Coordinate> boundary;
            Coordinate center;
            BoundingBox bounds;
            uint32_t owner_id = 0;
            TerrainType terrain = TerrainType::PLAINS;
            ClimateZone climate = ClimateZone::TEMPERATE;
            std::vector<uint32_t> neighbors;
            bool is_coastal = false;
            bool has_river = false;

            ProvinceData() = default;
        };

    } // namespace map

    // Compatibility namespace for types
    namespace types {
        using SettlementType = uint8_t;
    }

} // namespace game
