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
        int selected_treaty_target_ = 0;    // For treaty proposals
        int selected_war_target_ = 0;       // For war declarations
        int selected_casus_belli_ = 0;

        // Confirmation dialog state
        bool show_war_confirmation_ = false;
        bool show_treaty_break_confirmation_ = false;
        game::types::EntityID pending_war_target_ = 0;
        game::diplomacy::CasusBelli pending_casus_belli_ = game::diplomacy::CasusBelli::NONE;
        std::string pending_treaty_id_to_break_;

        void RenderOverviewTab();
        void RenderRelationsTab();
        void RenderTreatiesTab();
        void RenderWarTab();
    };
}
