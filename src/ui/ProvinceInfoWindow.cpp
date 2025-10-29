// ============================================================================
// ProvinceInfoWindow.cpp - Detailed province information panel implementation
// ============================================================================

#include "ui/ProvinceInfoWindow.h"
#include <imgui.h>
#include <sstream>
#include <iomanip>

namespace ui {

    ProvinceInfoWindow::ProvinceInfoWindow()
        : selected_province_(nullptr)
        , visible_(true) {
    }

    void ProvinceInfoWindow::Render() {
        if (!visible_ || !selected_province_) {
            return;
        }

        // Position window on the left side of screen
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;

        ImVec2 window_pos = ImVec2(work_pos.x + 10, work_pos.y + 60);
        ImVec2 window_size = ImVec2(350, work_size.y - 80);

        ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(0.9f);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin("Province Information", &visible_, window_flags)) {
            RenderHeader();
            ImGui::Separator();

            // Tabbed interface for different sections
            if (ImGui::BeginTabBar("ProvinceInfoTabs")) {
                if (ImGui::BeginTabItem("Overview")) {
                    RenderPopulationSection();
                    ImGui::Spacing();
                    RenderEconomySection();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Administration")) {
                    RenderAdministrationSection();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Geography")) {
                    RenderGeographySection();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void ProvinceInfoWindow::SetSelectedProvince(const game::Province* province) {
        selected_province_ = province;
    }

    void ProvinceInfoWindow::ClearSelection() {
        selected_province_ = nullptr;
    }

    void ProvinceInfoWindow::RenderHeader() {
        if (!selected_province_) return;

        // Province name (large and prominent)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.5f, 1.0f)); // Gold
        ImGui::SetWindowFontScale(1.5f);
        ImGui::Text("%s", selected_province_->name.c_str());
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        // Province ID
        ImGui::Text("Province ID: %d", selected_province_->id);

        // Owner nation
        if (selected_province_->owner_nation_id > 0) {
            ImGui::Text("Owner: Nation %d", selected_province_->owner_nation_id);
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Unowned Territory");
        }
    }

    void ProvinceInfoWindow::RenderPopulationSection() {
        if (!selected_province_) return;

        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Population");
        ImGui::Separator();

        ImGui::Text("Current Population:");
        ImGui::SameLine(200);
        ImGui::Text("%d", selected_province_->current_population);

        ImGui::Text("Base Population:");
        ImGui::SameLine(200);
        ImGui::Text("%d", selected_province_->base_population);

        // Population growth indicator
        int population_diff = selected_province_->current_population - selected_province_->base_population;
        ImGui::Text("Growth:");
        ImGui::SameLine(200);
        if (population_diff > 0) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "+%d", population_diff);
        } else if (population_diff < 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%d", population_diff);
        } else {
            ImGui::Text("0");
        }
    }

    void ProvinceInfoWindow::RenderEconomySection() {
        if (!selected_province_) return;

        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Economy");
        ImGui::Separator();

        ImGui::Text("Development Level:");
        ImGui::SameLine(200);
        ImGui::Text("%d", selected_province_->development_level);

        ImGui::Text("Tax Capacity:");
        ImGui::SameLine(200);
        ImGui::Text("%d ducats", selected_province_->base_tax_capacity);

        // Development bar
        ImGui::Text("Development:");
        float dev_percentage = static_cast<float>(selected_province_->development_level) / 10.0f;
        dev_percentage = std::min(dev_percentage, 1.0f);

        ImGui::ProgressBar(dev_percentage, ImVec2(-1, 0));
    }

    void ProvinceInfoWindow::RenderAdministrationSection() {
        if (!selected_province_) return;

        ImGui::TextColored(ImVec4(0.9f, 0.7f, 1.0f, 1.0f), "Administration");
        ImGui::Separator();

        // Admin Efficiency
        ImGui::Text("Administrative Efficiency:");
        ImGui::ProgressBar(selected_province_->admin_efficiency, ImVec2(-1, 0),
                          (std::to_string(static_cast<int>(selected_province_->admin_efficiency * 100)) + "%").c_str());

        ImGui::Spacing();

        // Autonomy
        ImGui::Text("Local Autonomy:");
        ImVec4 autonomy_color = selected_province_->autonomy > 0.5f ?
                               ImVec4(1.0f, 0.5f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.5f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, autonomy_color);
        ImGui::ProgressBar(selected_province_->autonomy, ImVec2(-1, 0),
                          (std::to_string(static_cast<int>(selected_province_->autonomy * 100)) + "%").c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // Stability
        ImGui::Text("Stability:");
        ImVec4 stability_color = selected_province_->stability > 0.5f ?
                                ImVec4(0.3f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.5f, 0.3f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, stability_color);
        ImGui::ProgressBar(selected_province_->stability, ImVec2(-1, 0),
                          (std::to_string(static_cast<int>(selected_province_->stability * 100)) + "%").c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // War Exhaustion
        if (selected_province_->war_exhaustion > 0.01f) {
            ImGui::Text("War Exhaustion:");
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::ProgressBar(selected_province_->war_exhaustion, ImVec2(-1, 0),
                              (std::to_string(static_cast<int>(selected_province_->war_exhaustion * 100)) + "%").c_str());
            ImGui::PopStyleColor();
        }
    }

    void ProvinceInfoWindow::RenderGeographySection() {
        if (!selected_province_) return;

        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Geography");
        ImGui::Separator();

        ImGui::Text("Coordinates:");
        ImGui::Text("  X: %.2f", selected_province_->x_coordinate);
        ImGui::Text("  Y: %.2f", selected_province_->y_coordinate);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextWrapped("Geographic features, terrain type, and climate information will be displayed here in future updates.");
    }

} // namespace ui
