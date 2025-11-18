#pragma once

#include "core/ECS/EntityManager.h"
#include "game/realm/RealmManager.h"
#include "imgui.h"

namespace ui {
    class RealmWindow {
    public:
        RealmWindow(core::ecs::EntityManager& entity_manager,
                   game::realm::RealmManager& realm_manager);
        ~RealmWindow() = default;

        void Render(bool* p_open = nullptr);

    private:
        core::ecs::EntityManager& entity_manager_;
        game::realm::RealmManager& realm_manager_;

        void RenderDynastyTab();
        void RenderSuccessionTab();
        void RenderCourtTab();
        void RenderVassalsTab();
    };
}
