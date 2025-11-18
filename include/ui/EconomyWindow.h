#pragma once

#include "core/ECS/EntityManager.h"
#include "game/economy/EconomicSystem.h"
#include "core/types/game_types.h"
#include "imgui.h"

namespace ui {
    class WindowManager; // Forward declaration
    /**
     * EconomyWindow - Comprehensive economy management window
     * Tabs: Treasury, Income, Expenses, Buildings, Development
     * Integrates with WindowManager for pin/unpin functionality
     */
    class EconomyWindow {
    public:
        EconomyWindow(core::ecs::EntityManager& entity_manager,
                     game::economy::EconomicSystem& economic_system);
        ~EconomyWindow() = default;

        void Render(WindowManager& window_manager, game::types::EntityID player_entity);

    private:
        core::ecs::EntityManager& entity_manager_;
        game::economy::EconomicSystem& economic_system_;
        game::types::EntityID current_player_entity_; // Set during Render()

        void RenderTreasuryTab();
        void RenderIncomeTab();
        void RenderExpensesTab();
        void RenderBuildingsTab();
        void RenderDevelopmentTab();
    };
}
