#include "ui/RealmWindow.h"
#include "ui/WindowManager.h"

namespace ui {

RealmWindow::RealmWindow(core::ecs::EntityManager& entity_manager,
                         game::realm::RealmManager& realm_manager)
    : entity_manager_(entity_manager)
    , realm_manager_(realm_manager) {
}

void RealmWindow::Render(WindowManager& window_manager, game::types::EntityID player_entity) {
    (void)player_entity; // Mark as unused for now

    if (!window_manager.BeginManagedWindow(WindowManager::WindowType::REALM, "Realm & Dynasty")) {
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

    window_manager.EndManagedWindow();
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
