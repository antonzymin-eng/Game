#include "ui/DiplomacyWindow.h"
#include "ui/PortraitGenerator.h"
#include "game/components/CharacterComponent.h"

namespace ui {

DiplomacyWindow::DiplomacyWindow(core::ecs::EntityManager& entity_manager,
                                 game::diplomacy::DiplomacySystem& diplomacy_system)
    : entity_manager_(entity_manager)
    , diplomacy_system_(diplomacy_system)
    , portrait_generator_(nullptr) {
}

void DiplomacyWindow::Render(bool* p_open) {
    if (!ImGui::Begin("Diplomacy", p_open)) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("DiplomacyTabs")) {
        if (ImGui::BeginTabItem("Overview")) {
            RenderOverviewTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Relations")) {
            RenderRelationsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Treaties")) {
            RenderTreatiesTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Wars")) {
            RenderWarTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void DiplomacyWindow::RenderOverviewTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("DIPLOMATIC OVERVIEW");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Text("Diplomatic relations and status");
}

void DiplomacyWindow::RenderRelationsTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("RELATIONS");
    ImGui::PopStyleColor();
    ImGui::Separator();

    // Placeholder text explaining portrait integration
    ImGui::TextWrapped("Relation standings with other nations");
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
                       "Portrait system ready - portraits will appear here when character data is available");

    // Example of how portraits would be displayed
    if (portrait_generator_) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("When integrated with character system, each nation's ruler portrait will appear here:");
        ImGui::Spacing();

        // Demonstration layout for future character portraits
        ImGui::BeginChild("RelationsList", ImVec2(0, 0), true);

        ImGui::Text("Example: [64x64 Portrait] Nation Name - Opinion: +50 (Friendly)");
        ImGui::Text("Future: Portraits will be generated procedurally for each ruler");

        ImGui::EndChild();
    }
}

void DiplomacyWindow::RenderTreatiesTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("TREATIES & AGREEMENTS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Text("Active treaties, alliances, and agreements");
}

void DiplomacyWindow::RenderWarTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("WARS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Text("Active wars and war goals");
}

} // namespace ui
