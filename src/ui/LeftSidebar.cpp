#include "ui/LeftSidebar.h"

namespace ui {

LeftSidebar::LeftSidebar(WindowManager& window_manager)
    : window_manager_(window_manager) {
    InitializeIcons();
}

void LeftSidebar::InitializeIcons() {
    // Initialize system icons in EU4-style order
    icons_ = {
        {"N", "Nation Overview (F1)", WindowManager::WindowType::NATION_OVERVIEW, IM_COL32(212, 175, 55, 255), "N"},
        {"E", "Economy (F2)", WindowManager::WindowType::ECONOMY, IM_COL32(100, 200, 100, 255), "$"},
        {"M", "Military (F3)", WindowManager::WindowType::MILITARY, IM_COL32(200, 100, 100, 255), "M"},
        {"D", "Diplomacy (F4)", WindowManager::WindowType::DIPLOMACY, IM_COL32(100, 150, 200, 255), "D"},
        {"T", "Technology (F5)", WindowManager::WindowType::TECHNOLOGY, IM_COL32(150, 100, 200, 255), "T"},
        {"P", "Population (F6)", WindowManager::WindowType::POPULATION, IM_COL32(200, 150, 100, 255), "P"},
        {"TR", "Trade (F7)", WindowManager::WindowType::TRADE, IM_COL32(150, 200, 100, 255), "TR"},
        {"R", "Realm & Dynasty (F8)", WindowManager::WindowType::REALM, IM_COL32(180, 140, 200, 255), "R"},
        {"A", "Administration (F9)", WindowManager::WindowType::ADMINISTRATION, IM_COL32(200, 180, 120, 255), "A"}
    };
}

void LeftSidebar::Render() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    // Sidebar dimensions
    float sidebar_width = 60.0f;
    float icon_size = 50.0f;
    float icon_spacing = 5.0f;
    float start_y = 50.0f; // Below top bar

    // Render sidebar background
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + start_y));
    ImGui::SetNextWindowSize(ImVec2(sidebar_width, screen_size.y - start_y));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar
                           | ImGuiWindowFlags_NoResize
                           | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoCollapse
                           | ImGuiWindowFlags_NoScrollbar
                           | ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("##LeftSidebar", nullptr, flags)) {
        float current_y = 0.0f;

        for (const auto& icon : icons_) {
            bool is_active = window_manager_.IsWindowOpen(icon.window_type);

            ImVec2 icon_pos = ImVec2(5, current_y);
            ImVec2 icon_size_vec = ImVec2(icon_size, icon_size);

            RenderIcon(icon, icon_pos, icon_size_vec, is_active);

            current_y += icon_size + icon_spacing;
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void LeftSidebar::RenderIcon(const SystemIcon& icon, const ImVec2& pos, const ImVec2& size, bool is_active) {
    ImGui::SetCursorPos(pos);

    ImVec2 screen_pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Determine button color
    ImU32 bg_color;
    ImU32 border_color;

    if (is_active) {
        bg_color = IM_COL32(51, 34, 17, 255);
        border_color = icon.color;
    } else {
        bg_color = IM_COL32(34, 24, 16, 200);
        border_color = IM_COL32(107, 93, 79, 255);
    }

    // Check if hovered
    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    bool is_hovered = mouse_pos.x >= screen_pos.x && mouse_pos.x <= screen_pos.x + size.x &&
                      mouse_pos.y >= screen_pos.y && mouse_pos.y <= screen_pos.y + size.y;

    if (is_hovered) {
        bg_color = IM_COL32(51, 34, 17, 240);
        border_color = icon.color;
    }

    // Draw button background
    draw_list->AddRectFilled(screen_pos, ImVec2(screen_pos.x + size.x, screen_pos.y + size.y), bg_color, 4.0f);

    // Draw border
    draw_list->AddRect(screen_pos, ImVec2(screen_pos.x + size.x, screen_pos.y + size.y), border_color, 4.0f, 0, 2.0f);

    // Draw icon text (centered)
    ImVec2 text_size = ImGui::CalcTextSize(icon.icon_text);
    ImVec2 text_pos = ImVec2(
        screen_pos.x + (size.x - text_size.x) * 0.5f,
        screen_pos.y + (size.y - text_size.y) * 0.5f
    );

    draw_list->AddText(text_pos, icon.color, icon.icon_text);

    // Invisible button for interaction
    ImGui::SetCursorPos(pos);
    ImGui::InvisibleButton(("##icon_" + std::string(icon.label)).c_str(), size);

    if (ImGui::IsItemClicked()) {
        window_manager_.ToggleWindow(icon.window_type);
    }

    // Tooltip
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", icon.tooltip);
    }
}

} // namespace ui
