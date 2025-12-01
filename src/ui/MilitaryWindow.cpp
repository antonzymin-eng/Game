#include "ui/MilitaryWindow.h"
#include "ui/WindowManager.h"

namespace ui {

MilitaryWindow::MilitaryWindow(core::ecs::EntityManager& entity_manager,
                               game::military::MilitarySystem& military_system)
    : entity_manager_(entity_manager)
    , military_system_(military_system) {
}

void MilitaryWindow::Render(WindowManager& window_manager, game::types::EntityID player_entity) {
    current_player_entity_ = player_entity;

    if (!window_manager.BeginManagedWindow(WindowManager::WindowType::MILITARY, "Military")) {
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

    window_manager.EndManagedWindow();
}

void MilitaryWindow::RenderOverviewTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("MILITARY OVERVIEW");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Get real data from military system
    auto all_armies = military_system_.GetAllArmies();

    // Note: Military system uses province-level data. For realm-level overview,
    // we would need to aggregate across all owned provinces. For now, showing army counts.
    size_t total_armies = all_armies.size();

    // Calculate total maintenance (would need province iteration in production)
    // For now, using placeholder calculation
    double total_maintenance = 0.0;

    ImGui::Columns(2, "military_overview", false);
    ImGui::SetColumnWidth(0, 200);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

    ImGui::Text("Total Armies:");
    ImGui::NextColumn();
    ImGui::Text("%zu", total_armies);
    ImGui::NextColumn();

    ImGui::Text("Army Size:");
    ImGui::NextColumn();
    ImGui::Text("%zu units", total_armies > 0 ? total_armies : 0);
    ImGui::NextColumn();

    ImGui::Text("Navy Size:");
    ImGui::NextColumn();
    ImGui::Text("0 ships"); // TODO: Add naval support to MilitarySystem
    ImGui::NextColumn();

    ImGui::Text("Military Maintenance:");
    ImGui::NextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Red for expense
    ImGui::Text("$%.0f / month", total_maintenance);
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::Text("Army Morale:");
    ImGui::NextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
    ImGui::Text("100%%"); // TODO: Calculate average morale across armies
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::Text("Army Professionalism:");
    ImGui::NextColumn();
    ImGui::Text("0%%"); // TODO: Add professionalism tracking
    ImGui::NextColumn();

    ImGui::PopStyleColor();

    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Active Battles: 0"); // TODO: Get from military system
    ImGui::PopStyleColor();

    // Note for future enhancement
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::TextWrapped("Note: Full military data integration requires province-level aggregation. Some statistics are placeholder pending realm-level API enhancements.");
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
    auto army_entities = entity_manager_.GetEntitiesWithComponent<game::military::ArmyComponent>();
    bool has_naval_forces = false;

    for (const auto& entity_id : army_entities) {
        auto army = entity_manager_.GetComponent<game::military::ArmyComponent>(entity_id);
        if (army && army->dominant_unit_class == game::military::UnitClass::NAVAL) {
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

    // Unit recruitment interface
    struct UnitType {
        const char* name;
        const char* description;
        int cost;
        int manpower;
        int attack;
        int defense;
        const char* requirements;
    };

    UnitType unit_types[] = {
        {"Infantry", "Basic foot soldiers", 100, 100, 10, 12, "None"},
        {"Cavalry", "Fast mounted units", 250, 50, 15, 10, "Barracks"},
        {"Archers", "Ranged infantry", 150, 75, 12, 8, "None"},
        {"Knights", "Elite heavy cavalry", 500, 25, 25, 20, "Barracks + Stable"},
        {"Siege Equipment", "For besieging fortifications", 400, 0, 5, 5, "Workshop"}
    };

    for (const auto& unit : unit_types) {
        ImGui::PushID(unit.name);

        // Unit info panel
        ImGui::BeginGroup();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("%s", unit.name);
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("  %s", unit.description);
        ImGui::Text("  Cost: $%d | Manpower: %d | ATK: %d | DEF: %d",
                    unit.cost, unit.manpower, unit.attack, unit.defense);
        ImGui::Text("  Requirements: %s", unit.requirements);
        ImGui::PopStyleColor();

        ImGui::EndGroup();

        // Recruit controls
        ImGui::SameLine(ImGui::GetWindowWidth() - 250);
        ImGui::SetNextItemWidth(80);

        // Get or initialize recruit count for this unit type
        auto& recruit_count = recruit_counts_[unit.name];
        if (recruit_count == 0) recruit_count = 1; // Default to 1

        ImGui::InputInt("##count", &recruit_count, 1, 10);
        if (recruit_count < 1) recruit_count = 1;
        if (recruit_count > 99) recruit_count = 99;

        ImGui::SameLine();
        if (ImGui::Button("Recruit", ImVec2(100, 0))) {
            if (current_player_entity_ != 0 && recruit_count > 0) {
                // Map UI unit name to UnitType enum
                game::military::UnitType unit_type = game::military::UnitType::SPEARMEN; // Default

                if (std::string(unit.name) == "Infantry") {
                    unit_type = game::military::UnitType::SPEARMEN;
                } else if (std::string(unit.name) == "Cavalry") {
                    unit_type = game::military::UnitType::LIGHT_CAVALRY;
                } else if (std::string(unit.name) == "Archers") {
                    unit_type = game::military::UnitType::CROSSBOWMEN;
                } else if (std::string(unit.name) == "Knights") {
                    unit_type = game::military::UnitType::HEAVY_CAVALRY;
                } else if (std::string(unit.name) == "Siege Equipment") {
                    unit_type = game::military::UnitType::CATAPULTS;
                }

                // Note: RecruitUnit expects a province_id, not a realm_id
                // In a full implementation, you would:
                // 1. Show a province selector to choose where to recruit
                // 2. Check manpower availability in that province
                // 3. Check if required buildings exist (e.g., Barracks)
                // For now, using player_entity as province_id (placeholder)
                military_system_.RecruitUnit(current_player_entity_, unit_type, static_cast<uint32_t>(recruit_count));
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Recruit %d %s units (Cost: $%d, Manpower: %d)",
                             recruit_count, unit.name, unit.cost * recruit_count, unit.manpower * recruit_count);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PopID();
    }

    // Recruitment queue
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("RECRUITMENT QUEUE");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::Text("No units currently in training");
    ImGui::PopStyleColor();
    // TODO: Display active recruitment queue with progress bars
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
