#pragma once

#include <functional>
#include <string>
#include <vector>

namespace ui {
    /**
     * MainMenuUI - Main menu screen with game options
     * Includes: New Game, Load Game, Save Game, Settings, Quit
     */
    class MainMenuUI {
    public:
        enum class MenuAction {
            NONE,
            NEW_GAME,
            LOAD_GAME,
            SAVE_GAME,
            SETTINGS,
            QUIT_TO_DESKTOP
        };

        MainMenuUI();
        ~MainMenuUI();

        void Render();
        void Update();

        MenuAction GetLastAction() const { return last_action_; }
        void ClearAction() { last_action_ = MenuAction::NONE; }

    private:
        MenuAction last_action_;
        int selected_index_;
        float animation_time_;

        struct MenuItem {
            std::string label;
            MenuAction action;
            bool enabled;
        };

        std::vector<MenuItem> menu_items_;

        void InitializeMenuItems();
        void RenderBackground();
        void RenderLogo();
        void RenderMenuItems();
        void RenderNewsPanel();
        void RenderStatsPanel();
        bool RenderMenuItem(const MenuItem& item, int index, const ImVec2& pos, const ImVec2& size);
    };
}
