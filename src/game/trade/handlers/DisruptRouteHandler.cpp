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
        ::core::threading::ThreadSafeMessageBus& message_bus,
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
        
        // Handle idempotent disruption (already DISRUPTED)
        if (old_status == TradeStatus::DISRUPTED) {
            // Already disrupted - extend/worsen the disruption
            // Only save pre-disruption state if not already saved
            if (route.pre_disruption_volume == 0.0) {
                // This means it's a fresh disruption, save state
                route.pre_disruption_volume = route.current_volume;
                route.pre_disruption_safety = route.safety_rating;
            }
            
            // Extend recovery time (adds to existing countdown)
            route.recovery_months_remaining += m_duration_months;
            route.is_recovering = false;  // Reset recovery if it had started
            route.recovery_progress = 0.0;
            
            // Further reduce volume if it wasn't already at minimum
            if (route.current_volume > route.pre_disruption_volume * 0.05) {
                route.current_volume *= 0.5; // Additional 50% reduction
            }
            
            std::cout << "[TradeSystem] Extended disruption for route " << m_route_id 
                     << " (new duration: " << route.recovery_months_remaining << " months)" << std::endl;
            
            return TradeRouteOperationResult::Success(
                "Route disruption extended: " + m_cause,
                m_route_id,
                -route.current_volume * route.profitability * 0.5
            );
        }
        
        // Fresh disruption - save pre-disruption state
        route.pre_disruption_volume = route.current_volume;
        route.pre_disruption_safety = route.safety_rating;

        // Apply disruption
        route.status = TradeStatus::DISRUPTED;
        route.disruption_count++;

        // Set up recovery parameters
        route.is_recovering = false;  // Not yet recovering, still disrupted
        route.recovery_progress = 0.0;
        route.recovery_months_remaining = m_duration_months;

        // Reduce volume and efficiency
        double volume_before = route.current_volume;
        route.current_volume *= 0.1; // Severe reduction during disruption
        route.safety_rating *= 0.3;

        // Calculate economic impact (clear semantics)
        double volume_delta = route.current_volume - volume_before;  // Negative
        double monthly_profit_before = volume_before * route.profitability * route.source_price;
        double monthly_profit_after = route.current_volume * route.profitability * route.source_price;
        double monthly_profit_delta = monthly_profit_after - monthly_profit_before;  // Negative
        double total_impact = monthly_profit_delta * m_duration_months;  // Total loss over duration

        // Publish event with detailed impact metrics
        PublishRouteDisrupted(route, volume_before, monthly_profit_delta, total_impact);

        std::cout << "[TradeSystem] Disrupted route " << m_route_id << " due to: " << m_cause 
                 << " (monthly loss: " << -monthly_profit_delta << ")" << std::endl;

        return TradeRouteOperationResult::Success(
            "Route disrupted: " + m_cause,
            m_route_id,
            monthly_profit_delta  // Return monthly delta as impact
        );
    }

    bool DisruptRouteHandler::Validate(std::string& failure_reason) const {
        // Check if route exists
        auto route_it = m_active_routes.find(m_route_id);
        if (route_it == m_active_routes.end()) {
            failure_reason = "Route does not exist";
            return false;
        }

        const TradeRoute& route = route_it->second;
        
        // Check route status - cannot disrupt ABANDONED routes
        if (route.status == TradeStatus::ABANDONED) {
            failure_reason = "Cannot disrupt abandoned route - route is permanently closed";
            return false;
        }
        
        // SEASONAL_CLOSED routes can be disrupted (they'll reopen as DISRUPTED)
        // ESTABLISHING routes can be disrupted (delays establishment)
        // ACTIVE routes can be disrupted (normal case)
        // DISRUPTED routes can be disrupted again (idempotent - extends/worsens disruption)

        return true;
    }

    double DisruptRouteHandler::GetEstimatedImpact() const {
        auto route_it = m_active_routes.find(m_route_id);
        if (route_it == m_active_routes.end()) {
            return 0.0;
        }

        const TradeRoute& route = route_it->second;
        
        // Estimate monthly profit loss: current profit - disrupted profit (90% reduction)
        double current_monthly_profit = route.current_volume * route.profitability * route.source_price;
        double disrupted_volume = route.current_volume * 0.1;  // 90% reduction
        double disrupted_monthly_profit = disrupted_volume * route.profitability * route.source_price;
        double monthly_delta = disrupted_monthly_profit - current_monthly_profit;  // Negative
        
        return monthly_delta;  // Return monthly delta (negative = loss)
    }

    void DisruptRouteHandler::PublishRouteDisrupted(const TradeRoute& route, 
                                                    double volume_before,
                                                    double monthly_profit_delta, 
                                                    double total_impact) {
        messages::TradeRouteDisrupted event;
        event.route_id = route.route_id;
        event.source_province = route.source_province;
        event.destination_province = route.destination_province;
        event.resource = route.resource;
        event.disruption_cause = m_cause;
        event.estimated_duration_months = m_duration_months;
        
        // Clear impact metrics
        event.monthly_profit_delta = monthly_profit_delta;  // Negative value (loss per month)
        event.total_impact_over_duration = total_impact;     // Total loss over full duration
        event.volume_before = volume_before;
        event.volume_after = route.current_volume;

        m_message_bus.Publish(event);
    }

} // namespace game::trade
