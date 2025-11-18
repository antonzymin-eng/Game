#include "ui/InGameHUD.h"
#include "imgui.h"
#include <cmath>

namespace ui {

InGameHUD::InGameHUD()
    : menu_requested_(false)
    , show_minimap_(true)
    , show_tooltips_(true) {
}

void InGameHUD::Render() {
    RenderTopBar();
    RenderResourcePanel();
    RenderQuickActions();
    RenderNotifications();
    if (show_minimap_) {
        RenderMinimap();
    }
    RenderBottomBar();
}

void InGameHUD::Update() {
    // Update logic if needed
}

void InGameHUD::RenderTopBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float bar_height = 40.0f;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(screen_size.x, bar_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.92f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20, 0));

    if (ImGui::Begin("##TopBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        // Nation name
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("%s", game_stats_.nation_name.c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        // Menu button (ESC)
        if (ImGui::Button("MENU (ESC)")) {
            menu_requested_ = true;
        }

        // Right-aligned items
        ImGui::SameLine(screen_size.x - 350);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
        ImGui::Text("Prestige: %d", game_stats_.prestige);
        ImGui::SameLine();
        ImGui::Text("Stability: %d%%", game_stats_.stability);
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void InGameHUD::RenderResourcePanel() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float panel_width = 350.0f;
    float panel_height = 90.0f;
    float panel_x = screen_size.x - panel_width - 10;
    float panel_y = 50.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + panel_x, viewport->Pos.y + panel_y));
    ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    if (ImGui::Begin("##Resources", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("RESOURCES");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Treasury
        ImGui::Columns(2, "resources", false);
        ImGui::SetColumnWidth(0, 120);

        auto RenderResource = [](const char* icon, const char* label, int value, int change) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
            ImGui::Text("%s %s:", icon, label);
            ImGui::PopStyleColor();
            ImGui::NextColumn();

            ImU32 change_color = change >= 0 ? IM_COL32(100, 200, 100, 255) : IM_COL32(200, 100, 100, 255);
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%d (%s%d)", value, change >= 0 ? "+" : "", change);

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
            ImGui::Text("%s", buffer);
            ImGui::PopStyleColor();
            ImGui::NextColumn();
        };

        RenderResource("$", "Treasury", game_stats_.treasury, game_stats_.monthly_income);
        RenderResource("M", "Manpower", game_stats_.manpower, 0);

        ImGui::Columns(1);
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void InGameHUD::RenderQuickActions() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float panel_width = 60.0f;
    float panel_height = 250.0f;
    float panel_x = 10.0f;
    float panel_y = screen_size.y * 0.5f - panel_height * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + panel_x, viewport->Pos.y + panel_y));
    ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.13f, 0.07f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.20f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.23f, 0.12f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 5));

    if (ImGui::Begin("##QuickActions", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImVec2 button_size(50, 45);

        // Quick action buttons with tooltips
        if (ImGui::Button("N##nation", button_size)) {
            // Open nation overview
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Nation Overview (F1)");
        }

        if (ImGui::Button("M##military", button_size)) {
            // Open military view
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Military (F2)");
        }

        if (ImGui::Button("E##economy", button_size)) {
            // Open economy view
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Economy (F3)");
        }

        if (ImGui::Button("D##diplomacy", button_size)) {
            // Open diplomacy view
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Diplomacy (F4)");
        }

        if (ImGui::Button("T##tech", button_size)) {
            // Open technology view
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Technology (F5)");
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
}

void InGameHUD::RenderNotifications() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float panel_width = 300.0f;
    float panel_height = 150.0f;
    float panel_x = screen_size.x - panel_width - 10;
    float panel_y = screen_size.y - panel_height - 60;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + panel_x, viewport->Pos.y + panel_y));
    ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.88f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

    if (ImGui::Begin("##Notifications", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("NOTIFICATIONS");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginChild("NotificationScroll");

        // Example notifications
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::BulletText("New technology available");
        ImGui::BulletText("Province development complete");
        ImGui::BulletText("Trade route established");
        ImGui::PopStyleColor();

        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void InGameHUD::RenderMinimap() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float minimap_size = 200.0f;
    float panel_x = 10.0f;
    float panel_y = screen_size.y - minimap_size - 50;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + panel_x, viewport->Pos.y + panel_y));
    ImGui::SetNextWindowSize(ImVec2(minimap_size, minimap_size));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.88f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

    if (ImGui::Begin("##Minimap", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();

        // Background
        draw_list->AddRectFilled(
            canvas_pos,
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(30, 50, 70, 255)
        );

        // Simple minimap representation
        draw_list->AddRectFilled(
            ImVec2(canvas_pos.x + 20, canvas_pos.y + 20),
            ImVec2(canvas_pos.x + canvas_size.x - 20, canvas_pos.y + canvas_size.y - 20),
            IM_COL32(100, 140, 100, 255)
        );

        // Border
        draw_list->AddRect(
            canvas_pos,
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(107, 93, 79, 255),
            0.0f,
            0,
            1.0f
        );

        // "MINIMAP" label
        draw_list->AddText(
            ImVec2(canvas_pos.x + 5, canvas_pos.y + 5),
            IM_COL32(212, 175, 55, 200),
            "MINIMAP"
        );
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void InGameHUD::RenderBottomBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float bar_height = 30.0f;
    float bar_y = screen_size.y - bar_height;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + bar_y));
    ImGui::SetNextWindowSize(ImVec2(screen_size.x, bar_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.92f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 5));

    if (ImGui::Begin("##BottomBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("Click on provinces to view details | Right-click to interact | Use WASD or arrow keys to pan camera");
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

} // namespace ui
