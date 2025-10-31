// ============================================================================
// ProvinceGeometry.cpp - Province Geometry Operations Implementation
// Mechanica Imperii
// ============================================================================

#include "map/ProvinceGeometry.h"
#include "map/GeographicUtils.h"
#include <cmath>

namespace game {
    namespace map {

        bool ProvinceGeometry::IsValidBoundary(const std::vector<Coordinate>& boundary) {
            return boundary.size() >= 3;
        }

        void ProvinceGeometry::FixBoundaryWinding(std::vector<Coordinate>& boundary) {
            // Stub: Calculate winding order and reverse if needed
            // For now, assume boundary is correct
        }

        bool ProvinceGeometry::AreNeighbors(const std::vector<Coordinate>& province1,
                                           const std::vector<Coordinate>& province2,
                                           double tolerance) {
            // Stub: Check if provinces share an edge
            // Simplified check: if any points are within tolerance
            for (const auto& p1 : province1) {
                for (const auto& p2 : province2) {
                    if (GeoUtils::CalculateDistance(p1, p2) < tolerance) {
                        return true;
                    }
                }
            }
            return false;
        }

        std::vector<Coordinate> ProvinceGeometry::GetSharedBorder(const std::vector<Coordinate>& province1,
                                                                  const std::vector<Coordinate>& province2,
                                                                  double tolerance) {
            // Stub: Find shared edge points
            std::vector<Coordinate> shared;
            for (const auto& p1 : province1) {
                for (const auto& p2 : province2) {
                    if (GeoUtils::CalculateDistance(p1, p2) < tolerance) {
                        shared.push_back(p1);
                    }
                }
            }
            return shared;
        }

        std::vector<uint32_t> ProvinceGeometry::Triangulate(const std::vector<Coordinate>& boundary) {
            // Stub: Ear clipping or similar triangulation algorithm would go here
            // For now, return simple fan triangulation
            std::vector<uint32_t> indices;
            if (boundary.size() < 3) return indices;

            for (size_t i = 1; i < boundary.size() - 1; ++i) {
                indices.push_back(0);
                indices.push_back(static_cast<uint32_t>(i));
                indices.push_back(static_cast<uint32_t>(i + 1));
            }
            return indices;
        }

        std::vector<Coordinate> ProvinceGeometry::SimplifyForLOD(const std::vector<Coordinate>& boundary,
                                                                 RenderLevel lod) {
            double tolerance = CalculateSimplificationTolerance(lod);
            return GeoUtils::SimplifyPolygon(boundary, tolerance);
        }

        bool ProvinceGeometry::IsConvex(const std::vector<Coordinate>& boundary) {
            // Stub: Check if polygon is convex
            if (boundary.size() < 3) return false;
            return false; // Assume not convex for now
        }

        bool ProvinceGeometry::IsSelfIntersecting(const std::vector<Coordinate>& boundary) {
            // Stub: Check for self-intersections
            // For now, assume no self-intersections
            return false;
        }

        double ProvinceGeometry::CalculateSimplificationTolerance(RenderLevel lod) {
            switch (lod) {
                case RenderLevel::LOD0: return 0.1;
                case RenderLevel::LOD1: return 0.05;
                case RenderLevel::LOD2: return 0.02;
                case RenderLevel::LOD3: return 0.01;
                case RenderLevel::LOD4: return 0.005;
                default: return 0.02;
            }
        }

    } // namespace map
} // namespace game
