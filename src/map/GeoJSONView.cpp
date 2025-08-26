#include "map/GeoJSONView.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>

namespace map {

    using json = nlohmann::json;

    static inline ImVec2 project(float lon, float lat) { return ImVec2(lon, lat); }

    ImU32 GeoJSONView::ownerColor(int idx, int alpha) const {
        if (idx < 0) return IM_COL32(200, 200, 200, alpha);
        unsigned h = 2166136261u ^ (unsigned)idx; h = (h ^ (h >> 13)) * 1274126177u;
        int r = 80 + (h & 127), g = 80 + ((h >> 8) & 127), b = 80 + ((h >> 16) & 127);
        return IM_COL32(r, g, b, alpha);
    }

    const char* GeoJSONView::provinceName(int polyIndex) const {
        if (polyIndex < 0 || polyIndex >= (int)polys_.size()) return "";
        return polys_[polyIndex].name.c_str();
    }

    bool GeoJSONView::load(const char* path) {
        loaded_ = false; error_.clear();
        polys_.clear(); owners_.clear(); shapes_.clear();
        minLon = 1e9f; minLat = 1e9f; maxLon = -1e9f; maxLat = -1e9f;

        std::ifstream f(path, std::ios::binary);
        if (!f) { error_ = "Cannot open GeoJSON"; return false; }

        json root;
        try {
            f >> root;
        }
        catch (const std::exception&) {
            error_ = "JSON parse failed";
            return false;
        }

        if (!root.is_object() || !root.contains("features") || !root["features"].is_array()) {
            error_ = "No features[]";
            return false;
        }

        auto ownerIndexOf = [&](const std::string& name)->int {
            if (name.empty()) return -1;
            for (size_t i = 0; i < owners_.size(); ++i) if (owners_[i].name == name) return (int)i;
            owners_.push_back({ name }); return (int)owners_.size() - 1;
            };

        auto parse_ring = [&](const json& ringv) {
            std::vector<ImVec2> pts;
            if (!ringv.is_array()) return pts;
            for (const auto& ptv : ringv) {
                if (!ptv.is_array() || ptv.size() < 2) continue;
                float lon = (float)ptv[0].get<double>();
                float lat = (float)ptv[1].get<double>();
                ImVec2 p = project(lon, lat);
                pts.push_back(p);
                minLon = std::min(minLon, p.x); maxLon = std::max(maxLon, p.x);
                minLat = std::min(minLat, p.y); maxLat = std::max(maxLat, p.y);
            }
            return pts;
            };

        const auto& feats = root["features"];
        for (const auto& feat : feats) {
            if (!feat.is_object()) continue;

            const json& props = feat.value("properties", json::object());
            std::string owner = props.value("owner_name", std::string());
            std::string pname = props.value("name", std::string());
            int oidx = ownerIndexOf(owner);

            const json& geom = feat.value("geometry", json::object());
            std::string gtype = geom.value("type", std::string());
            if (gtype.empty() || !geom.contains("coordinates")) continue;
            const json& coords = geom["coordinates"];

            if (gtype == "Polygon") {
                if (!coords.is_array() || coords.empty()) continue;
                auto exterior = parse_ring(coords[0]);
                if (exterior.size() >= 3) { PolyLL P; P.ll = std::move(exterior); P.owner = oidx; P.name = pname; polys_.push_back(std::move(P)); }
            }
            else if (gtype == "MultiPolygon") {
                if (!coords.is_array()) continue;
                for (const auto& polyv : coords) {
                    if (!polyv.is_array() || polyv.empty()) continue;
                    auto exterior = parse_ring(polyv[0]);
                    if (exterior.size() >= 3) { PolyLL P; P.ll = std::move(exterior); P.owner = oidx; P.name = pname; polys_.push_back(std::move(P)); }
                }
            }
        }

        loaded_ = !polys_.empty();
        if (!loaded_) error_ = "No polygons found";
        lastOrigin_ = ImVec2(0, 0); lastSize_ = ImVec2(0, 0); lastZoom_ = 0.0f; lastPan_ = ImVec2(1e9f, 1e9f); // force recompute on first fit
        return loaded_;
    }

    void GeoJSONView::fitTo(const ImVec2& origin, const ImVec2& size, float zoom, ImVec2 pan) {
        if (!loaded_) return;
        if (origin.x == lastOrigin_.x && origin.y == lastOrigin_.y &&
            size.x == lastSize_.x && size.y == lastSize_.y &&
            zoom == lastZoom_ && pan.x == lastPan_.x && pan.y == lastPan_.y) {
            return; // no change
        }
        lastOrigin_ = origin; lastSize_ = size; lastZoom_ = zoom; lastPan_ = pan;
        shapes_.clear();
        float dx = (maxLon - minLon);
        float dy = (maxLat - minLat);
        if (dx <= 0 || dy <= 0) return;
        float sx = size.x / dx, sy = size.y / dy, s = std::min(sx, sy) * zoom;
        ImVec2 off(origin.x + (size.x - s * dx) * 0.5f + pan.x, origin.y + (size.y - s * dy) * 0.5f + pan.y);
        shapes_.reserve(polys_.size());
        for (int i = 0; i < (int)polys_.size(); ++i) {
            const auto& P = polys_[i];
            Shape S; S.ownerIndex = P.owner; S.polyIndex = i;
            S.points.reserve(P.ll.size());
            for (const auto& q : P.ll) {
                float x = off.x + (q.x - minLon) * s;
                float y = off.y + (maxLat - q.y) * s;
                S.points.push_back(ImVec2(x, y));
            }
            shapes_.push_back(std::move(S));
        }
    }

    void GeoJSONView::draw(ImDrawList* dl, float alphaFill, float wire) const {
        if (shapes_.empty()) return;
        for (const auto& S : shapes_) {
            if (S.points.size() < 3) continue;
            dl->AddConvexPolyFilled(S.points.data(), (int)S.points.size(), ownerColor(S.ownerIndex, (int)(alphaFill * 255)));
            dl->AddPolyline(S.points.data(), (int)S.points.size(), IM_COL32(0, 0, 0, (int)(wire * 255)), true, 1.0f);
        }
    }

    void GeoJSONView::drawSelection(ImDrawList* dl, int polyIndex, float thickness) const {
        if (polyIndex < 0) return;
        for (const auto& S : shapes_) {
            if (S.polyIndex != polyIndex) continue;
            if (S.points.size() < 3) continue;
            dl->AddPolyline(S.points.data(), (int)S.points.size(), IM_COL32(255, 255, 0, 255), true, thickness);
            break;
        }
    }

    bool GeoJSONView::pointInPolygon(const std::vector<ImVec2>& poly, ImVec2 p) {
        bool inside = false;
        int n = (int)poly.size();
        for (int i = 0, j = n - 1; i < n; j = i++) {
            const ImVec2& a = poly[i];
            const ImVec2& b = poly[j];
            bool intersect = ((a.y > p.y) != (b.y > p.y)) &&
                (p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y + 1e-6f) + a.x);
            if (intersect) inside = !inside;
        }
        return inside;
    }

    int GeoJSONView::pick(ImVec2 p) const {
        // iterate back-to-front (last drawn on top). Here we draw in order, so check reverse.
        for (int k = (int)shapes_.size() - 1; k >= 0; --k) {
            const auto& S = shapes_[k];
            if (S.points.size() < 3) continue;
            if (pointInPolygon(S.points, p)) return S.polyIndex;
        }
        return -1;
    }

} // namespace map
