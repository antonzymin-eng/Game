// ============================================================================
// EconomicComponents.h - ECS Components for Economic System
// Created: October 11, 2025 - ECS Integration Implementation
// Location: include/game/economy/EconomicComponents.h
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>

namespace game::economy {

    // ============================================================================
    // Trade Route Data Structure
    // ============================================================================

    struct TradeRoute {
        game::types::EntityID from_province;
        game::types::EntityID to_province;
        double efficiency = 0.0;
        int base_value = 0;
        bool is_active = true;

        TradeRoute() = default;
        TradeRoute(game::types::EntityID from, game::types::EntityID to, double eff, int value)
            : from_province(from), to_province(to),
              efficiency(std::max(0.0, std::min(1.0, eff))), // Clamp efficiency to [0, 1]
              base_value(std::max(0, value)), is_active(true) {
        }
    };

    // ============================================================================
    // Economic Event Data Structure  
    // ============================================================================

    struct EconomicEvent {
        enum class Type : int {
            GOOD_HARVEST,
            BAD_HARVEST,
            MERCHANT_CARAVAN,
            BANDIT_RAID,
            PLAGUE_OUTBREAK,
            MARKET_BOOM,
            TRADE_DISRUPTION,
            TAX_REVOLT,
            MERCHANT_GUILD_FORMATION
        };

        Type type = Type::GOOD_HARVEST;
        game::types::EntityID affected_province = 0;
        int duration_months = 0;
        double effect_magnitude = 0.0;
        std::string description;
        bool is_active = true;
    };

    // ============================================================================
    // Economic Component - Manages economic state for a province/realm
    // ============================================================================

    struct EconomicComponent : public game::core::Component<EconomicComponent> {
        // Thread safety mutexes
        mutable std::mutex trade_routes_mutex;
        mutable std::mutex resources_mutex;

        // Default and copy constructors
        EconomicComponent() = default;
        EconomicComponent(const EconomicComponent& other);

        // Treasury and income tracking
        int treasury = 1000;
        int monthly_income = 0;
        int monthly_expenses = 0;
        int net_income = 0;

        // Tax system
        double tax_rate = 0.1;
        int tax_income = 0;
        double tax_collection_efficiency = 0.8;

        // Trade system
        int trade_income = 0;
        int tribute_income = 0;  // Income from vassals and conquered territories
        double trade_efficiency = 1.0;
        std::vector<TradeRoute> active_trade_routes;

        // Economic indicators
        double inflation_rate = 0.02;
        double economic_growth = 0.0;
        double wealth_inequality = 0.3;
        double employment_rate = 0.7;
        double average_wages = 50.0;

        // Infrastructure
        double infrastructure_quality = 0.5;
        int infrastructure_investment = 0;
        double road_network_efficiency = 0.6;

        // Market conditions
        double market_demand = 1.0;
        double market_supply = 1.0;
        double price_index = 100.0;

        // Resource production
        std::unordered_map<std::string, int> resource_production;
        std::unordered_map<std::string, int> resource_consumption;
        std::unordered_map<std::string, double> resource_prices;

        // Population economic data
        int taxable_population = 0;
        int productive_workers = 0;
        double consumer_spending = 0.0;
        double luxury_demand = 0.0;

        std::string GetComponentTypeName() const override;
    };

    // ============================================================================
    // Trade Component - Manages trade routes and commercial activity
    // ============================================================================

    struct TradeComponent : public game::core::Component<TradeComponent> {
        std::vector<TradeRoute> outgoing_routes;
        std::vector<TradeRoute> incoming_routes;

        // Trade node properties
        double trade_node_efficiency = 1.0;
        int trade_node_value = 0;
        bool is_trade_center = false;

        // Merchant activity
        int active_merchants = 0;
        double merchant_guild_power = 0.0;

        // Trade goods
        std::unordered_map<std::string, int> exported_goods;
        std::unordered_map<std::string, int> imported_goods;
        std::unordered_map<std::string, double> trade_good_prices;

        // Trade modifiers
        double piracy_risk = 0.1;
        double diplomatic_trade_modifier = 1.0;
        double technology_trade_modifier = 1.0;

        std::string GetComponentTypeName() const override;
    };

    // ============================================================================
    // Economic Events Component - Manages economic events and their effects
    // ============================================================================

    struct EconomicEventsComponent : public game::core::Component<EconomicEventsComponent> {
        std::vector<EconomicEvent> active_events;

        // Event generation parameters
        double event_frequency_modifier = 1.0;
        int months_since_last_event = 0;

        // Event effects tracking
        std::unordered_map<EconomicEvent::Type, double> event_type_modifiers;
        std::unordered_map<std::string, double> temporary_economic_modifiers;

        // Historical event tracking
        std::vector<EconomicEvent> event_history;
        int max_history_size = 50;

        std::string GetComponentTypeName() const override;
    };

    // ============================================================================
    // Market Component - Manages local market conditions and prices
    // ============================================================================

    struct MarketComponent : public game::core::Component<MarketComponent> {
        // Local market data
        std::unordered_map<std::string, double> local_prices;
        std::unordered_map<std::string, int> local_supply;
        std::unordered_map<std::string, int> local_demand;

        // Market characteristics
        double market_size = 1.0;
        double market_sophistication = 0.5;
        bool has_marketplace = false;
        bool has_port = false;

        // Price volatility
        std::unordered_map<std::string, double> price_volatility;
        std::unordered_map<std::string, double> seasonal_modifiers;

        // Market events
        std::vector<std::string> market_disruptions;
        int market_stability = 100;

        std::string GetComponentTypeName() const override;
    };

    // ============================================================================
    // Treasury Component - Manages financial reserves and expenditures
    // ============================================================================

    struct TreasuryComponent : public game::core::Component<TreasuryComponent> {
        // Main treasury
        int gold_reserves = 1000;
        int silver_reserves = 5000;
        int emergency_fund = 0;
        
        // Income sources
        int tax_income = 0;
        int trade_income = 0;
        int tribute_income = 0;
        int loan_income = 0;
        int other_income = 0;
        
        // Expenditure categories
        int military_expenses = 0;
        int administrative_expenses = 0;
        int infrastructure_expenses = 0;
        int court_expenses = 0;
        int debt_payments = 0;
        int other_expenses = 0;
        
        // Financial management
        std::vector<int> outstanding_loans;
        std::vector<double> loan_interest_rates;
        double credit_rating = 0.8;
        int max_borrowing_capacity = 5000;
        
        // Financial history
        std::vector<int> monthly_balance_history;
        int max_history_months = 24;
        
        std::string GetComponentTypeName() const override;
    };

} // namespace game::economy