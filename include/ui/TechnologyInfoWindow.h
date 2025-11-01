#pragma once

#include "core/ecs/EntityManager.h"
#include "game/technology/TechnologySystem.h"
#include "game/technology/TechnologyComponents.h"
#include "core/types/game_types.h"
#include <imgui.h>
#include <string>
#include <vector>
#include <unordered_set>

namespace ui {

/**
 * @brief Technology Tree Viewer Window
 *
 * Provides a comprehensive interface for viewing technology research progress,
 * managing research focus, and visualizing the technology tree with dependencies.
 */
class TechnologyInfoWindow {
public:
    /**
     * @brief Construct a new Technology Info Window
     *
     * @param entity_manager Reference to the ECS entity manager
     * @param tech_system Reference to the technology system
     */
    TechnologyInfoWindow(
        ::core::ecs::EntityManager& entity_manager,
        game::technology::TechnologySystem& tech_system
    );

    ~TechnologyInfoWindow() = default;

    // Non-copyable
    TechnologyInfoWindow(const TechnologyInfoWindow&) = delete;
    TechnologyInfoWindow& operator=(const TechnologyInfoWindow&) = delete;

    /**
     * @brief Render the technology window
     *
     * Called every frame to display the UI. Handles all ImGui rendering.
     */
    void Render();

    /**
     * @brief Update the window state (deprecated - kept for compatibility)
     */
    void Update();

    /**
     * @brief Set the visibility of the window
     *
     * @param visible True to show the window, false to hide it
     */
    void SetVisible(bool visible);

    /**
     * @brief Check if the window is visible
     *
     * @return true if visible, false otherwise
     */
    bool IsVisible() const;

    /**
     * @brief Toggle the window visibility
     */
    void ToggleVisibility();

    /**
     * @brief Set the selected nation/entity
     *
     * @param entity_id The entity ID to view technologies for
     */
    void SetSelectedEntity(game::types::EntityID entity_id);

private:
    // Dependencies
    ::core::ecs::EntityManager& entity_manager_;
    game::technology::TechnologySystem& tech_system_;

    // UI State
    bool visible_ = true;
    int current_tab_ = 0;
    game::types::EntityID selected_entity_ = 0;

    // Selected technology for details
    game::technology::TechnologyType selected_technology_ = game::technology::TechnologyType::INVALID;

    // Filter/search state
    char search_buffer_[256] = {0};
    bool filter_by_category_ = false;
    game::technology::TechnologyCategory selected_category_ = game::technology::TechnologyCategory::AGRICULTURAL;
    bool show_only_available_ = false;
    bool show_only_researching_ = false;

    // Helper methods for rendering tabs
    void RenderHeader();
    void RenderResearchOverviewTab();
    void RenderTechnologyTreeTab();
    void RenderInnovationTab();
    void RenderKnowledgeNetworkTab();

    // Tree rendering helpers
    void RenderTechnologyNode(
        game::technology::TechnologyType tech_type,
        const game::technology::ResearchComponent* research_comp,
        float x, float y
    );
    void RenderCategoryTree(
        game::technology::TechnologyCategory category,
        const game::technology::ResearchComponent* research_comp
    );

    // Details panels
    void RenderTechnologyDetails(game::technology::TechnologyType tech_type);
    void RenderResearchProgress(const game::technology::ResearchComponent* research_comp);
    void RenderInnovationMetrics(const game::technology::InnovationComponent* innovation_comp);
    void RenderKnowledgeStats(const game::technology::KnowledgeComponent* knowledge_comp);

    // Utility methods
    std::string GetTechnologyName(game::technology::TechnologyType tech) const;
    std::string GetTechnologyDescription(game::technology::TechnologyType tech) const;
    std::string GetCategoryName(game::technology::TechnologyCategory category) const;
    std::string GetResearchStateName(game::technology::ResearchState state) const;
    const char* GetCategoryIcon(game::technology::TechnologyCategory category) const;

    // Color helpers
    ImVec4 GetResearchStateColor(game::technology::ResearchState state) const;
    ImVec4 GetCategoryColor(game::technology::TechnologyCategory category) const;
    ImVec4 GetProgressColor(double progress) const;

    // Technology tree helpers
    bool IsTechnologyAvailable(
        game::technology::TechnologyType tech,
        const game::technology::ResearchComponent* research_comp
    ) const;
    std::vector<game::technology::TechnologyType> GetPrerequisites(
        game::technology::TechnologyType tech
    ) const;
    std::vector<game::technology::TechnologyType> GetTechnologiesInCategory(
        game::technology::TechnologyCategory category
    ) const;
};

} // namespace ui