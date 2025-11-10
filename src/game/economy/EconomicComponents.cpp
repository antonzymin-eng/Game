// ============================================================================
// EconomicComponents.cpp - ECS Component Method Implementations
// Created: October 13, 2025 - Following Architecture Database Patterns
// Location: src/game/economy/EconomicComponents.cpp
// ============================================================================

#include "game/economy/EconomicComponents.h"
#include "core/logging/Logger.h"

namespace game::economy {

    // ============================================================================
    // EconomicComponent Methods
    // ============================================================================

    EconomicComponent::EconomicComponent(const EconomicComponent& other)
        : treasury(other.treasury),
          monthly_income(other.monthly_income),
          monthly_expenses(other.monthly_expenses),
          net_income(other.net_income),
          tax_rate(other.tax_rate),
          tax_income(other.tax_income),
          tax_collection_efficiency(other.tax_collection_efficiency),
          trade_income(other.trade_income),
          tribute_income(other.tribute_income),
          trade_efficiency(other.trade_efficiency),
          active_trade_routes(other.active_trade_routes),
          inflation_rate(other.inflation_rate),
          economic_growth(other.economic_growth),
          wealth_inequality(other.wealth_inequality),
          employment_rate(other.employment_rate),
          average_wages(other.average_wages),
          infrastructure_quality(other.infrastructure_quality),
          infrastructure_investment(other.infrastructure_investment),
          road_network_efficiency(other.road_network_efficiency),
          market_demand(other.market_demand),
          market_supply(other.market_supply),
          price_index(other.price_index),
          resource_production(other.resource_production),
          resource_consumption(other.resource_consumption),
          resource_prices(other.resource_prices),
          taxable_population(other.taxable_population),
          productive_workers(other.productive_workers),
          consumer_spending(other.consumer_spending),
          luxury_demand(other.luxury_demand)
    {
        // Mutexes are not copied
    }

    std::string EconomicComponent::GetComponentTypeName() const {
        return "EconomicComponent";
    }

    // ============================================================================
    // TradeComponent Methods
    // ============================================================================

    std::string TradeComponent::GetComponentTypeName() const {
        return "TradeComponent";
    }

    // ============================================================================
    // EconomicEventsComponent Methods
    // ============================================================================

    std::string EconomicEventsComponent::GetComponentTypeName() const {
        return "EconomicEventsComponent";
    }

    // ============================================================================
    // MarketComponent Methods
    // ============================================================================

    std::string MarketComponent::GetComponentTypeName() const {
        return "MarketComponent";
    }

    // ============================================================================
    // TreasuryComponent Methods
    // ============================================================================

    std::string TreasuryComponent::GetComponentTypeName() const {
        return "TreasuryComponent";
    }

} // namespace game::economy