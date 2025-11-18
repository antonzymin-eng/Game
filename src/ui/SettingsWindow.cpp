#include "ui/SettingsWindow.h"

namespace ui {

SettingsWindow::SettingsWindow()
    : fullscreen_(false)
    , resolution_index_(2) // 1280x720
    , vsync_(true)
    , fps_limit_(60)
    , ui_scale_(1.0f)
    , master_volume_(0.8f)
    , music_volume_(0.6f)
    , sfx_volume_(0.7f)
    , mute_audio_(false)
    , game_speed_(3) // Normal speed
    , auto_pause_(false)
    , tooltips_enabled_(true)
    , tutorial_hints_(true)
    , autosave_interval_(10) {
}

void SettingsWindow::Render(WindowManager& window_manager) {
    if (!window_manager.BeginManagedWindow(WindowManager::WindowType::PERFORMANCE, "Settings")) {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("GAME SETTINGS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Tab bar
    if (ImGui::BeginTabBar("SettingsTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Graphics")) {
            RenderGraphicsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Audio")) {
            RenderAudioTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Gameplay")) {
            RenderGameplayTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Controls")) {
            RenderControlsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Advanced")) {
            RenderAdvancedTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    // Bottom buttons
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Apply Settings", ImVec2(150, 0))) {
        ApplySettings();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Apply all settings changes");
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset to Defaults", ImVec2(150, 0))) {
        ResetToDefaults();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reset all settings to default values");
    }

    window_manager.EndManagedWindow();
}

void SettingsWindow::RenderGraphicsTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("GRAPHICS SETTINGS");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Fullscreen
    ImGui::Checkbox("Fullscreen", &fullscreen_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enable fullscreen mode");
    }

    // Resolution
    ImGui::Text("Resolution:");
    const char* resolutions[] = {
        "800x600", "1024x768", "1280x720", "1920x1080", "2560x1440"
    };
    ImGui::SetNextItemWidth(200);
    ImGui::Combo("##resolution", &resolution_index_, resolutions, IM_ARRAYSIZE(resolutions));

    // VSync
    ImGui::Checkbox("VSync", &vsync_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Synchronize frame rate with monitor refresh rate");
    }

    // FPS Limit
    ImGui::Text("FPS Limit:");
    ImGui::SetNextItemWidth(200);
    ImGui::SliderInt("##fpslimit", &fps_limit_, 30, 144);

    // UI Scale
    ImGui::Text("UI Scale:");
    ImGui::SetNextItemWidth(200);
    ImGui::SliderFloat("##uiscale", &ui_scale_, 0.5f, 2.0f, "%.1fx");
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Scale UI elements (requires restart)");
    }

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::Text("Note: Some graphics settings require restarting the game.");
    ImGui::PopStyleColor();
}

void SettingsWindow::RenderAudioTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("AUDIO SETTINGS");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Mute all
    ImGui::Checkbox("Mute All Audio", &mute_audio_);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Master volume (convert to percentage for display)
    ImGui::Text("Master Volume:");
    ImGui::SetNextItemWidth(300);
    float master_percent = master_volume_ * 100.0f;
    if (ImGui::SliderFloat("##master", &master_percent, 0.0f, 100.0f, "%.0f%%")) {
        master_volume_ = master_percent / 100.0f;
        // TODO: Apply to audio system
    }

    // Music volume (convert to percentage for display)
    ImGui::Text("Music Volume:");
    ImGui::SetNextItemWidth(300);
    float music_percent = music_volume_ * 100.0f;
    if (ImGui::SliderFloat("##music", &music_percent, 0.0f, 100.0f, "%.0f%%")) {
        music_volume_ = music_percent / 100.0f;
        // TODO: Apply to audio system
    }

    // SFX volume (convert to percentage for display)
    ImGui::Text("Sound Effects Volume:");
    ImGui::SetNextItemWidth(300);
    float sfx_percent = sfx_volume_ * 100.0f;
    if (ImGui::SliderFloat("##sfx", &sfx_percent, 0.0f, 100.0f, "%.0f%%")) {
        sfx_volume_ = sfx_percent / 100.0f;
        // TODO: Apply to audio system
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Test sound buttons
    if (ImGui::Button("Test Music", ImVec2(150, 0))) {
        // TODO: Play test music
    }
    ImGui::SameLine();
    if (ImGui::Button("Test Sound Effect", ImVec2(150, 0))) {
        // TODO: Play test SFX
    }
}

void SettingsWindow::RenderGameplayTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("GAMEPLAY SETTINGS");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Game speed
    ImGui::Text("Default Game Speed:");
    const char* speeds[] = {"Slow", "Moderate", "Normal", "Fast", "Very Fast"};
    ImGui::SetNextItemWidth(200);
    ImGui::Combo("##gamespeed", &game_speed_, speeds, IM_ARRAYSIZE(speeds));

    // Auto-pause
    ImGui::Checkbox("Auto-pause on Events", &auto_pause_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Automatically pause the game when important events occur");
    }

    // Tooltips
    ImGui::Checkbox("Show Tooltips", &tooltips_enabled_);

    // Tutorial hints
    ImGui::Checkbox("Show Tutorial Hints", &tutorial_hints_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Display helpful hints for new players");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Autosave
    ImGui::Text("Autosave Interval:");
    ImGui::SetNextItemWidth(200);
    ImGui::SliderInt("##autosave", &autosave_interval_, 5, 60, "%d minutes");

    ImGui::Spacing();

    if (ImGui::Button("Clear All Autosaves", ImVec2(150, 0))) {
        // TODO: Implement clear autosaves
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Delete all autosave files to free disk space");
    }
}

void SettingsWindow::RenderControlsTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("KEYBOARD SHORTCUTS");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Display keyboard shortcuts
    struct Shortcut {
        const char* action;
        const char* key;
    };

    Shortcut shortcuts[] = {
        {"Nation Overview", "F1"},
        {"Economy Window", "F2"},
        {"Military Window", "F3"},
        {"Diplomacy Window", "F4"},
        {"Technology Window", "F5"},
        {"Population Window", "F6"},
        {"Trade Window", "F7"},
        {"Realm Window", "F8"},
        {"Pause/Unpause", "SPACE"},
        {"Quick Save", "F5"},
        {"Quick Load", "F9"},
        {"Close Window", "ESC"},
        {"Reload Config", "CTRL+R"}
    };

    ImGui::BeginChild("ShortcutList", ImVec2(0, 300), true);

    ImGui::Columns(2, "shortcuts", false);
    ImGui::SetColumnWidth(0, 300);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Action");
    ImGui::NextColumn();
    ImGui::Text("Shortcut");
    ImGui::NextColumn();
    ImGui::PopStyleColor();

    ImGui::Separator();

    for (const auto& shortcut : shortcuts) {
        ImGui::Text("%s", shortcut.action);
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("%s", shortcut.key);
        ImGui::PopStyleColor();
        ImGui::NextColumn();
    }

    ImGui::Columns(1);

    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::Text("Note: Keyboard remapping will be available in a future update.");
    ImGui::PopStyleColor();
}

void SettingsWindow::RenderAdvancedTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("ADVANCED SETTINGS");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    ImGui::Text("⚠ Warning: These settings are for advanced users only");
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Thread count
    static int thread_count = 4;
    ImGui::Text("Worker Threads:");
    ImGui::SetNextItemWidth(200);
    ImGui::SliderInt("##threads", &thread_count, 1, 16);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Number of worker threads for game systems (requires restart)");
    }

    // Debug mode
    static bool debug_mode = false;
    ImGui::Checkbox("Enable Debug Mode", &debug_mode);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Show debug information and performance metrics");
    }

    // Console
    static bool enable_console = false;
    ImGui::Checkbox("Enable Developer Console", &enable_console);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Press ~ to open developer console");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Reset Configuration", ImVec2(150, 0))) {
        // TODO: Reset config files
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reset all configuration files to defaults");
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear Cache", ImVec2(150, 0))) {
        // TODO: Clear game cache
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear all cached data");
    }
}

void SettingsWindow::ApplySettings() {
    // TODO: Apply settings to game systems
    // - Graphics: Resolution, fullscreen, vsync
    // - Audio: Volume levels
    // - Gameplay: Speed, autosave interval
}

void SettingsWindow::ResetToDefaults() {
    fullscreen_ = false;
    resolution_index_ = 2;
    vsync_ = true;
    fps_limit_ = 60;
    ui_scale_ = 1.0f;
    master_volume_ = 0.8f;
    music_volume_ = 0.6f;
    sfx_volume_ = 0.7f;
    mute_audio_ = false;
    game_speed_ = 3;
    auto_pause_ = false;
    tooltips_enabled_ = true;
    tutorial_hints_ = true;
    autosave_interval_ = 10;
}

} // namespace ui
