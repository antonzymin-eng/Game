// ============================================================================
// Mechanica Imperii - Trade System Header
// Created: September 22, 2025, 11:30 AM
// Location: include/game/trade/TradeSystem.h
// ============================================================================

#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <atomic>
#include <mutex>
#include <optional>
#include <queue>

// Core system includes
#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/threading/ThreadedSystemManager.h"
#include "core/types/game_types.h"
#include "utils/RandomGenerator.h"

// Forward declare TradeRepository to avoid circular dependency
namespace game::trade {
    class TradeRepository;
}

// Forward declarations
namespace Json {
    class Value;
}

namespace game::province {
    class EnhancedProvinceSystem;
}

namespace game::trade {

    // ========================================================================
    // Trade System Types and Enums
    // ========================================================================

    enum class RouteType {
        LAND = 0,
        RIVER = 1,
        COASTAL = 2,
        SEA = 3,
        OVERLAND_LONG = 4
    };

    enum class TradeStatus {
        ACTIVE = 0,
        DISRUPTED = 1,
        SEASONAL_CLOSED = 2,
        ABANDONED = 3,
        ESTABLISHING = 4
    };

    enum class HubType {
        LOCAL_MARKET = 0,
        REGIONAL_HUB = 1,
        MAJOR_TRADING_CENTER = 2,
        INTERNATIONAL_PORT = 3,
        CROSSROADS = 4
    };

    enum class PriceMovement {
        STABLE = 0,
        RISING = 1,
        FALLING = 2,
        VOLATILE = 3,
        SHOCK_UP = 4,
        SHOCK_DOWN = 5
    };

    // ========================================================================
    // Trade Route Data Structures
    // ========================================================================

    struct TradeRoute {
        std::string route_id;
        types::EntityID source_province;
        types::EntityID destination_province;
        types::ResourceType resource;
        RouteType route_type = RouteType::LAND;
        TradeStatus status = TradeStatus::ESTABLISHING;

        // Economic data
        double base_volume = 0.0;          // Monthly trade volume (units)
        double current_volume = 0.0;       // Adjusted for current conditions
        double profitability = 0.0;       // Profit margin (0.0-1.0)
        double transport_cost_per_unit = 0.0;
        double source_price = 0.0;
        double destination_price = 0.0;

        // Route characteristics
        double distance_km = 0.0;
        double safety_rating = 1.0;        // 0.0-1.0, 1.0 = completely safe
        double efficiency_rating = 1.0;    // 0.0-2.0, infrastructure quality
        double seasonal_modifier = 1.0;    // Current seasonal adjustment
        
        // Route features
        bool uses_rivers = false;
        bool uses_roads = false;
        bool uses_sea_route = false;
        bool passes_hostile_territory = false;
        bool requires_special_permits = false;

        // Historical tracking
        int established_year = 1066;
        double total_goods_moved = 0.0;
        double lifetime_profit = 0.0;
        int disruption_count = 0;

        // Recovery tracking (for DISRUPTED status)
        bool is_recovering = false;
        double recovery_progress = 0.0;      // 0.0-1.0, progress toward full recovery
        double recovery_months_remaining = 0.0;  // Countdown to full recovery
        double pre_disruption_volume = 0.0;  // Volume before disruption
        double pre_disruption_safety = 1.0;  // Safety before disruption

        TradeRoute() = default;
        TradeRoute(const std::string& id, types::EntityID src, types::EntityID dst, types::ResourceType res);

        // Utility methods
        bool IsViable() const;
        double GetEffectiveVolume() const;
        std::string GetRouteDescription() const;
    };

    struct TradeHub {
        types::EntityID province_id;
        std::string hub_name;
        HubType hub_type = HubType::LOCAL_MARKET;

        // Economic capacity
        double max_throughput_capacity = 100.0;  // Monthly volume capacity
        double current_utilization = 0.0;        // 0.0-1.0 utilization rate
        double infrastructure_bonus = 1.0;       // Infrastructure multiplier
        double security_rating = 1.0;            // Merchant security

        // Specialization and expertise
        std::unordered_set<types::ResourceType> specialized_goods;
        std::unordered_map<types::ResourceType, double> handling_efficiency;
        std::unordered_map<types::ResourceType, double> price_influence;

        // Connected trade network
        std::vector<std::string> incoming_route_ids;
        std::vector<std::string> outgoing_route_ids;
        std::unordered_set<types::EntityID> trading_partners;

        // Historical development
        int establishment_year = 1066;
        double reputation_rating = 1.0;          // 0.5-2.0, affects trade volume
        int upgrade_level = 1;                   // 1-5, determines capabilities
        
        TradeHub() = default;
        explicit TradeHub(types::EntityID province, const std::string& name);

        // Utility methods
        bool CanHandleVolume(double additional_volume) const;
        double GetEffectiveCapacity() const;
        void AddRoute(const std::string& route_id, bool is_incoming);
        void RemoveRoute(const std::string& route_id);
    };

    struct TradeGoodProperties {
        types::ResourceType resource_type;

        // Basic properties
        double base_value_per_unit = 1.0;
        double bulk_factor = 1.0;             // Transport difficulty multiplier
        double perishability = 0.0;           // Spoilage rate over distance/time
        double luxury_factor = 0.0;           // 0.0-1.0, affects demand patterns

        // Market dynamics
        double demand_elasticity = 1.0;       // Price sensitivity of demand
        double supply_elasticity = 1.0;       // Price sensitivity of supply
        double volatility = 0.1;              // Price volatility factor
        
        // Seasonal patterns
        std::unordered_map<int, double> seasonal_demand; // Month -> multiplier
        std::unordered_map<int, double> seasonal_supply;

        // Historical context
        bool available_in_period = true;
        int introduction_year = 1000;
        int obsolescence_year = 9999;

        TradeGoodProperties() = default;
        explicit TradeGoodProperties(types::ResourceType type);

        // Utility methods
        double GetSeasonalDemandMultiplier(int month) const;
        double GetSeasonalSupplyMultiplier(int month) const;
        bool IsAvailable(int year) const;
    };

    // ========================================================================
    // Market Data and Pricing
    // ========================================================================

    struct MarketData {
        types::EntityID province_id;
        types::ResourceType resource;

        // Current market state
        double current_price = 1.0;
        double base_price = 1.0;
        double supply_level = 1.0;            // 0.0-2.0+, 1.0 = balanced
        double demand_level = 1.0;
        
        // Price movement tracking
        PriceMovement trend = PriceMovement::STABLE;
        double price_change_rate = 0.0;       // Per month
        double volatility_index = 0.1;
        
        // Historical data (simplified)
        double avg_price_12_months = 1.0;
        double max_price_12_months = 1.0;
        double min_price_12_months = 1.0;

        MarketData() = default;
        MarketData(types::EntityID province, types::ResourceType resource);

        // Price analysis
        bool IsPriceAboveAverage() const;
        bool IsExperiencingShock() const;
        double GetPriceDeviation() const;
    };

    // ========================================================================
    // ECS Components
    // ========================================================================

    struct TradeRouteComponent : public game::core::Component<TradeRouteComponent> {
        // Store only IDs - canonical route data is in TradeSystem::m_active_routes
        std::vector<std::string> active_route_ids;
        std::unordered_set<std::string> route_id_set;  // For fast lookups
        
        // Cached aggregates (updated when routes change)
        double total_monthly_volume = 0.0;
        double total_monthly_profit = 0.0;
    };

    struct TradeHubComponent : public game::core::Component<TradeHubComponent> {
        TradeHub hub_data;
        std::unordered_map<types::ResourceType, MarketData> market_data;
        double monthly_throughput = 0.0;
        int merchant_count = 0;
    };

    struct TradeInventoryComponent : public game::core::Component<TradeInventoryComponent> {
        std::unordered_map<types::ResourceType, double> stored_goods;
        std::unordered_map<types::ResourceType, double> reserved_goods;
        std::unordered_map<types::ResourceType, double> in_transit_goods;
        double total_storage_capacity = 1000.0;
        double current_utilization = 0.0;
    };

    // ========================================================================
    // Trade Events (Message Bus Integration)
    // ========================================================================

    namespace messages {
        struct TradeRouteEstablished {
            std::string route_id;
            types::EntityID source_province;
            types::EntityID destination_province;
            types::ResourceType resource;
            double expected_monthly_profit;
            RouteType route_type;
            std::string establishment_reason;
        };

        struct TradeRouteDisrupted {
            std::string route_id;
            types::EntityID source_province;
            types::EntityID destination_province;
            types::ResourceType resource;
            std::string disruption_cause;
            double estimated_duration_months;
            
            // Impact metrics (clear semantics)
            double monthly_profit_delta;      // Change in monthly profit (negative = loss)
            double total_impact_over_duration; // Total economic impact over full duration
            double volume_before;              // Volume before disruption
            double volume_after;               // Volume after disruption
        };

        struct TradeRouteRecovered {
            std::string route_id;
            types::EntityID source_province;
            types::EntityID destination_province;
            types::ResourceType resource;
            double recovery_time_months;
            double restored_volume;
        };

        struct TradeHubEvolved {
            types::EntityID province_id;
            HubType old_type;
            HubType new_type;
            double new_capacity;
            std::vector<types::ResourceType> new_specializations;
            std::string evolution_trigger;
        };

        struct PriceShockOccurred {
            types::EntityID province_id;
            types::ResourceType resource;
            double old_price;
            double new_price;
            PriceMovement shock_type;
            std::string shock_cause;
            double expected_duration_months;
        };

        struct TradeVolumeChanged {
            types::EntityID province_id;
            types::ResourceType resource;
            double old_volume;
            double new_volume;
            double volume_change_percent;
            std::string change_reason;
        };

        struct MarketConditionsChanged {
            types::EntityID province_id;
            std::unordered_map<types::ResourceType, double> price_changes;
            std::unordered_map<types::ResourceType, double> supply_changes;
            std::unordered_map<types::ResourceType, double> demand_changes;
            std::string change_cause;
        };
    }

    // ========================================================================
    // Trade Route Pathfinding and Analysis
    // ========================================================================

    class TradePathfinder {
    public:
        struct PathNode {
            types::EntityID province_id;
            double cost_to_reach = 0.0;
            double estimated_total_cost = 0.0;
            types::EntityID parent_province;
            RouteType connection_type = RouteType::LAND;
        };

        struct RoutePath {
            std::vector<types::EntityID> waypoints;
            std::vector<RouteType> connection_types;
            double total_distance = 0.0;
            double total_cost = 0.0;
            double estimated_travel_time_days = 0.0;
            double safety_rating = 1.0;
        };

        TradePathfinder();
        ~TradePathfinder() = default;

        // Pathfinding methods
        std::optional<RoutePath> FindOptimalRoute(types::EntityID source, types::EntityID destination,
                                                 types::ResourceType resource);
        std::vector<RoutePath> FindAlternativeRoutes(types::EntityID source, types::EntityID destination,
                                                    types::ResourceType resource, int max_alternatives = 3);

        // Route analysis
        double CalculateRouteCost(const RoutePath& path, types::ResourceType resource);
        double CalculateRouteSafety(const RoutePath& path);
        double CalculateRouteEfficiency(const RoutePath& path);
        
        // Network analysis
        void UpdateNetworkConnectivity();
        bool IsRouteViable(types::EntityID source, types::EntityID destination, double max_distance = 1000.0);

        // Configuration
        void SetMaxSearchDistance(double max_km);
        void SetCostWeights(double distance_weight, double safety_weight, double efficiency_weight);

    private:
        double m_max_search_distance = 2000.0;
        double m_distance_weight = 1.0;
        double m_safety_weight = 0.3;
        double m_efficiency_weight = 0.2;
        
        // Internal pathfinding methods
        std::vector<types::EntityID> GetNeighboringProvinces(types::EntityID province_id);
        double GetConnectionCost(types::EntityID from, types::EntityID to, RouteType connection_type);
        RouteType DetermineConnectionType(types::EntityID from, types::EntityID to);
        double HeuristicDistance(types::EntityID from, types::EntityID to);
    };

    // ========================================================================
    // Main Trade System Class
    // ========================================================================

    class TradeSystem {
    public:
        struct PerformanceMetrics {
            double route_calculation_ms = 0.0;
            double price_update_ms = 0.0;
            double hub_processing_ms = 0.0;
            double total_update_ms = 0.0;
            int active_routes_count = 0;
            int active_hubs_count = 0;
            bool performance_warning = false;
        };

        explicit TradeSystem(::core::ecs::ComponentAccessManager& access_manager,
                           ::core::threading::ThreadSafeMessageBus& message_bus);
        ~TradeSystem();

        // System lifecycle (ThreadedSystem interface)
        void Initialize();
        void Update(float deltaTime);
        void Shutdown();

        // Threading integration
        ::core::threading::ThreadingStrategy GetThreadingStrategy() const;
        std::string GetThreadingRationale() const;

        // ====================================================================
        // Trade Route Management
        // ====================================================================

        // Route creation and management
        std::string EstablishTradeRoute(types::EntityID source, types::EntityID destination,
                                       types::ResourceType resource, RouteType preferred_type = RouteType::LAND);
        bool DisruptTradeRoute(const std::string& route_id, const std::string& cause, 
                              double duration_months = 3.0);
        bool RestoreTradeRoute(const std::string& route_id);
        void AbandonTradeRoute(const std::string& route_id);

        // Route optimization
        void OptimizeTradeRoutes(types::EntityID province_id);
        void OptimizeAllTradeRoutes();
        std::vector<std::string> FindProfitableRouteOpportunities(types::EntityID province_id, int max_suggestions = 5);

        // Route queries
        std::vector<TradeRoute> GetRoutesFromProvince(types::EntityID province_id) const;
        std::vector<TradeRoute> GetRoutesToProvince(types::EntityID province_id) const;
        std::vector<TradeRoute> GetRoutesForResource(types::ResourceType resource) const;
        std::optional<TradeRoute> GetRoute(const std::string& route_id) const;
        std::vector<TradeRoute> GetAllTradeRoutes() const;

        // ====================================================================
        // Trade Hub Management
        // ====================================================================

        // Hub creation and evolution
        void CreateTradeHub(types::EntityID province_id, const std::string& hub_name, HubType initial_type);
        void EvolveTradeHub(types::EntityID province_id);
        void UpgradeTradeHub(types::EntityID province_id, int new_level);

        // Hub queries
        std::optional<TradeHub> GetTradeHub(types::EntityID province_id) const;
        std::vector<types::EntityID> GetTradingPartners(types::EntityID province_id) const;
        HubType DetermineOptimalHubType(types::EntityID province_id) const;
        std::vector<TradeHub> GetAllTradeHubs() const;

        // ====================================================================
        // Market and Pricing System
        // ====================================================================

        // Price calculations
        double CalculateMarketPrice(types::EntityID province_id, types::ResourceType resource) const;
        double CalculateSupplyLevel(types::EntityID province_id, types::ResourceType resource) const;
        double CalculateDemandLevel(types::EntityID province_id, types::ResourceType resource) const;
        
        // Market dynamics
        void UpdateMarketPrices();
        void ApplyPriceShock(types::EntityID province_id, types::ResourceType resource, 
                           double shock_magnitude, const std::string& cause);
        void ProcessSeasonalAdjustments(int current_month);

        // Market queries
        MarketData GetMarketData(types::EntityID province_id, types::ResourceType resource) const;
        std::vector<types::EntityID> FindBestMarkets(types::ResourceType resource, bool buying = false) const;
        double GetRegionalAveragePrice(types::ResourceType resource, types::EntityID center_province, 
                                     double radius_km = 500.0) const;

        // ====================================================================
        // Time Management Integration
        // ====================================================================
        
        /**
         * @brief Set the current game year (to be called by TimeManagementSystem)
         * @param year The current game year
         */
        void SetCurrentGameYear(int year);
        
        /**
         * @brief Get the current game year
         * @return Current year for establishing routes
         */
        int GetCurrentGameYear() const;

        // ====================================================================
        // Economic Analysis
        // ====================================================================

        // Trade statistics
        double GetTotalTradeVolume(types::EntityID province_id) const;
        double GetTradeVolumeForResource(types::EntityID province_id, types::ResourceType resource) const;
        double GetProvinceTradeIncome(types::EntityID province_id) const;
        double GetProvinceTradeExpenses(types::EntityID province_id) const;
        double GetNetTradeBalance(types::EntityID province_id) const;

        // Route profitability
        double CalculateRouteProfitability(const TradeRoute& route) const;
        double CalculateRouteProfitability(const std::string& route_id) const;
        double EstimateRouteProfitability(types::EntityID source, types::EntityID destination,
                                        types::ResourceType resource) const;
        std::vector<std::string> GetMostProfitableRoutes(int count = 10) const;
        double CalculateTransportCost(types::EntityID source, types::EntityID destination,
                                     types::ResourceType resource) const;

        // ====================================================================
        // Geographic and Infrastructure
        // ====================================================================

        // Distance and connectivity
        double CalculateDistance(types::EntityID province1, types::EntityID province2) const;
        double CalculateRouteEfficiency(types::EntityID source, types::EntityID destination) const;
        double CalculateRouteSafety(types::EntityID source, types::EntityID destination) const;
        
        // Infrastructure queries
        bool HasRiverConnection(types::EntityID province1, types::EntityID province2) const;
        bool HasRoadConnection(types::EntityID province1, types::EntityID province2) const;
        bool HasSeaRoute(types::EntityID province1, types::EntityID province2) const;
        RouteType GetOptimalRouteType(types::EntityID source, types::EntityID destination) const;

        // ====================================================================
        // System Configuration and Integration
        // ====================================================================

        // System configuration
        void SetUpdateFrequency(double updates_per_second);
        void SetMaxTradeDistance(double max_distance_km);
        void SetMinProfitabilityThreshold(double min_profit_margin);
        void EnableTradeLogging(bool enable);

        // Performance monitoring
        PerformanceMetrics GetPerformanceMetrics() const;
        void ResetPerformanceMetrics();

        // Integration with other systems
        void SetProvinceSystem(game::province::EnhancedProvinceSystem* province_system);

        // Save/Load support
        void SaveState(Json::Value& state) const;
        void LoadState(const Json::Value& state);

    private:
        // Core system references
        ::core::ecs::ComponentAccessManager& m_access_manager;
        ::core::threading::ThreadSafeMessageBus& m_message_bus;
        std::unique_ptr<TradeRepository> m_repository;

        // Subsystems
        std::unique_ptr<TradePathfinder> m_pathfinder;
        
        // Trade data storage
        std::unordered_map<std::string, TradeRoute> m_active_routes;
        std::unordered_map<types::EntityID, TradeHub> m_trade_hubs;
        std::unordered_map<types::ResourceType, TradeGoodProperties> m_trade_goods;
        std::unordered_map<std::string, MarketData> m_market_data; // key: province_id + resource_type

        // Update timing and performance
        float m_accumulated_time = 0.0f;
        double m_update_frequency = 0.2;  // 5 updates per second
        float m_price_update_interval = 30.0f; // Update prices every 30 seconds
        float m_price_accumulated_time = 0.0f;
        
        // Performance tracking
        PerformanceMetrics m_performance_metrics;
        std::chrono::steady_clock::time_point m_last_performance_check;
        
        // System configuration
        double m_max_trade_distance = 2000.0;
        double m_min_profitability_threshold = 0.05; // 5% minimum profit
        bool m_logging_enabled = false;
        
        // External system references
        game::province::EnhancedProvinceSystem* m_province_system = nullptr;
        
        // Game time tracking (to be wired to TimeManagementSystem)
        int m_current_game_year = 1066;  // Default start year

        // ====================================================================
        // Thread Safety - Mutex Protection Documentation
        // ====================================================================
        // IMPORTANT: TradeSystem declares THREAD_POOL threading strategy but
        // has known thread safety limitations with component access.
        //
        // MUTEX PROTECTION:
        // - m_trade_mutex protects:
        //   * m_active_routes (all read/write operations)
        //   * m_trade_hubs (all read/write operations)
        //   * m_trade_goods (read-only after initialization, but protected for safety)
        //
        // - m_market_mutex protects:
        //   * m_market_data (all read/write operations)
        //
        // THREAD SAFETY STATUS:
        // ✅ ThreadSafeMessageBus usage (thread-safe event publishing)
        // ✅ Mutex-protected internal data structures
        // ⚠️  Component access via repository not fully thread-safe (see TradeRepository.h)
        // ⚠️  Province system pointer access not synchronized
        //
        // RECOMMENDATION FOR PRODUCTION:
        // Consider using MAIN_THREAD strategy until component access is fully secured,
        // OR implement entity-level locking in ComponentAccessManager.
        //
        // ====================================================================
        mutable std::mutex m_trade_mutex;    // Protects: m_active_routes, m_trade_hubs, m_trade_goods
        mutable std::mutex m_market_mutex;   // Protects: m_market_data
        
        // Performance management
        size_t m_max_routes_per_frame = 25;
        size_t m_routes_processed_this_frame = 0;

        // ====================================================================
        // Internal Implementation Methods
        // ====================================================================

        // Route management internals
        void ProcessTradeFlow(TradeRoute& route, float delta_time);
        void UpdateRouteConditions(TradeRoute& route);
        void ProcessRouteRecovery(TradeRoute& route, float delta_time);
        bool IsRouteViable(const TradeRoute& route) const;
        std::string GenerateRouteId(types::EntityID source, types::EntityID destination, 
                                   types::ResourceType resource) const;

        // Market dynamics internals
        void UpdateSupplyDemandLevels(types::EntityID province_id, types::ResourceType resource);
        void ApplyMarketForces(MarketData& market_data, double supply_change, double demand_change);
        void ProcessPriceStabilization(MarketData& market_data);

        // Hub management internals
        void UpdateHubUtilization(TradeHub& hub);
        void UpdateHubSpecializations(TradeHub& hub);
        void CalculateHubReputation(TradeHub& hub);
        double DetermineHubCapacityNeed(types::EntityID province_id) const;

        // Performance and optimization
        void OptimizeRouteStorage();
        void CleanupAbandonedRoutes();
        void UpdatePerformanceMetrics();
        
        // Message publishing
        void PublishTradeRouteEstablished(const TradeRoute& route, const std::string& reason);
        void PublishTradeRouteDisrupted(const TradeRoute& route, const std::string& cause, double duration);
        void PublishHubEvolution(const TradeHub& hub, HubType old_type, const std::string& trigger);
        void PublishPriceShock(types::EntityID province_id, types::ResourceType resource, 
                              double old_price, double new_price, const std::string& cause);
        void PublishTradeVolumeChanged(types::EntityID province_id, types::ResourceType resource,
                                      double old_volume, double new_volume, const std::string& reason);
        void PublishMarketConditionsChanged(types::EntityID province_id,
                                           const std::unordered_map<types::ResourceType, double>& price_changes,
                                           const std::unordered_map<types::ResourceType, double>& supply_changes,
                                           const std::unordered_map<types::ResourceType, double>& demand_changes,
                                           const std::string& cause);

        // Initialization and configuration
        void InitializeTradeGoods();
        void InitializeDefaultHubs();
        void LoadTradeConfiguration();
        
        // Utility methods
        std::string GetProvinceNameSafe(types::EntityID province_id) const;
        std::string GetResourceNameSafe(types::ResourceType resource) const;
        void LogTradeActivity(const std::string& message) const;
        
        // Helper methods for trade calculations
        std::unordered_map<types::ResourceType, double> GetProvinceSupplyData(types::EntityID province_id) const;
        const TradeGoodProperties* GetTradeGood(types::ResourceType resource) const;
        void ProcessPriceShocks();
        double CalculateHubCapacity(types::EntityID province_id) const;
        double DetermineHubInfrastructureBonus(types::EntityID province_id) const;
    };

} // namespace game::trade