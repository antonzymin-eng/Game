#include "ui/MilitaryWindow.h"

namespace ui {

MilitaryWindow::MilitaryWindow(core::ecs::EntityManager& entity_manager,
                               game::military::MilitarySystem& military_system)
    : entity_manager_(entity_manager)
    , military_system_(military_system)
    , active_tab_(0) {
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

    // Fleet overview statistics
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Naval Overview");
    ImGui::PopStyleColor();

    ImGui::Columns(4, "navy_overview", false);

    // Statistics
    ImGui::Text("Total Fleets:");
    ImGui::NextColumn();
    ImGui::Text("0");
    ImGui::NextColumn();

    ImGui::Text("Total Ships:");
    ImGui::NextColumn();
    ImGui::Text("0");
    ImGui::NextColumn();

    ImGui::Text("Ships at Sea:");
    ImGui::NextColumn();
    ImGui::Text("0");
    ImGui::NextColumn();

    ImGui::Text("Ships in Port:");
    ImGui::NextColumn();
    ImGui::Text("0");
    ImGui::NextColumn();

    ImGui::Text("Naval Firepower:");
    ImGui::NextColumn();
    ImGui::Text("0");
    ImGui::NextColumn();

    ImGui::Text("Active Blockades:");
    ImGui::NextColumn();
    ImGui::Text("0");
    ImGui::NextColumn();

    ImGui::Columns(1);
    ImGui::Separator();
    ImGui::Spacing();

    // Fleet list
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Fleets");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::Columns(6, "navy_fleets", false);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

    // Header
    ImGui::Text("Fleet Name");
    ImGui::NextColumn();
    ImGui::Text("Ships");
    ImGui::NextColumn();
    ImGui::Text("Strength");
    ImGui::NextColumn();
    ImGui::Text("Firepower");
    ImGui::NextColumn();
    ImGui::Text("Location");
    ImGui::NextColumn();
    ImGui::Text("Status");
    ImGui::NextColumn();

    ImGui::Separator();

    ImGui::PopStyleColor();

    // List fleets - Query ArmyComponent entities with NAVAL dominant class
    auto armies = entity_manager_.GetAllComponentsOfType<game::military::ArmyComponent>();
    bool has_naval_forces = false;

    for (const auto& army : armies) {
        if (army->dominant_unit_class == game::military::UnitClass::NAVAL) {
            has_naval_forces = true;

            // Fleet name
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 1.0f, 1.0f));
            ImGui::Text("%s", army->army_name.c_str());
            ImGui::PopStyleColor();
            ImGui::NextColumn();

            // Number of ships
            ImGui::Text("%zu", army->units.size());
            ImGui::NextColumn();

            // Total strength
            ImGui::Text("%u", army->total_strength);
            ImGui::NextColumn();

            // Firepower (estimated from composition)
            uint32_t firepower = 0;
            for (const auto& unit : army->units) {
                if (unit.type == game::military::UnitType::SHIPS_OF_THE_LINE) firepower += 2000;
                else if (unit.type == game::military::UnitType::WAR_GALLEONS) firepower += 1500;
                else if (unit.type == game::military::UnitType::GALLEONS) firepower += 1000;
                else firepower += 500;
            }
            ImGui::Text("%u", firepower);
            ImGui::NextColumn();

            // Location (current province ID)
            ImGui::Text("Province %u", army->current_location);
            ImGui::NextColumn();

            // Status
            if (army->is_in_battle) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                ImGui::Text("In Battle");
                ImGui::PopStyleColor();
            } else if (army->is_besieging) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.3f, 1.0f));
                ImGui::Text("Blockading");
                ImGui::PopStyleColor();
            } else if (army->is_active) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
                ImGui::Text("At Sea");
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                ImGui::Text("In Port");
                ImGui::PopStyleColor();
            }
            ImGui::NextColumn();
        }
    }

    if (!has_naval_forces) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("No fleets");
        ImGui::PopStyleColor();
    }

    ImGui::Columns(1);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Fleet composition breakdown (for selected fleet)
    if (has_naval_forces) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
        ImGui::Text("Ship Types");
        ImGui::PopStyleColor();
        ImGui::Spacing();

        ImGui::Columns(2, "ship_types", false);

        ImGui::Text("Ships of the Line:");
        ImGui::NextColumn();
        ImGui::Text("0");
        ImGui::NextColumn();

        ImGui::Text("War Galleons:");
        ImGui::NextColumn();
        ImGui::Text("0");
        ImGui::NextColumn();

        ImGui::Text("Galleons:");
        ImGui::NextColumn();
        ImGui::Text("0");
        ImGui::NextColumn();

        ImGui::Text("Carracks:");
        ImGui::NextColumn();
        ImGui::Text("0");
        ImGui::NextColumn();

        ImGui::Text("Cogs:");
        ImGui::NextColumn();
        ImGui::Text("0");
        ImGui::NextColumn();

        ImGui::Text("Galleys:");
        ImGui::NextColumn();
        ImGui::Text("0");
        ImGui::NextColumn();

        ImGui::Columns(1);
    }
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
