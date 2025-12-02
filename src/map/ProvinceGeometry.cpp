// ============================================================================
// ProvinceGeometry.cpp - Province Geometry Operations Implementation
// Mechanica Imperii
// ============================================================================

#include "map/ProvinceGeometry.h"
#include "map/GeographicUtils.h"
#include <cmath>
#include <algorithm>

namespace game {
    namespace map {

        // ====================================================================
        // Helper Functions for Line Segment Geometry
        // ====================================================================

        namespace {
            // Calculate cross product of vectors (p1-p0) and (p2-p0)
            double CrossProduct(const Coordinate& p0, const Coordinate& p1, const Coordinate& p2) {
                return (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x);
            }

            // Check if point p lies on line segment (a, b) within tolerance
            bool PointOnSegment(const Coordinate& p, const Coordinate& a, const Coordinate& b, double tolerance) {
                // Check if p is within bounding box of segment with tolerance
                double min_x = std::min(a.x, b.x) - tolerance;
                double max_x = std::max(a.x, b.x) + tolerance;
                double min_y = std::min(a.y, b.y) - tolerance;
                double max_y = std::max(a.y, b.y) + tolerance;

                if (p.x < min_x || p.x > max_x || p.y < min_y || p.y > max_y) {
                    return false;
                }

                // Check if point is collinear with segment
                double cross = std::abs(CrossProduct(a, b, p));
                double segment_length = GeoUtils::CalculateDistance(a, b);

                // Point is on segment if cross product is near zero (collinear)
                // and point is within bounding box
                return cross <= tolerance * segment_length;
            }

            // Check if two line segments intersect or touch
            // Returns true if segments share any point or overlap
            bool SegmentsIntersect(const Coordinate& a1, const Coordinate& a2,
                                  const Coordinate& b1, const Coordinate& b2,
                                  double tolerance) {
                // Quick bounding box check for optimization
                double a_min_x = std::min(a1.x, a2.x) - tolerance;
                double a_max_x = std::max(a1.x, a2.x) + tolerance;
                double a_min_y = std::min(a1.y, a2.y) - tolerance;
                double a_max_y = std::max(a1.y, a2.y) + tolerance;

                double b_min_x = std::min(b1.x, b2.x) - tolerance;
                double b_max_x = std::max(b1.x, b2.x) + tolerance;
                double b_min_y = std::min(b1.y, b2.y) - tolerance;
                double b_max_y = std::max(b1.y, b2.y) + tolerance;

                // Bounding boxes don't intersect
                if (a_max_x < b_min_x || a_min_x > b_max_x ||
                    a_max_y < b_min_y || a_min_y > b_max_y) {
                    return false;
                }

                // Check if endpoints of one segment touch the other segment
                if (PointOnSegment(a1, b1, b2, tolerance) ||
                    PointOnSegment(a2, b1, b2, tolerance) ||
                    PointOnSegment(b1, a1, a2, tolerance) ||
                    PointOnSegment(b2, a1, a2, tolerance)) {
                    return true;
                }

                // Check if segments actually intersect using cross products
                double d1 = CrossProduct(b1, b2, a1);
                double d2 = CrossProduct(b1, b2, a2);
                double d3 = CrossProduct(a1, a2, b1);
                double d4 = CrossProduct(a1, a2, b2);

                // Segments intersect if points are on opposite sides of each other
                if (((d1 > tolerance && d2 < -tolerance) || (d1 < -tolerance && d2 > tolerance)) &&
                    ((d3 > tolerance && d4 < -tolerance) || (d3 < -tolerance && d4 > tolerance))) {
                    return true;
                }

                return false;
            }

            // Calculate the length of overlap between two collinear segments
            double CalculateOverlapLength(const Coordinate& a1, const Coordinate& a2,
                                         const Coordinate& b1, const Coordinate& b2,
                                         double tolerance) {
                // Project all points onto a common axis (use the longer segment's direction)
                double dx = a2.x - a1.x;
                double dy = a2.y - a1.y;
                double length = std::sqrt(dx * dx + dy * dy);

                if (length < tolerance) {
                    return 0.0;
                }

                // Normalize direction
                dx /= length;
                dy /= length;

                // Project all points onto this axis
                double t_a1 = 0.0;
                double t_a2 = length;
                double t_b1 = (b1.x - a1.x) * dx + (b1.y - a1.y) * dy;
                double t_b2 = (b2.x - a1.x) * dx + (b2.y - a1.y) * dy;

                // Sort projections of b segment
                if (t_b1 > t_b2) {
                    std::swap(t_b1, t_b2);
                }

                // Calculate overlap
                double overlap_start = std::max(t_a1, t_b1);
                double overlap_end = std::min(t_a2, t_b2);

                if (overlap_end > overlap_start) {
                    return overlap_end - overlap_start;
                }

                return 0.0;
            }
        }

        bool ProvinceGeometry::IsValidBoundary(const std::vector<Coordinate>& boundary) {
            return boundary.size() >= 3;
        }

        void ProvinceGeometry::FixBoundaryWinding(std::vector<Coordinate>& boundary) {
            // Calculate the signed area to determine winding order
            if (boundary.size() < 3) return;

            double signed_area = 0.0;
            for (size_t i = 0; i < boundary.size(); ++i) {
                const Coordinate& p1 = boundary[i];
                const Coordinate& p2 = boundary[(i + 1) % boundary.size()];
                signed_area += (p2.x - p1.x) * (p2.y + p1.y);
            }

            // If signed area is negative, boundary is clockwise - reverse it for counter-clockwise
            if (signed_area < 0.0) {
                std::reverse(boundary.begin(), boundary.end());
            }
        }

        bool ProvinceGeometry::AreNeighbors(const std::vector<Coordinate>& province1,
                                           const std::vector<Coordinate>& province2,
                                           double tolerance) {
            if (province1.size() < 3 || province2.size() < 3) {
                return false;
            }

            // Check if any edge from province1 intersects/touches any edge from province2
            for (size_t i = 0; i < province1.size(); ++i) {
                const Coordinate& p1_start = province1[i];
                const Coordinate& p1_end = province1[(i + 1) % province1.size()];

                for (size_t j = 0; j < province2.size(); ++j) {
                    const Coordinate& p2_start = province2[j];
                    const Coordinate& p2_end = province2[(j + 1) % province2.size()];

                    if (SegmentsIntersect(p1_start, p1_end, p2_start, p2_end, tolerance)) {
                        return true;
                    }
                }
            }

            return false;
        }

        std::vector<Coordinate> ProvinceGeometry::GetSharedBorder(const std::vector<Coordinate>& province1,
                                                                  const std::vector<Coordinate>& province2,
                                                                  double tolerance) {
            std::vector<Coordinate> shared;

            if (province1.size() < 3 || province2.size() < 3) {
                return shared;
            }

            // Find all points where edges intersect or touch
            for (size_t i = 0; i < province1.size(); ++i) {
                const Coordinate& p1_start = province1[i];
                const Coordinate& p1_end = province1[(i + 1) % province1.size()];

                for (size_t j = 0; j < province2.size(); ++j) {
                    const Coordinate& p2_start = province2[j];
                    const Coordinate& p2_end = province2[(j + 1) % province2.size()];

                    if (SegmentsIntersect(p1_start, p1_end, p2_start, p2_end, tolerance)) {
                        // Check if segments overlap (collinear)
                        double overlap = CalculateOverlapLength(p1_start, p1_end, p2_start, p2_end, tolerance);

                        if (overlap > tolerance) {
                            // Segments overlap - add the overlapping points
                            // For simplicity, add endpoints that are on the other segment
                            if (PointOnSegment(p1_start, p2_start, p2_end, tolerance)) {
                                shared.push_back(p1_start);
                            }
                            if (PointOnSegment(p1_end, p2_start, p2_end, tolerance)) {
                                shared.push_back(p1_end);
                            }
                            if (PointOnSegment(p2_start, p1_start, p1_end, tolerance)) {
                                shared.push_back(p2_start);
                            }
                            if (PointOnSegment(p2_end, p1_start, p1_end, tolerance)) {
                                shared.push_back(p2_end);
                            }
                        } else {
                            // Segments just touch at a point - add shared endpoint
                            if (GeoUtils::CalculateDistance(p1_start, p2_start) < tolerance) {
                                shared.push_back(p1_start);
                            } else if (GeoUtils::CalculateDistance(p1_start, p2_end) < tolerance) {
                                shared.push_back(p1_start);
                            } else if (GeoUtils::CalculateDistance(p1_end, p2_start) < tolerance) {
                                shared.push_back(p1_end);
                            } else if (GeoUtils::CalculateDistance(p1_end, p2_end) < tolerance) {
                                shared.push_back(p1_end);
                            }
                        }
                    }
                }
            }

            // Remove duplicate points (within tolerance)
            std::vector<Coordinate> unique_shared;
            for (const auto& point : shared) {
                bool is_duplicate = false;
                for (const auto& existing : unique_shared) {
                    if (GeoUtils::CalculateDistance(point, existing) < tolerance) {
                        is_duplicate = true;
                        break;
                    }
                }
                if (!is_duplicate) {
                    unique_shared.push_back(point);
                }
            }

            return unique_shared;
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
