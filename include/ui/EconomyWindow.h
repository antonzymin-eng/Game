#pragma once

#include "core/ECS/EntityManager.h"
#include "game/economy/EconomicSystem.h"
#include "game/province/ProvinceSystem.h"
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
                     game::economy::EconomicSystem& economic_system,
                     game::province::ProvinceSystem& province_system);
        ~EconomyWindow() = default;

        void Render(WindowManager& window_manager, game::types::EntityID player_entity);

    private:
        core::ecs::EntityManager& entity_manager_;
        game::economy::EconomicSystem& economic_system_;
        game::province::ProvinceSystem& province_system_;
        game::types::EntityID current_player_entity_; // Set during Render()

        // UI state for interactive elements
        float tax_rate_slider_ = 0.10f; // 10% default tax rate
        game::types::EntityID selected_province_for_building_ = 0; // Province to build in

        // Economic action constants
        static constexpr int LOAN_AMOUNT = 1000;
        static constexpr int EMERGENCY_TAX_REVENUE = 500;
        static constexpr int DIPLOMATIC_GIFT_AMOUNT = 200;

        void RenderTreasuryTab();
        void RenderIncomeTab();
        void RenderExpensesTab();
        void RenderBuildingsTab();
        void RenderDevelopmentTab();
    };
}
