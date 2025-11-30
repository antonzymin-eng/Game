#pragma once

#include "core/ECS/EntityManager.h"
#include "game/administration/AdministrativeSystem.h"
#include "core/types/game_types.h"
#include "imgui.h"

namespace ui {
    class WindowManager; // Forward declaration

    /**
     * AdministrativeWindow - Comprehensive administrative management window
     * Tabs: Overview, Officials, Bureaucracy, Law & Order
     * Integrates with WindowManager for pin/unpin functionality
     */
    class AdministrativeWindow {
    public:
        AdministrativeWindow(core::ecs::EntityManager& entity_manager,
                            game::administration::AdministrativeSystem& administrative_system);
        ~AdministrativeWindow() = default;

        void Render(WindowManager& window_manager, game::types::EntityID player_entity);

    private:
        core::ecs::EntityManager& entity_manager_;
        game::administration::AdministrativeSystem& administrative_system_;
        game::types::EntityID current_player_entity_; // Set during Render()

        // UI state for interactive elements
        char new_official_name_[64] = "";
        int selected_official_type_ = 0;
        int new_governance_type_ = 0;

        void RenderOverviewTab();
        void RenderOfficialsTab();
        void RenderBureaucracyTab();
        void RenderLawOrderTab();

        // Helper functions
        const char* GetOfficialTypeName(game::administration::OfficialType type) const;
        const char* GetGovernanceTypeName(game::administration::GovernanceType type) const;
        const char* GetOfficialTraitName(game::administration::OfficialTrait trait) const;
        ImVec4 GetEfficiencyColor(double efficiency) const;
    };
}
