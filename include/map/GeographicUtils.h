// ============================================================================
// GeographicUtils.h - Geographic Utility Functions
// Mechanica Imperii - Coordinate conversions and geographic calculations
// ============================================================================

#pragma once

#include "MapData.h"
#include <vector>
#include <utility>

namespace game {
    namespace map {

        // ============================================================================
        // Geographic Utilities
        // ============================================================================

        namespace GeoUtils {
            // Distance calculations
            double CalculateDistance(const Coordinate& a, const Coordinate& b);
            double CalculateDistanceSquared(const Coordinate& a, const Coordinate& b);

            // Area calculations
            double CalculatePolygonArea(const std::vector<Coordinate>& polygon);

            // Centroid calculations
            Coordinate CalculateCentroid(const std::vector<Coordinate>& points);
            Coordinate CalculatePolygonCentroid(const std::vector<Coordinate>& polygon);

            // Bounding box calculations
            BoundingBox CalculateBoundingBox(const std::vector<Coordinate>& points);

            // Point-in-polygon test
            bool PointInPolygon(const Coordinate& point, const std::vector<Coordinate>& polygon);

            // Coordinate transformations
            Coordinate LatLonToGame(double latitude, double longitude,
                                   double min_lat, double max_lat,
                                   double min_lon, double max_lon,
                                   double game_width, double game_height);
            std::pair<double, double> GameToLatLon(const Coordinate& game_coord,
                                                    double min_lat, double max_lat,
                                                    double min_lon, double max_lon,
                                                    double game_width, double game_height);

            // Polygon simplification
            std::vector<Coordinate> SimplifyPolygon(const std::vector<Coordinate>& polygon,
                                                    double tolerance);
        }

    } // namespace map
} // namespace game
