#pragma once

#include "core/ECS/EntityManager.h"
#include "game/economy/EconomicSystem.h"
#include "game/military/MilitarySystem.h"
#include "core/types/game_types.h"
#include <string>
#include <functional>

// Forward declarations
namespace ui {
    class SaveLoadDialog;
    class WindowManager;
}

namespace ui {
    // Forward declarations
    class SaveLoadDialog;
    class SettingsWindow;
    class WindowManager;

    /**
     * InGameHUD - Heads-Up Display for the main game view
     * Shows important game information: resources, date, notifications, quick actions
     * Now connected to live game systems for real-time data
     */
    class InGameHUD {
    public:
        InGameHUD(core::ecs::EntityManager& entity_manager,
                 game::economy::EconomicSystem& economic_system,
                 game::military::MilitarySystem& military_system,
                 SaveLoadDialog* save_load_dialog,
                 SettingsWindow* settings_window,
                 WindowManager* window_manager);
        ~InGameHUD() = default;

        void Render(game::types::EntityID player_entity);
        void Update();

        bool IsMenuRequested() const { return menu_requested_; }
        void ClearMenuRequest() { menu_requested_ = false; }

        bool IsPauseMenuOpen() const { return show_pause_menu_; }
        void TogglePauseMenu() { show_pause_menu_ = !show_pause_menu_; }

        // Set dependencies for pause menu functionality
        // Note: InGameHUD does NOT own these pointers - they are managed externally
        void SetSaveLoadDialog(SaveLoadDialog* dialog) noexcept { save_load_dialog_ = dialog; }
        void SetWindowManager(WindowManager* manager) noexcept { window_manager_ = manager; }

    private:
        core::ecs::EntityManager& entity_manager_;
        game::economy::EconomicSystem& economic_system_;
        game::military::MilitarySystem& military_system_;
        SaveLoadDialog* save_load_dialog_;
        SettingsWindow* settings_window_;
        WindowManager* window_manager_;

        bool menu_requested_;
        bool show_minimap_;
        bool show_tooltips_;
        bool show_pause_menu_;
        bool show_exit_confirmation_;

        void RenderTopBar(game::types::EntityID player_entity);
        void RenderResourcePanel(game::types::EntityID player_entity);
        void RenderNotifications();
        void RenderMinimap();
        void RenderBottomBar();
        void RenderPauseMenu();
        void RenderExitConfirmation();
    };
}

