// ============================================================================
// ProvinceInfoWindow.cpp - Detailed province information panel implementation
// ============================================================================

#include "ui/ProvinceInfoWindow.h"
#include "ui/WindowManager.h"
#include "map/render/MapRenderer.h"
#include "game/province/ProvinceSystem.h"
#include "game/military/MilitaryComponents.h"
#include "game/population/PopulationComponents.h"
#include <imgui.h>
#include <sstream>
#include <iomanip>

namespace ui {

    ProvinceInfoWindow::ProvinceInfoWindow(
        ::core::ecs::EntityManager& entity_manager,
        game::map::MapRenderer& map_renderer
    )
        : entity_manager_(entity_manager)
        , map_renderer_(map_renderer)
        , current_player_entity_(0)
        , visible_(true) {
    }

    void ProvinceInfoWindow::Render(WindowManager& window_manager, game::types::EntityID player_entity) {
        current_player_entity_ = player_entity;

        if (!window_manager.BeginManagedWindow(WindowManager::WindowType::PROVINCE_INFO, "Province Information")) {
            return;
        }

        // Get selected province from map renderer
        auto selected_province = map_renderer_.GetSelectedProvince();
        if (selected_province.id == 0) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a province to view information");
            window_manager.EndManagedWindow();
            return;
        }

        // Get province data component
        auto province_data = entity_manager_.GetComponent<game::province::ProvinceDataComponent>(selected_province);
        if (!province_data) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No data available for this province");
            window_manager.EndManagedWindow();
            return;
        }

        RenderHeader();
        ImGui::Separator();

        // Tabbed interface for different sections
        if (ImGui::BeginTabBar("ProvinceInfoTabs")) {
            if (ImGui::BeginTabItem("Overview")) {
                RenderOverviewTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Buildings")) {
                RenderBuildingsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Military")) {
                RenderMilitaryTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Population")) {
                RenderPopulationTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Religion")) {
                RenderReligionTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Administration")) {
                RenderAdministrationTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Geography")) {
                RenderGeographyTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        window_manager.EndManagedWindow();
    }

    void ProvinceInfoWindow::RenderHeader() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto province_data = entity_manager_.GetComponent<game::province::ProvinceDataComponent>(selected_province);

        if (!province_data) return;

        // Province name (large and prominent)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.5f, 1.0f)); // Gold
        ImGui::SetWindowFontScale(1.5f);
        ImGui::Text("%s", province_data->name.c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        // Province ID
        ImGui::Text("Province ID: %lu", selected_province.id);

        // Owner nation
        if (province_data->owner_nation > 0) {
            ImGui::Text("Owner: Nation %lu", static_cast<unsigned long>(province_data->owner_nation));
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Unowned Territory");
        }
    }

    void ProvinceInfoWindow::RenderOverviewTab() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto province_data = entity_manager_.GetComponent<game::province::ProvinceDataComponent>(selected_province);
        auto pop_comp = entity_manager_.GetComponent<game::population::PopulationComponent>(selected_province);
        auto prosperity = entity_manager_.GetComponent<game::province::ProvinceProsperityComponent>(selected_province);

        if (!province_data) return;

        // Population summary
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Population");
        ImGui::Separator();

        if (pop_comp) {
            ImGui::Text("Total Population:");
            ImGui::SameLine(200);
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%d", pop_comp->total_population);

            ImGui::Text("Growth Rate:");
            ImGui::SameLine(200);
            float growth_pct = pop_comp->growth_rate * 100.0f;
            ImVec4 growth_color = growth_pct >= 0 ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            ImGui::TextColored(growth_color, "%.2f%%", growth_pct);

            ImGui::Text("Happiness:");
            ImGui::SameLine(200);
            ImGui::Text("%.1f%%", pop_comp->average_happiness * 100.0f);
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No population data");
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Economy summary
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Economy");
        ImGui::Separator();

        ImGui::Text("Development Level:");
        ImGui::SameLine(200);
        ImGui::Text("%d / %d", province_data->development_level, province_data->max_development);

        // Development progress bar
        float dev_percentage = static_cast<float>(province_data->development_level) / static_cast<float>(province_data->max_development);
        ImGui::ProgressBar(dev_percentage, ImVec2(-1, 0));

        if (prosperity) {
            ImGui::Text("Prosperity:");
            ImGui::SameLine(200);
            ImGui::Text("%.1f%%", prosperity->prosperity_level * 100.0f);

            ImGui::Text("Economic Factor:");
            ImGui::SameLine(200);
            ImGui::Text("%.1f%%", prosperity->economic_factor * 100.0f);
        }
    }

    void ProvinceInfoWindow::RenderBuildingsTab() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto buildings = entity_manager_.GetComponent<game::province::ProvinceBuildingsComponent>(selected_province);

        if (!buildings) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No building data available");
            ImGui::Spacing();
            ImGui::TextWrapped("This province does not have the ProvinceBuildingsComponent. Building data may not be initialized yet.");
            return;
        }

        // Production Buildings
        ImGui::TextColored(ImVec4(0.9f, 0.7f, 1.0f, 1.0f), "Production Buildings");
        ImGui::Separator();

        ImGui::BeginTable("ProductionBuildings", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Building", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        for (const auto& [building_type, level] : buildings->production_buildings) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", GetBuildingName(static_cast<int>(building_type), true));
            ImGui::TableNextColumn();
            if (level > 0) {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%d", level);
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Not Built");
            }
        }

        ImGui::EndTable();

        ImGui::Spacing();
        ImGui::Spacing();

        // Infrastructure Buildings
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Infrastructure");
        ImGui::Separator();

        ImGui::BeginTable("InfrastructureBuildings", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Building", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        for (const auto& [building_type, level] : buildings->infrastructure_buildings) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", GetBuildingName(static_cast<int>(building_type), false));
            ImGui::TableNextColumn();
            if (level > 0) {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%d", level);
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Not Built");
            }
        }

        ImGui::EndTable();

        ImGui::Spacing();
        ImGui::Spacing();

        // Building capacity
        ImGui::Text("Building Slots:");
        ImGui::SameLine();
        ImGui::Text("%d / %d", buildings->current_buildings, buildings->max_buildings);

        float capacity_pct = static_cast<float>(buildings->current_buildings) / static_cast<float>(buildings->max_buildings);
        ImGui::ProgressBar(capacity_pct, ImVec2(-1, 0));

        // Construction queue
        if (!buildings->construction_queue.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Construction in Progress:");
            ImGui::Text("Building: %s", GetBuildingName(static_cast<int>(buildings->construction_queue[0]), true));
            ImGui::Text("Progress:");
            ImGui::ProgressBar(static_cast<float>(buildings->construction_progress), ImVec2(-1, 0));
        }
    }

    void ProvinceInfoWindow::RenderMilitaryTab() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto military = entity_manager_.GetComponent<game::military::MilitaryComponent>(selected_province);

        if (!military) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No military data available");
            ImGui::Spacing();
            ImGui::TextWrapped("This province does not have the MilitaryComponent. Military infrastructure may not be initialized yet.");
            return;
        }

        // Military Summary
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Military Strength");
        ImGui::Separator();

        uint32_t total_strength = military->GetTotalGarrisonStrength();
        ImGui::Text("Total Garrison Strength:");
        ImGui::SameLine(220);
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "%u", total_strength);

        ImGui::Text("Active Armies:");
        ImGui::SameLine(220);
        ImGui::Text("%zu", military->active_armies.size());

        ImGui::Spacing();
        ImGui::Spacing();

        // Garrison Units
        ImGui::TextColored(ImVec4(0.9f, 0.7f, 1.0f, 1.0f), "Garrison Units");
        ImGui::Separator();

        if (military->garrison_units.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No garrison units");
        } else {
            ImGui::BeginTable("GarrisonUnits", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
            ImGui::TableSetupColumn("Unit Type", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Strength", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Experience", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("Morale", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableHeadersRow();

            for (const auto& unit : military->garrison_units) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", GetUnitTypeName(static_cast<int>(unit.type)));

                ImGui::TableNextColumn();
                float strength_pct = static_cast<float>(unit.current_strength) / static_cast<float>(unit.max_strength);
                ImVec4 strength_color = strength_pct > 0.7f ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) :
                                       strength_pct > 0.3f ? ImVec4(1.0f, 1.0f, 0.4f, 1.0f) :
                                       ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                ImGui::TextColored(strength_color, "%u/%u", unit.current_strength, unit.max_strength);

                ImGui::TableNextColumn();
                ImGui::Text("%.1f", unit.experience);

                ImGui::TableNextColumn();
                // Color code morale states
                const char* morale_name = GetMoraleStateName(static_cast<int>(unit.morale));
                ImVec4 morale_color;
                switch (unit.morale) {
                    case game::military::MoraleState::ROUTING:
                    case game::military::MoraleState::BROKEN:
                        morale_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red
                        break;
                    case game::military::MoraleState::WAVERING:
                        morale_color = ImVec4(1.0f, 1.0f, 0.3f, 1.0f); // Yellow
                        break;
                    case game::military::MoraleState::STEADY:
                        morale_color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f); // Gray
                        break;
                    case game::military::MoraleState::CONFIDENT:
                        morale_color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // Green
                        break;
                    case game::military::MoraleState::FANATICAL:
                        morale_color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f); // Cyan
                        break;
                    default:
                        morale_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
                        break;
                }
                ImGui::TextColored(morale_color, "%s", morale_name);
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Military Infrastructure
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Military Infrastructure");
        ImGui::Separator();

        ImGui::Text("Recruitment Capacity:");
        ImGui::SameLine(220);
        ImGui::Text("%u", military->recruitment_capacity);

        ImGui::Text("Training Facilities:");
        ImGui::SameLine(220);
        ImGui::Text("%.1f%%", military->training_facilities * 100.0f);
        ImGui::ProgressBar(static_cast<float>(military->training_facilities), ImVec2(-1, 0));

        ImGui::Text("Supply Infrastructure:");
        ImGui::SameLine(220);
        ImGui::Text("%.1f%%", military->supply_infrastructure * 100.0f);
        ImGui::ProgressBar(static_cast<float>(military->supply_infrastructure), ImVec2(-1, 0));

        ImGui::Text("Barracks Level:");
        ImGui::SameLine(220);
        ImGui::Text("%.0f", military->barracks_level);

        ImGui::Spacing();

        // Military Budget
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Military Spending");
        ImGui::Separator();

        ImGui::Text("Total Budget:");
        ImGui::SameLine(220);
        ImGui::Text("%.0f gold", military->military_budget);

        ImGui::Text("Recruitment:");
        ImGui::SameLine(220);
        ImGui::Text("%.0f gold", military->recruitment_spending);

        ImGui::Text("Maintenance:");
        ImGui::SameLine(220);
        ImGui::Text("%.0f gold", military->maintenance_spending);

        ImGui::Text("Equipment:");
        ImGui::SameLine(220);
        ImGui::Text("%.0f gold", military->equipment_spending);
    }

    void ProvinceInfoWindow::RenderPopulationTab() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto pop_comp = entity_manager_.GetComponent<game::population::PopulationComponent>(selected_province);

        if (!pop_comp) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No population data available");
            ImGui::Spacing();
            ImGui::TextWrapped("This province does not have the PopulationComponent. Population data may not be initialized yet.");
            return;
        }

        // Population Statistics
        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Population Statistics");
        ImGui::Separator();

        ImGui::Text("Total Population:");
        ImGui::SameLine(200);
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%d", pop_comp->total_population);

        ImGui::Text("Population Density:");
        ImGui::SameLine(200);
        ImGui::Text("%.1f/km²", pop_comp->population_density);

        ImGui::Text("Growth Rate:");
        ImGui::SameLine(200);
        float growth_pct = pop_comp->growth_rate * 100.0f;
        ImVec4 growth_color = growth_pct >= 0 ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
        ImGui::TextColored(growth_color, "%.2f%%", growth_pct);

        ImGui::Spacing();

        ImGui::Text("Birth Rate:");
        ImGui::SameLine(200);
        ImGui::Text("%.2f%%", pop_comp->birth_rate_average * 100.0f);

        ImGui::Text("Death Rate:");
        ImGui::SameLine(200);
        ImGui::Text("%.2f%%", pop_comp->death_rate_average * 100.0f);

        ImGui::Text("Migration Rate:");
        ImGui::SameLine(200);
        ImGui::Text("%.2f%%", pop_comp->migration_net_rate * 100.0f);

        ImGui::Spacing();
        ImGui::Spacing();

        // Demographics
        if (ImGui::CollapsingHeader("Demographics", ImGuiTreeNodeFlags_DefaultOpen)) {
            int total = pop_comp->total_population;
            if (total > 0) {
                ImGui::Text("Age Distribution:");
                ImGui::Indent();
                float children_pct = (pop_comp->total_children * 100.0f) / total;
                float adults_pct = (pop_comp->total_adults * 100.0f) / total;
                float elderly_pct = (pop_comp->total_elderly * 100.0f) / total;

                ImGui::Text("Children: %d (%.1f%%)", pop_comp->total_children, children_pct);
                ImGui::Text("Adults: %d (%.1f%%)", pop_comp->total_adults, adults_pct);
                ImGui::Text("Elderly: %d (%.1f%%)", pop_comp->total_elderly, elderly_pct);
                ImGui::Unindent();

                ImGui::Spacing();

                ImGui::Text("Gender Distribution:");
                ImGui::Indent();
                float males_pct = (pop_comp->total_males * 100.0f) / total;
                float females_pct = (pop_comp->total_females * 100.0f) / total;
                ImGui::Text("Males: %d (%.1f%%)", pop_comp->total_males, males_pct);
                ImGui::Text("Females: %d (%.1f%%)", pop_comp->total_females, females_pct);
                ImGui::Unindent();
            }
        }

        ImGui::Spacing();

        // Quality of Life
        if (ImGui::CollapsingHeader("Quality of Life", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Happiness:");
            ImGui::SameLine(200);
            ImGui::Text("%.1f%%", pop_comp->average_happiness * 100.0f);

            ImGui::Text("Health:");
            ImGui::SameLine(200);
            ImGui::Text("%.1f%%", pop_comp->average_health * 100.0f);

            ImGui::Text("Literacy:");
            ImGui::SameLine(200);
            ImGui::Text("%.1f%%", pop_comp->average_literacy * 100.0f);

            ImGui::Text("Average Wealth:");
            ImGui::SameLine(200);
            ImGui::Text("%.0f gold", pop_comp->average_wealth);
        }

        ImGui::Spacing();

        // Employment
        if (ImGui::CollapsingHeader("Employment")) {
            ImGui::Text("Employment Rate:");
            ImGui::SameLine(200);
            ImGui::Text("%.1f%%", pop_comp->overall_employment_rate * 100.0f);

            ImGui::Spacing();
            ImGui::Indent();
            ImGui::Text("Productive Workers: %d", pop_comp->productive_workers);
            ImGui::Text("Unemployed Seeking: %d", pop_comp->unemployed_seeking);
            ImGui::Text("Non-Productive Income: %d", pop_comp->non_productive_income);
            ImGui::Text("Dependents: %d", pop_comp->dependents);
            ImGui::Unindent();
        }
    }

    void ProvinceInfoWindow::RenderReligionTab() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto pop_comp = entity_manager_.GetComponent<game::population::PopulationComponent>(selected_province);

        if (!pop_comp) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No population data available");
            ImGui::Spacing();
            ImGui::TextWrapped("This province does not have the PopulationComponent. Religion and culture data is stored within the population component.");
            return;
        }

        // Religion Distribution
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Religious Composition");
        ImGui::Separator();

        if (pop_comp->religion_distribution.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No religious data available");
        } else {
            ImGui::BeginTable("ReligionTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
            ImGui::TableSetupColumn("Religion", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Population", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Percentage", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            for (const auto& [religion, count] : pop_comp->religion_distribution) {
                float pct = (count * 100.0f) / pop_comp->total_population;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", religion.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%d", count);

                ImGui::TableNextColumn();
                ImGui::Text("%.1f%%", pct);
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Culture Distribution
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.9f, 1.0f), "Cultural Composition");
        ImGui::Separator();

        if (pop_comp->culture_distribution.empty()) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No cultural data available");
        } else {
            ImGui::BeginTable("CultureTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
            ImGui::TableSetupColumn("Culture", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Population", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Percentage", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            for (const auto& [culture, count] : pop_comp->culture_distribution) {
                float pct = (count * 100.0f) / pop_comp->total_population;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", culture.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%d", count);

                ImGui::TableNextColumn();
                ImGui::Text("%.1f%%", pct);
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Social Dynamics
        ImGui::TextColored(ImVec4(0.9f, 0.7f, 1.0f, 1.0f), "Social Dynamics");
        ImGui::Separator();

        ImGui::Text("Religious Conversion Rate:");
        ImGui::SameLine(250);
        ImGui::Text("%.2f%%", pop_comp->religious_conversion_rate * 100.0f);

        ImGui::Text("Cultural Assimilation Rate:");
        ImGui::SameLine(250);
        ImGui::Text("%.2f%%", pop_comp->cultural_assimilation_rate * 100.0f);

        ImGui::Text("Social Mobility:");
        ImGui::SameLine(250);
        ImGui::Text("%.2f%%", pop_comp->social_mobility_average * 100.0f);

        ImGui::Text("Inter-Class Tension:");
        ImGui::SameLine(250);
        ImGui::Text("%.1f%%", pop_comp->inter_class_tension * 100.0f);
    }

    void ProvinceInfoWindow::RenderAdministrationTab() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto province_data = entity_manager_.GetComponent<game::province::ProvinceDataComponent>(selected_province);

        if (!province_data) return;

        ImGui::TextColored(ImVec4(0.9f, 0.7f, 1.0f, 1.0f), "Administrative Status");
        ImGui::Separator();

        // Autonomy
        ImGui::Text("Local Autonomy:");
        ImVec4 autonomy_color = province_data->autonomy > 0.5 ?
                               ImVec4(1.0f, 0.5f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.5f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, autonomy_color);
        ImGui::ProgressBar(static_cast<float>(province_data->autonomy), ImVec2(-1, 0),
                          (std::to_string(static_cast<int>(province_data->autonomy * 100)) + "%").c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // Stability
        ImGui::Text("Stability:");
        ImVec4 stability_color = province_data->stability > 0.5 ?
                                ImVec4(0.3f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.5f, 0.3f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, stability_color);
        ImGui::ProgressBar(static_cast<float>(province_data->stability), ImVec2(-1, 0),
                          (std::to_string(static_cast<int>(province_data->stability * 100)) + "%").c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // War Exhaustion
        if (province_data->war_exhaustion > 0.01) {
            ImGui::Text("War Exhaustion:");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::ProgressBar(static_cast<float>(province_data->war_exhaustion), ImVec2(-1, 0),
                              (std::to_string(static_cast<int>(province_data->war_exhaustion * 100)) + "%").c_str());
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Development
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Development");
        ImGui::Separator();

        ImGui::Text("Current Level:");
        ImGui::SameLine(200);
        ImGui::Text("%d / %d", province_data->development_level, province_data->max_development);

        float dev_pct = static_cast<float>(province_data->development_level) / static_cast<float>(province_data->max_development);
        ImGui::ProgressBar(dev_pct, ImVec2(-1, 0));

        ImGui::Spacing();
        ImGui::Spacing();

        // Geographic Info
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Geographic Data");
        ImGui::Separator();

        ImGui::Text("Area:");
        ImGui::SameLine(200);
        ImGui::Text("%.1f km²", province_data->area);

        ImGui::Text("Coordinates:");
        ImGui::SameLine(200);
        ImGui::Text("(%.2f, %.2f)", province_data->x_coordinate, province_data->y_coordinate);
    }

    void ProvinceInfoWindow::RenderGeographyTab() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto province_data = entity_manager_.GetComponent<game::province::ProvinceDataComponent>(selected_province);

        if (!province_data) return;

        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Geographic Information");
        ImGui::Separator();

        ImGui::Text("Coordinates:");
        ImGui::Text("  X: %.2f", province_data->x_coordinate);
        ImGui::Text("  Y: %.2f", province_data->y_coordinate);

        ImGui::Spacing();

        ImGui::Text("Area:");
        ImGui::Text("  %.1f km²", province_data->area);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped("Geographic features, terrain type, and climate information will be displayed here in future updates.");
    }

    const char* ProvinceInfoWindow::GetBuildingName(int building_type, bool is_production) {
        if (is_production) {
            switch (static_cast<game::province::ProductionBuilding>(building_type)) {
                case game::province::ProductionBuilding::FARM: return "Farm";
                case game::province::ProductionBuilding::MARKET: return "Market";
                case game::province::ProductionBuilding::SMITHY: return "Smithy";
                case game::province::ProductionBuilding::WORKSHOP: return "Workshop";
                case game::province::ProductionBuilding::MINE: return "Mine";
                case game::province::ProductionBuilding::TEMPLE: return "Temple";
                default: return "Unknown";
            }
        } else {
            switch (static_cast<game::province::InfrastructureBuilding>(building_type)) {
                case game::province::InfrastructureBuilding::ROAD: return "Road";
                case game::province::InfrastructureBuilding::PORT: return "Port";
                case game::province::InfrastructureBuilding::FORTRESS: return "Fortress";
                case game::province::InfrastructureBuilding::UNIVERSITY: return "University";
                default: return "Unknown";
            }
        }
    }

    const char* ProvinceInfoWindow::GetUnitTypeName(int unit_type) {
        using UT = game::military::UnitType;
        switch (static_cast<UT>(unit_type)) {
            // Infantry
            case UT::LEVIES: return "Levies";
            case UT::SPEARMEN: return "Spearmen";
            case UT::SWORDSMEN: return "Swordsmen";
            case UT::CROSSBOWMEN: return "Crossbowmen";
            case UT::LONGBOWMEN: return "Longbowmen";
            case UT::MEN_AT_ARMS: return "Men-at-Arms";
            case UT::PIKEMEN: return "Pikemen";
            case UT::ARQUEBUSIERS: return "Arquebusiers";
            case UT::MUSKETEERS: return "Musketeers";

            // Cavalry
            case UT::LIGHT_CAVALRY: return "Light Cavalry";
            case UT::HEAVY_CAVALRY: return "Heavy Cavalry";
            case UT::MOUNTED_ARCHERS: return "Mounted Archers";
            case UT::DRAGOONS: return "Dragoons";

            // Siege Equipment
            case UT::CATAPULTS: return "Catapults";
            case UT::TREBUCHETS: return "Trebuchets";
            case UT::CANNONS: return "Cannons";
            case UT::SIEGE_TOWERS: return "Siege Towers";

            // Naval Units
            case UT::GALLEYS: return "Galleys";
            case UT::COGS: return "Cogs";
            case UT::CARRACKS: return "Carracks";
            case UT::GALLEONS: return "Galleons";
            case UT::WAR_GALLEONS: return "War Galleons";
            case UT::SHIPS_OF_THE_LINE: return "Ships of the Line";

            default: return "Unknown Unit";
        }
    }

    const char* ProvinceInfoWindow::GetMoraleStateName(int morale_state) {
        using MS = game::military::MoraleState;
        switch (static_cast<MS>(morale_state)) {
            case MS::ROUTING: return "Routing";
            case MS::BROKEN: return "Broken";
            case MS::WAVERING: return "Wavering";
            case MS::STEADY: return "Steady";
            case MS::CONFIDENT: return "Confident";
            case MS::FANATICAL: return "Fanatical";
            default: return "Unknown";
        }
    }

} // namespace ui
