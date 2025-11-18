#pragma once

#include "core/ECS/EntityManager.h"
#include "game/economy/EconomicSystem.h"
#include "imgui.h"

namespace ui {
    /**
     * EconomyWindow - Comprehensive economy management window
     * Tabs: Treasury, Income, Expenses, Buildings, Development
     */
    class EconomyWindow {
    public:
        EconomyWindow(core::ecs::EntityManager& entity_manager,
                     game::economy::EconomicSystem& economic_system);
        ~EconomyWindow() = default;

        void Render(bool* p_open = nullptr);

    private:
        core::ecs::EntityManager& entity_manager_;
        game::economy::EconomicSystem& economic_system_;

        int active_tab_;

        void RenderTreasuryTab();
        void RenderIncomeTab();
        void RenderExpensesTab();
        void RenderBuildingsTab();
        void RenderDevelopmentTab();
    };
}
