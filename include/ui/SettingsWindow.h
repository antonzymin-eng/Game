#pragma once

#include "imgui.h"
#include "ui/WindowManager.h"
#include "core/types/game_types.h"

namespace ui {
    /**
     * SettingsWindow - Game settings and configuration
     * Tabs: Graphics, Audio, Gameplay, Controls, Advanced
     */
    class SettingsWindow {
    public:
        SettingsWindow();
        ~SettingsWindow() = default;

        void Render(WindowManager& window_manager);

        // Apply settings to game systems
        void ApplySettings();
        void ResetToDefaults();

    private:
        // Graphics settings
        bool fullscreen_;
        int resolution_index_;
        bool vsync_;
        int fps_limit_;
        float ui_scale_;

        // Audio settings
        float master_volume_;
        float music_volume_;
        float sfx_volume_;
        bool mute_audio_;

        // Gameplay settings
        int game_speed_;
        bool auto_pause_;
        bool tooltips_enabled_;
        bool tutorial_hints_;
        int autosave_interval_; // minutes

        // Confirmation dialog states
        bool show_clear_autosaves_confirmation_ = false;
        bool show_reset_config_confirmation_ = false;
        bool show_clear_cache_confirmation_ = false;

        // Render methods
        void RenderGraphicsTab();
        void RenderAudioTab();
        void RenderGameplayTab();
        void RenderControlsTab();
        void RenderAdvancedTab();

        // Confirmation dialog renderers
        void RenderClearAutosavesConfirmation();
        void RenderResetConfigConfirmation();
        void RenderClearCacheConfirmation();
    };
}
