// ============================================================================
// Mechanica Imperii - Disrupt Route Handler Implementation
// ============================================================================

#include "game/trade/handlers/DisruptRouteHandler.h"
#include <iostream>

namespace game::trade {

    DisruptRouteHandler::DisruptRouteHandler(
        const std::string& route_id,
        const std::string& cause,
        double duration_months,
        std::unordered_map<std::string, TradeRoute>& active_routes,
        core::messaging::ThreadSafeMessageBus& message_bus,
        std::mutex& trade_mutex
    )
        : m_route_id(route_id)
        , m_cause(cause)
        , m_duration_months(duration_months)
        , m_active_routes(active_routes)
        , m_message_bus(message_bus)
        , m_trade_mutex(trade_mutex) {
    }

    TradeRouteOperationResult DisruptRouteHandler::Execute(
        const std::unordered_map<std::string, double>& parameters) {

        std::lock_guard<std::mutex> lock(m_trade_mutex);

        // Validate
        std::string failure_reason;
        if (!Validate(failure_reason)) {
            return TradeRouteOperationResult::Failure(failure_reason);
        }

        // Get route
        auto route_it = m_active_routes.find(m_route_id);
        TradeRoute& route = route_it->second;

        // Store old status
        TradeStatus old_status = route.status;

        // Apply disruption
        route.status = TradeStatus::DISRUPTED;
        route.disruption_count++;

        // Reduce volume and efficiency
        double volume_before = route.current_volume;
        route.current_volume *= 0.1; // Severe reduction during disruption
        route.safety_rating *= 0.3;

        // Calculate economic impact
        double economic_impact = (volume_before - route.current_volume) * route.profitability;

        // Publish event
        PublishRouteDisrupted(route);

        std::cout << "[TradeSystem] Disrupted route " << m_route_id << " due to: " << m_cause << std::endl;

        return TradeRouteOperationResult::Success(
            "Route disrupted: " + m_cause,
            m_route_id,
            -economic_impact // Negative impact
        );
    }

    bool DisruptRouteHandler::Validate(std::string& failure_reason) const {
        // Check if route exists
        auto route_it = m_active_routes.find(m_route_id);
        if (route_it == m_active_routes.end()) {
            failure_reason = "Route does not exist";
            return false;
        }

        // Route exists and can be disrupted
        return true;
    }

    double DisruptRouteHandler::GetEstimatedImpact() const {
        auto route_it = m_active_routes.find(m_route_id);
        if (route_it == m_active_routes.end()) {
            return 0.0;
        }

        const TradeRoute& route = route_it->second;
        // Estimate loss: 90% of current volume * profitability
        return -route.current_volume * 0.9 * route.profitability;
    }

    void DisruptRouteHandler::PublishRouteDisrupted(const TradeRoute& route) {
        messages::TradeRouteDisrupted event;
        event.route_id = route.route_id;
        event.source_province = route.source_province;
        event.destination_province = route.destination_province;
        event.resource = route.resource;
        event.disruption_cause = m_cause;
        event.estimated_duration_months = m_duration_months;
        event.economic_impact = route.GetEffectiveVolume() * route.profitability;

        m_message_bus.Publish(event);
    }

} // namespace game::trade
