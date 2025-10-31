#include "ui/MapModeSelector.h"
#include "imgui.h"

namespace ui {

MapModeSelector::MapModeSelector(game::map::MapRenderer& map_renderer)
    : map_renderer_(map_renderer),
      visible_(true),
      current_mode_(MapMode::POLITICAL),
      expanded_(false) {
}

void MapModeSelector::Render() {
    if (!visible_) {
        return;
    }

    // Set window position and size
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;

    // Position in bottom-left corner
    ImVec2 window_pos = ImVec2(work_pos.x + 10, work_pos.y + work_size.y - (expanded_ ? 400 : 100));
    ImVec2 window_size = ImVec2(expanded_ ? 300 : 150, expanded_ ? 390 : 90);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);

    // Window flags - no title bar, no resize
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                     ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("MapModeSelector", &visible_, window_flags)) {
        if (expanded_) {
            RenderExpandedMode();
        } else {
            RenderCompactMode();
        }
    }
    ImGui::End();
}

void MapModeSelector::SetVisible(bool visible) {
    visible_ = visible;
}

bool MapModeSelector::IsVisible() const {
    return visible_;
}

void MapModeSelector::ToggleVisibility() {
    visible_ = !visible_;
}

void MapModeSelector::SetMapMode(MapMode mode) {
    if (current_mode_ != mode) {
        current_mode_ = mode;
        ApplyMapMode(mode);

        if (on_mode_changed_callback_) {
            on_mode_changed_callback_(mode);
        }
    }
}

MapModeSelector::MapMode MapModeSelector::GetCurrentMapMode() const {
    return current_mode_;
}

void MapModeSelector::SetOnMapModeChanged(std::function<void(MapMode)> callback) {
    on_mode_changed_callback_ = callback;
}

void MapModeSelector::RenderCompactMode() {
    // Current mode display
    std::string current_name = GetModeName(current_mode_);
    const char* current_icon = GetModeIcon(current_mode_);
    ImVec4 current_color = GetModeColor(current_mode_);

    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Map Mode:");
    ImGui::TextColored(current_color, "%s %s", current_icon, current_name.c_str());

    // Expand button
    if (ImGui::Button("Change Mode", ImVec2(-1, 0))) {
        expanded_ = true;
    }
}

void MapModeSelector::RenderExpandedMode() {
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Select Map Mode");
    ImGui::Separator();

    ImGui::BeginChild("MapModes", ImVec2(0, -30), true);

    // Render all map mode buttons
    for (int i = 0; i < static_cast<int>(MapMode::COUNT); ++i) {
        MapMode mode = static_cast<MapMode>(i);
        std::string mode_name = GetModeName(mode);
        const char* mode_icon = GetModeIcon(mode);

        RenderModeButton(mode, mode_name.c_str(), mode_icon);
    }

    ImGui::EndChild();

    // Collapse button
    if (ImGui::Button("Close", ImVec2(-1, 0))) {
        expanded_ = false;
    }
}

void MapModeSelector::RenderModeButton(MapMode mode, const char* label, const char* icon) {
    bool is_current = (mode == current_mode_);
    ImVec4 mode_color = GetModeColor(mode);

    // Highlight current mode
    if (is_current) {
        ImGui::PushStyleColor(ImGuiCol_Button, mode_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(mode_color.x * 1.2f, mode_color.y * 1.2f, mode_color.z * 1.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(mode_color.x * 0.8f, mode_color.y * 0.8f, mode_color.z * 0.8f, 1.0f));
    }

    std::string button_label = std::string(icon) + " " + label;
    if (ImGui::Button(button_label.c_str(), ImVec2(-1, 40))) {
        SetMapMode(mode);
        expanded_ = false;
    }

    if (is_current) {
        ImGui::PopStyleColor(3);
    }

    // Show description on hover
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextColored(mode_color, "%s", label);
        ImGui::Separator();
        std::string description = GetModeDescription(mode);
        ImGui::TextWrapped("%s", description.c_str());
        ImGui::EndTooltip();
    }
}

std::string MapModeSelector::GetModeName(MapMode mode) const {
    switch (mode) {
        case MapMode::POLITICAL: return "Political";
        case MapMode::TERRAIN: return "Terrain";
        case MapMode::TRADE: return "Trade";
        case MapMode::MILITARY: return "Military";
        case MapMode::DIPLOMATIC: return "Diplomatic";
        case MapMode::RELIGIOUS: return "Religious";
        case MapMode::CULTURAL: return "Cultural";
        case MapMode::DEVELOPMENT: return "Development";
        case MapMode::POPULATION: return "Population";
        default: return "Unknown";
    }
}

std::string MapModeSelector::GetModeDescription(MapMode mode) const {
    switch (mode) {
        case MapMode::POLITICAL:
            return "Shows political borders and nation colors. View your realm and neighboring nations.";
        case MapMode::TERRAIN:
            return "Shows terrain types including mountains, forests, plains, and water bodies.";
        case MapMode::TRADE:
            return "Shows trade routes, trade hubs, and commercial activity between provinces.";
        case MapMode::MILITARY:
            return "Shows army positions, fortifications, and ongoing battles.";
        case MapMode::DIPLOMATIC:
            return "Shows alliances, rivalries, and diplomatic relationships between nations.";
        case MapMode::RELIGIOUS:
            return "Shows the distribution of religions across provinces.";
        case MapMode::CULTURAL:
            return "Shows cultural groups and their geographical distribution.";
        case MapMode::DEVELOPMENT:
            return "Shows province development levels including infrastructure and technology.";
        case MapMode::POPULATION:
            return "Shows population density and demographic distribution across provinces.";
        default:
            return "No description available.";
    }
}

const char* MapModeSelector::GetModeIcon(MapMode mode) const {
    switch (mode) {
        case MapMode::POLITICAL: return "[P]";
        case MapMode::TERRAIN: return "[T]";
        case MapMode::TRADE: return "[$]";
        case MapMode::MILITARY: return "[M]";
        case MapMode::DIPLOMATIC: return "[D]";
        case MapMode::RELIGIOUS: return "[R]";
        case MapMode::CULTURAL: return "[C]";
        case MapMode::DEVELOPMENT: return "[+]";
        case MapMode::POPULATION: return "[=]";
        default: return "[?]";
    }
}

ImVec4 MapModeSelector::GetModeColor(MapMode mode) const {
    switch (mode) {
        case MapMode::POLITICAL:
            return ImVec4(0.8f, 0.3f, 0.3f, 1.0f); // Red
        case MapMode::TERRAIN:
            return ImVec4(0.4f, 0.7f, 0.3f, 1.0f); // Green
        case MapMode::TRADE:
            return ImVec4(1.0f, 0.9f, 0.3f, 1.0f); // Gold
        case MapMode::MILITARY:
            return ImVec4(0.6f, 0.3f, 0.3f, 1.0f); // Dark Red
        case MapMode::DIPLOMATIC:
            return ImVec4(0.5f, 0.5f, 0.9f, 1.0f); // Blue
        case MapMode::RELIGIOUS:
            return ImVec4(0.9f, 0.9f, 0.9f, 1.0f); // White
        case MapMode::CULTURAL:
            return ImVec4(0.8f, 0.5f, 0.9f, 1.0f); // Purple
        case MapMode::DEVELOPMENT:
            return ImVec4(0.3f, 0.7f, 0.9f, 1.0f); // Cyan
        case MapMode::POPULATION:
            return ImVec4(0.9f, 0.6f, 0.3f, 1.0f); // Orange
        default:
            return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
    }
}

void MapModeSelector::ApplyMapMode(MapMode mode) {
    // Get renderer settings
    auto& settings = map_renderer_.GetSettings();

    // Reset all mode flags
    settings.use_political_mode = false;
    settings.use_terrain_mode = false;
    settings.use_trade_mode = false;

    // Apply the selected mode
    switch (mode) {
        case MapMode::POLITICAL:
            settings.use_political_mode = true;
            settings.show_political_colors = true;
            map_renderer_.SetLayerVisible(game::map::MapLayer::POLITICAL_BORDERS, true);
            break;

        case MapMode::TERRAIN:
            settings.use_terrain_mode = true;
            settings.show_terrain_colors = true;
            map_renderer_.SetLayerVisible(game::map::MapLayer::TERRAIN_BASE, true);
            break;

        case MapMode::TRADE:
            settings.use_trade_mode = true;
            map_renderer_.SetLayerVisible(game::map::MapLayer::TRADE_ROUTES, true);
            break;

        case MapMode::MILITARY:
            map_renderer_.SetLayerVisible(game::map::MapLayer::MILITARY_UNITS, true);
            map_renderer_.SetLayerVisible(game::map::MapLayer::POLITICAL_BORDERS, true);
            break;

        case MapMode::DIPLOMATIC:
            settings.use_political_mode = true;
            // Would need additional diplomatic overlay layer
            break;

        case MapMode::RELIGIOUS:
            // Would need religious overlay
            settings.use_political_mode = true;
            break;

        case MapMode::CULTURAL:
            // Would need cultural overlay
            settings.use_political_mode = true;
            break;

        case MapMode::DEVELOPMENT:
            // Would need development overlay
            settings.use_political_mode = true;
            break;

        case MapMode::POPULATION:
            // Would need population density overlay
            settings.use_political_mode = true;
            break;

        default:
            settings.use_political_mode = true;
            break;
    }

    // Update the renderer with new settings
    map_renderer_.UpdateSettings(settings);
}

} // namespace ui
