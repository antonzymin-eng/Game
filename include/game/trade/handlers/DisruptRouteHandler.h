// ============================================================================
// Mechanica Imperii - Disrupt Route Handler
// Handles Trade Route Disruption Logic
// ============================================================================

#pragma once

#include "game/trade/handlers/ITradeRouteHandler.h"
#include "game/trade/TradeSystem.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace game::trade {

    /**
     * @brief Handler for disrupting existing trade routes
     *
     * Responsibilities:
     * - Validate route exists and is active
     * - Apply disruption effects (reduced volume, safety)
     * - Publish route disrupted event
     */
    class DisruptRouteHandler : public ITradeRouteHandler {
    public:
        DisruptRouteHandler(
            const std::string& route_id,
            const std::string& cause,
            double duration_months,
            std::unordered_map<std::string, TradeRoute>& active_routes,
            core::messaging::ThreadSafeMessageBus& message_bus,
            std::mutex& trade_mutex
        );

        TradeRouteOperationResult Execute(
            const std::unordered_map<std::string, double>& parameters
        ) override;

        bool Validate(std::string& failure_reason) const override;
        std::string GetOperationName() const override { return "DisruptTradeRoute"; }
        double GetEstimatedImpact() const override;

    private:
        std::string m_route_id;
        std::string m_cause;
        double m_duration_months;

        std::unordered_map<std::string, TradeRoute>& m_active_routes;
        core::messaging::ThreadSafeMessageBus& m_message_bus;
        std::mutex& m_trade_mutex;

        void PublishRouteDisrupted(const TradeRoute& route);
    };

} // namespace game::trade
