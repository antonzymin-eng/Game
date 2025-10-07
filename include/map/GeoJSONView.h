
#pragma once
#include <vector>
#include <string>
#include "imgui.h"

namespace map {

struct Shape {
    std::vector<ImVec2> points; // screen-space after fitTo
    int ownerIndex = -1;
    int polyIndex = -1; // index into model polys_
};

struct Owner { std::string name; };

class GeoJSONView {
public:
    bool load(const char* path);
    void fitTo(const ImVec2& origin, const ImVec2& size, float zoom=1.0f, ImVec2 pan=ImVec2(0,0));
    void draw(ImDrawList* dl, float alphaFill = 0.6f, float wire = 0.6f) const;
    void drawSelection(ImDrawList* dl, int polyIndex, float thickness = 2.0f) const;
    bool isLoaded() const { return loaded_; }
    const char* lastError() const { return error_.c_str(); }

    // Picking in screen space. Returns polyIndex or -1.
    int pick(ImVec2 p) const;

    // Legend helpers
    int ownersCount() const { return (int)owners_.size(); }
    const char* ownerName(int idx) const { return (idx>=0 && idx<(int)owners_.size()) ? owners_[idx].name.c_str() : ""; }
    ImU32 ownerColor(int idx, int alpha = 255) const;

    // Province info
    const char* provinceName(int polyIndex) const;

private:
    bool loaded_ = false;
    std::string error_;
    struct PolyLL { std::vector<ImVec2> ll; int owner=-1; std::string name; };
    std::vector<PolyLL> polys_;
    std::vector<Owner> owners_;
    float minLon=  1e9f, minLat=  1e9f, maxLon= -1e9f, maxLat= -1e9f;

    // cache
    std::vector<Shape> shapes_;
    ImVec2 lastOrigin_ = ImVec2(0,0);
    ImVec2 lastSize_   = ImVec2(0,0);
    float  lastZoom_   = 1.0f;
    ImVec2 lastPan_    = ImVec2(0,0);

    static bool pointInPolygon(const std::vector<ImVec2>& poly, ImVec2 p);
};

} // namespace map
