// Created: October 30, 2025
// Location: src/game/economy/TradeEconomicBridge.cpp
// Trade-Economic Bridge System Implementation

#include "../../../include/game/economy/TradeEconomicBridge.h"
#include "../../../include/game/economy/EconomicComponents.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <chrono>

namespace mechanica {
namespace integration {

// ============================================================================
// Constructor
// ============================================================================

TradeEconomicBridge::TradeEconomicBridge() {
    // Load configuration from GameConfig
    auto& config = game::config::GameConfig::Instance();

    m_config.bridge_update_interval = config.GetDouble("trade_bridge.update_interval_days", 1.0);

    m_config.trade_income_to_treasury_ratio = config.GetDouble("trade_bridge.trade_income_to_treasury_ratio", 0.9);
    m_config.customs_tax_rate = config.GetDouble("trade_bridge.customs_tax_rate", 0.05);
    m_config.merchant_tax_rate = config.GetDouble("trade_bridge.merchant_tax_rate", 0.02);

    m_config.low_treasury_trade_penalty = config.GetDouble("trade_bridge.low_treasury_trade_penalty", 0.3);
    m_config.treasury_threshold_ratio = config.GetDouble("trade_bridge.treasury_threshold_ratio", 0.2);
    m_config.high_tax_trade_penalty = config.GetDouble("trade_bridge.high_tax_trade_penalty", 0.4);
    m_config.tax_threshold = config.GetDouble("trade_bridge.tax_threshold", 0.25);

    m_config.infrastructure_trade_bonus = config.GetDouble("trade_bridge.infrastructure_trade_bonus", 0.5);
    m_config.infrastructure_threshold = config.GetDouble("trade_bridge.infrastructure_threshold", 0.7);
    m_config.road_network_trade_multiplier = config.GetDouble("trade_bridge.road_network_trade_multiplier", 1.5);

    m_config.price_volatility_threshold = config.GetDouble("trade_bridge.price_volatility_threshold", 0.3);
    m_config.demand_supply_imbalance_threshold = config.GetDouble("trade_bridge.demand_supply_imbalance_threshold", 0.4);
    m_config.luxury_wealth_threshold = config.GetDouble("trade_bridge.luxury_wealth_threshold", 100.0);
    m_config.luxury_demand_multiplier = config.GetDouble("trade_bridge.luxury_demand_multiplier", 0.15);

    m_config.trade_collapse_threshold = config.GetDouble("trade_bridge.trade_collapse_threshold", 0.3);
    m_config.economic_instability_threshold = config.GetDouble("trade_bridge.economic_instability_threshold", 0.3);
    m_config.imbalance_threshold = config.GetDouble("trade_bridge.imbalance_threshold", 0.6);

    m_config.default_trade_efficiency = config.GetDouble("trade_bridge.default_trade_efficiency", 1.0);
    m_config.default_economic_stability = config.GetDouble("trade_bridge.default_economic_stability", 1.0);
    m_config.default_infrastructure_quality = config.GetDouble("trade_bridge.default_infrastructure_quality", 0.6);
    m_config.default_population_wealth = config.GetDouble("trade_bridge.default_population_wealth", 50.0);

    m_config.infrastructure_investment_trade_ratio = config.GetDouble("trade_bridge.infrastructure_investment_trade_ratio", 0.2);
    m_config.trade_subsidy_effectiveness = config.GetDouble("trade_bridge.trade_subsidy_effectiveness", 0.3);
    m_config.capital_availability_multiplier = config.GetDouble("trade_bridge.capital_availability_multiplier", 0.1);

    m_config.balance_trade_weight = config.GetDouble("trade_bridge.balance_trade_weight", 0.5);
    m_config.balance_economic_weight = config.GetDouble("trade_bridge.balance_economic_weight", 0.5);

    m_config.crisis_severity_increase = config.GetDouble("trade_bridge.crisis_severity_increase", 0.15);
    m_config.crisis_severity_decrease = config.GetDouble("trade_bridge.crisis_severity_decrease", 0.05);
    m_config.crisis_reset_threshold = config.GetDouble("trade_bridge.crisis_reset_threshold", 0.1);

    m_config.max_history_size = config.GetInt("trade_bridge.max_history_size", 12);
    m_config.performance_log_interval = config.GetDouble("trade_bridge.performance_log_interval", 10.0);
}

// ============================================================================
// System Lifecycle Implementation
// ============================================================================

void TradeEconomicBridge::Initialize() {
    std::cout << "Initializing Trade-Economic Bridge System..." << std::endl;

    if (!m_entity_manager) {
        throw std::runtime_error("TradeEconomicBridge: EntityManager not set");
    }

    if (!m_message_bus) {
        throw std::runtime_error("TradeEconomicBridge: MessageBus not set");
    }

    if (!m_trade_system) {
        std::cout << "Warning: TradeSystem not set - some features will be limited" << std::endl;
    }

    if (!m_economic_system) {
        std::cout << "Warning: EconomicSystem not set - some features will be limited" << std::endl;
    }

    m_updates_this_frame.store(0);
    m_last_performance_log.store(0.0);

    std::cout << "Trade-Economic Bridge System initialized successfully" << std::endl;
}

void TradeEconomicBridge::Update(core::ecs::EntityManager& entities,
    ::core::threading::ThreadSafeMessageBus& message_bus,
    double delta_time) {

    m_entity_manager = &entities;
    m_message_bus = &message_bus;

    m_updates_this_frame.fetch_add(1);

    // Get entities with economic components
    auto entities_with_economic = entities.GetEntitiesWithComponent<game::economy::EconomicComponent>();

    for (auto entity_id : entities_with_economic) {
        auto bridge_comp = entities.GetComponent<TradeEconomicBridgeComponent>(entity_id);
        if (!bridge_comp) {
            bridge_comp = entities.AddComponent<TradeEconomicBridgeComponent>(entity_id);
            std::cout << "Created trade-economic bridge component for entity " << entity_id.id << std::endl;
        }

        double current_time = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        double update_interval_seconds = m_config.bridge_update_interval * 24.0 * 3600.0;
        if (current_time - bridge_comp->last_update_time < update_interval_seconds) {
            continue;
        }

        UpdateEntityBridge(static_cast<game::types::EntityID>(entity_id.id), *bridge_comp, delta_time);
        bridge_comp->last_update_time = current_time;
    }

    LogPerformanceMetrics();
}

void TradeEconomicBridge::Shutdown() {
    m_entity_manager = nullptr;
    m_message_bus = nullptr;
    m_trade_system = nullptr;
    m_economic_system = nullptr;

    std::cout << "Trade-Economic Bridge System shut down" << std::endl;
}

// ============================================================================
// Threading Interface
// ============================================================================

core::threading::ThreadingStrategy TradeEconomicBridge::GetThreadingStrategy() const {
    return core::threading::ThreadingStrategy::THREAD_POOL;
}

// ============================================================================
// Serialization Interface
// ============================================================================

Json::Value TradeEconomicBridge::Serialize(int version) const {
    Json::Value root;
    root["system_name"] = GetSystemName();
    root["version"] = version;
    // Add additional serialization as needed
    return root;
}

bool TradeEconomicBridge::Deserialize(const Json::Value& data, int version) {
    if (!data.isMember("system_name") || data["system_name"].asString() != GetSystemName()) {
        return false;
    }
    // Add additional deserialization as needed
    return true;
}

std::string TradeEconomicBridge::GetSystemName() const {
    return "TradeEconomicBridge";
}

// ============================================================================
// Core Bridge Calculation Methods
// ============================================================================

TradeEconomicEffects TradeEconomicBridge::CalculateTradeEffects(game::types::EntityID entity_id) {
    TradeEconomicEffects effects;

    if (!m_entity_manager) return effects;

    auto trade_route_comp = m_entity_manager->GetComponent<game::trade::TradeRouteComponent>(
        core::ecs::EntityID(entity_id, 1));
    auto trade_hub_comp = m_entity_manager->GetComponent<game::trade::TradeHubComponent>(
        core::ecs::EntityID(entity_id, 1));

    // Calculate trade route income
    if (trade_route_comp && m_trade_system) {
        // Use TradeSystem API to get actual route data
        auto outgoing_routes = m_trade_system->GetRoutesFromProvince(entity_id);
        auto incoming_routes = m_trade_system->GetRoutesToProvince(entity_id);
        
        double total_income = 0.0;
        double total_volume = 0.0;
        
        for (const auto& route : outgoing_routes) {
            if (route.IsViable()) {
                total_income += route.GetEffectiveVolume() * route.profitability * route.source_price;
                total_volume += route.GetEffectiveVolume();
            }
        }
        
        for (const auto& route : incoming_routes) {
            if (route.IsViable()) {
                total_volume += route.GetEffectiveVolume();
            }
        }
        
        effects.trade_route_income = total_income;
        effects.trade_volume = total_volume;
        effects.trade_profitability = total_volume > 0.0 ? (total_income / total_volume) : 0.0;
    }

    // Calculate trade hub value
    if (trade_hub_comp && m_trade_system) {
        auto hub = m_trade_system->GetTradeHub(entity_id);
        if (hub.has_value()) {
            effects.trade_hub_value = hub->current_utilization * hub->max_throughput_capacity * 10.0;
            effects.merchant_activity_level = trade_hub_comp->merchant_count;
        }
    }

    // Calculate import/export balance
    effects.import_export_balance = effects.trade_route_income * 0.1; // Simplified calculation

    // Calculate trade efficiency
    effects.trade_efficiency = m_config.default_trade_efficiency;

    // Calculate customs revenue
    effects.customs_revenue = CalculateCustomsRevenue(effects);

    // Calculate market price index
    effects.market_price_index = 100.0; // Default baseline

    // Calculate international trade ratio
    if (trade_route_comp && trade_route_comp->active_route_ids.size() > 0) {
        effects.international_trade_ratio = 0.3; // Simplified
    }

    return effects;
}

EconomicTradeContribution TradeEconomicBridge::CalculateEconomicContributions(game::types::EntityID entity_id) {
    EconomicTradeContribution contributions;

    if (!m_entity_manager) return contributions;

    auto economic_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!economic_comp) return contributions;

    // Available capital for trade investment
    contributions.available_capital = static_cast<double>(economic_comp->treasury);

    // Tax burden affecting trade profitability
    contributions.tax_burden = economic_comp->tax_rate;

    // Infrastructure quality affecting trade efficiency
    contributions.infrastructure_quality = economic_comp->infrastructure_quality;

    // Economic stability
    double inflation_penalty = std::max(0.0, economic_comp->inflation_rate - 0.02) * 10.0;
    contributions.economic_stability = std::max(0.0, 1.0 - inflation_penalty);

    // Population wealth affects demand
    contributions.population_wealth = economic_comp->average_wages;

    // Luxury demand
    contributions.luxury_demand = CalculateLuxuryDemand(contributions.population_wealth);

    // Investment capacity
    double net_income = static_cast<double>(economic_comp->net_income);
    contributions.investment_capacity = std::max(0.0, net_income * 0.2);

    // Trade subsidy (from infrastructure investment)
    contributions.trade_subsidy = economic_comp->infrastructure_investment *
        m_config.infrastructure_investment_trade_ratio;

    // Market demand modifier
    contributions.market_demand_modifier = CalculateMarketDemandModifier(
        contributions.economic_stability, contributions.population_wealth);

    // Credit rating
    contributions.credit_rating = 0.8; // Simplified

    return contributions;
}

void TradeEconomicBridge::ApplyTradeEffectsToEconomy(game::types::EntityID entity_id,
    const TradeEconomicEffects& effects) {
    if (!m_entity_manager) return;

    auto economic_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!economic_comp) return;

    // Calculate total trade income (routes + customs)
    double total_trade_income = CalculateTradeIncome(effects);

    // Update economic component
    economic_comp->trade_income = static_cast<int>(total_trade_income);

    // Add to treasury if economic system is available
    if (m_economic_system) {
        int income_to_add = static_cast<int>(total_trade_income * m_config.trade_income_to_treasury_ratio);
        m_economic_system->AddMoney(entity_id, income_to_add);

        std::cout << "Entity " << entity_id << " received " << income_to_add
                  << " from trade (routes: " << effects.trade_route_income
                  << ", customs: " << effects.customs_revenue << ")" << std::endl;
    }

    // Update trade efficiency in economic component
    economic_comp->trade_efficiency = effects.trade_efficiency;

    // Market price effects
    economic_comp->price_index = static_cast<float>(effects.market_price_index);
}

void TradeEconomicBridge::ApplyEconomicContributionsToTrade(game::types::EntityID entity_id,
    const EconomicTradeContribution& contributions) {
    if (!m_entity_manager || !m_trade_system) return;

    auto trade_route_comp = m_entity_manager->GetComponent<game::trade::TradeRouteComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!trade_route_comp) return;

    // Calculate trade modifiers based on economic conditions
    double tax_penalty = CalculateTaxPenaltyOnTrade(contributions.tax_burden);
    double treasury_constraint = CalculateTreasuryConstraint(contributions.available_capital);
    double infrastructure_bonus = CalculateInfrastructureBonus(contributions.infrastructure_quality);

    // Apply economic modifiers to trade system
    double total_modifier = (1.0 - tax_penalty) * treasury_constraint * (1.0 + infrastructure_bonus) *
                            contributions.economic_stability * contributions.market_demand_modifier;

    // Placeholder - actual application would modify trade routes
    std::cout << "Entity " << entity_id << " trade efficiency modifier calculated: "
              << total_modifier << std::endl;
}

// ============================================================================
// System Configuration
// ============================================================================

void TradeEconomicBridge::SetTradeSystem(game::trade::TradeSystem* trade_system) {
    m_trade_system = trade_system;
}

void TradeEconomicBridge::SetEconomicSystem(game::economy::EconomicSystem* economic_system) {
    m_economic_system = economic_system;
}

// ============================================================================
// Crisis Detection
// ============================================================================

void TradeEconomicBridge::ProcessCrisisDetection(game::types::EntityID entity_id) {
    if (!m_entity_manager || !m_message_bus) return;

    auto bridge_comp = m_entity_manager->GetComponent<TradeEconomicBridgeComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!bridge_comp) return;

    bool trade_crisis = DetectTradeCrisis(*bridge_comp);
    if (trade_crisis && !bridge_comp->trade_crisis) {
        bridge_comp->trade_crisis = true;

        TradeCrisisEvent crisis_event;
        crisis_event.affected_entity = entity_id;
        crisis_event.crisis_severity = bridge_comp->crisis_severity;
        crisis_event.crisis_type = "trade_collapse";
        crisis_event.contributing_factors = { "low_trade_volume", "high_costs" };

        m_message_bus->Publish(crisis_event);
        std::cout << "Trade crisis detected for entity " << entity_id << std::endl;
    }

    bool economic_crisis = DetectEconomicCrisis(*bridge_comp);
    if (economic_crisis && !bridge_comp->economic_crisis) {
        bridge_comp->economic_crisis = true;

        TradeEconomicImbalanceEvent imbalance_event;
        imbalance_event.affected_entity = entity_id;
        imbalance_event.imbalance_level = bridge_comp->crisis_severity;
        imbalance_event.primary_cause = "economic_instability";
        imbalance_event.requires_intervention = true;

        m_message_bus->Publish(imbalance_event);
        std::cout << "Economic crisis affecting trade for entity " << entity_id << std::endl;
    }

    // Reset crisis if conditions improve
    if (!trade_crisis && bridge_comp->trade_crisis) {
        bridge_comp->trade_crisis = false;
        bridge_comp->crisis_severity = std::max(0.0, bridge_comp->crisis_severity - m_config.crisis_severity_decrease);
    }

    if (!economic_crisis && bridge_comp->economic_crisis) {
        bridge_comp->economic_crisis = false;
        bridge_comp->crisis_severity = std::max(0.0, bridge_comp->crisis_severity - m_config.crisis_severity_decrease);
    }
}

// ============================================================================
// Public Metrics Interface
// ============================================================================

TradeEconomicBridge::BridgeHealthMetrics TradeEconomicBridge::GetBridgeHealth(game::types::EntityID entity_id) const {
    BridgeHealthMetrics metrics;

    if (!m_entity_manager) return metrics;

    auto bridge_comp = m_entity_manager->GetComponent<TradeEconomicBridgeComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!bridge_comp) return metrics;

    metrics.trade_economic_balance = bridge_comp->trade_economic_balance;
    metrics.crisis_active = bridge_comp->trade_crisis || bridge_comp->economic_crisis;
    metrics.crisis_severity = bridge_comp->crisis_severity;

    // Calculate trends
    if (bridge_comp->trade_income_history.size() >= 2) {
        size_t last = bridge_comp->trade_income_history.size() - 1;
        metrics.trade_income_trend = bridge_comp->trade_income_history[last] -
                                      bridge_comp->trade_income_history[last - 1];
    }

    if (bridge_comp->economic_health_history.size() >= 2) {
        size_t last = bridge_comp->economic_health_history.size() - 1;
        metrics.economic_health_trend = bridge_comp->economic_health_history[last] -
                                         bridge_comp->economic_health_history[last - 1];
    }

    // Identify primary issue
    if (metrics.crisis_active) {
        if (bridge_comp->trade_crisis) {
            metrics.primary_issue = "Trade volume collapse";
        } else if (bridge_comp->economic_crisis) {
            metrics.primary_issue = "Economic instability";
        }
    } else {
        metrics.primary_issue = "Stable";
    }

    return metrics;
}

// ============================================================================
// Internal Calculation Helpers
// ============================================================================

double TradeEconomicBridge::CalculateTradeIncome(const TradeEconomicEffects& effects) const {
    return effects.trade_route_income + effects.customs_revenue;
}

double TradeEconomicBridge::CalculateCustomsRevenue(const TradeEconomicEffects& effects) const {
    return effects.trade_volume * m_config.customs_tax_rate +
           effects.merchant_activity_level * m_config.merchant_tax_rate;
}

double TradeEconomicBridge::CalculateTaxPenaltyOnTrade(double tax_rate) const {
    if (tax_rate > m_config.tax_threshold) {
        double excess_tax = tax_rate - m_config.tax_threshold;
        return excess_tax * m_config.high_tax_trade_penalty;
    }
    return 0.0;
}

double TradeEconomicBridge::CalculateTreasuryConstraint(double available_capital) const {
    double threshold = 1000.0 * m_config.treasury_threshold_ratio;
    if (available_capital < threshold) {
        return 1.0 - m_config.low_treasury_trade_penalty;
    }
    return 1.0;
}

double TradeEconomicBridge::CalculateInfrastructureBonus(double infrastructure_quality) const {
    if (infrastructure_quality > m_config.infrastructure_threshold) {
        return (infrastructure_quality - m_config.infrastructure_threshold) * m_config.infrastructure_trade_bonus;
    }
    return 0.0;
}

double TradeEconomicBridge::CalculateLuxuryDemand(double population_wealth) const {
    double excess_wealth = std::max(0.0, population_wealth - m_config.luxury_wealth_threshold);
    return excess_wealth * m_config.luxury_demand_multiplier;
}

double TradeEconomicBridge::CalculateMarketDemandModifier(double economic_stability, double population_wealth) const {
    double base_demand = economic_stability * (population_wealth / m_config.default_population_wealth);
    return std::max(0.5, std::min(2.0, base_demand));
}

// ============================================================================
// Crisis Detection Helpers
// ============================================================================

bool TradeEconomicBridge::DetectTradeCrisis(const TradeEconomicBridgeComponent& bridge_comp) const {
    // Check if trade income is critically low
    if (bridge_comp.trade_income_history.size() >= 3) {
        double recent_avg = 0.0;
        for (size_t i = bridge_comp.trade_income_history.size() - 3;
             i < bridge_comp.trade_income_history.size(); ++i) {
            recent_avg += bridge_comp.trade_income_history[i];
        }
        recent_avg /= 3.0;

        if (recent_avg < m_config.trade_collapse_threshold * 1000.0) {
            return true;
        }
    }

    return bridge_comp.trade_effects.trade_efficiency < m_config.trade_collapse_threshold;
}

bool TradeEconomicBridge::DetectEconomicCrisis(const TradeEconomicBridgeComponent& bridge_comp) const {
    return bridge_comp.economic_contributions.economic_stability < m_config.economic_instability_threshold;
}

bool TradeEconomicBridge::DetectImbalance(const TradeEconomicBridgeComponent& bridge_comp) const {
    return std::abs(bridge_comp.trade_economic_balance - 0.5) > m_config.imbalance_threshold;
}

// ============================================================================
// Update Helpers
// ============================================================================

void TradeEconomicBridge::UpdateEntityBridge(game::types::EntityID entity_id,
                                              TradeEconomicBridgeComponent& bridge_comp,
                                              double delta_time) {
    // Calculate current effects and contributions
    TradeEconomicEffects trade_effects = CalculateTradeEffects(entity_id);
    EconomicTradeContribution economic_contributions = CalculateEconomicContributions(entity_id);

    // Store in bridge component
    bridge_comp.trade_effects = trade_effects;
    bridge_comp.economic_contributions = economic_contributions;

    // Apply effects
    ApplyTradeEffectsToEconomy(entity_id, trade_effects);
    ApplyEconomicContributionsToTrade(entity_id, economic_contributions);

    // Calculate balance
    double trade_health = trade_effects.trade_efficiency * (trade_effects.trade_profitability + 0.5);
    double economic_health = economic_contributions.economic_stability *
                            (economic_contributions.available_capital / 1000.0);

    bridge_comp.trade_economic_balance =
        (trade_health * m_config.balance_trade_weight +
         economic_health * m_config.balance_economic_weight) /
        (m_config.balance_trade_weight + m_config.balance_economic_weight);

    // Update historical data
    UpdateHistoricalData(bridge_comp, CalculateTradeIncome(trade_effects), economic_health);

    // Detect crises
    ProcessCrisisDetection(entity_id);
}

void TradeEconomicBridge::UpdateHistoricalData(TradeEconomicBridgeComponent& bridge_comp,
                                                double trade_income,
                                                double economic_health) {
    bridge_comp.trade_income_history.push_back(trade_income);
    if (bridge_comp.trade_income_history.size() > static_cast<size_t>(m_config.max_history_size)) {
        bridge_comp.trade_income_history.erase(bridge_comp.trade_income_history.begin());
    }

    bridge_comp.economic_health_history.push_back(economic_health);
    if (bridge_comp.economic_health_history.size() > static_cast<size_t>(m_config.max_history_size)) {
        bridge_comp.economic_health_history.erase(bridge_comp.economic_health_history.begin());
    }
}

void TradeEconomicBridge::LogPerformanceMetrics() {
    double current_time = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    double last_log = m_last_performance_log.load();
    if (current_time - last_log > m_config.performance_log_interval) {
        int updates = m_updates_this_frame.load();
        std::cout << "[TradeEconomicBridge] Performance: " << updates
                  << " updates in last " << m_config.performance_log_interval << "s" << std::endl;

        m_updates_this_frame.store(0);
        m_last_performance_log.store(current_time);
    }
}

} // namespace integration
} // namespace mechanica
