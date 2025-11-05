// ============================================================================
// Mechanica Imperii - Establish Route Handler
// Handles Trade Route Establishment Logic
// ============================================================================

#pragma once

#include "game/trade/handlers/ITradeRouteHandler.h"
#include "game/trade/TradeRepository.h"
#include "game/trade/TradeCalculator.h"
#include "game/trade/TradeSystem.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace game::trade {

    /**
     * @brief Handler for establishing new trade routes
     *
     * Responsibilities:
     * - Find optimal path between provinces
     * - Calculate route economics
     * - Validate route viability
     * - Create route and update components
     * - Publish route established event
     */
    class EstablishRouteHandler : public ITradeRouteHandler {
    public:
        EstablishRouteHandler(
            types::EntityID source,
            types::EntityID destination,
            types::ResourceType resource,
            RouteType preferred_type,
            TradeRepository& repository,
            TradePathfinder& pathfinder,
            std::unordered_map<std::string, TradeRoute>& active_routes,
            std::unordered_map<types::EntityID, TradeHub>& trade_hubs,
            const std::unordered_map<types::ResourceType, TradeGoodProperties>& trade_goods,
            core::messaging::ThreadSafeMessageBus& message_bus,
            std::mutex& trade_mutex,
            double min_profitability_threshold
        );

        TradeRouteOperationResult Execute(
            const std::unordered_map<std::string, double>& parameters
        ) override;

        bool Validate(std::string& failure_reason) const override;
        std::string GetOperationName() const override { return "EstablishTradeRoute"; }
        double GetEstimatedImpact() const override;

    private:
        // Route parameters
        types::EntityID m_source;
        types::EntityID m_destination;
        types::ResourceType m_resource;
        RouteType m_preferred_type;

        // System references
        TradeRepository& m_repository;
        TradePathfinder& m_pathfinder;
        std::unordered_map<std::string, TradeRoute>& m_active_routes;
        std::unordered_map<types::EntityID, TradeHub>& m_trade_hubs;
        const std::unordered_map<types::ResourceType, TradeGoodProperties>& m_trade_goods;
        core::messaging::ThreadSafeMessageBus& m_message_bus;
        std::mutex& m_trade_mutex;

        // Configuration
        double m_min_profitability_threshold;

        // Helper methods
        std::string GenerateRouteId() const;
        bool IsRouteViable(const TradeRoute& route) const;
        const TradeGoodProperties* GetTradeGood() const;
        void PublishRouteEstablished(const TradeRoute& route);
    };

} // namespace game::trade
