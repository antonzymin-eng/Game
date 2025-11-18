#pragma once

#include "core/ECS/EntityManager.h"
#include "game/military/MilitarySystem.h"
#include "imgui.h"

namespace ui {
    /**
     * MilitaryWindow - Comprehensive military management window
     * Tabs: Overview, Army, Navy, Recruitment, Battles
     */
    class MilitaryWindow {
    public:
        MilitaryWindow(core::ecs::EntityManager& entity_manager,
                      game::military::MilitarySystem& military_system);
        ~MilitaryWindow() = default;

        void Render(bool* p_open = nullptr);

    private:
        core::ecs::EntityManager& entity_manager_;
        game::military::MilitarySystem& military_system_;

        void RenderOverviewTab();
        void RenderArmyTab();
        void RenderNavyTab();
        void RenderRecruitmentTab();
        void RenderBattlesTab();
    };
}
