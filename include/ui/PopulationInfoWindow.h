#pragma once

#include "core/ECS/EntityManager.h"

namespace game::map {
    class MapRenderer;
}

namespace ui {
    class PopulationInfoWindow {
    public:
        PopulationInfoWindow(
            ::core::ecs::EntityManager& entity_manager,
            game::map::MapRenderer& map_renderer
        );
        ~PopulationInfoWindow();
        
        void Render();
        void Update();
        
        void SetVisible(bool visible) { is_visible_ = visible; }
        bool IsVisible() const { return is_visible_; }
        
    private:
        // ECS access
        ::core::ecs::EntityManager& entity_manager_;
        game::map::MapRenderer& map_renderer_;
        
        // UI state
        bool is_visible_ = true;
        
        // Helper methods
        void RenderPopulationStats();
        void RenderDemographics();
        void RenderEmployment();
        void RenderCultureReligion();
    };
}