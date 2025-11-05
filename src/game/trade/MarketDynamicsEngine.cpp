// ============================================================================
// Mechanica Imperii - Market Dynamics Engine Implementation
// ============================================================================

#include "game/trade/MarketDynamicsEngine.h"
#include "game/trade/TradeCalculator.h"
#include "utils/RandomGenerator.h"
#include <iostream>
#include <algorithm>

namespace game::trade {

    MarketDynamicsEngine::MarketDynamicsEngine(
        std::unordered_map<std::string, MarketData>& market_data,
        const std::unordered_map<types::ResourceType, TradeGoodProperties>& trade_goods,
        ::core::threading::ThreadSafeMessageBus& message_bus,
        std::mutex& market_mutex
    )
        : m_market_data(market_data)
        , m_trade_goods(trade_goods)
        , m_message_bus(message_bus)
        , m_market_mutex(market_mutex) {
    }

    // ========================================================================
    // Market Updates
    // ========================================================================

    void MarketDynamicsEngine::UpdateAllPrices() {
        std::lock_guard<std::mutex> lock(m_market_mutex);

        for (auto& [market_key, market] : m_market_data) {
            UpdateSupplyDemand(market.province_id, market.resource);

            // Calculate supply and demand changes
            double new_supply = TradeCalculator::CalculateSupplyLevel(market.province_id, market.resource);
            double new_demand = TradeCalculator::CalculateDemandLevel(market.province_id, market.resource);

            double supply_change = new_supply - market.supply_level;
            double demand_change = new_demand - market.demand_level;

            // Apply market forces
            ApplyMarketForces(market, supply_change, demand_change);

            // Apply stabilization
            ProcessPriceStabilization(market);
        }

        // Process any pending price shocks
        ProcessPriceShocks();
    }

    void MarketDynamicsEngine::UpdateSupplyDemand(types::EntityID province_id, types::ResourceType resource) {
        std::string market_key = GetMarketKey(province_id, resource);
        auto market_it = m_market_data.find(market_key);

        if (market_it != m_market_data.end()) {
            MarketData& market = market_it->second;

            // Gradual adjustment towards equilibrium
            double new_supply = TradeCalculator::CalculateSupplyLevel(province_id, resource);
            double new_demand = TradeCalculator::CalculateDemandLevel(province_id, resource);

            market.supply_level += (new_supply - market.supply_level) * 0.1; // 10% adjustment per update
            market.demand_level += (new_demand - market.demand_level) * 0.1;
        }
    }

    void MarketDynamicsEngine::ApplyMarketForces(MarketData& market, double supply_change, double demand_change) {
        // Calculate price change from market forces
        double price_change = TradeCalculator::CalculatePriceChangeFromMarketForces(
            supply_change, demand_change, 0.5);

        double old_price = market.current_price;

        // Apply price change
        market.current_price *= (1.0 + price_change);
        market.current_price = TradeCalculator::ClampPrice(market.current_price, 0.1, 100.0);

        // Update price change rate
        market.price_change_rate = price_change;

        // Update trend
        UpdatePriceTrend(market, price_change);
    }

    // ========================================================================
    // Price Shocks
    // ========================================================================

    void MarketDynamicsEngine::ApplyPriceShock(types::EntityID province_id, types::ResourceType resource,
                                              double shock_magnitude, const std::string& cause) {
        std::lock_guard<std::mutex> lock(m_market_mutex);

        MarketData& market = GetOrCreateMarket(province_id, resource);
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

        // Increase volatility
        market.volatility_index = std::min(1.0, market.volatility_index + std::abs(shock_magnitude));

        // Publish event
        PublishPriceShock(province_id, resource, old_price, market.current_price, cause);

        std::cout << "[MarketDynamics] Price shock applied to resource " << static_cast<int>(resource)
                  << " in province " << province_id << ": " << cause << std::endl;
    }

    void MarketDynamicsEngine::ProcessPriceShocks() {
        // Process random price shocks to simulate market volatility
        utils::RandomGenerator& rng = utils::RandomGenerator::getInstance();

        if (rng.randomFloat(0.0f, 1.0f) < 0.001) { // 0.1% chance per update
            // Random price shock
            if (m_market_data.empty()) return;

            auto market_it = m_market_data.begin();
            std::advance(market_it, rng.randomInt(0, static_cast<int>(m_market_data.size()) - 1));

            double shock_magnitude = rng.randomFloat(-0.3f, 0.3f); // ±30% price shock
            ApplyPriceShock(market_it->second.province_id, market_it->second.resource,
                          shock_magnitude, "Market volatility");
        }
    }

    // ========================================================================
    // Seasonal Adjustments
    // ========================================================================

    void MarketDynamicsEngine::ProcessSeasonalAdjustments(int current_month) {
        std::lock_guard<std::mutex> lock(m_market_mutex);

        for (auto& [market_key, market] : m_market_data) {
            ApplySeasonalAdjustment(market, current_month);
        }

        std::cout << "[MarketDynamics] Applied seasonal adjustments for month " << current_month << std::endl;
    }

    void MarketDynamicsEngine::ApplySeasonalAdjustment(MarketData& market, int month) {
        auto trade_good = GetTradeGood(market.resource);
        if (!trade_good) return;

        // Apply seasonal demand modifier
        double seasonal_demand = trade_good->GetSeasonalDemandMultiplier(month);
        double seasonal_supply = trade_good->GetSeasonalSupplyMultiplier(month);

        // Adjust market levels
        market.demand_level *= seasonal_demand;
        market.supply_level *= seasonal_supply;

        // Recalculate price based on new supply/demand
        double price_adjustment = (seasonal_demand / seasonal_supply);
        market.current_price *= price_adjustment;
        market.current_price = TradeCalculator::ClampPrice(market.current_price, 0.1, 50.0);
    }

    // ========================================================================
    // Price Stabilization
    // ========================================================================

    void MarketDynamicsEngine::ProcessPriceStabilization(MarketData& market) {
        // Gradually return prices to historical averages
        double stabilization_adjustment = TradeCalculator::CalculateStabilizationAdjustment(
            market.current_price, market.avg_price_12_months, 0.05);

        market.current_price += stabilization_adjustment;

        // Reduce volatility over time
        ReduceVolatility(market);
    }

    void MarketDynamicsEngine::ReduceVolatility(MarketData& market) {
        market.volatility_index *= 0.99; // 1% reduction per update
        market.volatility_index = std::max(0.01, market.volatility_index);
    }

    // ========================================================================
    // Market Queries
    // ========================================================================

    MarketData MarketDynamicsEngine::GetMarketData(types::EntityID province_id, types::ResourceType resource) const {
        std::lock_guard<std::mutex> lock(m_market_mutex);

        std::string market_key = GetMarketKey(province_id, resource);
        auto market_it = m_market_data.find(market_key);

        if (market_it != m_market_data.end()) {
            return market_it->second;
        }

        // Return default market data if not found
        MarketData default_market(province_id, resource);
        auto trade_good = GetTradeGood(resource);
        default_market.current_price = trade_good ? trade_good->base_value_per_unit : 1.0;
        default_market.supply_level = TradeCalculator::CalculateSupplyLevel(province_id, resource);
        default_market.demand_level = TradeCalculator::CalculateDemandLevel(province_id, resource);

        return default_market;
    }

    MarketData& MarketDynamicsEngine::GetOrCreateMarket(types::EntityID province_id, types::ResourceType resource) {
        std::string market_key = GetMarketKey(province_id, resource);
        auto market_it = m_market_data.find(market_key);

        if (market_it == m_market_data.end()) {
            // Create new market data
            MarketData new_market(province_id, resource);
            auto trade_good = GetTradeGood(resource);
            new_market.current_price = trade_good ? trade_good->base_value_per_unit : 1.0;
            new_market.supply_level = TradeCalculator::CalculateSupplyLevel(province_id, resource);
            new_market.demand_level = TradeCalculator::CalculateDemandLevel(province_id, resource);

            m_market_data[market_key] = new_market;
            market_it = m_market_data.find(market_key);
        }

        return market_it->second;
    }

    bool MarketDynamicsEngine::HasMarket(types::EntityID province_id, types::ResourceType resource) const {
        std::string market_key = GetMarketKey(province_id, resource);
        return m_market_data.find(market_key) != m_market_data.end();
    }

    std::string MarketDynamicsEngine::GetMarketKey(types::EntityID province_id, types::ResourceType resource) {
        return std::to_string(province_id) + "_" + std::to_string(static_cast<int>(resource));
    }

    // ========================================================================
    // Private Helper Methods
    // ========================================================================

    const TradeGoodProperties* MarketDynamicsEngine::GetTradeGood(types::ResourceType resource) const {
        auto it = m_trade_goods.find(resource);
        return (it != m_trade_goods.end()) ? &it->second : nullptr;
    }

    void MarketDynamicsEngine::PublishPriceShock(types::EntityID province_id, types::ResourceType resource,
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

    void MarketDynamicsEngine::UpdatePriceTrend(MarketData& market, double price_change) {
        if (price_change > 0.1) {
            market.trend = PriceMovement::RISING;
        } else if (price_change < -0.1) {
            market.trend = PriceMovement::FALLING;
        } else {
            market.trend = PriceMovement::STABLE;
        }
    }

} // namespace game::trade
