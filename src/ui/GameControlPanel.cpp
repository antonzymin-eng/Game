// ============================================================================
// GameControlPanel.cpp - Main game control UI implementation
// ============================================================================

#include "ui/GameControlPanel.h"
#include <imgui.h>
#include <sstream>
#include <iomanip>

namespace ui {

    GameControlPanel::GameControlPanel()
        : current_speed_(GameSpeed::PAUSED)
        , visible_(true) {
        // Initialize with default date
        current_date_ = game::time::GameDate(1000, 1, 1);
    }

    void GameControlPanel::Render() {
        if (!visible_) return;

        // Position panel at top-center of screen
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;

        ImVec2 window_pos = ImVec2(work_pos.x + work_size.x * 0.5f, work_pos.y + 10.0f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.5f, 0.0f));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                       ImGuiWindowFlags_AlwaysAutoResize |
                                       ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoFocusOnAppearing |
                                       ImGuiWindowFlags_NoNav;

        ImGui::SetNextWindowBgAlpha(0.85f); // Slightly transparent

        if (ImGui::Begin("GameControl", nullptr, window_flags)) {
            // Date display (large and prominent)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.7f, 1.0f)); // Gold color
            ImGui::SetWindowFontScale(1.3f);

            std::ostringstream date_stream;
            date_stream << std::setw(2) << std::setfill('0') << current_date_.day << " "
                       << current_date_.month << " "
                       << current_date_.year;

            ImGui::Text("%s", date_stream.str().c_str());
            ImGui::SetWindowFontScale(1.0f);
            ImGui::PopStyleColor();

            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();

            // Speed control buttons
            ImGui::BeginGroup();

            // Pause button
            bool is_paused = (current_speed_ == GameSpeed::PAUSED);
            if (is_paused) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
            }
            if (ImGui::Button(GetSpeedIcon(GameSpeed::PAUSED), ImVec2(40, 30))) {
                current_speed_ = GameSpeed::PAUSED;
            }
            if (is_paused) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Pause");
            }

            ImGui::SameLine();

            // Speed 1 button
            bool is_speed1 = (current_speed_ == GameSpeed::SPEED_1);
            if (is_speed1) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
            }
            if (ImGui::Button(GetSpeedIcon(GameSpeed::SPEED_1), ImVec2(40, 30))) {
                current_speed_ = GameSpeed::SPEED_1;
            }
            if (is_speed1) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Normal Speed");
            }

            ImGui::SameLine();

            // Speed 2 button
            bool is_speed2 = (current_speed_ == GameSpeed::SPEED_2);
            if (is_speed2) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
            }
            if (ImGui::Button(GetSpeedIcon(GameSpeed::SPEED_2), ImVec2(40, 30))) {
                current_speed_ = GameSpeed::SPEED_2;
            }
            if (is_speed2) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("2x Speed");
            }

            ImGui::SameLine();

            // Speed 3 button
            bool is_speed3 = (current_speed_ == GameSpeed::SPEED_3);
            if (is_speed3) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
            }
            if (ImGui::Button(GetSpeedIcon(GameSpeed::SPEED_3), ImVec2(40, 30))) {
                current_speed_ = GameSpeed::SPEED_3;
            }
            if (is_speed3) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("3x Speed");
            }

            ImGui::SameLine();

            // Speed 4 button
            bool is_speed4 = (current_speed_ == GameSpeed::SPEED_4);
            if (is_speed4) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
            }
            if (ImGui::Button(GetSpeedIcon(GameSpeed::SPEED_4), ImVec2(40, 30))) {
                current_speed_ = GameSpeed::SPEED_4;
            }
            if (is_speed4) {
                ImGui::PopStyleColor();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("4x Speed");
            }

            ImGui::EndGroup();

            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();

            // Quick save/load buttons
            if (ImGui::Button("Save", ImVec2(50, 30))) {
                // TODO: Trigger save
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Quick Save (F5)");
            }

            ImGui::SameLine();

            if (ImGui::Button("Load", ImVec2(50, 30))) {
                // TODO: Trigger load
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Quick Load (F9)");
            }
        }
        ImGui::End();
    }

    void GameControlPanel::SetCurrentDate(const game::time::GameDate& date) {
        current_date_ = date;
    }

    std::string GameControlPanel::GetSpeedButtonLabel(GameSpeed speed) const {
        switch (speed) {
        case GameSpeed::PAUSED: return "||";
        case GameSpeed::SPEED_1: return ">";
        case GameSpeed::SPEED_2: return ">>";
        case GameSpeed::SPEED_3: return ">>>";
        case GameSpeed::SPEED_4: return ">>>>";
        default: return "?";
        }
    }

    const char* GameControlPanel::GetSpeedIcon(GameSpeed speed) const {
        switch (speed) {
        case GameSpeed::PAUSED: return "||";
        case GameSpeed::SPEED_1: return ">";
        case GameSpeed::SPEED_2: return ">>";
        case GameSpeed::SPEED_3: return ">>>";
        case GameSpeed::SPEED_4: return ">>>>";
        default: return "?";
        }
    }

} // namespace ui
