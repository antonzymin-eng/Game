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

        // Cached component pointers (refreshed each frame for performance)
        struct CachedEconomyData {
            game::economy::EconomicComponent* economic = nullptr;
            game::economy::TreasuryComponent* treasury = nullptr;
        };
        CachedEconomyData cached_data_;

        // UI state for interactive elements
        float tax_rate_slider_ = 0.10f; // 10% default tax rate
        float previous_tax_rate_ = 0.10f; // For debouncing toast notifications
        bool tax_slider_active_ = false; // Track if slider is being dragged
        game::types::EntityID selected_province_for_building_ = 0; // Province to build in

        // Helper methods for getting economic data (use cached components)
        int GetTaxIncome() const;
        int GetTradeIncome() const;
        int GetTributeIncome() const;
        int GetOtherIncome() const;

        int GetMilitaryExpenses() const;
        int GetAdministrativeExpenses() const;
        int GetInfrastructureExpenses() const;
        int GetInterestExpenses() const;
        int GetOtherExpenses() const;

        void RefreshCachedComponents();
        void ApplyTaxRate(float new_tax_rate);

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
