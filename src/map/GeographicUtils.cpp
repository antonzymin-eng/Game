// ============================================================================
// GeographicUtils.cpp - Geographic Utility Functions Implementation
// Mechanica Imperii
// ============================================================================

#include "map/GeographicUtils.h"
#include <cmath>
#include <algorithm>

namespace game {
    namespace map {
        namespace GeoUtils {

            double CalculateDistance(const Coordinate& a, const Coordinate& b) {
                return std::sqrt(CalculateDistanceSquared(a, b));
            }

            double CalculateDistanceSquared(const Coordinate& a, const Coordinate& b) {
                double dx = b.x - a.x;
                double dy = b.y - a.y;
                return dx * dx + dy * dy;
            }

            double CalculatePolygonArea(const std::vector<Coordinate>& polygon) {
                if (polygon.size() < 3) return 0.0;

                double area = 0.0;
                for (size_t i = 0; i < polygon.size(); ++i) {
                    size_t j = (i + 1) % polygon.size();
                    area += polygon[i].x * polygon[j].y;
                    area -= polygon[j].x * polygon[i].y;
                }
                return std::abs(area) * 0.5;
            }

            Coordinate CalculateCentroid(const std::vector<Coordinate>& points) {
                if (points.empty()) return Coordinate();

                double sum_x = 0.0;
                double sum_y = 0.0;
                for (const auto& point : points) {
                    sum_x += point.x;
                    sum_y += point.y;
                }
                return Coordinate(sum_x / points.size(), sum_y / points.size());
            }

            Coordinate CalculatePolygonCentroid(const std::vector<Coordinate>& polygon) {
                if (polygon.size() < 3) return CalculateCentroid(polygon);

                double area = CalculatePolygonArea(polygon);
                if (area == 0.0) return CalculateCentroid(polygon);

                double cx = 0.0;
                double cy = 0.0;
                for (size_t i = 0; i < polygon.size(); ++i) {
                    size_t j = (i + 1) % polygon.size();
                    double factor = (polygon[i].x * polygon[j].y - polygon[j].x * polygon[i].y);
                    cx += (polygon[i].x + polygon[j].x) * factor;
                    cy += (polygon[i].y + polygon[j].y) * factor;
                }
                double divisor = 6.0 * area;
                return Coordinate(cx / divisor, cy / divisor);
            }

            BoundingBox CalculateBoundingBox(const std::vector<Coordinate>& points) {
                if (points.empty()) return BoundingBox();

                double min_x = points[0].x;
                double max_x = points[0].x;
                double min_y = points[0].y;
                double max_y = points[0].y;

                for (const auto& point : points) {
                    min_x = std::min(min_x, point.x);
                    max_x = std::max(max_x, point.x);
                    min_y = std::min(min_y, point.y);
                    max_y = std::max(max_y, point.y);
                }

                return BoundingBox(min_x, min_y, max_x, max_y);
            }

            bool PointInPolygon(const Coordinate& point, const std::vector<Coordinate>& polygon) {
                if (polygon.size() < 3) return false;

                bool inside = false;
                for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
                    double xi = polygon[i].x, yi = polygon[i].y;
                    double xj = polygon[j].x, yj = polygon[j].y;

                    bool intersect = ((yi > point.y) != (yj > point.y)) &&
                                    (point.x < (xj - xi) * (point.y - yi) / (yj - yi) + xi);
                    if (intersect) inside = !inside;
                }
                return inside;
            }

            Coordinate LatLonToGame(double latitude, double longitude,
                                   double min_lat, double max_lat,
                                   double min_lon, double max_lon,
                                   double game_width, double game_height) {
                double x = (longitude - min_lon) / (max_lon - min_lon) * game_width - game_width / 2.0;
                double y = (latitude - min_lat) / (max_lat - min_lat) * game_height - game_height / 2.0;
                return Coordinate(x, y);
            }

            std::pair<double, double> GameToLatLon(const Coordinate& game_coord,
                                                    double min_lat, double max_lat,
                                                    double min_lon, double max_lon,
                                                    double game_width, double game_height) {
                double lon = (game_coord.x + game_width / 2.0) / game_width * (max_lon - min_lon) + min_lon;
                double lat = (game_coord.y + game_height / 2.0) / game_height * (max_lat - min_lat) + min_lat;
                return {lat, lon};
            }

            std::vector<Coordinate> SimplifyPolygon(const std::vector<Coordinate>& polygon,
                                                    double tolerance) {
                // Stub: Douglas-Peucker algorithm would go here
                // For now, just return every nth point based on tolerance
                if (polygon.size() <= 3) return polygon;

                std::vector<Coordinate> simplified;
                int step = std::max(1, static_cast<int>(1.0 / tolerance));
                for (size_t i = 0; i < polygon.size(); i += step) {
                    simplified.push_back(polygon[i]);
                }
                // Ensure last point is included
                if (!simplified.empty() && simplified.back().x != polygon.back().x) {
                    simplified.push_back(polygon.back());
                }
                return simplified;
            }

        } // namespace GeoUtils
    } // namespace map
} // namespace game
