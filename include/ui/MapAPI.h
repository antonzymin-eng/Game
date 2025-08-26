#pragma once
#include "imgui.h"

namespace ui {

struct MapInfo {
    ImTextureID texture = (ImTextureID)0;
    int         width   = 0;
    int         height  = 0;
    bool        loaded  = false;
};

struct ProvinceSummary {
    int         id       = -1;
    const char* name     = "";
    const char* owner    = "";
    const char* terrain  = "";
    ImVec2      centroid = ImVec2(0, 0);
};

// Camera & view
MapInfo     MapGetInfo();
float       MapGetZoom();
ImVec2      MapGetPan();
void        MapSetPan(ImVec2);
void        MapSetZoom(float);
void        MapCenterOnImagePixel(int cx, int cy, ImVec2 viewportSize);
void        MapCenterOnProvince(int provinceId, ImVec2 viewportSize);

// Provinces
int         MapGetProvinceCount();
int         MapGetProvinceListSize();
int         MapGetProvinceIdAt(int index);
inline int  MapGetNthProvinceId(int index) { return MapGetProvinceIdAt(index); }
bool        MapGetProvinceSummary(int id, ProvinceSummary* out);

// Settlements (placeholder)
int         MapGetSettlementCount(int provinceId);
bool        MapGetSettlement(int provinceId, int idx, ImVec2* px, const char** label);

// --- NEW: Province metadata provider hook ---
// If set, this function will be called to enrich ProvinceSummary (name/owner/terrain/etc.).
// Return true if you filled fields; false to leave defaults.
using ProvinceMetaProvider = bool(*)(int id, ProvinceSummary* out);
void        SetProvinceMetaProvider(ProvinceMetaProvider fn);

} // namespace ui
