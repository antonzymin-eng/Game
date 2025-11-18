#include "ui/MilitaryWindow.h"

namespace ui {

MilitaryWindow::MilitaryWindow(core::ecs::EntityManager& entity_manager,
                               game::military::MilitarySystem& military_system)
    : entity_manager_(entity_manager)
    , military_system_(military_system) {
}

void MilitaryWindow::Render(bool* p_open) {
    if (!ImGui::Begin("Military", p_open)) {
        ImGui::End();
        return;
    }

    // Tab bar
    if (ImGui::BeginTabBar("MilitaryTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Overview")) {
            RenderOverviewTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Army")) {
            RenderArmyTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Navy")) {
            RenderNavyTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Recruitment")) {
            RenderRecruitmentTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Battles")) {
            RenderBattlesTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void MilitaryWindow::RenderOverviewTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("MILITARY OVERVIEW");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(2, "military_overview", false);
    ImGui::SetColumnWidth(0, 200);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

    ImGui::Text("Total Manpower:");
    ImGui::NextColumn();
    ImGui::Text("0 / 0");
    ImGui::NextColumn();

    ImGui::Text("Army Size:");
    ImGui::NextColumn();
    ImGui::Text("0 units");
    ImGui::NextColumn();

    ImGui::Text("Navy Size:");
    ImGui::NextColumn();
    ImGui::Text("0 ships");
    ImGui::NextColumn();

    ImGui::Text("Military Maintenance:");
    ImGui::NextColumn();
    ImGui::Text("$0 / month");
    ImGui::NextColumn();

    ImGui::Text("Army Morale:");
    ImGui::NextColumn();
    ImGui::Text("100%%");
    ImGui::NextColumn();

    ImGui::Text("Army Professionalism:");
    ImGui::NextColumn();
    ImGui::Text("0%%");
    ImGui::NextColumn();

    ImGui::PopStyleColor();

    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Active Battles: 0");
    ImGui::PopStyleColor();
}

void MilitaryWindow::RenderArmyTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("ARMY");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(5, "army_units", false);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

    // Header
    ImGui::Text("Unit Name");
    ImGui::NextColumn();
    ImGui::Text("Type");
    ImGui::NextColumn();
    ImGui::Text("Size");
    ImGui::NextColumn();
    ImGui::Text("Location");
    ImGui::NextColumn();
    ImGui::Text("Status");
    ImGui::NextColumn();

    ImGui::Separator();

    ImGui::PopStyleColor();

    // TODO: List actual army units
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::Text("No units");
    ImGui::NextColumn();
    ImGui::Text("-");
    ImGui::NextColumn();
    ImGui::Text("-");
    ImGui::NextColumn();
    ImGui::Text("-");
    ImGui::NextColumn();
    ImGui::Text("-");
    ImGui::NextColumn();
    ImGui::PopStyleColor();

    ImGui::Columns(1);
}

void MilitaryWindow::RenderNavyTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("NAVY");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(5, "navy_units", false);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

    // Header
    ImGui::Text("Fleet Name");
    ImGui::NextColumn();
    ImGui::Text("Type");
    ImGui::NextColumn();
    ImGui::Text("Ships");
    ImGui::NextColumn();
    ImGui::Text("Location");
    ImGui::NextColumn();
    ImGui::Text("Status");
    ImGui::NextColumn();

    ImGui::Separator();

    ImGui::PopStyleColor();

    // TODO: List actual naval units
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::Text("No fleets");
    ImGui::PopStyleColor();

    ImGui::Columns(1);
}

void MilitaryWindow::RenderRecruitmentTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("RECRUITMENT");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Available Manpower: 0");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Recruitment options and unit templates will be displayed here");

    // TODO: Add recruitment interface
}

void MilitaryWindow::RenderBattlesTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("ACTIVE BATTLES");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::Text("No active battles");
    ImGui::PopStyleColor();

    // TODO: List active battles with links to battle viewer
}

} // namespace ui
