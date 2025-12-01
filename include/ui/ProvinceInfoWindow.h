// ============================================================================
// ProvinceInfoWindow.h - Detailed province information panel
// Created: October 29, 2025
// ============================================================================

#pragma once

#include "core/ECS/EntityManager.h"
#include "core/types/game_types.h"
#include <memory>

// Forward declarations for component types
namespace game::province {
    struct ProvinceDataComponent;
    struct ProvinceBuildingsComponent;
    struct ProvinceProsperityComponent;
}

namespace game::military {
    struct MilitaryComponent;
}

namespace game::population {
    struct PopulationComponent;
}

namespace game::map {
    class MapRenderer;
}

namespace ui {
    class WindowManager;

    /**
     * @brief Province information window - shows detailed province data
     *
     * Displays when a province is selected on the map.
     * Shows population, economy, administration, military, etc.
     */
    class ProvinceInfoWindow {
    public:
        ProvinceInfoWindow(
            ::core::ecs::EntityManager& entity_manager,
            game::map::MapRenderer& map_renderer
        );
        ~ProvinceInfoWindow() = default;

        /**
         * @brief Render the province info window
         */
        void Render(WindowManager& window_manager, game::types::EntityID player_entity);

        /**
         * @brief Set visibility
         */
        void SetVisible(bool visible) { visible_ = visible; }

        /**
         * @brief Check if window is visible
         */
        bool IsVisible() const { return visible_; }

    private:
        // ECS access
        ::core::ecs::EntityManager& entity_manager_;
        game::map::MapRenderer& map_renderer_;
        game::types::EntityID current_player_entity_;
        bool visible_ = true;

        // Component caching for performance
        ::core::ecs::EntityID cached_province_id_{0};
        std::shared_ptr<game::province::ProvinceDataComponent> cached_province_data_;
        std::shared_ptr<game::province::ProvinceBuildingsComponent> cached_buildings_;
        std::shared_ptr<game::military::MilitaryComponent> cached_military_;
        std::shared_ptr<game::population::PopulationComponent> cached_population_;
        std::shared_ptr<game::province::ProvinceProsperityComponent> cached_prosperity_;

        // Helper methods for component access
        void UpdateComponentCache(::core::ecs::EntityID province_id);
        void ClearComponentCache();

        // Render methods
        void RenderHeader();
        void RenderOverviewTab();
        void RenderBuildingsTab();
        void RenderMilitaryTab();
        void RenderPopulationTab();
        void RenderReligionTab();
        void RenderAdministrationTab();
        void RenderGeographyTab();

        // Helper methods
        const char* GetBuildingName(int building_type, bool is_production);
        const char* GetUnitTypeName(int unit_type);
        const char* GetMoraleStateName(int morale_state);
    };

} // namespace ui
