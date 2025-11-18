#include "ui/RealmWindow.h"

namespace ui {

RealmWindow::RealmWindow(core::ecs::EntityManager& entity_manager,
                         game::realm::RealmManager& realm_manager)
    : entity_manager_(entity_manager)
    , realm_manager_(realm_manager) {
}

void RealmWindow::Render(bool* p_open) {
    if (!ImGui::Begin("Realm & Dynasty", p_open)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("RealmTabs")) {
        if (ImGui::BeginTabItem("Dynasty")) {
            RenderDynastyTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Succession")) {
            RenderSuccessionTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Court")) {
            RenderCourtTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Vassals")) {
            RenderVassalsTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void RealmWindow::RenderDynastyTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("DYNASTY");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Text("Dynasty information and family tree");
}

void RealmWindow::RenderSuccessionTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("SUCCESSION");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Text("Succession laws and heirs");
}

void RealmWindow::RenderCourtTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("COURT");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Text("Court members and advisors");
}

void RealmWindow::RenderVassalsTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("VASSALS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Text("Vassal realms and their status");
}

} // namespace ui
