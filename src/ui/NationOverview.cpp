#include "ui/NationOverview.h"
#include "imgui.h"
#include "ui/MapAPI.h"
#include "ui/Toast.h"

namespace ui {
void ShowNationOverview(bool* open) {
    if (!*open) return;
    ImGui::SetNextWindowSize(ImVec2(520, 420), ImGuiCond_Appearing);
    if (ImGui::Begin("Nation Overview", open, ImGuiWindowFlags_NoSavedSettings)) {
        auto info = MapGetInfo();
        ImGui::Text("Realm: %s", "—");
        ImGui::Text("Provinces known: %d", MapGetProvinceCount());
        ImGui::Separator();
        ImGui::TextDisabled("Government");
        ImGui::BulletText("Legitimacy: %s", "—");
        ImGui::BulletText("Stability: %s", "—");
        ImGui::Separator();
        ImGui::TextDisabled("Economy");
        ImGui::BulletText("Treasury: %s", "—");
        ImGui::BulletText("Income/Expenses: %s", "—");
        ImGui::Separator();
        if (ImGui::Button("Close")) *open = false;
    }
    ImGui::End();
}
} // namespace ui
