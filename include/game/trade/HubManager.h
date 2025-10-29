// ============================================================================
// Mechanica Imperii - Hub Manager Header
// Manages Trade Hub Lifecycle and Evolution
// ============================================================================

#pragma once

#include "game/trade/TradeSystem.h"
#include "game/trade/TradeRepository.h"
#include "game/trade/TradeCalculator.h"
#include "core/messaging/MessageBus.h"
#include <unordered_map>
#include <mutex>
#include <vector>

namespace game::trade {

    /**
     * @brief Manages trade hub creation, evolution, and maintenance
     *
     * Responsibilities:
     * - Create new trade hubs
     * - Evolve hubs based on trade volume
     * - Upgrade hub infrastructure
     * - Update hub utilization and specializations
     * - Calculate hub reputation
     */
    class HubManager {
    public:
        HubManager(
            TradeRepository& repository,
            std::unordered_map<types::EntityID, TradeHub>& trade_hubs,
            const std::unordered_map<std::string, TradeRoute>& active_routes,
            core::messaging::ThreadSafeMessageBus& message_bus,
            std::mutex& trade_mutex
        );

        // ====================================================================
        // Hub Creation and Evolution
        // ====================================================================

        /**
         * @brief Create a new trade hub at a province
         */
        void CreateHub(types::EntityID province_id, const std::string& hub_name, HubType initial_type);

        /**
         * @brief Evolve a hub to a more advanced type based on trade patterns
         */
        bool EvolveHub(types::EntityID province_id);

        /**
         * @brief Upgrade hub infrastructure level
         */
        bool UpgradeHub(types::EntityID province_id, int new_level);

        /**
         * @brief Determine optimal hub type based on volume and connectivity
         */
        HubType DetermineOptimalType(types::EntityID province_id) const;

        // ====================================================================
        // Hub Queries
        // ====================================================================

        /**
         * @brief Get hub data for a province
         */
        TradeHub* GetHub(types::EntityID province_id);
        const TradeHub* GetHub(types::EntityID province_id) const;

        /**
         * @brief Check if province has a hub
         */
        bool HasHub(types::EntityID province_id) const;

        /**
         * @brief Get all trading partners for a province
         */
        std::vector<types::EntityID> GetTradingPartners(types::EntityID province_id) const;

        // ====================================================================
        // Hub Maintenance
        // ====================================================================

        /**
         * @brief Update hub utilization based on current trade volume
         */
        void UpdateUtilization(TradeHub& hub);

        /**
         * @brief Update hub specializations based on trade patterns
         */
        void UpdateSpecializations(TradeHub& hub);

        /**
         * @brief Calculate and update hub reputation
         */
        void CalculateReputation(TradeHub& hub);

        /**
         * @brief Update all hubs (called during system update)
         */
        void UpdateAllHubs();

    private:
        TradeRepository& m_repository;
        std::unordered_map<types::EntityID, TradeHub>& m_trade_hubs;
        const std::unordered_map<std::string, TradeRoute>& m_active_routes;
        core::messaging::ThreadSafeMessageBus& m_message_bus;
        std::mutex& m_trade_mutex;

        // Helper methods
        void PublishHubEvolution(const TradeHub& hub, HubType old_type, const std::string& trigger);
        double GetTotalTradeVolume(types::EntityID province_id) const;
        int GetRouteCount(types::EntityID province_id) const;
    };

} // namespace game::trade
