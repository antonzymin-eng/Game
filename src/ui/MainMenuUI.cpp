#include "ui/MainMenuUI.h"
#include "imgui.h"
#include <cmath>

namespace ui {

MainMenuUI::MainMenuUI()
    : last_action_(MenuAction::NONE)
    , selected_index_(-1)
    , animation_time_(0.0f) {
    InitializeMenuItems();
}

MainMenuUI::~MainMenuUI() {
}

void MainMenuUI::InitializeMenuItems() {
    menu_items_ = {
        {"NEW GAME", MenuAction::NEW_GAME, true},
        {"LOAD GAME", MenuAction::LOAD_GAME, true},
        {"SAVE GAME", MenuAction::SAVE_GAME, false}, // Disabled until in-game
        {"SETTINGS", MenuAction::SETTINGS, true},
        {"QUIT TO DESKTOP", MenuAction::QUIT_TO_DESKTOP, true}
    };
}

void MainMenuUI::Render() {
    // Full-screen window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                           | ImGuiWindowFlags_NoSavedSettings
                           | ImGuiWindowFlags_NoFocusOnAppearing
                           | ImGuiWindowFlags_NoNav
                           | ImGuiWindowFlags_NoMove
                           | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.13f, 0.14f, 0.06f, 1.0f));

    if (ImGui::Begin("##MainMenu", nullptr, flags)) {
        RenderBackground();
        RenderLogo();
        RenderMenuItems();
        RenderNewsPanel();
        RenderStatsPanel();
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void MainMenuUI::Update() {
    animation_time_ += ImGui::GetIO().DeltaTime;
    // TODO: Use animation_time_ for menu item fade-in/stagger effects
}

void MainMenuUI::RenderBackground() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Vignette effect
    ImU32 vignette_center = IM_COL32(34, 24, 16, 0);
    ImU32 vignette_edge = IM_COL32(0, 0, 0, 200);

    draw_list->AddRectFilledMultiColor(
        viewport->Pos,
        ImVec2(viewport->Pos.x + screen_size.x, viewport->Pos.y + screen_size.y),
        vignette_center, vignette_center, vignette_edge, vignette_edge
    );
}

void MainMenuUI::RenderLogo() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float logo_y = 80.0f;

    // Main title
    const char* title = "MECHANICA IMPERII";
    float title_scale = 2.5f;
    ImVec2 title_size = ImGui::CalcTextSize(title);
    title_size.x *= title_scale;

    float title_x = screen_size.x * 0.25f - title_size.x * 0.5f; // Center in left panel

    ImU32 gold_color = IM_COL32(212, 175, 55, 255);

    // Shadow
    draw_list->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize() * title_scale,
        ImVec2(viewport->Pos.x + title_x + 3, viewport->Pos.y + logo_y + 3),
        IM_COL32(0, 0, 0, 180),
        title
    );

    // Main text
    draw_list->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize() * title_scale,
        ImVec2(viewport->Pos.x + title_x, viewport->Pos.y + logo_y),
        gold_color,
        title
    );

    // Subtitle
    const char* subtitle = "Grand Strategy";
    ImVec2 subtitle_size = ImGui::CalcTextSize(subtitle);
    float subtitle_scale = 1.0f;
    subtitle_size.x *= subtitle_scale;

    float subtitle_x = screen_size.x * 0.25f - subtitle_size.x * 0.5f;
    float subtitle_y = logo_y + (title_size.y / title_scale) + 25.0f;

    draw_list->AddText(
        ImGui::GetFont(),
        ImGui::GetFontSize() * subtitle_scale,
        ImVec2(viewport->Pos.x + subtitle_x, viewport->Pos.y + subtitle_y),
        IM_COL32(201, 169, 97, 255),
        subtitle
    );
}

void MainMenuUI::RenderMenuItems() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float menu_start_y = screen_size.y * 0.35f;
    float menu_item_height = 50.0f;
    float menu_item_spacing = 15.0f;
    float menu_width = 400.0f;
    float menu_x = screen_size.x * 0.25f - menu_width * 0.5f;

    for (size_t i = 0; i < menu_items_.size(); ++i) {
        float item_y = menu_start_y + i * (menu_item_height + menu_item_spacing);
        ImVec2 pos = ImVec2(menu_x, item_y);
        ImVec2 size = ImVec2(menu_width, menu_item_height);

        if (RenderMenuItem(menu_items_[i], static_cast<int>(i), pos, size)) {
            last_action_ = menu_items_[i].action;
        }
    }
}

bool MainMenuUI::RenderMenuItem(const MenuItem& item, int index, const ImVec2& pos, const ImVec2& size) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 screen_pos = ImVec2(viewport->Pos.x + pos.x, viewport->Pos.y + pos.y);
    ImVec2 screen_pos_max = ImVec2(screen_pos.x + size.x, screen_pos.y + size.y);

    ImGuiIO& io = ImGui::GetIO();
    bool is_hovered = io.MousePos.x >= screen_pos.x && io.MousePos.x <= screen_pos_max.x &&
                      io.MousePos.y >= screen_pos.y && io.MousePos.y <= screen_pos_max.y;
    bool is_clicked = is_hovered && ImGui::IsMouseClicked(0) && item.enabled;

    if (is_hovered && item.enabled) {
        selected_index_ = index;
    }

    // Determine colors based on state
    ImU32 bg_color;
    ImU32 border_color;
    ImU32 text_color;
    float offset_x = 0.0f;

    if (!item.enabled) {
        bg_color = IM_COL32(34, 24, 16, 180);
        border_color = IM_COL32(107, 93, 79, 150);
        text_color = IM_COL32(107, 93, 79, 180);
    } else if (is_hovered) {
        bg_color = IM_COL32(51, 34, 17, 240);
        border_color = IM_COL32(212, 175, 55, 255);
        text_color = IM_COL32(212, 175, 55, 255);
        offset_x = 10.0f;

        // Glow effect
        draw_list->AddRect(
            ImVec2(screen_pos.x - 2, screen_pos.y - 2),
            ImVec2(screen_pos_max.x + 2 + offset_x, screen_pos_max.y + 2),
            IM_COL32(212, 175, 55, 100),
            4.0f,
            0,
            3.0f
        );
    } else {
        bg_color = IM_COL32(34, 24, 16, 220);
        border_color = IM_COL32(107, 93, 79, 255);
        text_color = IM_COL32(212, 175, 55, 255);
    }

    // Background
    draw_list->AddRectFilled(
        ImVec2(screen_pos.x + offset_x, screen_pos.y),
        ImVec2(screen_pos_max.x + offset_x, screen_pos_max.y),
        bg_color,
        4.0f
    );

    // Border
    draw_list->AddRect(
        ImVec2(screen_pos.x + offset_x, screen_pos.y),
        ImVec2(screen_pos_max.x + offset_x, screen_pos_max.y),
        border_color,
        4.0f,
        0,
        2.0f
    );

    // Left accent line
    draw_list->AddRectFilled(
        ImVec2(screen_pos.x + offset_x, screen_pos.y),
        ImVec2(screen_pos.x + 4 + offset_x, screen_pos_max.y),
        IM_COL32(212, 175, 55, 255),
        0.0f
    );

    // Text
    ImVec2 text_size = ImGui::CalcTextSize(item.label.c_str());
    ImVec2 text_pos = ImVec2(
        screen_pos.x + 30.0f + offset_x,
        screen_pos.y + (size.y - text_size.y) * 0.5f
    );

    draw_list->AddText(text_pos, text_color, item.label.c_str());

    return is_clicked;
}

void MainMenuUI::RenderNewsPanel() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float panel_width = screen_size.x * 0.4f;
    float panel_height = screen_size.y * 0.45f;
    float panel_x = screen_size.x * 0.55f;
    float panel_y = 60.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + panel_x, viewport->Pos.y + panel_y));
    ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.13f, 0.09f, 0.06f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.13f, 0.09f, 0.06f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

    if (ImGui::Begin("LATEST DISPATCHES", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
        ImGui::TextWrapped("Version 1.2.0 Released");
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::TextWrapped("New features include enhanced population dynamics, improved AI decision-making, and performance optimizations achieving 98%% CPU reduction through attention management.");
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
        ImGui::Text("17 November 2025");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
        ImGui::TextWrapped("Trade System Overhaul");
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::TextWrapped("Trade routes now feature dynamic pricing, hub management, and realistic disruption mechanics. Naval trade and piracy systems fully operational.");
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
        ImGui::Text("10 November 2025");
        ImGui::PopStyleColor();

        ImGui::PopTextWrapPos();
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
}

void MainMenuUI::RenderStatsPanel() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float panel_width = screen_size.x * 0.4f;
    float panel_height = screen_size.y * 0.3f;
    float panel_x = screen_size.x * 0.55f;
    float panel_y = screen_size.y * 0.45f + 90.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + panel_x, viewport->Pos.y + panel_y));
    ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.13f, 0.09f, 0.06f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.13f, 0.09f, 0.06f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));

    if (ImGui::Begin("GAME STATISTICS", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Columns(2, "stats", false);

        auto RenderStat = [](const char* label, const char* value) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.54f, 0.45f, 0.33f, 1.0f));
            ImGui::Text("%s", label);
            ImGui::PopStyleColor();
            ImGui::NextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
            ImGui::Text("%s", value);
            ImGui::PopStyleColor();
            ImGui::NextColumn();
        };

        RenderStat("Playable Nations:", "500+");
        RenderStat("Provinces:", "~5,000");
        RenderStat("Historical Characters:", "3,000+");
        RenderStat("Time Period:", "1000-1900 AD");
        RenderStat("Total Playtime:", "0h 0m");

        ImGui::Columns(1);
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
}

} // namespace ui
