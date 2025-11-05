// ============================================================================
// Mechanica Imperii - Trade System Implementation
// Created: September 22, 2025, 11:45 AM
// Location: src/game/trade/TradeSystem.cpp
// ============================================================================

#include "game/trade/TradeSystem.h"
// Note: EnhancedProvinceSystem is optional - can be set via SetProvinceSystem()
// #include "game/province/EnhancedProvinceSystem.h"
#include "core/logging/Logger.h"
#include "utils/PlatformCompat.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <queue>
#include <unordered_set>

namespace game::trade {

    // ========================================================================
    // TradeRoute Implementation
    // ========================================================================

    TradeRoute::TradeRoute(const std::string& id, types::EntityID src, types::EntityID dst, types::ResourceType res)
        : route_id(id), source_province(src), destination_province(dst), resource(res) {}

    bool TradeRoute::IsViable() const {
        return status == TradeStatus::ACTIVE && 
               profitability > 0.05 && 
               safety_rating > 0.3 &&
               current_volume > 0.0;
    }

    double TradeRoute::GetEffectiveVolume() const {
        if (status != TradeStatus::ACTIVE) return 0.0;
        return current_volume * efficiency_rating * safety_rating * seasonal_modifier;
    }

    std::string TradeRoute::GetRouteDescription() const {
        std::ostringstream oss;
        oss << "Route " << route_id << ": " << source_province << " -> " << destination_province;
        oss << " (" << static_cast<int>(resource) << ")";
        oss << " [" << std::fixed << std::setprecision(1) << distance_km << "km]";
        return oss.str();
    }

    // ========================================================================
    // TradeHub Implementation
    // ========================================================================

    TradeHub::TradeHub(types::EntityID province, const std::string& name)
        : province_id(province), hub_name(name) {}

    bool TradeHub::CanHandleVolume(double additional_volume) const {
        return (current_utilization + additional_volume / max_throughput_capacity) <= 1.0;
    }

    double TradeHub::GetEffectiveCapacity() const {
        return max_throughput_capacity * infrastructure_bonus * (reputation_rating * 0.5 + 0.5);
    }

    void TradeHub::AddRoute(const std::string& route_id, bool is_incoming) {
        if (is_incoming) {
            incoming_route_ids.push_back(route_id);
        } else {
            outgoing_route_ids.push_back(route_id);
        }
    }

    void TradeHub::RemoveRoute(const std::string& route_id) {
        incoming_route_ids.erase(
            std::remove(incoming_route_ids.begin(), incoming_route_ids.end(), route_id),
            incoming_route_ids.end());
        outgoing_route_ids.erase(
            std::remove(outgoing_route_ids.begin(), outgoing_route_ids.end(), route_id),
            outgoing_route_ids.end());
    }

    // ========================================================================
    // TradeGoodProperties Implementation
    // ========================================================================

    TradeGoodProperties::TradeGoodProperties(types::ResourceType type) : resource_type(type) {
        // Set up default seasonal patterns (can be overridden)
        for (int month = 1; month <= 12; ++month) {
            seasonal_demand[month] = 1.0;
            seasonal_supply[month] = 1.0;
        }
    }

    double TradeGoodProperties::GetSeasonalDemandMultiplier(int month) const {
        auto it = seasonal_demand.find(month);
        return (it != seasonal_demand.end()) ? it->second : 1.0;
    }

    double TradeGoodProperties::GetSeasonalSupplyMultiplier(int month) const {
        auto it = seasonal_supply.find(month);
        return (it != seasonal_supply.end()) ? it->second : 1.0;
    }

    bool TradeGoodProperties::IsAvailable(int year) const {
        return available_in_period && year >= introduction_year && year <= obsolescence_year;
    }

    // ========================================================================
    // MarketData Implementation
    // ========================================================================

    MarketData::MarketData(types::EntityID province, types::ResourceType resource)
        : province_id(province), resource(resource) {}

    bool MarketData::IsPriceAboveAverage() const {
        return current_price > avg_price_12_months * 1.1; // 10% above average
    }

    bool MarketData::IsExperiencingShock() const {
        return trend == PriceMovement::SHOCK_UP || trend == PriceMovement::SHOCK_DOWN;
    }

    double MarketData::GetPriceDeviation() const {
        if (avg_price_12_months > 0.0) {
            return (current_price - avg_price_12_months) / avg_price_12_months;
        }
        return 0.0;
    }

    // ========================================================================
    // Component Implementations
    // ========================================================================
    // Note: Component type IDs and cloning are now handled by game::core::Component<T> base class

    // ========================================================================
    // TradePathfinder Implementation
    // ========================================================================

    TradePathfinder::TradePathfinder() {
        // Initialize default cost weights
        m_distance_weight = 1.0;
        m_safety_weight = 0.3;
        m_efficiency_weight = 0.2;
    }

    std::optional<TradePathfinder::RoutePath> TradePathfinder::FindOptimalRoute(
        types::EntityID source, types::EntityID destination, types::ResourceType resource) {
        
        // A* pathfinding implementation
        std::priority_queue<PathNode, std::vector<PathNode>, 
                           std::function<bool(const PathNode&, const PathNode&)>> open_set(
            [](const PathNode& a, const PathNode& b) {
                return a.estimated_total_cost > b.estimated_total_cost;
            });
        
        std::unordered_set<types::EntityID> closed_set;
        std::unordered_map<types::EntityID, PathNode> node_data;
        
        // Initialize start node
        PathNode start_node;
        start_node.province_id = source;
        start_node.cost_to_reach = 0.0;
        start_node.estimated_total_cost = HeuristicDistance(source, destination);
        start_node.parent_province = source; // Self-reference for start
        
        open_set.push(start_node);
        node_data[source] = start_node;
        
        while (!open_set.empty()) {
            PathNode current = open_set.top();
            open_set.pop();
            
            // Check if we reached the destination
            if (current.province_id == destination) {
                // Reconstruct path
                RoutePath result_path;
                std::vector<types::EntityID> path_nodes;
                std::vector<RouteType> connection_types;
                
                types::EntityID current_id = destination;
                while (current_id != source) {
                    path_nodes.push_back(current_id);
                    auto& node = node_data[current_id];
                    connection_types.push_back(node.connection_type);
                    current_id = node.parent_province;
                }
                path_nodes.push_back(source);
                
                // Reverse to get source->destination order
                std::reverse(path_nodes.begin(), path_nodes.end());
                std::reverse(connection_types.begin(), connection_types.end());
                
                result_path.waypoints = path_nodes;
                result_path.connection_types = connection_types;
                result_path.total_cost = current.cost_to_reach;
                result_path.total_distance = current.cost_to_reach; // Simplified
                result_path.estimated_travel_time_days = current.cost_to_reach / 50.0; // 50km/day average
                result_path.safety_rating = CalculateRouteSafety(result_path);
                
                return result_path;
            }
            
            closed_set.insert(current.province_id);
            
            // Explore neighbors
            auto neighbors = GetNeighboringProvinces(current.province_id);
            for (auto neighbor_id : neighbors) {
                if (closed_set.find(neighbor_id) != closed_set.end()) {
                    continue; // Already processed
                }
                
                RouteType connection_type = DetermineConnectionType(current.province_id, neighbor_id);
                double move_cost = GetConnectionCost(current.province_id, neighbor_id, connection_type);
                double new_cost = current.cost_to_reach + move_cost;
                
                // Check if this is within our search limits
                if (new_cost > m_max_search_distance) {
                    continue;
                }
                
                auto existing_node_it = node_data.find(neighbor_id);
                if (existing_node_it == node_data.end() || new_cost < existing_node_it->second.cost_to_reach) {
                    PathNode neighbor_node;
                    neighbor_node.province_id = neighbor_id;
                    neighbor_node.cost_to_reach = new_cost;
                    neighbor_node.estimated_total_cost = new_cost + HeuristicDistance(neighbor_id, destination);
                    neighbor_node.parent_province = current.province_id;
                    neighbor_node.connection_type = connection_type;
                    
                    node_data[neighbor_id] = neighbor_node;
                    open_set.push(neighbor_node);
                }
            }
        }
        
        return std::nullopt; // No path found
    }

    std::vector<TradePathfinder::RoutePath> TradePathfinder::FindAlternativeRoutes(
        types::EntityID source, types::EntityID destination, types::ResourceType resource, int max_alternatives) {
        
        std::vector<RoutePath> alternatives;
        
        // Find optimal route first
        auto optimal = FindOptimalRoute(source, destination, resource);
        if (optimal.has_value()) {
            alternatives.push_back(optimal.value());
        }
        
        // For now, return just the optimal route
        // A full implementation would use k-shortest paths algorithm
        return alternatives;
    }

    double TradePathfinder::CalculateRouteCost(const RoutePath& path, types::ResourceType resource) {
        double total_cost = 0.0;
        
        for (size_t i = 0; i < path.connection_types.size(); ++i) {
            RouteType connection = path.connection_types[i];
            double segment_distance = 50.0; // Simplified - would calculate actual distance
            
            // Apply different costs based on route type
            switch (connection) {
            case RouteType::LAND: total_cost += segment_distance * 1.0; break;
            case RouteType::RIVER: total_cost += segment_distance * 0.7; break;
            case RouteType::COASTAL: total_cost += segment_distance * 0.5; break;
            case RouteType::SEA: total_cost += segment_distance * 0.3; break;
            case RouteType::OVERLAND_LONG: total_cost += segment_distance * 1.5; break;
            }
        }
        
        return total_cost;
    }

    double TradePathfinder::CalculateRouteSafety(const RoutePath& path) {
        // Simplified safety calculation - would integrate with political/military systems
        double safety = 1.0;
        
        for (auto province_id : path.waypoints) {
            // Would check for: wars, banditry, political instability
            // For now, assume base safety with some random variation
            utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
            safety *= rng.randomFloat(0.8f, 1.0f);
        }
        
        return std::max(0.1, safety); // Minimum 10% safety
    }

    double TradePathfinder::CalculateRouteEfficiency(const RoutePath& path) {
        // Simplified efficiency calculation - would integrate with infrastructure
        double efficiency = 1.0;
        
        for (size_t i = 0; i < path.connection_types.size(); ++i) {
            RouteType connection = path.connection_types[i];
            
            switch (connection) {
            case RouteType::LAND: efficiency *= 0.8; break;
            case RouteType::RIVER: efficiency *= 1.2; break;
            case RouteType::COASTAL: efficiency *= 1.1; break;
            case RouteType::SEA: efficiency *= 1.5; break;
            case RouteType::OVERLAND_LONG: efficiency *= 0.6; break;
            }
        }
        
        return std::min(2.0, efficiency); // Maximum 200% efficiency
    }

    void TradePathfinder::UpdateNetworkConnectivity() {
        // This would rebuild connectivity data based on current game state
        // For now, just log that update occurred
        std::cout << "Trade network connectivity updated" << std::endl;
    }

    bool TradePathfinder::IsRouteViable(types::EntityID source, types::EntityID destination, double max_distance) {
        auto route = FindOptimalRoute(source, destination, types::ResourceType::FOOD); // Use food as test resource
        return route.has_value() && route->total_distance <= max_distance;
    }

    void TradePathfinder::SetMaxSearchDistance(double max_km) {
        m_max_search_distance = std::max(100.0, max_km); // Minimum 100km
    }

    void TradePathfinder::SetCostWeights(double distance_weight, double safety_weight, double efficiency_weight) {
        m_distance_weight = std::max(0.1, distance_weight);
        m_safety_weight = std::max(0.0, safety_weight);
        m_efficiency_weight = std::max(0.0, efficiency_weight);
    }

    std::vector<types::EntityID> TradePathfinder::GetNeighboringProvinces(types::EntityID province_id) {
        // Simplified - would query actual province system for neighbors
        std::vector<types::EntityID> neighbors;
        
        // For demonstration, create some mock neighbors
        for (int i = 1; i <= 4; ++i) {
            types::EntityID neighbor = province_id + i;
            if (neighbor != province_id) {
                neighbors.push_back(neighbor);
            }
        }
        
        return neighbors;
    }

    double TradePathfinder::GetConnectionCost(types::EntityID from, types::EntityID to, RouteType connection_type) {
        // Simplified distance calculation - would use actual geographic data
        double base_distance = 50.0; // Default 50km between adjacent provinces
        
        // Apply route type modifiers
        switch (connection_type) {
        case RouteType::LAND: return base_distance * 1.0;
        case RouteType::RIVER: return base_distance * 0.8;
        case RouteType::COASTAL: return base_distance * 0.9;
        case RouteType::SEA: return base_distance * 1.2; // Longer but more efficient
        case RouteType::OVERLAND_LONG: return base_distance * 2.0;
        }
        
        return base_distance;
    }

    RouteType TradePathfinder::DetermineConnectionType(types::EntityID from, types::EntityID to) {
        // Simplified determination - would check actual geographic features
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        
        int connection_type = rng.randomInt(0, 4);
        return static_cast<RouteType>(connection_type);
    }

    double TradePathfinder::HeuristicDistance(types::EntityID from, types::EntityID to) {
        // Simplified heuristic - would use actual coordinates
        return std::abs(static_cast<int>(to) - static_cast<int>(from)) * 50.0;
    }

    // ========================================================================
    // TradeSystem Implementation
    // ========================================================================

    TradeSystem::TradeSystem(::core::ecs::ComponentAccessManager& access_manager,
                           ::core::threading::ThreadSafeMessageBus& message_bus)
        : m_access_manager(access_manager)
        , m_message_bus(message_bus)
        , m_pathfinder(std::make_unique<TradePathfinder>()) {

        m_last_performance_check = std::chrono::steady_clock::now();
    }

    void TradeSystem::Initialize() {
        // Component types are registered automatically when first accessed

        // Initialize trade goods with historical properties
        InitializeTradeGoods();
        
        // Set up initial trade hubs for major medieval cities
        InitializeDefaultHubs();
        
        // Load configuration from external files
        LoadTradeConfiguration();
        
        // Subscribe to relevant messages
        m_message_bus.Subscribe<messages::TradeRouteDisrupted>(
            [this](const messages::TradeRouteDisrupted& event) {
                DisruptTradeRoute(event.route_id, event.disruption_cause, event.estimated_duration_months);
            });
        
        std::cout << "TradeSystem initialized with " << m_trade_goods.size() 
                  << " trade goods and " << m_trade_hubs.size() << " initial hubs." << std::endl;
    }

    void TradeSystem::Update(float deltaTime) {
        auto update_start = std::chrono::steady_clock::now();
        
        m_accumulated_time += deltaTime;
        m_price_accumulated_time += deltaTime;
        m_routes_processed_this_frame = 0;
        
        // Update trade routes at configured frequency
        if (m_accumulated_time >= (1.0f / m_update_frequency)) {
            std::lock_guard<std::mutex> lock(m_trade_mutex);
            
            // Process active trade routes
            for (auto& [route_id, route] : m_active_routes) {
                if (m_routes_processed_this_frame >= m_max_routes_per_frame) {
                    break; // Limit processing per frame
                }
                
                ProcessTradeFlow(route, m_accumulated_time);
                UpdateRouteConditions(route);
                m_routes_processed_this_frame++;
            }
            
            // Update trade hubs
            for (auto& [province_id, hub] : m_trade_hubs) {
                UpdateHubUtilization(hub);
                UpdateHubSpecializations(hub);
            }
            
            m_accumulated_time = 0.0f;
        }
        
        // Update market prices less frequently
        if (m_price_accumulated_time >= m_price_update_interval) {
            UpdateMarketPrices();
            m_price_accumulated_time = 0.0f;
        }
        
        // Update performance metrics
        UpdatePerformanceMetrics();
        
        auto update_end = std::chrono::steady_clock::now();
        m_performance_metrics.total_update_ms = 
            std::chrono::duration_cast<std::chrono::microseconds>(update_end - update_start).count() / 1000.0;
    }

    void TradeSystem::Shutdown() {
        // Save current trade state if needed
        std::lock_guard<std::mutex> trade_lock(m_trade_mutex);
        std::lock_guard<std::mutex> market_lock(m_market_mutex);
        
        // Clear all trade data
        m_active_routes.clear();
        m_trade_hubs.clear();
        m_market_data.clear();
        
        std::cout << "TradeSystem shutdown complete." << std::endl;
    }

    ::core::threading::ThreadingStrategy TradeSystem::GetThreadingStrategy() const {
        return ::core::threading::ThreadingStrategy::THREAD_POOL;
    }

    std::string TradeSystem::GetThreadingRationale() const {
        return "Trade System uses THREAD_POOL strategy for parallel processing of trade routes, "
               "market calculations, and pathfinding operations. Trade route optimization and "
               "price calculations are CPU-intensive and benefit from parallel execution. "
               "Hub evolution and route establishment can be processed independently.";
    }

// ========================================================================
    // Trade Route Management Implementation
    // ========================================================================

    std::string TradeSystem::EstablishTradeRoute(types::EntityID source, types::EntityID destination,
                                                types::ResourceType resource, RouteType preferred_type) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        // Generate unique route ID
        std::string route_id = GenerateRouteId(source, destination, resource);
        
        // Check if route already exists
        if (m_active_routes.find(route_id) != m_active_routes.end()) {
            LogTradeActivity("Route " + route_id + " already exists");
            return route_id;
        }
        
        // Find optimal path
        auto path_result = m_pathfinder->FindOptimalRoute(source, destination, resource);
        if (!path_result.has_value()) {
            LogTradeActivity("No viable path found for route " + route_id);
            return "";
        }
        
        // Create new trade route
        TradeRoute new_route(route_id, source, destination, resource);
        new_route.route_type = preferred_type;
        new_route.distance_km = path_result->total_distance;
        new_route.safety_rating = path_result->safety_rating;
        new_route.efficiency_rating = CalculateRouteEfficiency(source, destination);
        
        // Calculate economic parameters
        new_route.source_price = CalculateMarketPrice(source, resource);
        new_route.destination_price = CalculateMarketPrice(destination, resource);
        new_route.transport_cost_per_unit = CalculateTransportCost(source, destination, resource);
        new_route.profitability = CalculateRouteProfitability(new_route);
        
        // Set initial volume based on demand and supply
        double supply_level = CalculateSupplyLevel(source, resource);
        double demand_level = CalculateDemandLevel(destination, resource);
        new_route.base_volume = std::min(supply_level, demand_level) * 0.1; // 10% of available supply/demand
        new_route.current_volume = new_route.base_volume;
        
        // Check route viability
        if (!IsRouteViable(new_route)) {
            LogTradeActivity("Route " + route_id + " is not economically viable");
            return "";
        }
        
        // Add route to active routes
        new_route.status = TradeStatus::ACTIVE;
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        new_route.established_year = 1066 + rng.randomInt(0, 834); // Random year in game period
        
        m_active_routes[route_id] = new_route;
        
        // Update hub connections
        auto source_hub = m_trade_hubs.find(source);
        if (source_hub != m_trade_hubs.end()) {
            source_hub->second.AddRoute(route_id, false); // Outgoing
        }
        
        auto dest_hub = m_trade_hubs.find(destination);
        if (dest_hub != m_trade_hubs.end()) {
            dest_hub->second.AddRoute(route_id, true); // Incoming
        }
        
        // Create ECS components for provinces if they don't exist
        EnsureTradeComponentsExist(source);
        EnsureTradeComponentsExist(destination);

        // Add route to province components
        auto source_trade_comp = m_access_manager.GetComponentForWrite<TradeRouteComponent>(source);
        if (source_trade_comp) {
            source_trade_comp->active_routes.push_back(new_route);
            source_trade_comp->route_registry[route_id] = new_route;
        }
        
        // Publish establishment event
        PublishTradeRouteEstablished(new_route, "Economic opportunity identified");
        
        LogTradeActivity("Established trade route: " + new_route.GetRouteDescription());
        return route_id;
    }

    bool TradeSystem::DisruptTradeRoute(const std::string& route_id, const std::string& cause, double duration_months) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        auto route_it = m_active_routes.find(route_id);
        if (route_it == m_active_routes.end()) {
            return false;
        }
        
        TradeRoute& route = route_it->second;
        TradeStatus old_status = route.status;
        route.status = TradeStatus::DISRUPTED;
        route.disruption_count++;
        
        // Reduce volume and efficiency
        route.current_volume *= 0.1; // Severe reduction during disruption
        route.safety_rating *= 0.3;
        
        // Schedule restoration (simplified - would integrate with event system)
        // For now, just log the disruption
        
        PublishTradeRouteDisrupted(route, cause, duration_months);
        LogTradeActivity("Disrupted route " + route_id + " due to: " + cause);
        
        return true;
    }

    bool TradeSystem::RestoreTradeRoute(const std::string& route_id) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        auto route_it = m_active_routes.find(route_id);
        if (route_it == m_active_routes.end() || route_it->second.status != TradeStatus::DISRUPTED) {
            return false;
        }
        
        TradeRoute& route = route_it->second;
        route.status = TradeStatus::ACTIVE;
        
        // Restore volume and safety (gradual recovery would be more realistic)
        route.current_volume = route.base_volume * 0.8; // 80% recovery initially
        route.safety_rating = std::min(1.0, route.safety_rating * 2.0); // Improve safety
        
        LogTradeActivity("Restored trade route: " + route_id);
        return true;
    }

    void TradeSystem::AbandonTradeRoute(const std::string& route_id) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        auto route_it = m_active_routes.find(route_id);
        if (route_it == m_active_routes.end()) {
            return;
        }
        
        TradeRoute& route = route_it->second;
        route.status = TradeStatus::ABANDONED;
        
        // Remove from hub connections
        auto source_hub = m_trade_hubs.find(route.source_province);
        if (source_hub != m_trade_hubs.end()) {
            source_hub->second.RemoveRoute(route_id);
        }
        
        auto dest_hub = m_trade_hubs.find(route.destination_province);
        if (dest_hub != m_trade_hubs.end()) {
            dest_hub->second.RemoveRoute(route_id);
        }
        
        // Remove from active routes (will be cleaned up later)
        LogTradeActivity("Abandoned trade route: " + route_id);
        m_active_routes.erase(route_it);
    }

    void TradeSystem::OptimizeTradeRoutes(types::EntityID province_id) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        auto trade_comp = m_access_manager.GetComponent<TradeRouteComponent>(province_id);
        if (!trade_comp) {
            return;
        }
        
        // Analyze existing routes for profitability
        std::vector<std::string> unprofitable_routes;
        for (const auto& route : trade_comp->active_routes) {
            if (route.profitability < m_min_profitability_threshold) {
                unprofitable_routes.push_back(route.route_id);
            }
        }
        
        // Abandon unprofitable routes
        for (const auto& route_id : unprofitable_routes) {
            AbandonTradeRoute(route_id);
        }
        
        // Look for new profitable opportunities
        auto opportunities = FindProfitableRouteOpportunities(province_id, 3);
        for (const auto& opportunity : opportunities) {
            // Parse opportunity string to extract destination and resource
            // Simplified parsing - full implementation would be more robust
            LogTradeActivity("Trade opportunity identified: " + opportunity);
        }
    }

    void TradeSystem::OptimizeAllTradeRoutes() {
        auto entities = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<TradeRouteComponent>();
        for (auto entity_id : entities) {
            // Convert core::ecs::EntityID to game::types::EntityID
            OptimizeTradeRoutes(static_cast<game::types::EntityID>(entity_id.id));
        }
    }

    std::vector<std::string> TradeSystem::FindProfitableRouteOpportunities(types::EntityID province_id, int max_suggestions) {
        std::vector<std::string> opportunities;
        
        // Get all available resources at this province
        auto supply_data = GetProvinceSupplyData(province_id);
        
        // Find provinces with high demand for our surplus goods
        for (const auto& [resource, supply_level] : supply_data) {
            if (supply_level > 1.2) { // 20% surplus
                auto best_markets = FindBestMarkets(resource, true); // Find buyers
                
                for (auto market_id : best_markets) {
                    if (market_id != province_id && opportunities.size() < max_suggestions) {
                        double estimated_profit = EstimateRouteProfitability(province_id, market_id, resource);
                        if (estimated_profit > m_min_profitability_threshold) {
                            std::ostringstream oss;
                            oss << "Export " << static_cast<int>(resource) << " to province " 
                                << market_id << " (Est. profit: " << std::fixed << std::setprecision(1) 
                                << estimated_profit * 100 << "%)";
                            opportunities.push_back(oss.str());
                        }
                    }
                }
            }
        }
        
        return opportunities;
    }

    // ========================================================================
    // Market and Pricing System Implementation
    // ========================================================================

    double TradeSystem::CalculateMarketPrice(types::EntityID province_id, types::ResourceType resource) const {
        std::lock_guard<std::mutex> lock(m_market_mutex);
        
        std::string market_key = std::to_string(province_id) + "_" + std::to_string(static_cast<int>(resource));
        auto market_it = m_market_data.find(market_key);
        
        if (market_it != m_market_data.end()) {
            return market_it->second.current_price;
        }
        
        // Calculate base price if no market data exists
        auto trade_good = GetTradeGood(resource);
        double base_price = trade_good ? trade_good->base_value_per_unit : 1.0;
        
        // Apply supply/demand modifiers
        double supply = CalculateSupplyLevel(province_id, resource);
        double demand = CalculateDemandLevel(province_id, resource);
        
        double price_modifier = (demand / supply); // Basic supply/demand pricing
        price_modifier = std::clamp(price_modifier, 0.2, 5.0); // Reasonable bounds
        
        return base_price * price_modifier;
    }

    double TradeSystem::CalculateSupplyLevel(types::EntityID province_id, types::ResourceType resource) const {
        // Simplified supply calculation - would integrate with production systems
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        
        // Base supply varies by resource type and province characteristics
        double base_supply = 1.0;
        
        // Would check province production, population, infrastructure, etc.
        // For now, use random variation around 1.0
        double variation = rng.randomFloat(0.5f, 2.0f);
        
        return base_supply * variation;
    }

    double TradeSystem::CalculateDemandLevel(types::EntityID province_id, types::ResourceType resource) const {
        // Simplified demand calculation - would integrate with population systems
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        
        // Base demand varies by resource type and population characteristics
        double base_demand = 1.0;
        
        // Would check population size, wealth, cultural preferences, etc.
        // For now, use random variation around 1.0
        double variation = rng.randomFloat(0.6f, 1.8f);
        
        return base_demand * variation;
    }

    void TradeSystem::UpdateMarketPrices() {
        auto price_update_start = std::chrono::steady_clock::now();
        
        std::lock_guard<std::mutex> lock(m_market_mutex);
        
        // Update all market data
        for (auto& [market_key, market] : m_market_data) {
            UpdateSupplyDemandLevels(market.province_id, market.resource);
            
            // Apply market forces
            double supply_change = CalculateSupplyLevel(market.province_id, market.resource) - market.supply_level;
            double demand_change = CalculateDemandLevel(market.province_id, market.resource) - market.demand_level;
            
            ApplyMarketForces(market, supply_change, demand_change);
            ProcessPriceStabilization(market);
        }
        
        // Process any pending price shocks
        ProcessPriceShocks();
        
        auto price_update_end = std::chrono::steady_clock::now();
        m_performance_metrics.price_update_ms = 
            std::chrono::duration_cast<std::chrono::microseconds>(price_update_end - price_update_start).count() / 1000.0;
    }

    void TradeSystem::ApplyPriceShock(types::EntityID province_id, types::ResourceType resource, 
                                    double shock_magnitude, const std::string& cause) {
        std::lock_guard<std::mutex> lock(m_market_mutex);
        
        std::string market_key = std::to_string(province_id) + "_" + std::to_string(static_cast<int>(resource));
        auto market_it = m_market_data.find(market_key);
        
        if (market_it == m_market_data.end()) {
            // Create new market data if it doesn't exist
            MarketData new_market(province_id, resource);
            new_market.current_price = CalculateMarketPrice(province_id, resource);
            m_market_data[market_key] = new_market;
            market_it = m_market_data.find(market_key);
        }
        
        MarketData& market = market_it->second;
        double old_price = market.current_price;
        
        // Apply shock
        market.current_price *= (1.0 + shock_magnitude);
        market.current_price = std::max(0.1, market.current_price); // Minimum price
        
        // Update trend
        if (shock_magnitude > 0.5) {
            market.trend = PriceMovement::SHOCK_UP;
        } else if (shock_magnitude < -0.5) {
            market.trend = PriceMovement::SHOCK_DOWN;
        }
        
        market.volatility_index = std::min(1.0, market.volatility_index + std::abs(shock_magnitude));
        
        PublishPriceShock(province_id, resource, old_price, market.current_price, cause);
        LogTradeActivity("Price shock applied to " + GetResourceNameSafe(resource) + 
                        " in province " + std::to_string(province_id) + ": " + cause);
    }

    void TradeSystem::ProcessSeasonalAdjustments(int current_month) {
        std::lock_guard<std::mutex> lock(m_market_mutex);
        
        for (auto& [market_key, market] : m_market_data) {
            auto trade_good = GetTradeGood(market.resource);
            if (!trade_good) continue;
            
            // Apply seasonal demand modifier
            double seasonal_demand = trade_good->GetSeasonalDemandMultiplier(current_month);
            double seasonal_supply = trade_good->GetSeasonalSupplyMultiplier(current_month);
            
            // Adjust market levels
            market.demand_level *= seasonal_demand;
            market.supply_level *= seasonal_supply;
            
            // Recalculate price based on new supply/demand
            double price_adjustment = (seasonal_demand / seasonal_supply);
            market.current_price *= price_adjustment;
            market.current_price = std::clamp(market.current_price, 0.1, 50.0); // Reasonable bounds
        }
        
        LogTradeActivity("Applied seasonal adjustments for month " + std::to_string(current_month));
    }

    MarketData TradeSystem::GetMarketData(types::EntityID province_id, types::ResourceType resource) const {
        std::lock_guard<std::mutex> lock(m_market_mutex);
        
        std::string market_key = std::to_string(province_id) + "_" + std::to_string(static_cast<int>(resource));
        auto market_it = m_market_data.find(market_key);
        
        if (market_it != m_market_data.end()) {
            return market_it->second;
        }
        
        // Return default market data if not found
        MarketData default_market(province_id, resource);
        default_market.current_price = CalculateMarketPrice(province_id, resource);
        default_market.supply_level = CalculateSupplyLevel(province_id, resource);
        default_market.demand_level = CalculateDemandLevel(province_id, resource);
        
        return default_market;
    }

    std::vector<types::EntityID> TradeSystem::FindBestMarkets(types::ResourceType resource, bool buying) const {
        std::vector<std::pair<types::EntityID, double>> market_scores;
        
        // Collect all markets for this resource
        for (const auto& [market_key, market] : m_market_data) {
            if (market.resource == resource) {
                double score = buying ? (1.0 / market.current_price) : market.current_price; // Inverse for buying
                if (buying) {
                    score *= market.supply_level; // High supply good for buying
                } else {
                    score *= market.demand_level; // High demand good for selling
                }
                market_scores.emplace_back(market.province_id, score);
            }
        }
        
        // Sort by score (highest first)
        std::sort(market_scores.begin(), market_scores.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Extract province IDs
        std::vector<types::EntityID> best_markets;
        for (const auto& [province_id, score] : market_scores) {
            best_markets.push_back(province_id);
            if (best_markets.size() >= 10) break; // Return top 10
        }
        
        return best_markets;
    }

    double TradeSystem::GetRegionalAveragePrice(types::ResourceType resource, types::EntityID center_province, 
                                              double radius_km) const {
        std::lock_guard<std::mutex> lock(m_market_mutex);
        
        double total_price = 0.0;
        int market_count = 0;
        
        for (const auto& [market_key, market] : m_market_data) {
            if (market.resource == resource) {
                double distance = CalculateDistance(center_province, market.province_id);
                if (distance <= radius_km) {
                    total_price += market.current_price;
                    market_count++;
                }
            }
        }
        
        return (market_count > 0) ? (total_price / market_count) : 1.0;
    }

    // ========================================================================
    // Trade Hub Management Implementation
    // ========================================================================

    void TradeSystem::CreateTradeHub(types::EntityID province_id, const std::string& hub_name, HubType initial_type) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        // Check if hub already exists
        if (m_trade_hubs.find(province_id) != m_trade_hubs.end()) {
            LogTradeActivity("Trade hub already exists at province " + std::to_string(province_id));
            return;
        }
        
        // Create new trade hub
        TradeHub new_hub(province_id, hub_name);
        new_hub.hub_type = initial_type;
        new_hub.max_throughput_capacity = CalculateHubCapacity(province_id);
        new_hub.infrastructure_bonus = DetermineHubInfrastructureBonus(province_id);
        new_hub.security_rating = CalculateRouteSafety(province_id, province_id); // Self-assessment
        new_hub.reputation_rating = 1.0; // Start with neutral reputation
        
        // Set capacity based on hub type
        switch (initial_type) {
        case HubType::LOCAL_MARKET: new_hub.max_throughput_capacity *= 1.0; break;
        case HubType::REGIONAL_HUB: new_hub.max_throughput_capacity *= 2.0; break;
        case HubType::MAJOR_TRADING_CENTER: new_hub.max_throughput_capacity *= 5.0; break;
        case HubType::INTERNATIONAL_PORT: new_hub.max_throughput_capacity *= 10.0; break;
        case HubType::CROSSROADS: new_hub.max_throughput_capacity *= 3.0; break;
        }
        
        m_trade_hubs[province_id] = new_hub;
        
        // Create ECS component for the hub
        EnsureTradeComponentsExist(province_id);
        auto hub_comp = m_access_manager.GetComponentForWrite<TradeHubComponent>(province_id);
        if (hub_comp) {
            hub_comp->hub_data = new_hub;
        }
        
        LogTradeActivity("Created trade hub '" + hub_name + "' at province " + std::to_string(province_id));
    }

void TradeSystem::EvolveTradeHub(types::EntityID province_id) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        auto hub_it = m_trade_hubs.find(province_id);
        if (hub_it == m_trade_hubs.end()) {
            return;
        }
        
        TradeHub& hub = hub_it->second;
        HubType old_type = hub.hub_type;
        HubType optimal_type = DetermineOptimalHubType(province_id);
        
        if (optimal_type != old_type) {
            hub.hub_type = optimal_type;
            
            // Adjust capacity for new type
            double base_capacity = CalculateHubCapacity(province_id);
            switch (optimal_type) {
            case HubType::LOCAL_MARKET: hub.max_throughput_capacity = base_capacity * 1.0; break;
            case HubType::REGIONAL_HUB: hub.max_throughput_capacity = base_capacity * 2.0; break;
            case HubType::MAJOR_TRADING_CENTER: hub.max_throughput_capacity = base_capacity * 5.0; break;
            case HubType::INTERNATIONAL_PORT: hub.max_throughput_capacity = base_capacity * 10.0; break;
            case HubType::CROSSROADS: hub.max_throughput_capacity = base_capacity * 3.0; break;
            }
            
            // Update specializations
            UpdateHubSpecializations(hub);
            
            PublishHubEvolution(hub, old_type, "Economic growth and trade volume");
            LogTradeActivity("Hub at province " + std::to_string(province_id) + " evolved from " + 
                           std::to_string(static_cast<int>(old_type)) + " to " + 
                           std::to_string(static_cast<int>(optimal_type)));
        }
    }

    void TradeSystem::UpgradeTradeHub(types::EntityID province_id, int new_level) {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        auto hub_it = m_trade_hubs.find(province_id);
        if (hub_it == m_trade_hubs.end()) {
            return;
        }
        
        TradeHub& hub = hub_it->second;
        new_level = std::clamp(new_level, 1, 5);
        
        if (new_level > hub.upgrade_level) {
            hub.upgrade_level = new_level;
            
            // Improve hub capabilities
            hub.max_throughput_capacity *= (1.0 + (new_level - 1) * 0.25); // 25% per level
            hub.infrastructure_bonus = 1.0 + (new_level - 1) * 0.15; // 15% per level
            hub.security_rating = std::min(1.0, hub.security_rating + (new_level - 1) * 0.1);
            
            LogTradeActivity("Upgraded hub at province " + std::to_string(province_id) + 
                           " to level " + std::to_string(new_level));
        }
    }

    std::optional<TradeHub> TradeSystem::GetTradeHub(types::EntityID province_id) const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        auto hub_it = m_trade_hubs.find(province_id);
        if (hub_it != m_trade_hubs.end()) {
            return hub_it->second;
        }
        
        return std::nullopt;
    }

    std::vector<types::EntityID> TradeSystem::GetTradingPartners(types::EntityID province_id) const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        std::unordered_set<types::EntityID> partners;
        
        // Find all routes connected to this province
        for (const auto& [route_id, route] : m_active_routes) {
            if (route.source_province == province_id) {
                partners.insert(route.destination_province);
            } else if (route.destination_province == province_id) {
                partners.insert(route.source_province);
            }
        }
        
        return std::vector<types::EntityID>(partners.begin(), partners.end());
    }

    HubType TradeSystem::DetermineOptimalHubType(types::EntityID province_id) const {
        double total_volume = GetTotalTradeVolume(province_id);
        int route_count = GetRoutesFromProvince(province_id).size() + GetRoutesToProvince(province_id).size();

        // Determine optimal type based on volume and connectivity
        if (total_volume > 1000.0 && route_count > 20) {
            return HubType::INTERNATIONAL_PORT;
        } else if (total_volume > 500.0 && route_count > 10) {
            return HubType::MAJOR_TRADING_CENTER;
        } else if (route_count > 6) {
            return HubType::CROSSROADS;
        } else if (total_volume > 100.0 || route_count > 3) {
            return HubType::REGIONAL_HUB;
        } else {
            return HubType::LOCAL_MARKET;
        }
    }

    std::vector<TradeHub> TradeSystem::GetAllTradeHubs() const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);

        std::vector<TradeHub> hubs;
        hubs.reserve(m_trade_hubs.size());
        for (const auto& [province_id, hub] : m_trade_hubs) {
            hubs.push_back(hub);
        }

        return hubs;
    }

    // ========================================================================
    // Economic Analysis Implementation
    // ========================================================================

    double TradeSystem::GetTotalTradeVolume(types::EntityID province_id) const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        double total_volume = 0.0;
        
        for (const auto& [route_id, route] : m_active_routes) {
            if (route.source_province == province_id || route.destination_province == province_id) {
                total_volume += route.GetEffectiveVolume();
            }
        }
        
        return total_volume;
    }

    double TradeSystem::GetTradeVolumeForResource(types::EntityID province_id, types::ResourceType resource) const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        double resource_volume = 0.0;
        
        for (const auto& [route_id, route] : m_active_routes) {
            if (route.resource == resource && 
                (route.source_province == province_id || route.destination_province == province_id)) {
                resource_volume += route.GetEffectiveVolume();
            }
        }
        
        return resource_volume;
    }

    double TradeSystem::GetProvinceTradeIncome(types::EntityID province_id) const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        double total_income = 0.0;
        
        for (const auto& [route_id, route] : m_active_routes) {
            if (route.source_province == province_id && route.status == TradeStatus::ACTIVE) {
                // Income from selling goods
                double monthly_income = route.GetEffectiveVolume() * 
                                      (route.destination_price - route.source_price - route.transport_cost_per_unit);
                total_income += std::max(0.0, monthly_income);
            }
        }
        
        return total_income;
    }

    double TradeSystem::GetProvinceTradeExpenses(types::EntityID province_id) const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        double total_expenses = 0.0;
        
        for (const auto& [route_id, route] : m_active_routes) {
            if (route.destination_province == province_id && route.status == TradeStatus::ACTIVE) {
                // Expenses from buying goods
                double monthly_expense = route.GetEffectiveVolume() * 
                                       (route.source_price + route.transport_cost_per_unit);
                total_expenses += monthly_expense;
            }
            
            // Also count transport costs for routes originating here
            if (route.source_province == province_id && route.status == TradeStatus::ACTIVE) {
                total_expenses += route.GetEffectiveVolume() * route.transport_cost_per_unit;
            }
        }
        
        return total_expenses;
    }

    double TradeSystem::GetNetTradeBalance(types::EntityID province_id) const {
        return GetProvinceTradeIncome(province_id) - GetProvinceTradeExpenses(province_id);
    }

    double TradeSystem::CalculateRouteProfitability(const TradeRoute& route) const {
        if (route.destination_price <= route.source_price) {
            return 0.0; // No profit if destination price is not higher
        }

        double profit_per_unit = route.destination_price - route.source_price - route.transport_cost_per_unit;
        double profit_margin = profit_per_unit / route.source_price;

        // Adjust for risk factors
        profit_margin *= route.safety_rating * route.efficiency_rating;

        return std::max(0.0, profit_margin);
    }

    double TradeSystem::CalculateRouteProfitability(const std::string& route_id) const {
        auto it = m_active_routes.find(route_id);
        if (it == m_active_routes.end()) {
            return 0.0;
        }
        return CalculateRouteProfitability(it->second);
    }

    double TradeSystem::EstimateRouteProfitability(types::EntityID source, types::EntityID destination, 
                                                 types::ResourceType resource) const {
        double source_price = CalculateMarketPrice(source, resource);
        double dest_price = CalculateMarketPrice(destination, resource);
        double transport_cost = CalculateTransportCost(source, destination, resource);
        
        if (dest_price <= source_price) {
            return 0.0;
        }
        
        double profit_per_unit = dest_price - source_price - transport_cost;
        double profit_margin = profit_per_unit / source_price;
        
        // Adjust for estimated route factors
        double safety = CalculateRouteSafety(source, destination);
        double efficiency = CalculateRouteEfficiency(source, destination);
        
        profit_margin *= safety * efficiency;
        
        return std::max(0.0, profit_margin);
    }

    std::vector<std::string> TradeSystem::GetMostProfitableRoutes(int count) const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        std::vector<std::pair<std::string, double>> route_profits;
        
        for (const auto& [route_id, route] : m_active_routes) {
            if (route.status == TradeStatus::ACTIVE) {
                double profitability = CalculateRouteProfitability(route);
                route_profits.emplace_back(route_id, profitability);
            }
        }
        
        // Sort by profitability (highest first)
        std::sort(route_profits.begin(), route_profits.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::vector<std::string> most_profitable;
        for (int i = 0; i < count && i < route_profits.size(); ++i) {
            most_profitable.push_back(route_profits[i].first);
        }
        
        return most_profitable;
    }

    double TradeSystem::CalculateTransportCost(types::EntityID source, types::EntityID destination, 
                                             types::ResourceType resource) const {
        double distance = CalculateDistance(source, destination);
        
        auto trade_good = GetTradeGood(resource);
        double bulk_factor = trade_good ? trade_good->bulk_factor : 1.0;
        double perishability = trade_good ? trade_good->perishability : 0.0;
        
        // Base transport cost per km
        double base_cost_per_km = 0.01; // 1% of base value per 100km
        
        // Apply bulk factor (heavier goods cost more to transport)
        double transport_cost = distance * base_cost_per_km * bulk_factor;
        
        // Apply perishability penalty (spoilage risk)
        transport_cost *= (1.0 + perishability * distance / 1000.0);
        
        // Apply route efficiency modifier
        double efficiency = CalculateRouteEfficiency(source, destination);
        transport_cost /= efficiency;
        
        return transport_cost;
    }

    // ========================================================================
    // Geographic and Infrastructure Implementation
    // ========================================================================

    double TradeSystem::CalculateDistance(types::EntityID province1, types::EntityID province2) const {
        // Simplified distance calculation - would use actual coordinates in full implementation
        if (province1 == province2) return 0.0;
        
        // Generate consistent distance based on ID difference
        int id_diff = std::abs(static_cast<int>(province2) - static_cast<int>(province1));
        double base_distance = id_diff * 25.0; // 25km per ID unit difference
        
        // Add some variation to make it more realistic
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        double variation = rng.randomFloat(0.8f, 1.2f);
        
        return base_distance * variation;
    }

    double TradeSystem::CalculateRouteEfficiency(types::EntityID source, types::EntityID destination) const {
        // Simplified efficiency calculation - would check actual infrastructure
        double base_efficiency = 1.0;
        
        // Check for river connections (more efficient)
        if (HasRiverConnection(source, destination)) {
            base_efficiency *= 1.3;
        }
        
        // Check for road connections
        if (HasRoadConnection(source, destination)) {
            base_efficiency *= 1.2;
        }
        
        // Check for sea routes (very efficient for long distances)
        if (HasSeaRoute(source, destination)) {
            base_efficiency *= 1.5;
        }
        
        return std::min(2.0, base_efficiency); // Cap at 200% efficiency
    }

    double TradeSystem::CalculateRouteSafety(types::EntityID source, types::EntityID destination) const {
        // Simplified safety calculation - would integrate with military/political systems
        double base_safety = 0.9; // 90% base safety
        
        double distance = CalculateDistance(source, destination);
        
        // Longer routes are generally less safe
        double distance_penalty = distance / 2000.0; // Penalty starts after 2000km
        base_safety -= std::min(0.3, distance_penalty);
        
        // Random variation for different route conditions
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        double variation = rng.randomFloat(0.9f, 1.1f);
        
        return std::clamp(base_safety * variation, 0.1, 1.0);
    }

    bool TradeSystem::HasRiverConnection(types::EntityID province1, types::EntityID province2) const {
        // Simplified - would check actual geographic data
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        return rng.randomFloat(0.0f, 1.0f) < 0.3f; // 30% chance of river connection
    }

    bool TradeSystem::HasRoadConnection(types::EntityID province1, types::EntityID province2) const {
        // Simplified - would check actual infrastructure data
        double distance = CalculateDistance(province1, province2);
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        
        // Closer provinces more likely to have road connections
        double road_probability = std::max(0.1, 1.0 - distance / 1000.0);
        return rng.randomFloat(0.0f, 1.0f) < road_probability;
    }

    bool TradeSystem::HasSeaRoute(types::EntityID province1, types::EntityID province2) const {
        // Simplified - would check for coastal access and sea lanes
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        return rng.randomFloat(0.0f, 1.0f) < 0.2f; // 20% chance of sea route
    }

    RouteType TradeSystem::GetOptimalRouteType(types::EntityID source, types::EntityID destination) const {
        double distance = CalculateDistance(source, destination);
        
        // Determine optimal route type based on available connections and distance
        if (HasSeaRoute(source, destination) && distance > 500.0) {
            return RouteType::SEA; // Sea routes best for long distances
        } else if (HasRiverConnection(source, destination)) {
            return RouteType::RIVER; // Rivers efficient for medium distances
        } else if (HasRoadConnection(source, destination) && distance < 300.0) {
            return RouteType::LAND; // Roads good for short distances
        } else if (distance > 1000.0) {
            return RouteType::OVERLAND_LONG; // Long overland for distant places
        } else {
            return RouteType::LAND; // Default to land routes
        }
    }

    // ========================================================================
    // System Configuration and Save/Load
    // ========================================================================

    void TradeSystem::SaveState(Json::Value& state) const {
        std::lock_guard<std::mutex> trade_lock(m_trade_mutex);
        std::lock_guard<std::mutex> market_lock(m_market_mutex);
        
        // Save active routes
        Json::Value routes_array(Json::arrayValue);
        for (const auto& [route_id, route] : m_active_routes) {
            Json::Value route_data(Json::objectValue);
            route_data["id"] = std::string(route.route_id);
            route_data["source"] = static_cast<Json::Int>(route.source_province);
            route_data["destination"] = static_cast<Json::Int>(route.destination_province);
            route_data["resource"] = static_cast<Json::Int>(route.resource);
            route_data["type"] = static_cast<Json::Int>(route.route_type);
            route_data["status"] = static_cast<Json::Int>(route.status);
            route_data["base_volume"] = route.base_volume;
            route_data["current_volume"] = route.current_volume;
            route_data["profitability"] = route.profitability;
            route_data["distance"] = route.distance_km;
            route_data["safety"] = route.safety_rating;
            route_data["efficiency"] = route.efficiency_rating;
            route_data["established_year"] = static_cast<Json::Int>(route.established_year);
            
            routes_array.append(route_data);
        }
        state["active_routes"] = routes_array;
        
        // Save trade hubs
        Json::Value hubs_array(Json::arrayValue);
        for (const auto& [province_id, hub] : m_trade_hubs) {
            Json::Value hub_data(Json::objectValue);
            hub_data["province_id"] = static_cast<Json::Int>(province_id);
            hub_data["name"] = hub.hub_name;
            hub_data["type"] = static_cast<Json::Int>(hub.hub_type);
            hub_data["capacity"] = hub.max_throughput_capacity;
            hub_data["utilization"] = hub.current_utilization;
            hub_data["infrastructure_bonus"] = hub.infrastructure_bonus;
            hub_data["security_rating"] = hub.security_rating;
            hub_data["reputation"] = hub.reputation_rating;
            hub_data["upgrade_level"] = static_cast<Json::Int>(hub.upgrade_level);
            hub_data["establishment_year"] = static_cast<Json::Int>(hub.establishment_year);
            
            hubs_array.append(hub_data);
        }
        state["trade_hubs"] = hubs_array;
        
        // Save market data
        Json::Value markets_array(Json::arrayValue);
        for (const auto& [market_key, market] : m_market_data) {
            Json::Value market_data;
            market_data["province_id"] = static_cast<int>(market.province_id);
            market_data["resource"] = static_cast<int>(market.resource);
            market_data["current_price"] = market.current_price;
            market_data["supply_level"] = market.supply_level;
            market_data["demand_level"] = market.demand_level;
            market_data["trend"] = static_cast<int>(market.trend);
            market_data["volatility"] = market.volatility_index;
            
            markets_array.append(market_data);
        }
        state["market_data"] = markets_array;
        
        // Save configuration
        Json::Value config(Json::objectValue);
        config["max_trade_distance"] = m_max_trade_distance;
        config["min_profitability"] = m_min_profitability_threshold;
        config["update_frequency"] = static_cast<Json::Int>(m_update_frequency);
        config["logging_enabled"] = m_logging_enabled;
        state["config"] = config;
    }

    void TradeSystem::LoadState(const Json::Value& state) {
        std::lock_guard<std::mutex> trade_lock(m_trade_mutex);
        std::lock_guard<std::mutex> market_lock(m_market_mutex);
        
        // Clear existing data
        m_active_routes.clear();
        m_trade_hubs.clear();
        m_market_data.clear();
        
        // Load active routes
        if (state.isMember("active_routes")) {
            const Json::Value& routes_array = state["active_routes"];
            for (const auto& route_data : routes_array) {
                TradeRoute route;
                route.route_id = route_data["id"].asString();
                route.source_province = static_cast<types::EntityID>(route_data["source"].asInt());
                route.destination_province = static_cast<types::EntityID>(route_data["destination"].asInt());
                route.resource = static_cast<types::ResourceType>(route_data["resource"].asInt());
                route.route_type = static_cast<RouteType>(route_data["type"].asInt());
                route.status = static_cast<TradeStatus>(route_data["status"].asInt());
                route.base_volume = route_data["base_volume"].asDouble();
                route.current_volume = route_data["current_volume"].asDouble();
                route.profitability = route_data["profitability"].asDouble();
                route.distance_km = route_data["distance"].asDouble();
                route.safety_rating = route_data["safety"].asDouble();
                route.efficiency_rating = route_data["efficiency"].asDouble();
                route.established_year = route_data["established_year"].asInt();
                
                m_active_routes[route.route_id] = route;
            }
        }
        
        // Load trade hubs
        if (state.isMember("trade_hubs")) {
            const Json::Value& hubs_array = state["trade_hubs"];
            for (const auto& hub_data : hubs_array) {
                TradeHub hub;
                hub.province_id = static_cast<types::EntityID>(hub_data["province_id"].asInt());
                hub.hub_name = hub_data["name"].asString();
                hub.hub_type = static_cast<HubType>(hub_data["type"].asInt());
                hub.max_throughput_capacity = hub_data["capacity"].asDouble();
                hub.current_utilization = hub_data["utilization"].asDouble();
                hub.infrastructure_bonus = hub_data["infrastructure_bonus"].asDouble();
                hub.security_rating = hub_data["security_rating"].asDouble();
                hub.reputation_rating = hub_data["reputation"].asDouble();
                hub.upgrade_level = hub_data["upgrade_level"].asInt();
                hub.establishment_year = hub_data["establishment_year"].asInt();
                
                m_trade_hubs[hub.province_id] = hub;
            }
        }
        
        // Load market data
        if (state.isMember("market_data")) {
            const Json::Value& markets_array = state["market_data"];
            for (const auto& market_data : markets_array) {
                MarketData market;
                market.province_id = static_cast<types::EntityID>(market_data["province_id"].asInt());
                market.resource = static_cast<types::ResourceType>(market_data["resource"].asInt());
                market.current_price = market_data["current_price"].asDouble();
                market.supply_level = market_data["supply_level"].asDouble();
                market.demand_level = market_data["demand_level"].asDouble();
                market.trend = static_cast<PriceMovement>(market_data["trend"].asInt());
                market.volatility_index = market_data["volatility"].asDouble();
                
                std::string market_key = std::to_string(market.province_id) + "_" + 
                                       std::to_string(static_cast<int>(market.resource));
                m_market_data[market_key] = market;
            }
        }
        
        // Load configuration
        if (state.isMember("config")) {
            const Json::Value& config = state["config"];
            m_max_trade_distance = config["max_trade_distance"].asDouble();
            m_min_profitability_threshold = config["min_profitability"].asDouble();
            m_update_frequency = config["update_frequency"].asDouble();
            m_logging_enabled = config["logging_enabled"].asBool();
        }
        
        LogTradeActivity("Trade system state loaded successfully");
    }

    TradeSystem::PerformanceMetrics TradeSystem::GetPerformanceMetrics() const {
        return m_performance_metrics;
    }

    void TradeSystem::ResetPerformanceMetrics() {
        m_performance_metrics = PerformanceMetrics{};
    }

    void TradeSystem::SetProvinceSystem(game::province::EnhancedProvinceSystem* province_system) {
        m_province_system = province_system;
    }

    // ========================================================================
    // Internal Helper Methods Implementation
    // ========================================================================

    void TradeSystem::InitializeTradeGoods() {
        // Initialize historical trade goods with appropriate properties
        
        // Basic necessities
        TradeGoodProperties food(types::ResourceType::FOOD);
        food.base_value_per_unit = 0.5;
        food.bulk_factor = 1.5;
        food.perishability = 0.3;
        food.demand_elasticity = 0.5; // Inelastic demand
        m_trade_goods[types::ResourceType::FOOD] = food;
        
        // Raw materials (assuming WOOD represents general materials)
        TradeGoodProperties materials(types::ResourceType::WOOD);
        materials.base_value_per_unit = 1.0;
        materials.bulk_factor = 2.0;
        materials.perishability = 0.0;
        materials.demand_elasticity = 1.2;
        m_trade_goods[types::ResourceType::WOOD] = materials;
        
        // Metals (assuming IRON represents metals)
        TradeGoodProperties metals(types::ResourceType::IRON);
        metals.base_value_per_unit = 3.0;
        metals.bulk_factor = 3.0;
        metals.perishability = 0.0;
        metals.demand_elasticity = 1.5;
        m_trade_goods[types::ResourceType::IRON] = metals;
        
        // Luxury goods (assuming GOLD represents luxury)
        TradeGoodProperties luxury(types::ResourceType::GOLD);
        luxury.base_value_per_unit = 10.0;
        luxury.bulk_factor = 0.5;
        luxury.perishability = 0.0;
        luxury.luxury_factor = 1.0;
        luxury.demand_elasticity = 2.0; // Very elastic
        m_trade_goods[types::ResourceType::GOLD] = luxury;
        
        LogTradeActivity("Initialized " + std::to_string(m_trade_goods.size()) + " trade good types");
    }

    void TradeSystem::InitializeDefaultHubs() {
        // Create major medieval trading centers
        // These would be based on actual historical data
        
        CreateTradeHub(1001, "London", HubType::MAJOR_TRADING_CENTER);
        CreateTradeHub(1002, "Paris", HubType::MAJOR_TRADING_CENTER);
        CreateTradeHub(1003, "Venice", HubType::INTERNATIONAL_PORT);
        CreateTradeHub(1004, "Constantinople", HubType::INTERNATIONAL_PORT);
        CreateTradeHub(1005, "Cologne", HubType::REGIONAL_HUB);
        CreateTradeHub(1006, "Novgorod", HubType::REGIONAL_HUB);
        CreateTradeHub(1007, "Barcelona", HubType::REGIONAL_HUB);
        CreateTradeHub(1008, "Genoa", HubType::INTERNATIONAL_PORT);
        
        LogTradeActivity("Initialized " + std::to_string(m_trade_hubs.size()) + " default trade hubs");
    }

    void TradeSystem::LoadTradeConfiguration() {
        // In a full implementation, this would load from external config files
        // For now, set reasonable defaults
        
        m_max_trade_distance = 2000.0; // 2000km maximum trade distance
        m_min_profitability_threshold = 0.05; // 5% minimum profit
        m_update_frequency = 0.2; // 5 updates per second
        m_price_update_interval = 30.0f; // Update prices every 30 seconds
        m_max_routes_per_frame = 25; // Process up to 25 routes per frame
        
        LogTradeActivity("Trade system configuration loaded");
    }

    std::string TradeSystem::GenerateRouteId(types::EntityID source, types::EntityID destination, 
                                           types::ResourceType resource) const {
        std::ostringstream oss;
        oss << "route_" << source << "_" << destination << "_" << static_cast<int>(resource);
        return oss.str();
    }

    bool TradeSystem::IsRouteViable(const TradeRoute& route) const {
        return route.profitability >= m_min_profitability_threshold &&
               route.safety_rating > 0.2 &&
               route.distance_km <= m_max_trade_distance &&
               route.current_volume > 0.0;
    }

    void TradeSystem::EnsureTradeComponentsExist(types::EntityID province_id) {
        // Get entity manager for adding components
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;
        
        ::core::ecs::EntityID ecs_id{province_id};
        
        // Create trade components if they don't exist
        if (!entity_manager->HasComponent<TradeRouteComponent>(ecs_id)) {
            entity_manager->AddComponent<TradeRouteComponent>(ecs_id);
        }
        
        if (!entity_manager->HasComponent<TradeHubComponent>(ecs_id)) {
            entity_manager->AddComponent<TradeHubComponent>(ecs_id);
        }
        
        if (!entity_manager->HasComponent<TradeInventoryComponent>(ecs_id)) {
            entity_manager->AddComponent<TradeInventoryComponent>(ecs_id);
        }
    }

    void TradeSystem::LogTradeActivity(const std::string& message) const {
        if (m_logging_enabled) {
            std::cout << "[TradeSystem] " << message << std::endl;
        }
    }

    std::string TradeSystem::GetProvinceNameSafe(types::EntityID province_id) const {
        // Would integrate with province system to get actual names
        return "Province_" + std::to_string(province_id);
    }

    std::string TradeSystem::GetResourceNameSafe(types::ResourceType resource) const {
        // Would use type registry for actual resource names
        return "Resource_" + std::to_string(static_cast<int>(resource));
    }

    std::unordered_map<types::ResourceType, double> TradeSystem::GetProvinceSupplyData(types::EntityID province_id) const {
        std::unordered_map<types::ResourceType, double> supply_data;
        
        // Would integrate with production systems to get actual supply levels
        // For now, generate some mock data
        for (const auto& [resource_type, trade_good] : m_trade_goods) {
            supply_data[resource_type] = CalculateSupplyLevel(province_id, resource_type);
        }
        
        return supply_data;
    }

    double TradeSystem::CalculateHubCapacity(types::EntityID province_id) const {
        // Base capacity calculation - would integrate with population and infrastructure
        double base_capacity = 100.0;
        
        // Would check province population, infrastructure level, etc.
        // For now, use simple variation
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        double variation = rng.randomFloat(0.5f, 2.0f);
        
        return base_capacity * variation;
    }

    double TradeSystem::DetermineHubInfrastructureBonus(types::EntityID province_id) const {
        // Would check actual infrastructure systems
        // For now, return base bonus with some variation
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        return rng.randomFloat(0.8f, 1.5f);
    }

    void TradeSystem::ProcessTradeFlow(TradeRoute& route, float delta_time) {
        if (route.status != TradeStatus::ACTIVE) {
            return;
        }
        
        // Update route volume based on market conditions
        double supply = CalculateSupplyLevel(route.source_province, route.resource);
        double demand = CalculateDemandLevel(route.destination_province, route.resource);
        
        // Adjust volume towards optimal level
        double optimal_volume = std::min(supply, demand) * 0.1; // 10% of min(supply, demand)
        double volume_adjustment = (optimal_volume - route.current_volume) * 0.1 * delta_time;
        route.current_volume += volume_adjustment;
        route.current_volume = std::max(0.0, route.current_volume);
        
        // Update prices
        route.source_price = CalculateMarketPrice(route.source_province, route.resource);
        route.destination_price = CalculateMarketPrice(route.destination_province, route.resource);
        
        // Recalculate profitability
        route.profitability = CalculateRouteProfitability(route);
        
        // Update lifetime statistics
        route.total_goods_moved += route.GetEffectiveVolume() * delta_time / 3600.0; // Convert to hours
        route.lifetime_profit += route.GetEffectiveVolume() * 
                                (route.destination_price - route.source_price - route.transport_cost_per_unit) *
                                delta_time / 3600.0;
    }

    void TradeSystem::UpdateRouteConditions(TradeRoute& route) {
        // Update safety rating based on current conditions
        route.safety_rating = CalculateRouteSafety(route.source_province, route.destination_province);
        
        // Update efficiency rating
        route.efficiency_rating = CalculateRouteEfficiency(route.source_province, route.destination_province);
        
        // Update seasonal modifier (simplified)
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        route.seasonal_modifier = rng.randomFloat(0.8f, 1.2f);
        
        // Check if route should be disrupted due to low safety
        if (route.safety_rating < 0.3 && rng.randomFloat(0.0f, 1.0f) < 0.01) { // 1% chance per update
            DisruptTradeRoute(route.route_id, "Bandit activity", 3.0);
        }
    }

    void TradeSystem::UpdateHubUtilization(TradeHub& hub) {
        double total_volume = 0.0;
        
        // Calculate total volume passing through this hub
        for (const auto& route_id : hub.incoming_route_ids) {
            auto route_it = m_active_routes.find(route_id);
            if (route_it != m_active_routes.end()) {
                total_volume += route_it->second.GetEffectiveVolume();
            }
        }
        
        for (const auto& route_id : hub.outgoing_route_ids) {
            auto route_it = m_active_routes.find(route_id);
            if (route_it != m_active_routes.end()) {
                total_volume += route_it->second.GetEffectiveVolume();
            }
        }
        
        hub.current_utilization = total_volume / hub.GetEffectiveCapacity();
        hub.current_utilization = std::clamp(hub.current_utilization, 0.0, 1.5); // Allow overutilization
    }

    void TradeSystem::UpdateHubSpecializations(TradeHub& hub) {
        // Analyze trade patterns to determine specializations
        std::unordered_map<types::ResourceType, double> resource_volumes;
        
        for (const auto& route_id : hub.incoming_route_ids) {
            auto route_it = m_active_routes.find(route_id);
            if (route_it != m_active_routes.end()) {
                resource_volumes[route_it->second.resource] += route_it->second.GetEffectiveVolume();
            }
        }
        
        // Clear old specializations
        hub.specialized_goods.clear();
        
        // Add specializations for high-volume resources
        for (const auto& [resource, volume] : resource_volumes) {
            if (volume > hub.GetEffectiveCapacity() * 0.2) { // 20% of capacity threshold
                hub.specialized_goods.insert(resource);
                hub.handling_efficiency[resource] = 1.3; // 30% efficiency bonus
            }
        }
    }

    void TradeSystem::CalculateHubReputation(TradeHub& hub) {
        // Reputation based on successful trade volume, safety, and efficiency
        double trade_volume_factor = std::min(2.0, hub.current_utilization);
        double safety_factor = hub.security_rating;
        double efficiency_factor = hub.infrastructure_bonus;
        
        hub.reputation_rating = (trade_volume_factor + safety_factor + efficiency_factor) / 3.0;
        hub.reputation_rating = std::clamp(hub.reputation_rating, 0.5, 2.0);
    }

    void TradeSystem::UpdateSupplyDemandLevels(types::EntityID province_id, types::ResourceType resource) {
        // Update supply and demand levels based on current game state
        // This would integrate with production, population, and consumption systems
        
        std::string market_key = std::to_string(province_id) + "_" + std::to_string(static_cast<int>(resource));
        auto market_it = m_market_data.find(market_key);
        
        if (market_it != m_market_data.end()) {
            MarketData& market = market_it->second;
            
            // Gradual adjustment towards equilibrium
            double new_supply = CalculateSupplyLevel(province_id, resource);
            double new_demand = CalculateDemandLevel(province_id, resource);
            
            market.supply_level += (new_supply - market.supply_level) * 0.1; // 10% adjustment per update
            market.demand_level += (new_demand - market.demand_level) * 0.1;
        }
    }

    void TradeSystem::ApplyMarketForces(MarketData& market_data, double supply_change, double demand_change) {
        // Apply supply and demand changes to price
        double price_elasticity = 0.5; // How responsive price is to supply/demand changes
        
        double supply_effect = -supply_change * price_elasticity; // More supply = lower price
        double demand_effect = demand_change * price_elasticity;   // More demand = higher price
        
        double total_price_change = supply_effect + demand_effect;
        market_data.current_price *= (1.0 + total_price_change);
        market_data.current_price = std::clamp(market_data.current_price, 0.1, 100.0);
        
        // Update price change rate
        market_data.price_change_rate = total_price_change;
        
        // Update trend
        if (total_price_change > 0.1) {
            market_data.trend = PriceMovement::RISING;
        } else if (total_price_change < -0.1) {
            market_data.trend = PriceMovement::FALLING;
        } else {
            market_data.trend = PriceMovement::STABLE;
        }
    }

    void TradeSystem::ProcessPriceStabilization(MarketData& market_data) {
        // Gradually return prices to historical averages
        double stabilization_factor = 0.05; // 5% movement towards average per update
        
        if (market_data.avg_price_12_months > 0.0) {
            double price_deviation = market_data.current_price - market_data.avg_price_12_months;
            market_data.current_price -= price_deviation * stabilization_factor;
        }
        
        // Reduce volatility over time
        market_data.volatility_index *= 0.99; // 1% reduction per update
        market_data.volatility_index = std::max(0.01, market_data.volatility_index);
    }

    void TradeSystem::ProcessPriceShocks() {
        // Process any systemic price shocks
        // This would be triggered by major events, disasters, wars, etc.
        
        // For now, randomly apply minor shocks to simulate market volatility
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();
        
        if (rng.randomFloat(0.0f, 1.0f) < 0.001) { // 0.1% chance per update
            // Random price shock
            auto market_it = m_market_data.begin();
            std::advance(market_it, rng.randomInt(0, static_cast<int>(m_market_data.size()) - 1));
            
            double shock_magnitude = rng.randomFloat(-0.3f, 0.3f); // ±30% price shock
            ApplyPriceShock(market_it->second.province_id, market_it->second.resource, 
                          shock_magnitude, "Market volatility");
        }
    }

    void TradeSystem::UpdatePerformanceMetrics() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_last_performance_check);
        
        // Update metrics every 30 seconds
        if (elapsed.count() >= 30) {
            m_performance_metrics.active_routes_count = static_cast<int>(m_active_routes.size());
            m_performance_metrics.active_hubs_count = static_cast<int>(m_trade_hubs.size());
            
            // Performance warning if processing takes too long
            m_performance_metrics.performance_warning = m_performance_metrics.total_update_ms > 16.0;
            
            if (m_performance_metrics.performance_warning) {
                LogTradeActivity("Performance warning: Trade processing taking " + 
                               std::to_string(m_performance_metrics.total_update_ms) + "ms");
            }
            
            m_last_performance_check = now;
        }
    }

    void TradeSystem::OptimizeRouteStorage() {
        // Clean up abandoned routes
        auto route_it = m_active_routes.begin();
        while (route_it != m_active_routes.end()) {
            if (route_it->second.status == TradeStatus::ABANDONED) {
                route_it = m_active_routes.erase(route_it);
            } else {
                ++route_it;
            }
        }
    }

    void TradeSystem::CleanupAbandonedRoutes() {
        // Remove routes that have been unprofitable for too long
        std::vector<std::string> routes_to_abandon;
        
        for (const auto& [route_id, route] : m_active_routes) {
            if (route.status == TradeStatus::ACTIVE && route.profitability < 0.0) {
                // Mark routes with negative profitability for abandonment
                routes_to_abandon.push_back(route_id);
            }
        }
        
        for (const auto& route_id : routes_to_abandon) {
            AbandonTradeRoute(route_id);
        }
        
        if (!routes_to_abandon.empty()) {
            LogTradeActivity("Cleaned up " + std::to_string(routes_to_abandon.size()) + " unprofitable routes");
        }
    }

    void TradeSystem::PublishTradeRouteEstablished(const TradeRoute& route, const std::string& reason) {
        messages::TradeRouteEstablished event;
        event.route_id = route.route_id;
        event.source_province = route.source_province;
        event.destination_province = route.destination_province;
        event.resource = route.resource;
        event.expected_monthly_profit = route.GetEffectiveVolume() * route.profitability;
        event.route_type = route.route_type;
        event.establishment_reason = reason;
        
        m_message_bus.Publish(event);
    }

    void TradeSystem::PublishTradeRouteDisrupted(const TradeRoute& route, const std::string& cause, double duration) {
        messages::TradeRouteDisrupted event;
        event.route_id = route.route_id;
        event.source_province = route.source_province;
        event.destination_province = route.destination_province;
        event.resource = route.resource;
        event.disruption_cause = cause;
        event.estimated_duration_months = duration;
        event.economic_impact = route.GetEffectiveVolume() * route.profitability;
        
        m_message_bus.Publish(event);
    }

    void TradeSystem::PublishHubEvolution(const TradeHub& hub, HubType old_type, const std::string& trigger) {
        messages::TradeHubEvolved event;
        event.province_id = hub.province_id;
        event.old_type = old_type;
        event.new_type = hub.hub_type;
        event.new_capacity = hub.max_throughput_capacity;
        event.new_specializations = std::vector<types::ResourceType>(hub.specialized_goods.begin(), 
                                                                    hub.specialized_goods.end());
        event.evolution_trigger = trigger;
        
        m_message_bus.Publish(event);
    }

    void TradeSystem::PublishPriceShock(types::EntityID province_id, types::ResourceType resource, 
                                      double old_price, double new_price, const std::string& cause) {
        messages::PriceShockOccurred event;
        event.province_id = province_id;
        event.resource = resource;
        event.old_price = old_price;
        event.new_price = new_price;
        event.shock_type = (new_price > old_price) ? PriceMovement::SHOCK_UP : PriceMovement::SHOCK_DOWN;
        event.shock_cause = cause;
        event.expected_duration_months = 3.0; // Default duration
        
        m_message_bus.Publish(event);
    }

    // ========================================================================
    // Query Interface Implementation
    // ========================================================================

    std::vector<TradeRoute> TradeSystem::GetRoutesFromProvince(types::EntityID province_id) const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        std::vector<TradeRoute> routes;
        for (const auto& [route_id, route] : m_active_routes) {
            if (route.source_province == province_id) {
                routes.push_back(route);
            }
        }
        
        return routes;
    }

    std::vector<TradeRoute> TradeSystem::GetRoutesToProvince(types::EntityID province_id) const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        std::vector<TradeRoute> routes;
        for (const auto& [route_id, route] : m_active_routes) {
            if (route.destination_province == province_id) {
                routes.push_back(route);
            }
        }
        
        return routes;
    }

    std::vector<TradeRoute> TradeSystem::GetRoutesForResource(types::ResourceType resource) const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);
        
        std::vector<TradeRoute> routes;
        for (const auto& [route_id, route] : m_active_routes) {
            if (route.resource == resource) {
                routes.push_back(route);
            }
        }
        
        return routes;
    }

    std::optional<TradeRoute> TradeSystem::GetRoute(const std::string& route_id) const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);

        auto route_it = m_active_routes.find(route_id);
        if (route_it != m_active_routes.end()) {
            return route_it->second;
        }

        return std::nullopt;
    }

    std::vector<TradeRoute> TradeSystem::GetAllTradeRoutes() const {
        std::lock_guard<std::mutex> lock(m_trade_mutex);

        std::vector<TradeRoute> routes;
        routes.reserve(m_active_routes.size());
        for (const auto& [route_id, route] : m_active_routes) {
            routes.push_back(route);
        }

        return routes;
    }

    const TradeGoodProperties* TradeSystem::GetTradeGood(types::ResourceType resource) const {
        auto it = m_trade_goods.find(resource);
        return (it != m_trade_goods.end()) ? &it->second : nullptr;
    }

    // ========================================================================
    // Configuration Methods
    // ========================================================================

    void TradeSystem::SetUpdateFrequency(double updates_per_second) {
        m_update_frequency = std::clamp(updates_per_second, 0.1, 10.0);
    }

    void TradeSystem::SetMaxTradeDistance(double max_distance_km) {
        m_max_trade_distance = std::max(100.0, max_distance_km);
        m_pathfinder->SetMaxSearchDistance(m_max_trade_distance);
    }

    void TradeSystem::SetMinProfitabilityThreshold(double min_profit_margin) {
        m_min_profitability_threshold = std::clamp(min_profit_margin, 0.0, 0.5);
    }

    void TradeSystem::EnableTradeLogging(bool enable) {
        m_logging_enabled = enable;
    }

} // namespace game::trade
