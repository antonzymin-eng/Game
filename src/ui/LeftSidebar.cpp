#include "imgui.h"
#include <string>
#include "ScreenAPI.h"
#include "state/SimulationState.h"
#include "ui/LeftSidebar.h"

namespace ui {

// Minimal, safe implementation that draws a left dockable sidebar where system buttons live.
void DrawLeftSidebar(AppState& /*app*/, core::SimulationState& /*sim*/, float leftWidth) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    // Position left panel with a small top/bottom margin; full height.
    const float margin = 12.0f;
    ImVec2 pos = ImVec2(vp->Pos.x + margin, vp->Pos.y + 44.0f + margin); // below top bar
    ImVec2 size = ImVec2(leftWidth, vp->Size.y - (44.0f + margin * 2.0f));

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    if (ImGui::Begin("Systems", nullptr, ImGuiWindowFlags_NoCollapse)) {
        ImGui::BeginChild("SystemsList", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        // Basic categories as buttons (placeholders — hook your real handlers as desired)
        const char* sections[] = {
            "Overview","Economy","Military","Espionage","Diplomacy","Technology",
            "Religion","Trade","Culture","Administration","Population","Laws"
        };
        for (const char* s : sections) {
            ImGui::Button(s, ImVec2(-1, 0));
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

} // namespace ui