#pragma once

#include "core/ECS/EntityManager.h"
#include "game/military/MilitarySystem.h"
#include "core/types/game_types.h"
#include "imgui.h"
#include <unordered_map>
#include <string>

namespace ui {
    class WindowManager; // Forward declaration
    /**
     * MilitaryWindow - Comprehensive military management window
     * Tabs: Overview, Army, Navy, Recruitment, Battles
     * Integrates with WindowManager for pin/unpin functionality
     */
    class MilitaryWindow {
    public:
        MilitaryWindow(core::ecs::EntityManager& entity_manager,
                      game::military::MilitarySystem& military_system);
        ~MilitaryWindow() = default;

        void Render(WindowManager& window_manager, game::types::EntityID player_entity);

    private:
        core::ecs::EntityManager& entity_manager_;
        game::military::MilitarySystem& military_system_;
        game::types::EntityID current_player_entity_; // Set during Render()

        // UI state for interactive elements
        std::unordered_map<std::string, int> recruit_counts_; // Per-unit recruitment counts

        void RenderOverviewTab();
        void RenderArmyTab();
        void RenderNavyTab();
        void RenderRecruitmentTab();
        void RenderBattlesTab();
    };
}
