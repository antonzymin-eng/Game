#pragma once

#include "core/ECS/EntityManager.h"
#include "game/diplomacy/DiplomacySystem.h"
#include "imgui.h"

namespace ui {
    class DiplomacyWindow {
    public:
        DiplomacyWindow(core::ecs::EntityManager& entity_manager,
                       game::diplomacy::DiplomacySystem& diplomacy_system);
        ~DiplomacyWindow() = default;

        void Render(bool* p_open = nullptr);

    private:
        core::ecs::EntityManager& entity_manager_;
        game::diplomacy::DiplomacySystem& diplomacy_system_;

        void RenderOverviewTab();
        void RenderRelationsTab();
        void RenderTreatiesTab();
        void RenderWarTab();
    };
}
