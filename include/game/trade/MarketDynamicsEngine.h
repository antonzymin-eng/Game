// ============================================================================
// Mechanica Imperii - Market Dynamics Engine Header
// Manages Price Updates, Market Forces, and Price Shocks
// ============================================================================

#pragma once

#include "game/trade/TradeSystem.h"
#include "game/trade/TradeCalculator.h"
#include "core/messaging/MessageBus.h"
#include <unordered_map>
#include <mutex>
#include <string>

namespace game::trade {

    /**
     * @brief Manages market price dynamics and economic forces
     *
     * Responsibilities:
     * - Update market prices based on supply/demand
     * - Apply price shocks from events
     * - Process seasonal adjustments
     * - Handle price stabilization
     * - Manage market volatility
     */
    class MarketDynamicsEngine {
    public:
        MarketDynamicsEngine(
            std::unordered_map<std::string, MarketData>& market_data,
            const std::unordered_map<types::ResourceType, TradeGoodProperties>& trade_goods,
            core::messaging::ThreadSafeMessageBus& message_bus,
            std::mutex& market_mutex
        );

        // ====================================================================
        // Market Updates
        // ====================================================================

        /**
         * @brief Update all market prices based on supply/demand
         */
        void UpdateAllPrices();

        /**
         * @brief Update supply and demand levels for a specific market
         */
        void UpdateSupplyDemand(types::EntityID province_id, types::ResourceType resource);

        /**
         * @brief Apply market forces (supply/demand changes) to price
         */
        void ApplyMarketForces(MarketData& market, double supply_change, double demand_change);

        // ====================================================================
        // Price Shocks
        // ====================================================================

        /**
         * @brief Apply a price shock to a specific market
         */
        void ApplyPriceShock(types::EntityID province_id, types::ResourceType resource,
                           double shock_magnitude, const std::string& cause);

        /**
         * @brief Process random price shocks (market volatility)
         */
        void ProcessPriceShocks();

        // ====================================================================
        // Seasonal Adjustments
        // ====================================================================

        /**
         * @brief Apply seasonal price adjustments for all markets
         */
        void ProcessSeasonalAdjustments(int current_month);

        /**
         * @brief Apply seasonal adjustment to a specific market
         */
        void ApplySeasonalAdjustment(MarketData& market, int month);

        // ====================================================================
        // Price Stabilization
        // ====================================================================

        /**
         * @brief Process price stabilization (gradual return to average)
         */
        void ProcessPriceStabilization(MarketData& market);

        /**
         * @brief Reduce market volatility over time
         */
        void ReduceVolatility(MarketData& market);

        // ====================================================================
        // Market Queries
        // ====================================================================

        /**
         * @brief Get market data for a province and resource
         */
        MarketData GetMarketData(types::EntityID province_id, types::ResourceType resource) const;

        /**
         * @brief Get or create market data entry
         */
        MarketData& GetOrCreateMarket(types::EntityID province_id, types::ResourceType resource);

        /**
         * @brief Check if market exists
         */
        bool HasMarket(types::EntityID province_id, types::ResourceType resource) const;

        /**
         * @brief Get market key for storage
         */
        static std::string GetMarketKey(types::EntityID province_id, types::ResourceType resource);

    private:
        std::unordered_map<std::string, MarketData>& m_market_data;
        const std::unordered_map<types::ResourceType, TradeGoodProperties>& m_trade_goods;
        core::messaging::ThreadSafeMessageBus& m_message_bus;
        std::mutex& m_market_mutex;

        // Helper methods
        const TradeGoodProperties* GetTradeGood(types::ResourceType resource) const;
        void PublishPriceShock(types::EntityID province_id, types::ResourceType resource,
                              double old_price, double new_price, const std::string& cause);
        void UpdatePriceTrend(MarketData& market, double price_change);
    };

} // namespace game::trade
