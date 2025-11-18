#pragma once

#include "core/ECS/EntityManager.h"
#include "game/economy/EconomicSystem.h"
#include "game/military/MilitarySystem.h"
#include "core/types/game_types.h"
#include <string>
#include <functional>

namespace ui {
    /**
     * InGameHUD - Heads-Up Display for the main game view
     * Shows important game information: resources, date, notifications, quick actions
     * Now connected to live game systems for real-time data
     */
    class InGameHUD {
    public:
        InGameHUD(core::ecs::EntityManager& entity_manager,
                 game::economy::EconomicSystem& economic_system,
                 game::military::MilitarySystem& military_system);
        ~InGameHUD() = default;

        void Render(game::types::EntityID player_entity);
        void Update();

        bool IsMenuRequested() const { return menu_requested_; }
        void ClearMenuRequest() { menu_requested_ = false; }

        bool IsPauseMenuOpen() const { return show_pause_menu_; }
        void TogglePauseMenu() { show_pause_menu_ = !show_pause_menu_; }

    private:
        core::ecs::EntityManager& entity_manager_;
        game::economy::EconomicSystem& economic_system_;
        game::military::MilitarySystem& military_system_;

        bool menu_requested_;
        bool show_minimap_;
        bool show_tooltips_;
        bool show_pause_menu_;

        void RenderTopBar(game::types::EntityID player_entity);
        void RenderResourcePanel(game::types::EntityID player_entity);
        void RenderQuickActions();
        void RenderNotifications();
        void RenderMinimap();
        void RenderBottomBar();
        void RenderPauseMenu();
    };
}

