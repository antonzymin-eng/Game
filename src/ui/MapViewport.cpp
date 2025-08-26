#include "ui/MapViewport.h"
#include "ui/MapAPI.h"
#include "ScreenAPI.h"
#include "imgui.h"

#include <algorithm>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>

namespace ui {

// ---------------------------
// Camera / state
// ---------------------------
struct Camera2D {
    ImVec2 offset = ImVec2(0, 0);
    float  zoom   = 1.0f;
    ImVec2 world_size = ImVec2(4096, 2048);
};

static Camera2D g_cam;
static bool     g_cam_init = false;

static ImTextureID g_map_tex = (ImTextureID)0;
static float       g_right_reserve_px = 0.0f;
static ImVec2      g_last_viewport_size = ImVec2(0,0);

// Province picking buffers (optional index RGBA)
static std::vector<unsigned char> g_index_rgba;
static int   g_index_w = 0, g_index_h = 0, g_index_stride = 0;

// Placeholder province grid (when no index buffer)
static const int kGridCols = 16;
static const int kGridRows = 8;

// Hover / selection
static int  g_hover_province    = -1;
static int  g_selected_province = -1;

using ProvinceSelectedFn = void(*)(int);
static ProvinceSelectedFn g_on_select = nullptr;

// NEW: external metadata provider
static ProvinceMetaProvider g_meta_provider = nullptr;

// ---------------------------
// Helpers
// ---------------------------
static inline int colorToProvinceId(uint8_t r, uint8_t g, uint8_t b) {
    return int(r) | (int(g) << 8) | (int(b) << 16);
}

static inline ImVec2 worldToScreen(const Camera2D& c, ImVec2 w, ImVec2 origin) {
    return ImVec2(origin.x + (w.x * c.zoom) + c.offset.x,
                  origin.y + (w.y * c.zoom) + c.offset.y);
}

static inline ImVec2 screenToWorld(const Camera2D& c, ImVec2 s, ImVec2 origin) {
    return ImVec2((s.x - origin.x - c.offset.x) / c.zoom,
                  (s.y - origin.y - c.offset.y) / c.zoom);
}

static inline int pickProvinceFromIndexImage(int ix, int iy) {
    if (g_index_rgba.empty() || g_index_w <= 0 || g_index_h <= 0 || g_index_stride <= 0)
        return -1;
    ix = std::max(0, std::min(ix, g_index_w - 1));
    iy = std::max(0, std::min(iy, g_index_h - 1));
    const unsigned char* row = g_index_rgba.data() + (size_t)iy * (size_t)g_index_stride;
    const unsigned char* px  = row + (size_t)ix * 4;
    return colorToProvinceId(px[0], px[1], px[2]);
}

static inline int pickProvinceFromGrid(float wx, float wy) {
    // Synthetic grid IDs when no index buffer set
    float cellW = g_cam.world_size.x / (float)kGridCols;
    float cellH = g_cam.world_size.y / (float)kGridRows;
    int col = (int)std::floor(wx / cellW);
    int row = (int)std::floor(wy / cellH);
    if (col < 0 || col >= kGridCols || row < 0 || row >= kGridRows) return -1;
    return row * kGridCols + col;
}

// ---------------------------
// Public API from MapViewport.h
// ---------------------------
void SetMapTexture(ImTextureID tex, float width, float height) {
    g_map_tex = tex;
    if (width > 0 && height > 0) {
        g_cam.world_size = ImVec2(width, height);
    }
}

void SetRightPanelReserve(float px) { g_right_reserve_px = px > 0.0f ? px : 0.0f; }

void SetProvinceIndexBuffer(const unsigned char* rgba, int width, int height, int strideBytes) {
    if (!rgba || width <= 0 || height <= 0 || strideBytes < width * 4) {
        g_index_rgba.clear(); g_index_w = g_index_h = g_index_stride = 0; return;
    }
    g_index_w = width; g_index_h = height; g_index_stride = strideBytes;
    g_index_rgba.resize((size_t)height * (size_t)strideBytes);
    for (int y = 0; y < height; ++y) {
        std::memcpy(g_index_rgba.data() + (size_t)y * (size_t)strideBytes,
                    rgba + (size_t)y * (size_t)strideBytes,
                    (size_t)strideBytes);
    }
}

void SetProvinceSelectionCallback(ProvinceSelectedFn fn) { g_on_select = fn; }

int PickProvinceAtWorld(float worldX, float worldY, unsigned* outColorRGBA) {
    int id = -1;
    if (!g_index_rgba.empty()) {
        int ix = (int)std::round(worldX);
        int iy = (int)std::round(worldY);
        id = pickProvinceFromIndexImage(ix, iy);
        if (outColorRGBA && id >= 0) {
            const unsigned char* row = g_index_rgba.data() + (size_t)iy * (size_t)g_index_stride;
            const unsigned char* px  = row + (size_t)ix * 4;
            *outColorRGBA = (unsigned(px[0])      ) |
                            (unsigned(px[1]) <<  8) |
                            (unsigned(px[2]) << 16) |
                            (unsigned(px[3]) << 24);
        }
    } else {
        id = pickProvinceFromGrid(worldX, worldY);
        if (outColorRGBA) *outColorRGBA = 0;
    }
    return id;
}

int GetHoveredProvince() { return g_hover_province; }
int GetSelectedProvince() { return g_selected_province; }

bool tryLoadAssets() {
    // Placeholder: you can add io::FindAsset("map.png") here to auto-load.
    return false;
}

// ---------------------------
// MapAPI.h implementation
// ---------------------------
MapInfo MapGetInfo() {
    MapInfo info;
    info.texture = g_map_tex;
    info.width   = (int)g_cam.world_size.x;
    info.height  = (int)g_cam.world_size.y;
    info.loaded  = (g_map_tex != (ImTextureID)0);
    return info;
}

float  MapGetZoom() { return g_cam.zoom; }
ImVec2 MapGetPan()  { return g_cam.offset; }
void   MapSetPan(ImVec2 p)   { g_cam.offset = p; }
void   MapSetZoom(float z)   { g_cam.zoom = (z <= 0.05f ? 0.05f : (z > 32.0f ? 32.0f : z)); }

void MapCenterOnImagePixel(int cx, int cy, ImVec2 viewportSize) {
    // Center the camera so that (cx,cy) lands in the viewport center
    ImVec2 target = ImVec2((float)cx, (float)cy);
    ImVec2 wanted = ImVec2(viewportSize.x * 0.5f, viewportSize.y * 0.5f);
    g_cam.offset = ImVec2(wanted.x - target.x * g_cam.zoom,
                          wanted.y - target.y * g_cam.zoom);
}

void MapCenterOnProvince(int provinceId, ImVec2 viewportSize) {
    ProvinceSummary s;
    if (!MapGetProvinceSummary(provinceId, &s)) return;
    MapCenterOnImagePixel((int)std::round(s.centroid.x),
                          (int)std::round(s.centroid.y),
                          viewportSize);
}

int MapGetProvinceCount() {
    if (!g_index_rgba.empty()) {
        // If you want exact unique-count from the index map, build a set here.
        // Placeholder: return grid count until you decide to compute uniques.
    }
    return kGridCols * kGridRows;
}

int MapGetProvinceListSize() { return MapGetProvinceCount(); }

int MapGetProvinceIdAt(int index) {
    if (index < 0 || index >= MapGetProvinceListSize()) return -1;
    // Grid ID ordering: row-major
    return index;
}

bool MapGetProvinceSummary(int id, ProvinceSummary* out) {
    if (!out) return false;
    if (id < 0 || id >= kGridCols * kGridRows) { *out = {}; return false; }
    int row = id / kGridCols;
    int col = id % kGridCols;
    float cellW = g_cam.world_size.x / (float)kGridCols;
    float cellH = g_cam.world_size.y / (float)kGridRows;

    // Defaults
    out->id       = id;
    out->name     = "";
    out->owner    = "";
    out->terrain  = "";
    out->centroid = ImVec2((col + 0.5f) * cellW, (row + 0.5f) * cellH);

    // If external metadata provider is set, let it enrich the fields
    if (g_meta_provider) {
        ProvinceSummary tmp = *out;
        if (g_meta_provider(id, &tmp)) {
            // Only overwrite provided fields that are non-null
            out->name    = tmp.name    ? tmp.name    : out->name;
            out->owner   = tmp.owner   ? tmp.owner   : out->owner;
            out->terrain = tmp.terrain ? tmp.terrain : out->terrain;
            out->centroid= tmp.centroid;
        }
    }
    return true;
}

int  MapGetSettlementCount(int) { return 0; }
bool MapGetSettlement(int, int, ImVec2*, const char**) { return false; }

void SetProvinceMetaProvider(ProvinceMetaProvider fn) { g_meta_provider = fn; }

// ---------------------------
// Main draw (placeholder)
// ---------------------------
void DrawMapViewport(AppState&) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin  = ImGui::GetCursorScreenPos();
    ImVec2 avail   = ImGui::GetContentRegionAvail();
    g_last_viewport_size = avail;

    // One-time camera init
    if (!g_cam_init) {
        g_cam_init = true;
        g_cam.offset = ImVec2(0, 0);
        g_cam.zoom   = 1.0f;
    }

    // Base: placeholder background
    ImU32 bgCol = IM_COL32(40, 45, 52, 255);
    dl->AddRectFilled(origin, ImVec2(origin.x + avail.x, origin.y + avail.y), bgCol);

    // Optional map texture
    if (g_map_tex) {
        ImVec2 p0 = ImVec2(origin.x + g_cam.offset.x, origin.y + g_cam.offset.y);
        ImVec2 p1 = ImVec2(p0.x + g_cam.world_size.x * g_cam.zoom, p0.y + g_cam.world_size.y * g_cam.zoom);
        dl->AddImage(g_map_tex, p0, p1);
    } else {
        // Hatch pattern & banner
        for (float y = origin.y; y < origin.y + avail.y; y += 8.0f) {
            dl->AddLine(ImVec2(origin.x, y), ImVec2(origin.x + avail.x, y + 8.0f), IM_COL32(60, 66, 76, 255));
        }
        dl->AddText(ImVec2(origin.x + 12, origin.y + 12), IM_COL32_WHITE, "PLACEHOLDER MAP");
    }

    // Mouse hover → world pos → province
    ImVec2 mouse = ImGui::GetIO().MousePos;
    bool inBounds = mouse.x >= origin.x && mouse.x < origin.x + avail.x &&
                    mouse.y >= origin.y && mouse.y < origin.y + avail.y;
    if (inBounds) {
        ImVec2 world = screenToWorld(g_cam, mouse, origin);
        g_hover_province = PickProvinceAtWorld(world.x, world.y, nullptr);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            g_selected_province = g_hover_province;
            if (g_on_select) g_on_select(g_selected_province);
        }
    } else {
        g_hover_province = -1;
    }

    // Simple hover ring
    if (g_hover_province >= 0) {
        ProvinceSummary s{};
        if (MapGetProvinceSummary(g_hover_province, &s)) {
            ImVec2 p = worldToScreen(g_cam, s.centroid, origin);
            dl->AddCircle(p, 8.0f, IM_COL32(255,255,0,255), 24, 2.0f);
        }
    }

    // Reserve space (so parent layout keeps flowing)
    ImGui::Dummy(avail);
}

} // namespace ui
