#pragma once

#include "core/ecs/EntityManager.h"
#include "map/render/MapRenderer.h"
#include "game/trade/TradeSystem.h"
#include "game/economy/EconomicSystem.h"
#include "core/types/game_types.h"
#include <string>
#include <vector>

namespace ui {

/**
 * @brief Trade System UI Window
 *
 * Provides a comprehensive interface for viewing and managing trade routes,
 * trade hubs, market analysis, and trade opportunities. Features a tabbed
 * interface for organizing different aspects of the trade system.
 */
class TradeSystemWindow {
public:
    /**
     * @brief Construct a new Trade System Window
     *
     * @param entity_manager Reference to the ECS entity manager
     * @param map_renderer Reference to the map renderer for province selection
     * @param trade_system Reference to the trade system for trade data
     * @param economic_system Reference to the economic system for financial data
     */
    TradeSystemWindow(
        ::core::ecs::EntityManager& entity_manager,
        game::map::MapRenderer& map_renderer,
        game::trade::TradeSystem& trade_system,
        game::economy::EconomicSystem& economic_system
    );

    ~TradeSystemWindow() = default;

    // Non-copyable
    TradeSystemWindow(const TradeSystemWindow&) = delete;
    TradeSystemWindow& operator=(const TradeSystemWindow&) = delete;

    /**
     * @brief Render the trade system window
     *
     * Called every frame to display the UI. Handles all ImGui rendering.
     */
    void Render();

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
     * @brief Set the currently selected province
     *
     * @param province_id The entity ID of the selected province
     */
    void SetSelectedProvince(game::types::EntityID province_id);

    /**
     * @brief Clear the province selection
     */
    void ClearSelection();

private:
    // Dependencies
    ::core::ecs::EntityManager& entity_manager_;
    game::map::MapRenderer& map_renderer_;
    game::trade::TradeSystem& trade_system_;
    game::economy::EconomicSystem& economic_system_;

    // UI State
    bool visible_ = true;
    int current_tab_ = 0;
    game::types::EntityID selected_province_ = 0;
    std::string selected_route_id_;

    // Filter/search state
    char search_buffer_[256] = {0};
    bool filter_profitable_only_ = false;
    bool filter_active_only_ = true;
    game::types::ResourceType selected_resource_filter_ = game::types::ResourceType::FOOD;

    // Cached data (updated periodically)
    std::vector<game::trade::TradeRoute> cached_routes_;
    std::vector<game::trade::TradeHub> cached_hubs_;
    double last_cache_update_ = 0.0;
    const double cache_update_interval_ = 1.0; // Update every second

    // Helper methods for rendering tabs
    void RenderHeader();
    void RenderTradeRoutesTab();
    void RenderTradeHubsTab();
    void RenderMarketAnalysisTab();
    void RenderOpportunitiesTab();
    void RenderRouteDetailsPanel();
    void RenderHubDetailsPanel();

    // Utility methods
    void UpdateCachedData();
    std::string GetProvinceName(game::types::EntityID province_id) const;
    std::string GetResourceName(game::types::ResourceType resource) const;
    std::string GetRouteTypeName(game::trade::RouteType route_type) const;
    std::string GetStatusName(game::trade::TradeStatus status) const;
    std::string GetHubTypeName(game::trade::HubType hub_type) const;
    const char* GetResourceTypeIcon(game::types::ResourceType resource) const;

    // Color helpers
    struct ImVec4 GetProfitabilityColor(double profitability) const;
    struct ImVec4 GetStatusColor(game::trade::TradeStatus status) const;
    struct ImVec4 GetUtilizationColor(double utilization) const;
};

} // namespace ui
