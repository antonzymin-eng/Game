#include "imgui.h"
#include "ScreenAPI.h"
#include <array>
#include <string>

namespace ui {

// -----------------------------------------------------------
// Left Vertical Systems Dock (accordion with animation)
// -----------------------------------------------------------
struct PanelAnim {
    bool   target_open = false;  // desired state
    float  t           = 0.0f;   // 0=closed, 1=open (animated)
};

static int g_active_panel = -1;             // -1 collapsed by default
static PanelAnim g_anim[12];                // one per panel (max 12 categories)

static const std::array<const char*, 12> kPanels = {
    "Overview","Economy","Military","Espionage","Diplomacy","Technology",
    "Religion","Trade","Culture","Administration","Population","Laws"
};

static void AnimatePanels() {
    const float dt = ImGui::GetIO().DeltaTime;
    for (int i = 0; i < (int)kPanels.size(); ++i) {
        float target = g_anim[i].target_open ? 1.0f : 0.0f;
        // Smooth step toward target
        float v = g_anim[i].t;
        v += (target - v) * (1.0f - powf(0.0001f, dt * 60.0f)); // ~critically damped
        // Clamp
        if (v < 0.001f) v = 0.0f;
        if (v > 0.999f) v = 1.0f;
        g_anim[i].t = v;
    }
}

void DrawSystemsDockLeft(AppState&) {
    AnimatePanels();

    ImGuiViewport* vp = ImGui::GetMainViewport();
    const float top_h = 64.0f;
    const float margin = 12.0f;
    const float width = 360.0f;
    ImVec2 pos = ImVec2(vp->Pos.x + margin, vp->Pos.y + top_h + margin);
    ImVec2 size = ImVec2(width, vp->Size.y - (top_h + margin * 2.0f));

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    if (ImGui::Begin("Systems Dock", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // List of vertical panels
        for (int i = 0; i < (int)kPanels.size(); ++i) {
            ImGui::PushID(i);
            bool is_open = (g_active_panel == i);
            // Header button acts as exclusive toggle
            if (ImGui::Selectable(kPanels[i], is_open, ImGuiSelectableFlags_SpanAllColumns)) {
                if (is_open) {
                    g_active_panel = -1; // collapse if clicking active one
                } else {
                    g_active_panel = i;  // open this one and close others
                }
            }

            // Update targets for all panels based on g_active_panel
            g_anim[i].target_open = (g_active_panel == i);

            // Animated body
            float t = g_anim[i].t;
            if (t > 0.0f) {
                // Interpolate height: 0..1 -> 0..maxHeight
                const float max_h = 240.0f;
                float h = max_h * t * t; // ease-out
                ImGui::BeginChild("PanelBody", ImVec2(0, h), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                ImGui::TextDisabled("%s", kPanels[i]);
                ImGui::Separator();
                ImGui::Text("Content stub for %s.\nWire to your SystemsPanel code.", kPanels[i]);
                ImGui::EndChild();
            }
            ImGui::PopID();
            ImGui::Separator();
        }
    }
    ImGui::End();
}

// Placeholder to avoid link errors if referenced elsewhere
void DrawFloatingPanelButtons(AppState&) {}

} // namespace ui