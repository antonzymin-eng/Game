#include "ui/NationSelector.h"
#include "imgui.h"
#include <algorithm>
#include <cstring>

namespace ui {

NationSelector::NationSelector()
    : game_ready_(false)
    , selected_nation_index_(-1) {
    std::memset(search_buffer_, 0, sizeof(search_buffer_));

    // Initialize with some default nations for demonstration
    available_nations_ = {
        {"Kingdom of France", "FRA", 45, 15000000, true},
        {"Holy Roman Empire", "HRE", 120, 20000000, true},
        {"Kingdom of England", "ENG", 35, 4000000, true},
        {"Byzantine Empire", "BYZ", 55, 12000000, true},
        {"Kingdom of Poland", "POL", 30, 3000000, true},
        {"Kingdom of Castile", "CAS", 40, 6000000, true},
        {"Sultanate of Rum", "RUM", 25, 2000000, true},
        {"Kievan Rus'", "RUS", 50, 8000000, true},
        {"Kingdom of Hungary", "HUN", 28, 2500000, true},
        {"Kingdom of Norway", "NOR", 20, 500000, true}
    };
}

void NationSelector::SetAvailableNations(const std::vector<NationInfo>& nations) {
    available_nations_ = nations;
}

void NationSelector::Render() {
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

    if (ImGui::Begin("##NationSelector", nullptr, flags)) {
        RenderMapView();
        RenderNationList();
        RenderNationDetails();
        RenderGameOptions();
        RenderStartButton();
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void NationSelector::Update() {
    // Update logic if needed
}

void NationSelector::RenderMapView() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    // Map view takes most of the screen
    float map_width = screen_size.x * 0.65f;
    float map_height = screen_size.y;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(map_width, map_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.15f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (ImGui::Begin("##MapView", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();

        // Background - dark blue for ocean
        draw_list->AddRectFilled(
            canvas_pos,
            ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y),
            IM_COL32(30, 50, 70, 255)
        );

        // Placeholder: Draw some simple regions to represent map
        // In a real implementation, this would render the actual game map
        draw_list->AddText(
            ImVec2(canvas_pos.x + canvas_size.x * 0.5f - 100, canvas_pos.y + 20),
            IM_COL32(200, 200, 200, 255),
            "MAP VIEW - SELECT NATION"
        );

        // Draw simple shapes to represent different nations
        const ImU32 nation_colors[] = {
            IM_COL32(100, 100, 200, 255), // Blue
            IM_COL32(200, 100, 100, 255), // Red
            IM_COL32(100, 200, 100, 255), // Green
            IM_COL32(200, 200, 100, 255), // Yellow
            IM_COL32(150, 100, 200, 255), // Purple
        };

        for (int i = 0; i < 5; ++i) {
            float x = canvas_pos.x + 100 + (i * 150);
            float y = canvas_pos.y + canvas_size.y * 0.4f;
            float size = 80.0f;

            // Simple hexagon-like shape
            draw_list->AddRectFilled(
                ImVec2(x, y),
                ImVec2(x + size, y + size),
                nation_colors[i % 5],
                5.0f
            );

            // Border
            draw_list->AddRect(
                ImVec2(x, y),
                ImVec2(x + size, y + size),
                IM_COL32(50, 50, 50, 255),
                5.0f,
                0,
                2.0f
            );

            // Check if hovered
            ImVec2 mouse_pos = ImGui::GetIO().MousePos;
            if (mouse_pos.x >= x && mouse_pos.x <= x + size &&
                mouse_pos.y >= y && mouse_pos.y <= y + size) {

                // Highlight
                draw_list->AddRect(
                    ImVec2(x - 2, y - 2),
                    ImVec2(x + size + 2, y + size + 2),
                    IM_COL32(212, 175, 55, 255),
                    5.0f,
                    0,
                    3.0f
                );

                // Click to select
                if (ImGui::IsMouseClicked(0) && i < static_cast<int>(available_nations_.size())) {
                    selected_nation_index_ = i;
                }
            }

            // Show selected
            if (selected_nation_index_ == i) {
                draw_list->AddRect(
                    ImVec2(x - 3, y - 3),
                    ImVec2(x + size + 3, y + size + 3),
                    IM_COL32(212, 175, 55, 255),
                    5.0f,
                    0,
                    4.0f
                );
            }
        }

        // Instructions
        draw_list->AddText(
            ImVec2(canvas_pos.x + 20, canvas_pos.y + canvas_size.y - 40),
            IM_COL32(180, 180, 180, 255),
            "Click on the map to select a nation, or choose from the list on the right"
        );
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void NationSelector::RenderNationList() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float panel_width = screen_size.x * 0.35f;
    float panel_height = screen_size.y * 0.5f;
    float panel_x = screen_size.x * 0.65f;
    float panel_y = 0;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + panel_x, viewport->Pos.y + panel_y));
    ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.13f, 0.09f, 0.06f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.13f, 0.09f, 0.06f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));

    if (ImGui::Begin("NATION SELECTION", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        // Search box
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.83f, 0.83f, 1.0f));
        ImGui::Text("Search:");
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##search", search_buffer_, sizeof(search_buffer_));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Nation list
        ImGui::BeginChild("NationListScroll", ImVec2(0, 0), false);

        std::string search_str = search_buffer_;
        std::transform(search_str.begin(), search_str.end(), search_str.begin(), ::tolower);

        for (size_t i = 0; i < available_nations_.size(); ++i) {
            const auto& nation = available_nations_[i];

            // Filter by search
            std::string nation_name_lower = nation.name;
            std::transform(nation_name_lower.begin(), nation_name_lower.end(),
                         nation_name_lower.begin(), ::tolower);

            if (!search_str.empty() && nation_name_lower.find(search_str) == std::string::npos) {
                continue;
            }

            bool is_selected = (selected_nation_index_ == static_cast<int>(i));

            // Selection background
            if (is_selected) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.17f, 0.09f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.30f, 0.20f, 0.10f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.30f, 0.20f, 0.10f, 1.0f));
            }

            if (ImGui::Selectable(("##nation_" + std::to_string(i)).c_str(), is_selected, 0, ImVec2(0, 60))) {
                selected_nation_index_ = static_cast<int>(i);
            }

            if (is_selected) {
                ImGui::PopStyleColor(3);
            }

            // Draw nation info on top of selectable
            ImVec2 item_pos = ImGui::GetItemRectMin();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            // Nation name
            draw_list->AddText(
                ImVec2(item_pos.x + 10, item_pos.y + 5),
                IM_COL32(212, 175, 55, 255),
                nation.name.c_str()
            );

            // Nation tag
            draw_list->AddText(
                ImVec2(item_pos.x + 10, item_pos.y + 25),
                IM_COL32(139, 115, 85, 255),
                ("Tag: " + nation.tag).c_str()
            );

            // Stats
            char stats[128];
            snprintf(stats, sizeof(stats), "Provinces: %d | Pop: %d",
                    nation.provinces, nation.population);
            draw_list->AddText(
                ImVec2(item_pos.x + 10, item_pos.y + 42),
                IM_COL32(155, 135, 115, 255),
                stats
            );
        }

        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
}

void NationSelector::RenderNationDetails() {
    // Validate selection index
    if (selected_nation_index_ < 0 || selected_nation_index_ >= static_cast<int>(available_nations_.size())) {
        return;
    }

    const NationInfo& selected_nation = available_nations_[selected_nation_index_];

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float panel_width = screen_size.x * 0.35f;
    float panel_height = screen_size.y * 0.25f;
    float panel_x = screen_size.x * 0.65f;
    float panel_y = screen_size.y * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + panel_x, viewport->Pos.y + panel_y));
    ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.13f, 0.09f, 0.06f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.13f, 0.09f, 0.06f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));

    if (ImGui::Begin("NATION DETAILS", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("%s", selected_nation.name.c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Columns(2, "details", false);

        auto RenderDetail = [](const char* label, const char* value) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.54f, 0.45f, 0.33f, 1.0f));
            ImGui::Text("%s", label);
            ImGui::PopStyleColor();
            ImGui::NextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
            ImGui::Text("%s", value);
            ImGui::PopStyleColor();
            ImGui::NextColumn();
        };

        char buffer[64];

        RenderDetail("Tag:", selected_nation.tag.c_str());

        snprintf(buffer, sizeof(buffer), "%d", selected_nation.provinces);
        RenderDetail("Provinces:", buffer);

        snprintf(buffer, sizeof(buffer), "%d", selected_nation.population);
        RenderDetail("Population:", buffer);

        RenderDetail("Playable:", selected_nation.is_playable ? "Yes" : "No");

        ImGui::Columns(1);
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
}

void NationSelector::RenderGameOptions() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float panel_width = screen_size.x * 0.35f;
    float panel_height = screen_size.y * 0.2f - 10;
    float panel_x = screen_size.x * 0.65f;
    float panel_y = screen_size.y * 0.75f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + panel_x, viewport->Pos.y + panel_y));
    ImGui::SetNextWindowSize(ImVec2(panel_width, panel_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.06f, 0.04f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.42f, 0.36f, 0.31f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.13f, 0.09f, 0.06f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.13f, 0.09f, 0.06f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));

    if (ImGui::Begin("GAME OPTIONS", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

        // Start year
        ImGui::Text("Start Year:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::SliderInt("##year", &game_options_.start_year, 1000, 1800);

        // Ironman mode
        ImGui::Checkbox("Ironman Mode", &game_options_.ironman_mode);

        // Historical events
        ImGui::Checkbox("Historical Events", &game_options_.historical_events);

        // Difficulty
        ImGui::Text("Difficulty:");
        ImGui::SameLine();
        const char* difficulties[] = { "Easy", "Normal", "Hard", "Very Hard" };
        static int difficulty_idx = 1;
        ImGui::SetNextItemWidth(150);
        if (ImGui::Combo("##difficulty", &difficulty_idx, difficulties, IM_ARRAYSIZE(difficulties))) {
            game_options_.difficulty = difficulties[difficulty_idx];
        }

        ImGui::PopStyleColor();
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
}

void NationSelector::RenderStartButton() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 screen_size = viewport->Size;

    float button_width = 300.0f;
    float button_height = 50.0f;
    float button_x = screen_size.x * 0.65f + (screen_size.x * 0.35f - button_width) * 0.5f;
    float button_y = screen_size.y * 0.95f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + button_x, viewport->Pos.y + button_y));
    ImGui::SetNextWindowSize(ImVec2(button_width, button_height));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (ImGui::Begin("##StartButton", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.13f, 0.07f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.20f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.23f, 0.12f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);

        bool can_start = (selected_nation_index_ >= 0 && selected_nation_index_ < static_cast<int>(available_nations_.size()));

        if (!can_start) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("START GAME", ImVec2(button_width, button_height))) {
            game_ready_ = true;
        }

        if (!can_start) {
            ImGui::EndDisabled();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

} // namespace ui
