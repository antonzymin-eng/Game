#pragma once

#include "core/ECS/EntityManager.h"
#include "game/diplomacy/DiplomacySystem.h"
#include "core/types/game_types.h"
#include "imgui.h"

namespace ui {
    class PortraitGenerator;
    class WindowManager;

    class DiplomacyWindow {
    public:
        DiplomacyWindow(core::ecs::EntityManager& entity_manager,
                       game::diplomacy::DiplomacySystem& diplomacy_system);
        ~DiplomacyWindow() = default;

        void Render(WindowManager& window_manager, game::types::EntityID player_entity);

        /**
         * @brief Set the portrait generator for rendering character portraits
         */
        void SetPortraitGenerator(PortraitGenerator* generator) { portrait_generator_ = generator; }

    private:
        core::ecs::EntityManager& entity_manager_;
        game::diplomacy::DiplomacySystem& diplomacy_system_;
        PortraitGenerator* portrait_generator_ = nullptr;

        // UI state
        game::types::EntityID current_player_entity_ = 0;
        int selected_nation_index_ = 0;
        int selected_casus_belli_ = 0;

        void RenderOverviewTab();
        void RenderRelationsTab();
        void RenderTreatiesTab();
        void RenderWarTab();
    };
}
