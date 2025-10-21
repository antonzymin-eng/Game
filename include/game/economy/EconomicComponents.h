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

namespace game::economy {

    // ============================================================================
    // Trade Route Data Structure
    // ============================================================================

    struct TradeRoute {
        game::types::EntityID from_province;
        game::types::EntityID to_province;
        float efficiency = 0.0f;
        int base_value = 0;
        bool is_active = true;

        TradeRoute() = default;
        TradeRoute(game::types::EntityID from, game::types::EntityID to, float eff, int value)
            : from_province(from), to_province(to), efficiency(eff), base_value(value), is_active(true) {
        }
    };

    // ============================================================================
    // Economic Event Data Structure  
    // ============================================================================

    struct EconomicEvent {
        enum Type {
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

        Type type = GOOD_HARVEST;
        game::types::EntityID affected_province = 0;
        int duration_months = 0;
        float effect_magnitude = 0.0f;
        std::string description;
        bool is_active = true;
    };

    // ============================================================================
    // Economic Component - Manages economic state for a province/realm
    // ============================================================================

    struct EconomicComponent : public game::core::Component<EconomicComponent> {
        // Treasury and income tracking
        int treasury = 1000;
        int monthly_income = 0;
        int monthly_expenses = 0;
        int net_income = 0;

        // Tax system
        float tax_rate = 0.1f;
        int tax_income = 0;
        float tax_collection_efficiency = 0.8f;

        // Trade system
        int trade_income = 0;
        float trade_efficiency = 1.0f;
        std::vector<TradeRoute> active_trade_routes;

        // Economic indicators
        float inflation_rate = 0.02f;
        float economic_growth = 0.0f;
        float wealth_inequality = 0.3f;
        float employment_rate = 0.7f;
        float average_wages = 50.0f;

        // Infrastructure
        float infrastructure_quality = 0.5f;
        int infrastructure_investment = 0;
        float road_network_efficiency = 0.6f;

        // Market conditions
        float market_demand = 1.0f;
        float market_supply = 1.0f;
        float price_index = 100.0f;

        // Resource production
        std::unordered_map<std::string, int> resource_production;
        std::unordered_map<std::string, int> resource_consumption;
        std::unordered_map<std::string, float> resource_prices;

        // Population economic data
        int taxable_population = 0;
        int productive_workers = 0;
        float consumer_spending = 0.0f;
        float luxury_demand = 0.0f;

        std::string GetComponentTypeName() const override;
    };

    // ============================================================================
    // Trade Component - Manages trade routes and commercial activity
    // ============================================================================

    struct TradeComponent : public game::core::Component<TradeComponent> {
        std::vector<TradeRoute> outgoing_routes;
        std::vector<TradeRoute> incoming_routes;
        
        // Trade node properties
        float trade_node_efficiency = 1.0f;
        int trade_node_value = 0;
        bool is_trade_center = false;
        
        // Merchant activity
        int active_merchants = 0;
        float merchant_guild_power = 0.0f;
        
        // Trade goods
        std::unordered_map<std::string, int> exported_goods;
        std::unordered_map<std::string, int> imported_goods;
        std::unordered_map<std::string, float> trade_good_prices;
        
        // Trade modifiers
        float piracy_risk = 0.1f;
        float diplomatic_trade_modifier = 1.0f;
        float technology_trade_modifier = 1.0f;
        
        std::string GetComponentTypeName() const override;
    };

    // ============================================================================
    // Economic Events Component - Manages economic events and their effects
    // ============================================================================

    struct EconomicEventsComponent : public game::core::Component<EconomicEventsComponent> {
        std::vector<EconomicEvent> active_events;
        
        // Event generation parameters
        float event_frequency_modifier = 1.0f;
        int months_since_last_event = 0;
        
        // Event effects tracking
        std::unordered_map<EconomicEvent::Type, float> event_type_modifiers;
        std::unordered_map<std::string, float> temporary_economic_modifiers;
        
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
        std::unordered_map<std::string, float> local_prices;
        std::unordered_map<std::string, int> local_supply;
        std::unordered_map<std::string, int> local_demand;
        
        // Market characteristics
        float market_size = 1.0f;
        float market_sophistication = 0.5f;
        bool has_marketplace = false;
        bool has_port = false;
        
        // Price volatility
        std::unordered_map<std::string, float> price_volatility;
        std::unordered_map<std::string, float> seasonal_modifiers;
        
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
        std::vector<float> loan_interest_rates;
        float credit_rating = 0.8f;
        int max_borrowing_capacity = 5000;
        
        // Financial history
        std::vector<int> monthly_balance_history;
        int max_history_months = 24;
        
        std::string GetComponentTypeName() const override;
    };

} // namespace game::economy