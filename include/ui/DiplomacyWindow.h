#pragma once

#include "core/ECS/EntityManager.h"
#include "game/diplomacy/DiplomacySystem.h"
#include "core/types/game_types.h"
#include "imgui.h"

namespace ui {
    class WindowManager; // Forward declaration

    class DiplomacyWindow {
    public:
        DiplomacyWindow(core::ecs::EntityManager& entity_manager,
                       game::diplomacy::DiplomacySystem& diplomacy_system);
        ~DiplomacyWindow() = default;

        void Render(WindowManager& window_manager, game::types::EntityID player_entity);

    private:
        core::ecs::EntityManager& entity_manager_;
        game::diplomacy::DiplomacySystem& diplomacy_system_;

        void RenderOverviewTab();
        void RenderRelationsTab();
        void RenderTreatiesTab();
        void RenderWarTab();
    };
}
