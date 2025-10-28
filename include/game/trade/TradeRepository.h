// ============================================================================
// Mechanica Imperii - Trade Repository Header
// Component Access Layer for Trade System
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "game/trade/TradeSystem.h"
#include <memory>
#include <vector>
#include <optional>

namespace game::trade {

    /**
     * @brief Repository pattern for accessing trade-related ECS components
     *
     * Encapsulates all component access logic to:
     * - Eliminate boilerplate code
     * - Centralize component creation
     * - Provide type-safe access
     * - Simplify testing
     */
    class TradeRepository {
    public:
        explicit TradeRepository(core::ecs::ComponentAccessManager& access_manager);
        ~TradeRepository() = default;

        // ====================================================================
        // TradeRouteComponent Access
        // ====================================================================

        /**
         * @brief Get trade route component for a province
         * @return Component pointer or nullptr if not found
         */
        std::shared_ptr<TradeRouteComponent> GetRouteComponent(types::EntityID province_id);
        std::shared_ptr<const TradeRouteComponent> GetRouteComponent(types::EntityID province_id) const;

        /**
         * @brief Get or create trade route component
         * @return Component pointer (guaranteed non-null)
         */
        std::shared_ptr<TradeRouteComponent> GetOrCreateRouteComponent(types::EntityID province_id);

        /**
         * @brief Check if province has trade route component
         */
        bool HasRouteComponent(types::EntityID province_id) const;

        // ====================================================================
        // TradeHubComponent Access
        // ====================================================================

        /**
         * @brief Get trade hub component for a province
         * @return Component pointer or nullptr if not found
         */
        std::shared_ptr<TradeHubComponent> GetHubComponent(types::EntityID province_id);
        std::shared_ptr<const TradeHubComponent> GetHubComponent(types::EntityID province_id) const;

        /**
         * @brief Get or create trade hub component
         * @return Component pointer (guaranteed non-null)
         */
        std::shared_ptr<TradeHubComponent> GetOrCreateHubComponent(types::EntityID province_id);

        /**
         * @brief Check if province has trade hub component
         */
        bool HasHubComponent(types::EntityID province_id) const;

        // ====================================================================
        // TradeInventoryComponent Access
        // ====================================================================

        /**
         * @brief Get trade inventory component for a province
         * @return Component pointer or nullptr if not found
         */
        std::shared_ptr<TradeInventoryComponent> GetInventoryComponent(types::EntityID province_id);
        std::shared_ptr<const TradeInventoryComponent> GetInventoryComponent(types::EntityID province_id) const;

        /**
         * @brief Get or create trade inventory component
         * @return Component pointer (guaranteed non-null)
         */
        std::shared_ptr<TradeInventoryComponent> GetOrCreateInventoryComponent(types::EntityID province_id);

        /**
         * @brief Check if province has trade inventory component
         */
        bool HasInventoryComponent(types::EntityID province_id) const;

        // ====================================================================
        // Bulk Operations
        // ====================================================================

        /**
         * @brief Ensure all trade components exist for a province
         * Creates missing components if needed
         */
        void EnsureAllTradeComponents(types::EntityID province_id);

        /**
         * @brief Get all provinces with trade route components
         */
        std::vector<types::EntityID> GetAllTradeProvinces() const;

        /**
         * @brief Get all provinces with trade hubs
         */
        std::vector<types::EntityID> GetAllHubProvinces() const;

        /**
         * @brief Check if province has all trade components
         */
        bool HasAllTradeComponents(types::EntityID province_id) const;

        // ====================================================================
        // Component Creation
        // ====================================================================

        /**
         * @brief Create route component with default values
         */
        std::shared_ptr<TradeRouteComponent> CreateRouteComponent(types::EntityID province_id);

        /**
         * @brief Create hub component with hub data
         */
        std::shared_ptr<TradeHubComponent> CreateHubComponent(types::EntityID province_id, const TradeHub& hub_data);

        /**
         * @brief Create inventory component with capacity
         */
        std::shared_ptr<TradeInventoryComponent> CreateInventoryComponent(types::EntityID province_id, double capacity = 1000.0);

        // ====================================================================
        // Component Removal
        // ====================================================================

        /**
         * @brief Remove all trade components from a province
         */
        void RemoveAllTradeComponents(types::EntityID province_id);

    private:
        core::ecs::ComponentAccessManager& m_access_manager;
    };

} // namespace game::trade
