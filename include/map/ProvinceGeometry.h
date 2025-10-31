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

        class ProvinceGeometry {
        public:
            // Boundary operations
            static bool IsValidBoundary(const std::vector<Coordinate>& boundary);
            static void FixBoundaryWinding(std::vector<Coordinate>& boundary);

            // Neighbor detection
            static bool AreNeighbors(const std::vector<Coordinate>& province1,
                                    const std::vector<Coordinate>& province2,
                                    double tolerance = 0.001);
            static std::vector<Coordinate> GetSharedBorder(const std::vector<Coordinate>& province1,
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
