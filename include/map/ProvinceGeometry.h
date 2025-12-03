// ============================================================================
// ProvinceGeometry.h - Province Geometry Operations
// Mechanica Imperii - Geometric operations for provinces
// ============================================================================

#pragma once

#include "MapData.h"
#include <vector>

namespace game {
    namespace map {

        // ============================================================================
        // Province Geometry Operations
        // ============================================================================

        // Result of adjacency check with border length
        struct AdjacencyResult {
            bool are_neighbors = false;
            double border_length = 0.0;
        };

        class ProvinceGeometry {
        public:
            // Boundary operations
            static bool IsValidBoundary(const std::vector<Coordinate>& boundary);
            static void FixBoundaryWinding(std::vector<Coordinate>& boundary);
            static void RemoveDuplicatePoints(std::vector<Coordinate>& boundary, double tolerance = 0.001);

            // Neighbor detection (fast check, returns bool only)
            // Use when you only need to know if provinces are neighbors (yes/no)
            // Can return early after finding first intersection
            static bool AreNeighbors(const std::vector<Coordinate>& province1,
                                    const std::vector<Coordinate>& province2,
                                    double tolerance = 0.001);

            // Get shared border points between provinces
            static std::vector<Coordinate> GetSharedBorder(const std::vector<Coordinate>& province1,
                                                          const std::vector<Coordinate>& province2,
                                                          double tolerance = 0.001);

            // Calculate total length of shared border
            // Use when you already know provinces are neighbors
            static double CalculateBorderLength(const std::vector<Coordinate>& province1,
                                               const std::vector<Coordinate>& province2,
                                               double tolerance = 0.001);

            // Combined adjacency check and border length calculation (RECOMMENDED)
            // Computes neighbor status AND border length in a single pass (2x faster than calling both)
            // Use when you need border length for influence/diplomacy calculations
            // Returns: {are_neighbors: bool, border_length: double}
            static AdjacencyResult CheckAdjacency(const std::vector<Coordinate>& province1,
                                                 const std::vector<Coordinate>& province2,
                                                 double tolerance = 0.001);

            // Triangulation
            static std::vector<uint32_t> Triangulate(const std::vector<Coordinate>& boundary);

            // Simplification for LOD
            static std::vector<Coordinate> SimplifyForLOD(const std::vector<Coordinate>& boundary,
                                                         RenderLevel lod);

            // Geometry validation
            static bool IsConvex(const std::vector<Coordinate>& boundary);
            static bool IsSelfIntersecting(const std::vector<Coordinate>& boundary);

        private:
            static double CalculateSimplificationTolerance(RenderLevel lod);
        };

    } // namespace map
} // namespace game
