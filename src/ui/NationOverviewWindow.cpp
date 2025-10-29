// ============================================================================
// NationOverviewWindow.cpp - Nation statistics overview implementation
// ============================================================================

#include "ui/NationOverviewWindow.h"
#include <imgui.h>

namespace ui {

    NationOverviewWindow::NationOverviewWindow()
        : visible_(false) {
    }

    void NationOverviewWindow::Render() {
        if (!visible_) return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;

        // Center the window
        ImVec2 window_size(600, 500);
        ImVec2 window_pos(work_pos.x + (work_size.x - window_size.x) * 0.5f,
                         work_pos.y + (work_size.y - window_size.y) * 0.5f);

        ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Nation Overview", &visible_)) {
            // Nation name and flag (placeholder)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.5f, 1.0f));
            ImGui::SetWindowFontScale(1.5f);
            ImGui::Text("Your Nation");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();

            ImGui::Separator();

            // Tabbed interface
            if (ImGui::BeginTabBar("NationTabs")) {
                if (ImGui::BeginTabItem("Economy")) {
                    RenderEconomyTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Military")) {
                    RenderMilitaryTab();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Diplomacy")) {
                    RenderDiplomacyTab();
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    void NationOverviewWindow::RenderEconomyTab() {
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Treasury");
        ImGui::Separator();

        // Placeholder economic data
        ImGui::Text("Treasury:");
        ImGui::SameLine(200);
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "10,000 ducats");

        ImGui::Text("Monthly Income:");
        ImGui::SameLine(200);
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "+500 ducats");

        ImGui::Text("Monthly Expenses:");
        ImGui::SameLine(200);
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "-300 ducats");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Income Breakdown");
        ImGui::Separator();

        ImGui::BulletText("Taxes: 300 ducats");
        ImGui::BulletText("Trade: 150 ducats");
        ImGui::BulletText("Production: 50 ducats");

        ImGui::Spacing();
        ImGui::TextWrapped("Connect to EconomicSystem for real-time data");
    }

    void NationOverviewWindow::RenderMilitaryTab() {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Military Forces");
        ImGui::Separator();

        // Placeholder military data
        ImGui::Text("Total Forces:");
        ImGui::SameLine(200);
        ImGui::Text("5,000");

        ImGui::Text("Infantry:");
        ImGui::SameLine(200);
        ImGui::Text("3,000");

        ImGui::Text("Cavalry:");
        ImGui::SameLine(200);
        ImGui::Text("1,500");

        ImGui::Text("Artillery:");
        ImGui::SameLine(200);
        ImGui::Text("500");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Military Capacity");
        ImGui::Separator();

        ImGui::Text("Force Limit:");
        ImGui::SameLine(200);
        ImGui::Text("6,000");

        ImGui::Text("Manpower:");
        ImGui::SameLine(200);
        ImGui::Text("2,500");

        ImGui::Spacing();
        ImGui::TextWrapped("Connect to MilitarySystem for real-time data");
    }

    void NationOverviewWindow::RenderDiplomacyTab() {
        ImGui::TextColored(ImVec4(0.7f, 1.0f, 0.7f, 1.0f), "Diplomatic Relations");
        ImGui::Separator();

        // Placeholder diplomatic data
        ImGui::Text("Allies:");
        ImGui::SameLine(200);
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "3");

        ImGui::Text("Rivals:");
        ImGui::SameLine(200);
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "2");

        ImGui::Text("Trade Partners:");
        ImGui::SameLine(200);
        ImGui::Text("5");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.9f, 0.7f, 1.0f, 1.0f), "Treaties");
        ImGui::Separator();

        ImGui::BulletText("Alliance with Nation A");
        ImGui::BulletText("Trade Agreement with Nation B");
        ImGui::BulletText("Non-Aggression Pact with Nation C");

        ImGui::Spacing();
        ImGui::TextWrapped("Connect to DiplomacySystem for real-time data");
    }

} // namespace ui
