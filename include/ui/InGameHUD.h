#pragma once

#include <string>
#include <functional>

namespace ui {
    /**
     * InGameHUD - Heads-Up Display for the main game view
     * Shows important game information: resources, date, notifications, quick actions
     */
    class InGameHUD {
    public:
        struct GameStats {
            std::string nation_name = "Kingdom";
            int treasury = 0;
            int monthly_income = 0;
            int manpower = 0;
            int prestige = 0;
            int stability = 50;
        };

        InGameHUD();
        ~InGameHUD() = default;

        void Render();
        void Update();

        void SetGameStats(const GameStats& stats) { game_stats_ = stats; }
        bool IsMenuRequested() const { return menu_requested_; }
        void ClearMenuRequest() { menu_requested_ = false; }

    private:
        GameStats game_stats_;
        bool menu_requested_;
        bool show_minimap_;
        bool show_tooltips_;

        void RenderTopBar();
        void RenderResourcePanel();
        void RenderQuickActions();
        void RenderNotifications();
        void RenderMinimap();
        void RenderBottomBar();
    };
}
