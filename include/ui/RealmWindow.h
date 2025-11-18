#pragma once

#include "core/ECS/EntityManager.h"
#include "game/realm/RealmManager.h"
#include "core/types/game_types.h"
#include "imgui.h"

namespace ui {
    class WindowManager; // Forward declaration

    class RealmWindow {
    public:
        RealmWindow(core::ecs::EntityManager& entity_manager,
                   game::realm::RealmManager& realm_manager);
        ~RealmWindow() = default;

        void Render(WindowManager& window_manager, game::types::EntityID player_entity);

    private:
        core::ecs::EntityManager& entity_manager_;
        game::realm::RealmManager& realm_manager_;

        void RenderDynastyTab();
        void RenderSuccessionTab();
        void RenderCourtTab();
        void RenderVassalsTab();
    };
}
