#include "ui/InGameHUD.h"
#include "imgui.h"
#include <cmath>

namespace ui {

InGameHUD::InGameHUD(core::ecs::EntityManager& entity_manager,
                     game::economy::EconomicSystem& economic_system,
                     game::military::MilitarySystem& military_system)
    : entity_manager_(entity_manager)
    , economic_system_(economic_system)
    , military_system_(military_system)
    , menu_requested_(false)
    , show_minimap_(true)
    , show_tooltips_(true)
    , show_pause_menu_(false) {
}

void InGameHUD::Render(game::types::EntityID player_entity) {
    RenderTopBar(player_entity);
    RenderResourcePanel(player_entity);
    RenderNotifications();
    if (show_minimap_) {
        RenderMinimap();
    }
    RenderBottomBar();

    if (show_pause_menu_) {
        RenderPauseMenu();
    }
}

void InGameHUD::Update() {
    // Update logic if needed
}

void InGameHUD::RenderTopBar(game::types::EntityID player_entity) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float bar_height = 40.0f;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(screen_size.x, bar_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.92f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(20, 0));

    if (ImGui::Begin("##TopBar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        // Nation name (TODO: Get from realm system)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("Kingdom of Francia"); // TODO: Real nation name
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextUnformatted("|");
        ImGui::SameLine();

        // Menu button (ESC)
        if (ImGui::Button("MENU (ESC)")) {
            show_pause_menu_ = !show_pause_menu_;
        }

        // Right-aligned items
        ImGui::SameLine(screen_size.x - 350);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
        ImGui::Text("Prestige: %d", 100); // TODO: Real prestige
        ImGui::SameLine();
        ImGui::Text("Stability: %d%%", 75); // TODO: Real stability
        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void InGameHUD::RenderResourcePanel(game::types::EntityID player_entity) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float panel_width = 350.0f;
    float panel_height = 120.0f;
    float panel_x = screen_size.x - panel_width - 10;
    float panel_y = 50.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + panel_x, viewport->Pos.y + panel_y));
    ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.92f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));

    if (ImGui::Begin("##ResourcePanel", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        // Get live data from economic system
        int treasury = economic_system_.GetTreasury(player_entity);
        int monthly_income = economic_system_.GetMonthlyIncome(player_entity);
        int monthly_expenses = economic_system_.GetMonthlyExpenses(player_entity);
        int net_income = economic_system_.GetNetIncome(player_entity);

        // Get military data
        auto armies = military_system_.GetAllArmies();
        size_t army_count = armies.size();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("RESOURCES");
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::Spacing();

        // Treasury row
        ImGui::Text("Treasury:");
        ImGui::SameLine(120);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.84f, 0.0f, 1.0f)); // Gold color
        ImGui::Text("$%d", treasury);
        ImGui::PopStyleColor();

        // Income row
        ImGui::Text("Monthly Income:");
        ImGui::SameLine(120);
        ImGui::PushStyleColor(ImGuiCol_Text, net_income >= 0 ?
                              ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::Text("%+d", net_income);
        ImGui::PopStyleColor();

        // Manpower row (TODO: Real manpower from population system)
        ImGui::Text("Manpower:");
        ImGui::SameLine(120);
        ImGui::Text("%d / %d", 0, 10000);

        // Armies row
        ImGui::Text("Armies:");
        ImGui::SameLine(120);
        ImGui::Text("%zu", army_count);
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void InGameHUD::RenderPauseMenu() {
    // Modal overlay
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 350), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.95f));

    if (ImGui::Begin("##PauseMenu", &show_pause_menu_,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("GAME PAUSED").x) * 0.5f);
        ImGui::Text("GAME PAUSED");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();

        // Center the buttons
        float button_width = 250.0f;
        float button_offset = (ImGui::GetWindowWidth() - button_width) * 0.5f;

        ImGui::SetCursorPosX(button_offset);
        if (ImGui::Button("Resume (ESC)", ImVec2(button_width, 40))) {
            show_pause_menu_ = false;
        }

        ImGui::Spacing();

        ImGui::SetCursorPosX(button_offset);
        if (ImGui::Button("Save Game", ImVec2(button_width, 40))) {
            // TODO: Show save dialog
        }

        ImGui::Spacing();

        ImGui::SetCursorPosX(button_offset);
        if (ImGui::Button("Load Game", ImVec2(button_width, 40))) {
            // TODO: Show load dialog
        }

        ImGui::Spacing();

        ImGui::SetCursorPosX(button_offset);
        if (ImGui::Button("Settings", ImVec2(button_width, 40))) {
            // TODO: Open settings window
        }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetCursorPosX(button_offset);
        if (ImGui::Button("Exit to Main Menu", ImVec2(button_width, 40))) {
            // TODO: Confirm and exit to menu
            menu_requested_ = true;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Return to main menu (unsaved progress will be lost)");
        }
    }
    ImGui::End();

    ImGui::PopStyleColor();
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
    float bottom_bar_height = 30.0f;
    float padding = 10.0f;
    float panel_y = screen_size.y - minimap_size - bottom_bar_height - padding;

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
