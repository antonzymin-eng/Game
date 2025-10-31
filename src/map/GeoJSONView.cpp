// ============================================================================
// GeoJSONView.cpp - ImGui GeoJSON Viewer Implementation
// Mechanica Imperii
// ============================================================================

#include "map/GeoJSONView.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <json/json.h>

namespace map {

    bool GeoJSONView::load(const char* path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            error_ = "Failed to open file";
            return false;
        }

        try {
            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(file, root)) {
                error_ = reader.getFormattedErrorMessages();
                return false;
            }

            if (!root.isMember("features") || !root["features"].isArray()) {
                error_ = "Invalid GeoJSON: missing features array";
                return false;
            }

            polys_.clear();
            owners_.clear();

            for (const auto& feature : root["features"]) {
                if (!feature.isMember("geometry")) continue;

                PolyLL poly;

                // Extract properties
                if (feature.isMember("properties")) {
                    const auto& props = feature["properties"];
                    if (props.isMember("name") && props["name"].isString()) {
                        poly.name = props["name"].asString();
                    }
                    if (props.isMember("owner") && props["owner"].isString()) {
                        std::string owner_name = props["owner"].asString();
                        // Find or create owner
                        poly.owner = -1;
                        for (size_t i = 0; i < owners_.size(); ++i) {
                            if (owners_[i].name == owner_name) {
                                poly.owner = static_cast<int>(i);
                                break;
                            }
                        }
                        if (poly.owner == -1) {
                            poly.owner = static_cast<int>(owners_.size());
                            owners_.push_back(Owner{owner_name});
                        }
                    }
                }

                // Parse geometry
                const auto& geom = feature["geometry"];
                if (geom.isMember("type") && geom.isMember("coordinates")) {
                    std::string geom_type = geom["type"].asString();
                    const auto& coords = geom["coordinates"];

                    if (geom_type == "Polygon" && coords.isArray() && !coords.empty()) {
                        // Parse first ring (outer boundary)
                        const auto& ring = coords[0];
                        for (const auto& point : ring) {
                            if (point.isArray() && point.size() >= 2) {
                                float lon = point[0].asFloat();
                                float lat = point[1].asFloat();
                                poly.ll.push_back(ImVec2(lon, lat));

                                // Update bounds
                                minLon = std::min(minLon, lon);
                                maxLon = std::max(maxLon, lon);
                                minLat = std::min(minLat, lat);
                                maxLat = std::max(maxLat, lat);
                            }
                        }
                    }
                }

                if (!poly.ll.empty()) {
                    polys_.push_back(poly);
                }
            }

            loaded_ = !polys_.empty();
            if (!loaded_) {
                error_ = "No valid polygons found";
            }

            return loaded_;

        } catch (const std::exception& e) {
            error_ = std::string("Exception: ") + e.what();
            return false;
        }
    }

    void GeoJSONView::fitTo(const ImVec2& origin, const ImVec2& size, float zoom, ImVec2 pan) {
        if (!loaded_) return;

        // Check if we need to rebuild cache
        if (origin.x != lastOrigin_.x || origin.y != lastOrigin_.y ||
            size.x != lastSize_.x || size.y != lastSize_.y ||
            zoom != lastZoom_ || pan.x != lastPan_.x || pan.y != lastPan_.y) {

            lastOrigin_ = origin;
            lastSize_ = size;
            lastZoom_ = zoom;
            lastPan_ = pan;

            shapes_.clear();

            float lonRange = maxLon - minLon;
            float latRange = maxLat - minLat;

            for (size_t i = 0; i < polys_.size(); ++i) {
                Shape shape;
                shape.ownerIndex = polys_[i].owner;
                shape.polyIndex = static_cast<int>(i);

                for (const auto& ll : polys_[i].ll) {
                    // Normalize to [0,1]
                    float nx = (ll.x - minLon) / lonRange;
                    float ny = 1.0f - (ll.y - minLat) / latRange; // Flip Y

                    // Apply zoom and pan
                    nx = (nx - 0.5f) * zoom + 0.5f + pan.x;
                    ny = (ny - 0.5f) * zoom + 0.5f + pan.y;

                    // Scale to screen space
                    float sx = origin.x + nx * size.x;
                    float sy = origin.y + ny * size.y;

                    shape.points.push_back(ImVec2(sx, sy));
                }

                shapes_.push_back(shape);
            }
        }
    }

    void GeoJSONView::draw(ImDrawList* dl, float alphaFill, float wire) const {
        if (!loaded_ || shapes_.empty()) return;

        for (const auto& shape : shapes_) {
            if (shape.points.size() < 3) continue;

            // Fill
            ImU32 fillColor = ownerColor(shape.ownerIndex, static_cast<int>(alphaFill * 255));
            dl->AddConvexPolyFilled(shape.points.data(), static_cast<int>(shape.points.size()), fillColor);

            // Border
            if (wire > 0.0f) {
                ImU32 borderColor = IM_COL32(50, 50, 50, static_cast<int>(wire * 255));
                dl->AddPolyline(shape.points.data(), static_cast<int>(shape.points.size()), borderColor, true, 1.0f);
            }
        }
    }

    void GeoJSONView::drawSelection(ImDrawList* dl, int polyIndex, float thickness) const {
        if (polyIndex < 0 || polyIndex >= static_cast<int>(shapes_.size())) return;

        const auto& shape = shapes_[polyIndex];
        if (shape.points.size() < 3) return;

        ImU32 highlightColor = IM_COL32(255, 255, 0, 255);
        dl->AddPolyline(shape.points.data(), static_cast<int>(shape.points.size()), highlightColor, true, thickness);
    }

    int GeoJSONView::pick(ImVec2 p) const {
        for (const auto& shape : shapes_) {
            if (pointInPolygon(shape.points, p)) {
                return shape.polyIndex;
            }
        }
        return -1;
    }

    ImU32 GeoJSONView::ownerColor(int idx, int alpha) const {
        if (idx < 0 || idx >= static_cast<int>(owners_.size())) {
            return IM_COL32(150, 150, 150, alpha);
        }

        // Generate pseudo-random color based on index
        int r = ((idx * 123) % 180) + 75;
        int g = ((idx * 456) % 180) + 75;
        int b = ((idx * 789) % 180) + 75;

        return IM_COL32(r, g, b, alpha);
    }

    const char* GeoJSONView::provinceName(int polyIndex) const {
        if (polyIndex >= 0 && polyIndex < static_cast<int>(polys_.size())) {
            return polys_[polyIndex].name.c_str();
        }
        return "";
    }

    bool GeoJSONView::pointInPolygon(const std::vector<ImVec2>& poly, ImVec2 p) {
        if (poly.size() < 3) return false;

        bool inside = false;
        for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
            float xi = poly[i].x, yi = poly[i].y;
            float xj = poly[j].x, yj = poly[j].y;

            bool intersect = ((yi > p.y) != (yj > p.y)) &&
                            (p.x < (xj - xi) * (p.y - yi) / (yj - yi) + xi);
            if (intersect) inside = !inside;
        }
        return inside;
    }

} // namespace map
