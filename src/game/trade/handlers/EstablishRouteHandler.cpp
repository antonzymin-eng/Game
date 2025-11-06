// ============================================================================
// Mechanica Imperii - Establish Route Handler Implementation
// ============================================================================

#include "game/trade/handlers/EstablishRouteHandler.h"
#include "game/trade/TradeCalculator.h"
#include "utils/RandomGenerator.h"
#include <sstream>
#include <iostream>

namespace game::trade {

    EstablishRouteHandler::EstablishRouteHandler(
        types::EntityID source,
        types::EntityID destination,
        types::ResourceType resource,
        RouteType preferred_type,
        TradeRepository& repository,
        TradePathfinder& pathfinder,
        std::unordered_map<std::string, TradeRoute>& active_routes,
        std::unordered_map<types::EntityID, TradeHub>& trade_hubs,
        const std::unordered_map<types::ResourceType, TradeGoodProperties>& trade_goods,
        ::core::threading::ThreadSafeMessageBus& message_bus,
        std::mutex& trade_mutex,
        double min_profitability_threshold,
        int current_game_year
    )
        : m_source(source)
        , m_destination(destination)
        , m_resource(resource)
        , m_preferred_type(preferred_type)
        , m_repository(repository)
        , m_pathfinder(pathfinder)
        , m_active_routes(active_routes)
        , m_trade_hubs(trade_hubs)
        , m_trade_goods(trade_goods)
        , m_message_bus(message_bus)
        , m_trade_mutex(trade_mutex)
        , m_min_profitability_threshold(min_profitability_threshold)
        , m_current_game_year(current_game_year) {
    }

    TradeRouteOperationResult EstablishRouteHandler::Execute(
        const std::unordered_map<std::string, double>& parameters) {

        // ====================================================================
        // PHASE 1: VALIDATION (No lock needed - read-only checks)
        // ====================================================================
        
        std::string failure_reason;
        if (!Validate(failure_reason)) {
            return TradeRouteOperationResult::Failure(failure_reason);
        }

        // Generate unique route ID
        std::string route_id = GenerateRouteId();

        // Quick check if route already exists (optimistic - will recheck with lock)
        {
            std::lock_guard<std::mutex> lock(m_trade_mutex);
            if (m_active_routes.find(route_id) != m_active_routes.end()) {
                return TradeRouteOperationResult::Failure("Route already exists");
            }
        }

        // ====================================================================
        // PHASE 2: PATHFINDING (No lock - expensive operation)
        // ====================================================================
        
        auto path_result = m_pathfinder.FindOptimalRoute(m_source, m_destination, m_resource);
        if (!path_result.has_value()) {
            return TradeRouteOperationResult::Failure("No viable path found");
        }

        // ====================================================================
        // PHASE 3: CALCULATIONS (No lock - CPU-bound operations)
        // ====================================================================
        
        // Create new trade route
        TradeRoute new_route(route_id, m_source, m_destination, m_resource);
        new_route.route_type = m_preferred_type;
        new_route.distance_km = path_result->total_distance;
        new_route.safety_rating = path_result->safety_rating;
        new_route.efficiency_rating = TradeCalculator::CalculateRouteEfficiency(
            false, false, false); // Simplified

        // Get trade good properties
        const auto* trade_good = GetTradeGood();
        double base_price = trade_good ? trade_good->base_value_per_unit : 1.0;

        // Calculate supply and demand
        double supply_level = TradeCalculator::CalculateSupplyLevel(m_source, m_resource);
        double demand_level = TradeCalculator::CalculateDemandLevel(m_destination, m_resource);

        // Calculate economic parameters
        new_route.source_price = TradeCalculator::CalculateMarketPrice(base_price, supply_level, demand_level);
        new_route.destination_price = TradeCalculator::CalculateMarketPrice(base_price, demand_level, supply_level);

        double bulk_factor = trade_good ? trade_good->bulk_factor : 1.0;
        double perishability = trade_good ? trade_good->perishability : 0.0;
        new_route.transport_cost_per_unit = TradeCalculator::CalculateTransportCost(
            new_route.distance_km, bulk_factor, perishability, new_route.efficiency_rating);

        new_route.profitability = TradeCalculator::CalculateRouteProfitability(new_route);

        // Set initial volume based on demand and supply
        new_route.base_volume = TradeCalculator::CalculateOptimalVolume(
            supply_level, demand_level, 1000.0); // 1000 capacity limit
        new_route.current_volume = new_route.base_volume;

        // Check route viability
        if (!IsRouteViable(new_route)) {
            return TradeRouteOperationResult::Failure("Route is not economically viable");
        }

        // Set route as active
        new_route.status = TradeStatus::ACTIVE;
        new_route.established_year = m_current_game_year;  // Use actual game year, not RNG

        // ====================================================================
        // PHASE 4: COMMIT (Lock acquired - mutating shared state)
        // ====================================================================
        
        {
            std::lock_guard<std::mutex> lock(m_trade_mutex);
            
            // Double-check route doesn't exist (race condition protection)
            if (m_active_routes.find(route_id) != m_active_routes.end()) {
                return TradeRouteOperationResult::Failure("Route already exists (race condition)");
            }

            // CANONICAL STORAGE: Store route in ONE place only
            m_active_routes[route_id] = new_route;

            // Update hub connections
            auto source_hub_it = m_trade_hubs.find(m_source);
            if (source_hub_it != m_trade_hubs.end()) {
                source_hub_it->second.AddRoute(route_id, false); // Outgoing
            }

            auto dest_hub_it = m_trade_hubs.find(m_destination);
            if (dest_hub_it != m_trade_hubs.end()) {
                dest_hub_it->second.AddRoute(route_id, true); // Incoming
            }
        } // Lock released here

        // ====================================================================
        // PHASE 5: COMPONENT UPDATES (Repository handles its own locking)
        // ====================================================================
        
        // Ensure trade components exist
        m_repository.EnsureAllTradeComponents(m_source);
        m_repository.EnsureAllTradeComponents(m_destination);

        // Add route ID to source component (not the full route!)
        auto source_trade_comp = m_repository.GetRouteComponent(m_source);
        if (source_trade_comp) {
            source_trade_comp->active_route_ids.push_back(route_id);
            source_trade_comp->route_id_set.insert(route_id);
            // Update cached aggregates
            source_trade_comp->total_monthly_volume += new_route.current_volume;
            source_trade_comp->total_monthly_profit += new_route.current_volume * new_route.profitability;
        }

        // Add route ID to destination component (FIXED: destination was missing!)
        auto dest_trade_comp = m_repository.GetRouteComponent(m_destination);
        if (dest_trade_comp) {
            dest_trade_comp->active_route_ids.push_back(route_id);
            dest_trade_comp->route_id_set.insert(route_id);
            // Update cached aggregates for destination too
            dest_trade_comp->total_monthly_volume += new_route.current_volume;
            dest_trade_comp->total_monthly_profit += new_route.current_volume * new_route.profitability;
        }

        // Publish establishment event (message bus handles its own locking)
        PublishRouteEstablished(new_route);

        // Calculate expected impact
        double expected_income = TradeCalculator::CalculateMonthlyIncome(
            new_route.current_volume,
            new_route.destination_price,
            new_route.source_price,
            new_route.transport_cost_per_unit
        );

        return TradeRouteOperationResult::Success(
            "Trade route established successfully",
            route_id,
            expected_income
        );
    }

    bool EstablishRouteHandler::Validate(std::string& failure_reason) const {
        // Check for same source and destination
        if (m_source == m_destination) {
            failure_reason = "Cannot establish trade route to the same province";
            return false;
        }

        // Validate source province exists
        if (!m_repository.ProvinceExists(m_source)) {
            failure_reason = "Source province (ID: " + std::to_string(m_source) + ") does not exist";
            return false;
        }

        // Validate destination province exists
        if (!m_repository.ProvinceExists(m_destination)) {
            failure_reason = "Destination province (ID: " + std::to_string(m_destination) + ") does not exist";
            return false;
        }

        // Validate resource type exists in trade goods
        if (!TradeRepository::ResourceExists(m_resource, m_trade_goods)) {
            failure_reason = "Resource type (ID: " + std::to_string(static_cast<int>(m_resource)) + ") is not a valid trade good";
            return false;
        }

        // Additional basic validation
        if (m_source == 0 || m_destination == 0) {
            failure_reason = "Invalid province IDs (cannot be zero)";
            return false;
        }

        return true;
    }

    double EstablishRouteHandler::GetEstimatedImpact() const {
        // Estimate potential profit
        double supply = TradeCalculator::CalculateSupplyLevel(m_source, m_resource);
        double demand = TradeCalculator::CalculateDemandLevel(m_destination, m_resource);
        double distance = TradeCalculator::CalculateDistance(m_source, m_destination);

        const auto* trade_good = GetTradeGood();
        double base_price = trade_good ? trade_good->base_value_per_unit : 1.0;
        double bulk_factor = trade_good ? trade_good->bulk_factor : 1.0;
        double perishability = trade_good ? trade_good->perishability : 0.0;

        double source_price = TradeCalculator::CalculateMarketPrice(base_price, supply, demand);
        double dest_price = TradeCalculator::CalculateMarketPrice(base_price, demand, supply);
        double transport_cost = TradeCalculator::CalculateTransportCost(
            distance, bulk_factor, perishability, 1.0);

        double profitability = TradeCalculator::EstimateRouteProfitability(
            source_price, dest_price, transport_cost, 0.9, 1.0);

        double estimated_volume = TradeCalculator::CalculateOptimalVolume(supply, demand, 1000.0);
        return estimated_volume * profitability * source_price; // Monthly profit estimate
    }

    std::string EstablishRouteHandler::GenerateRouteId() const {
        std::ostringstream oss;
        oss << "route_" << m_source << "_" << m_destination << "_" << static_cast<int>(m_resource);
        return oss.str();
    }

    bool EstablishRouteHandler::IsRouteViable(const TradeRoute& route) const {
        return route.profitability >= m_min_profitability_threshold &&
               route.safety_rating > 0.2 &&
               route.current_volume > 0.0;
    }

    const TradeGoodProperties* EstablishRouteHandler::GetTradeGood() const {
        auto it = m_trade_goods.find(m_resource);
        return (it != m_trade_goods.end()) ? &it->second : nullptr;
    }

    void EstablishRouteHandler::PublishRouteEstablished(const TradeRoute& route) {
        messages::TradeRouteEstablished event;
        event.route_id = route.route_id;
        event.source_province = route.source_province;
        event.destination_province = route.destination_province;
        event.resource = route.resource;
        event.expected_monthly_profit = route.GetEffectiveVolume() * route.profitability;
        event.route_type = route.route_type;
        event.establishment_reason = "Economic opportunity identified";

        m_message_bus.Publish(event);
    }

} // namespace game::trade
