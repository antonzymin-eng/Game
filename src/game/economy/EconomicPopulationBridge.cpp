// Created: September 27, 2025 - 14:50 PST
// Location: src/game/economic/EconomicPopulationBridge.cpp
// Economic-Population Bridge System - Fixed Implementation (Part 1 of 2)

#include "../../../include/game/economy/EconomicPopulationBridge.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <chrono>
#include "core/logging/Logger.h"

namespace mechanica {
namespace integration {

// ============================================================================
// Constructor
// ============================================================================

EconomicPopulationBridge::EconomicPopulationBridge() {
    // Load configuration from GameConfig
    auto& config = game::config::GameConfig::Instance();
    
    m_config.bridge_update_interval = config.GetDouble("economic_bridge.update_interval_days", 1.0);
    
    m_config.tax_happiness_base_effect = config.GetDouble("economic_bridge.tax_happiness_base_effect", -0.5);
    m_config.tax_happiness_scaling = config.GetDouble("economic_bridge.tax_happiness_scaling", -0.3);
    
    m_config.unemployment_happiness_penalty = config.GetDouble("economic_bridge.unemployment_happiness_penalty", -0.3);
    m_config.wage_happiness_scaling = config.GetDouble("economic_bridge.wage_happiness_scaling", 0.2);
    
    m_config.inequality_threshold = config.GetDouble("economic_bridge.inequality_threshold", 0.4);
    m_config.inequality_happiness_penalty = config.GetDouble("economic_bridge.inequality_happiness_penalty", -0.4);
    
    m_config.literacy_productivity_bonus = config.GetDouble("economic_bridge.literacy_productivity_bonus", 0.3);
    m_config.happiness_productivity_scaling = config.GetDouble("economic_bridge.happiness_productivity_scaling", 0.2);
    
    m_config.economic_output_crisis_threshold = config.GetDouble("economic_bridge.economic_output_crisis_threshold", 0.3);
    m_config.happiness_crisis_threshold = config.GetDouble("economic_bridge.happiness_crisis_threshold", 0.3);
    
    m_config.default_tax_rate = config.GetDouble("economic_bridge.default_tax_rate", 0.15);
    m_config.default_wages = config.GetDouble("economic_bridge.default_wages", 50.0);
    m_config.default_infrastructure_quality = config.GetDouble("economic_bridge.default_infrastructure_quality", 0.6);
    m_config.default_inflation_rate = config.GetDouble("economic_bridge.default_inflation_rate", 0.02);
    m_config.default_economic_growth = config.GetDouble("economic_bridge.default_economic_growth", 0.03);
    
    m_config.taxable_population_ratio = config.GetDouble("economic_bridge.taxable_population_ratio", 0.8);
    m_config.consumer_spending_multiplier = config.GetDouble("economic_bridge.consumer_spending_multiplier", 0.6);
    m_config.luxury_wealth_threshold = config.GetDouble("economic_bridge.luxury_wealth_threshold", 50.0);
    m_config.luxury_demand_multiplier = config.GetDouble("economic_bridge.luxury_demand_multiplier", 0.1);
    
    m_config.tax_collection_literacy_base = config.GetDouble("economic_bridge.tax_collection_literacy_base", 0.5);
    m_config.tax_collection_literacy_bonus = config.GetDouble("economic_bridge.tax_collection_literacy_bonus", 0.4);
    m_config.tax_collection_happiness_base = config.GetDouble("economic_bridge.tax_collection_happiness_base", 0.7);
    m_config.tax_collection_happiness_bonus = config.GetDouble("economic_bridge.tax_collection_happiness_bonus", 0.3);
    
    m_config.infrastructure_good_threshold = config.GetDouble("economic_bridge.infrastructure_good_threshold", 0.7);
    m_config.infrastructure_capacity_bonus = config.GetDouble("economic_bridge.infrastructure_capacity_bonus", 0.5);
    m_config.wealth_increase_trade_multiplier = config.GetDouble("economic_bridge.wealth_increase_trade_multiplier", 0.1);
    
    m_config.crisis_severity_increase = config.GetDouble("economic_bridge.crisis_severity_increase", 0.1);
    m_config.crisis_severity_decrease = config.GetDouble("economic_bridge.crisis_severity_decrease", 0.05);
    m_config.crisis_reset_threshold = config.GetDouble("economic_bridge.crisis_reset_threshold", 0.1);
    
    m_config.employment_crisis_threshold = config.GetDouble("economic_bridge.employment_crisis_threshold", 0.6);
    m_config.tax_efficiency_crisis_threshold = config.GetDouble("economic_bridge.tax_efficiency_crisis_threshold", 0.5);
    
    m_config.happiness_baseline = config.GetDouble("economic_bridge.happiness_baseline", 0.5);
    m_config.wealth_normalization = config.GetDouble("economic_bridge.wealth_normalization", 100.0);
    
    m_config.max_history_size = config.GetInt("economic_bridge.max_history_size", 12);
    m_config.performance_log_interval = config.GetDouble("economic_bridge.performance_log_interval", 10.0);
}

// ============================================================================
// System Lifecycle Implementation
// ============================================================================

void EconomicPopulationBridge::Initialize() {
    CORE_STREAM_INFO("EconomicPopulationBridge") << "Initializing Economic-Population Bridge System..." << std::endl;

    if (!m_entity_manager) {
        throw std::runtime_error("EconomicPopulationBridge: EntityManager not set");
    }

    if (!m_message_bus) {
        throw std::runtime_error("EconomicPopulationBridge: MessageBus not set");
    }

    if (!m_economic_system) {
        CORE_STREAM_INFO("EconomicPopulationBridge") << "Warning: EconomicSystem not set - some features will be limited" << std::endl;
    }

    m_updates_this_frame.store(0);
    m_last_performance_log.store(0.0);

    CORE_STREAM_INFO("EconomicPopulationBridge") << "Economic-Population Bridge System initialized successfully" << std::endl;
}

void EconomicPopulationBridge::Update(core::ecs::EntityManager& entities,
    ::core::threading::ThreadSafeMessageBus& message_bus,
    double delta_time) {

    m_entity_manager = &entities;
    m_message_bus = &message_bus;

    m_updates_this_frame.fetch_add(1);

    auto entities_with_population = entities.GetEntitiesWithComponent<game::population::PopulationComponent>();

    for (auto entity_id : entities_with_population) {
        auto bridge_comp = entities.GetComponent<EconomicPopulationBridgeComponent>(entity_id);
        if (!bridge_comp) {
            bridge_comp = entities.AddComponent<EconomicPopulationBridgeComponent>(entity_id);
            CORE_STREAM_INFO("EconomicPopulationBridge") << "Created bridge component for entity " << entity_id.id << std::endl;
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

void EconomicPopulationBridge::Shutdown() {
    m_entity_manager = nullptr;
    m_message_bus = nullptr;
    m_economic_system = nullptr;

    CORE_STREAM_INFO("EconomicPopulationBridge") << "Economic-Population Bridge System shut down" << std::endl;
}

// ============================================================================
// Threading Interface
// ============================================================================

core::threading::ThreadingStrategy EconomicPopulationBridge::GetThreadingStrategy() const {
    return core::threading::ThreadingStrategy::THREAD_POOL;
}

// ============================================================================
// Serialization Interface
// ============================================================================

std::string EconomicPopulationBridge::GetSystemName() const {
    return "EconomicPopulationBridge";
}

// ============================================================================
// Core Bridge Calculation Methods
// ============================================================================

EconomicPopulationEffects EconomicPopulationBridge::CalculateEconomicEffects(game::types::EntityID entity_id) {
    EconomicPopulationEffects effects;

    if (!m_entity_manager) return effects;

    auto pop_comp = m_entity_manager->GetComponent<game::population::PopulationComponent>(core::ecs::EntityID(entity_id, 1));
    if (!pop_comp) return effects;

    if (m_economic_system) {
        effects.tax_rate = m_config.default_tax_rate;
        effects.tax_happiness_modifier = CalculateTaxHappinessEffect(effects.tax_rate, pop_comp->average_happiness);
    }

    effects.employment_rate = pop_comp->overall_employment_rate;
    effects.average_wages = m_config.default_wages;

    double wealth_factor = pop_comp->average_wealth / m_config.wealth_normalization;
    effects.wealth_inequality = std::min(0.8, std::max(0.1, 1.0 - wealth_factor));

    if (m_economic_system) {
        double total_trade_income = 0.0;
        effects.trade_income_per_capita = total_trade_income / std::max<double>(1.0, static_cast<double>(pop_comp->total_population));
    }

    effects.infrastructure_quality = m_config.default_infrastructure_quality;
    effects.public_investment = 0.0;

    effects.inflation_rate = m_config.default_inflation_rate;
    effects.economic_growth = m_config.default_economic_growth;

    return effects;
}

PopulationEconomicContribution EconomicPopulationBridge::CalculatePopulationContributions(game::types::EntityID entity_id) {
    PopulationEconomicContribution contributions;

    if (!m_entity_manager) return contributions;

    auto pop_comp = m_entity_manager->GetComponent<game::population::PopulationComponent>(core::ecs::EntityID(entity_id, 1));
    if (!pop_comp) return contributions;

    contributions.total_workers = pop_comp->total_population * pop_comp->overall_employment_rate;
    contributions.skilled_worker_ratio = pop_comp->average_literacy;
    contributions.literacy_rate = pop_comp->average_literacy;

    contributions.taxable_population = pop_comp->total_population * m_config.taxable_population_ratio;
    contributions.tax_collection_efficiency = CalculateTaxCollectionEfficiency(
        pop_comp->average_literacy, pop_comp->average_happiness);

    contributions.consumer_spending = pop_comp->total_population * pop_comp->average_wealth * 
        m_config.consumer_spending_multiplier;
    
    double excess_wealth = std::max(0.0, pop_comp->average_wealth - m_config.luxury_wealth_threshold);
    contributions.luxury_demand = pop_comp->total_population * excess_wealth * m_config.luxury_demand_multiplier;

    contributions.innovation_factor = CalculateLiteracyProductivityBonus(pop_comp->average_literacy);
    contributions.productivity_modifier = CalculateHappinessProductivityBonus(pop_comp->average_happiness);

    return contributions;
}

void EconomicPopulationBridge::ApplyEconomicEffectsToPopulation(game::types::EntityID entity_id,
    const EconomicPopulationEffects& effects) {
    if (!m_entity_manager) return;

    auto pop_comp = m_entity_manager->GetComponent<game::population::PopulationComponent>(core::ecs::EntityID(entity_id, 1));
    if (!pop_comp) return;

    double tax_happiness_change = effects.tax_happiness_modifier * m_config.tax_happiness_scaling;
    pop_comp->average_happiness = std::max(0.0, std::min(1.0,
        pop_comp->average_happiness + tax_happiness_change));

    double employment_happiness_effect = CalculateEmploymentHappinessEffect(
        effects.employment_rate, effects.average_wages);
    pop_comp->average_happiness = std::max(0.0, std::min(1.0,
        pop_comp->average_happiness + employment_happiness_effect * m_config.unemployment_happiness_penalty));

    double inequality_effect = CalculateWealthInequalityEffect(
        effects.wealth_inequality, pop_comp->average_wealth);
    pop_comp->average_happiness = std::max(0.0, std::min(1.0,
        pop_comp->average_happiness + inequality_effect * m_config.inequality_happiness_penalty));

    if (effects.infrastructure_quality > m_config.infrastructure_good_threshold) {
        double capacity_bonus = (effects.infrastructure_quality - m_config.infrastructure_good_threshold) * 
            m_config.infrastructure_capacity_bonus;
    }

    if (effects.trade_income_per_capita > 0) {
        double wealth_increase = effects.trade_income_per_capita * m_config.wealth_increase_trade_multiplier;
        pop_comp->average_wealth = std::min(m_config.wealth_normalization, 
            pop_comp->average_wealth + wealth_increase);
    }
}

void EconomicPopulationBridge::ApplyPopulationContributionsToEconomy(game::types::EntityID entity_id,
    const PopulationEconomicContribution& contributions) {
    if (!m_economic_system) return;

    double base_tax_per_person = game::config::GameConfig::Instance().GetDouble(
        "economic_bridge.base_tax_per_person", 10.0);
    double base_tax_income = contributions.taxable_population * base_tax_per_person;
    double actual_tax_income = base_tax_income * contributions.tax_collection_efficiency;

    double productivity_multiplier = 1.0 + contributions.innovation_factor + contributions.productivity_modifier;
    double enhanced_economic_output = actual_tax_income * productivity_multiplier;

    m_economic_system->AddMoney(entity_id, static_cast<int>(enhanced_economic_output));

    CORE_STREAM_INFO("EconomicPopulationBridge") << "Entity " << entity_id << " contributed " << enhanced_economic_output
        << " to treasury (productivity multiplier: " << productivity_multiplier << ")" << std::endl;
}

// ============================================================================
// System Configuration
// ============================================================================

void EconomicPopulationBridge::SetEconomicSystem(game::economy::EconomicSystem* economic_system) {
    m_economic_system = economic_system;
}

// LINE 500 - CONTINUE IN PART 2
void EconomicPopulationBridge::ProcessCrisisDetection(game::types::EntityID entity_id) {
    if (!m_entity_manager || !m_message_bus) return;

    auto bridge_comp = m_entity_manager->GetComponent<EconomicPopulationBridgeComponent>(core::ecs::EntityID(entity_id, 1));
    if (!bridge_comp) return;

    bool economic_crisis = DetectEconomicCrisis(*bridge_comp);
    if (economic_crisis && !bridge_comp->economic_crisis) {
        bridge_comp->economic_crisis = true;

        EconomicCrisisEvent crisis_event;
        crisis_event.affected_entity = entity_id;
        crisis_event.crisis_severity = bridge_comp->crisis_severity;
        crisis_event.crisis_type = "economic_downturn";
        crisis_event.contributing_factors = { "low_productivity", "tax_inefficiency" };

        m_message_bus->Publish(crisis_event);
        CORE_STREAM_INFO("EconomicPopulationBridge") << "Economic crisis detected for entity " << entity_id << std::endl;
    }

    bool population_crisis = DetectPopulationCrisis(*bridge_comp);
    if (population_crisis && !bridge_comp->population_unrest) {
        bridge_comp->population_unrest = true;

        PopulationUnrestEvent unrest_event;
        unrest_event.affected_entity = entity_id;
        unrest_event.unrest_level = 1.0 - bridge_comp->economic_effects.tax_happiness_modifier;
        unrest_event.primary_cause = "high_taxation";
        
        double affected_percentage = game::config::GameConfig::Instance().GetDouble(
            "economic_bridge.unrest_affected_percentage", 0.6);
        unrest_event.affected_population_percentage = affected_percentage;

        m_message_bus->Publish(unrest_event);
        CORE_STREAM_INFO("EconomicPopulationBridge") << "Population unrest detected for entity " << entity_id << std::endl;
    }

    if (economic_crisis || population_crisis) {
        bridge_comp->crisis_severity = std::min(1.0, 
            bridge_comp->crisis_severity + m_config.crisis_severity_increase);
    }
    else {
        bridge_comp->crisis_severity = std::max(0.0, 
            bridge_comp->crisis_severity - m_config.crisis_severity_decrease);

        if (bridge_comp->crisis_severity < m_config.crisis_reset_threshold) {
            bridge_comp->economic_crisis = false;
            bridge_comp->population_unrest = false;
        }
    }
}

// ============================================================================
// Internal Calculation Helpers
// ============================================================================

double EconomicPopulationBridge::CalculateTaxHappinessEffect(double tax_rate, double base_happiness) const {
    double base_effect = m_config.tax_happiness_base_effect;
    double scaling_effect = tax_rate * m_config.tax_happiness_scaling;
    double min_happiness_factor = game::config::GameConfig::Instance().GetDouble(
        "economic_bridge.min_happiness_factor", 0.1);
    double happiness_factor = std::max(min_happiness_factor, base_happiness);

    return (base_effect + scaling_effect) * happiness_factor;
}

double EconomicPopulationBridge::CalculateEmploymentHappinessEffect(double employment_rate, double wages) const {
    double unemployment_penalty = (1.0 - employment_rate) * m_config.unemployment_happiness_penalty;
    double wage_bonus = (wages / m_config.wealth_normalization) * m_config.wage_happiness_scaling;

    return unemployment_penalty + wage_bonus;
}

double EconomicPopulationBridge::CalculateWealthInequalityEffect(double inequality, double average_wealth) const {
    if (inequality < m_config.inequality_threshold) {
        return 0.0;
    }

    double excess_inequality = inequality - m_config.inequality_threshold;
    double wealth_factor = 1.0 - (average_wealth / m_config.wealth_normalization);

    return -excess_inequality * m_config.inequality_happiness_penalty * wealth_factor;
}

double EconomicPopulationBridge::CalculateLiteracyProductivityBonus(double literacy_rate) const {
    return literacy_rate * m_config.literacy_productivity_bonus;
}

double EconomicPopulationBridge::CalculateHappinessProductivityBonus(double happiness_level) const {
    double happiness_above_baseline = std::max(0.0, happiness_level - m_config.happiness_baseline);
    return happiness_above_baseline * m_config.happiness_productivity_scaling;
}

double EconomicPopulationBridge::CalculateTaxCollectionEfficiency(double literacy_rate, double happiness_level) const {
    double literacy_factor = m_config.tax_collection_literacy_base + 
        (literacy_rate * m_config.tax_collection_literacy_bonus);
    double happiness_factor = m_config.tax_collection_happiness_base + 
        (happiness_level * m_config.tax_collection_happiness_bonus);

    return std::min(1.0, literacy_factor * happiness_factor);
}

// ============================================================================
// Crisis Detection Helpers
// ============================================================================

bool EconomicPopulationBridge::DetectEconomicCrisis(const EconomicPopulationBridgeComponent& bridge_comp) const {
    bool low_output = bridge_comp.population_contributions.productivity_modifier <
        m_config.economic_output_crisis_threshold;
    bool poor_tax_efficiency = bridge_comp.population_contributions.tax_collection_efficiency < 
        m_config.tax_efficiency_crisis_threshold;
    bool high_unemployment = bridge_comp.economic_effects.employment_rate < 
        m_config.employment_crisis_threshold;

    return low_output || poor_tax_efficiency || high_unemployment;
}

bool EconomicPopulationBridge::DetectPopulationCrisis(const EconomicPopulationBridgeComponent& bridge_comp) const {
    bool low_happiness = false;
    if (!bridge_comp.happiness_history.empty()) {
        double recent_happiness = bridge_comp.happiness_history.back();
        low_happiness = recent_happiness < m_config.happiness_crisis_threshold;
    }

    bool declining_trend = false;
    int min_trend_points = game::config::GameConfig::Instance().GetInt(
        "economic_bridge.min_trend_points", 3);
    if (bridge_comp.happiness_history.size() >= static_cast<size_t>(min_trend_points)) {
        auto& history = bridge_comp.happiness_history;
        size_t n = history.size();
        declining_trend = (history[n - 1] < history[n - 2]) && (history[n - 2] < history[n - 3]);
    }

    return low_happiness || declining_trend;
}

// ============================================================================
// Public Interface Methods
// ============================================================================

EconomicPopulationBridge::BridgeHealthMetrics
EconomicPopulationBridge::GetBridgeHealth(game::types::EntityID entity_id) const {
    BridgeHealthMetrics metrics;

    if (!m_entity_manager) return metrics;

    auto bridge_comp = m_entity_manager->GetComponent<EconomicPopulationBridgeComponent>(core::ecs::EntityID(entity_id, 1));
    if (!bridge_comp) return metrics;

    metrics.economic_population_balance = bridge_comp->economic_population_balance;
    metrics.crisis_active = bridge_comp->economic_crisis || bridge_comp->population_unrest;
    metrics.crisis_severity = bridge_comp->crisis_severity;

    double min_trend_threshold = game::config::GameConfig::Instance().GetDouble(
        "economic_bridge.trend_threshold", 0.1);

    if (bridge_comp->happiness_history.size() >= 2) {
        auto& history = bridge_comp->happiness_history;
        metrics.happiness_trend = history.back() - history[history.size() - 2];
    }

    if (bridge_comp->economic_output_history.size() >= 2) {
        auto& history = bridge_comp->economic_output_history;
        metrics.economic_output_trend = history.back() - history[history.size() - 2];
    }

    if (bridge_comp->economic_crisis) {
        metrics.primary_issue = "Economic productivity crisis";
    }
    else if (bridge_comp->population_unrest) {
        metrics.primary_issue = "Population happiness crisis";
    }
    else if (metrics.happiness_trend < -min_trend_threshold) {
        metrics.primary_issue = "Declining population happiness";
    }
    else if (metrics.economic_output_trend < -min_trend_threshold) {
        metrics.primary_issue = "Declining economic output";
    }
    else {
        metrics.primary_issue = "Stable";
    }

    return metrics;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

void EconomicPopulationBridge::UpdateEntityBridge(game::types::EntityID entity_id,
    EconomicPopulationBridgeComponent& bridge_comp,
    double delta_time) {

    auto economic_effects = CalculateEconomicEffects(entity_id);
    auto population_contributions = CalculatePopulationContributions(entity_id);

    bridge_comp.economic_effects = economic_effects;
    bridge_comp.population_contributions = population_contributions;

    ApplyEconomicEffectsToPopulation(entity_id, economic_effects);
    ApplyPopulationContributionsToEconomy(entity_id, population_contributions);

    if (m_entity_manager) {
        auto pop_comp = m_entity_manager->GetComponent<game::population::PopulationComponent>(core::ecs::EntityID(entity_id, 1));
        if (pop_comp) {
            UpdateHistoricalData(bridge_comp, pop_comp->average_happiness,
                population_contributions.productivity_modifier);
        }
    }

    double happiness_score = economic_effects.tax_happiness_modifier + m_config.happiness_baseline;
    double productivity_score = population_contributions.productivity_modifier;
    bridge_comp.economic_population_balance = (happiness_score + productivity_score) / 2.0;

    ProcessCrisisDetection(entity_id);
}

void EconomicPopulationBridge::UpdateHistoricalData(EconomicPopulationBridgeComponent& bridge_comp,
    double happiness, double economic_output) {
    bridge_comp.happiness_history.push_back(happiness);
    if (bridge_comp.happiness_history.size() > static_cast<size_t>(m_config.max_history_size)) {
        bridge_comp.happiness_history.erase(bridge_comp.happiness_history.begin());
    }

    bridge_comp.economic_output_history.push_back(economic_output);
    if (bridge_comp.economic_output_history.size() > static_cast<size_t>(m_config.max_history_size)) {
        bridge_comp.economic_output_history.erase(bridge_comp.economic_output_history.begin());
    }
}

void EconomicPopulationBridge::LogPerformanceMetrics() {
    double current_time = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    double last_log_time = m_last_performance_log.load();
    if (current_time - last_log_time > m_config.performance_log_interval) {
        int updates = m_updates_this_frame.exchange(0);

        CORE_STREAM_INFO("EconomicPopulationBridge") << "Economic-Population Bridge Performance: "
            << updates << " updates in last " << m_config.performance_log_interval << " seconds" << std::endl;

        m_last_performance_log.store(current_time);
    }
}

} // namespace integration
} // namespace mechanica
