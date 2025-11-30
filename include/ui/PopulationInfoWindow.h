#pragma once

#include "core/ECS/EntityManager.h"
#include "core/types/game_types.h"

namespace game::map {
    class MapRenderer;
}

namespace ui {
    class WindowManager; // Forward declaration

    class PopulationInfoWindow {
    public:
        PopulationInfoWindow(
            ::core::ecs::EntityManager& entity_manager,
            game::map::MapRenderer& map_renderer
        );
        ~PopulationInfoWindow();

        void Render(WindowManager& window_manager, game::types::EntityID player_entity);
        void Update();

    private:
        // ECS access
        ::core::ecs::EntityManager& entity_manager_;
        game::map::MapRenderer& map_renderer_;
        game::types::EntityID current_player_entity_; // Set during Render()

        // Helper methods
        void RenderPopulationStats();
        void RenderDemographics();
        void RenderEmployment();
        void RenderCultureReligion();
    };
}