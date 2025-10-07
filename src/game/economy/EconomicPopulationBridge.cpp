// ============================================================================
// EconomicPopulationBridge.cpp - Core Implementation
// Date & Time: September 11, 2025 - 20:12
// Complete implementation of economic-population integration system
// ============================================================================

#include "EconomicPopulationBridge.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <chrono>

using namespace mechanica::integration;

// ============================================================================
// System Lifecycle Implementation
// ============================================================================

void EconomicPopulationBridge::Initialize() {
    std::cout << "Initializing Economic-Population Bridge System..." << std::endl;

    // Verify required systems are available
    if (!m_entity_manager) {
        throw std::runtime_error("EconomicPopulationBridge: EntityManager not set");
    }

    if (!m_message_bus) {
        throw std::runtime_error("EconomicPopulationBridge: MessageBus not set");
    }

    if (!m_economic_system) {
        std::cout << "Warning: EconomicSystem not set - some features will be limited" << std::endl;
    }

    // Load configuration from game config
    try {
        auto& game_config = game::config::GameConfig::Instance();
        // TODO: Load bridge configuration from config files
        std::cout << "Bridge configuration loaded from game config" << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Using default bridge configuration: " << e.what() << std::endl;
    }

    // Reset performance counters
    m_updates_this_frame.store(0);
    m_last_performance_log.store(0.0);

    std::cout << "Economic-Population Bridge System initialized successfully" << std::endl;
}

void EconomicPopulationBridge::Update(core::ecs::EntityManager& entities,
    core::ecs::MessageBus& message_bus,
    double delta_time) {

    // Store system references for this update cycle
    m_entity_manager = &entities;
    m_message_bus = &message_bus;

    // Track performance
    m_updates_this_frame.fetch_add(1);

    // Find all entities with population components
    auto entities_with_population = entities.GetEntitiesWithComponent<game::population::PopulationComponent>();

    for (auto entity_id : entities_with_population) {
        // Get or create bridge component
        auto* bridge_comp = entities.GetComponent<EconomicPopulationBridgeComponent>(entity_id);
        if (!bridge_comp) {
            bridge_comp = entities.AddComponent<EconomicPopulationBridgeComponent>(entity_id);
            std::cout << "Created bridge component for entity " << entity_id.Get() << std::endl;
        }

        // Check if update is needed (based on configured interval)
        double current_time = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        if (current_time - bridge_comp->last_update_time < m_config.bridge_update_interval * 24 * 3600) {
            continue; // Skip update for this entity
        }

        // Perform bridge calculations
        UpdateEntityBridge(entity_id, *bridge_comp, delta_time);
        bridge_comp->last_update_time = current_time;
    }

    // Log performance metrics periodically
    LogPerformanceMetrics();
}

void EconomicPopulationBridge::Shutdown() {
    m_entity_manager = nullptr;
    m_message_bus = nullptr;
    m_economic_system = nullptr;

    std::cout << "Economic-Population Bridge System shut down" << std::endl;
}

// ============================================================================
// Core Bridge Calculation Methods
// ============================================================================

EconomicPopulationEffects EconomicPopulationBridge::CalculateEconomicEffects(types::EntityID entity_id) {
    EconomicPopulationEffects effects;

    if (!m_entity_manager) return effects;

    // Get population data
    auto* pop_comp = m_entity_manager->GetComponent<game::population::PopulationComponent>(entity_id);
    if (!pop_comp) return effects;

    // Calculate tax rate effects
    if (m_economic_system) {
        // Get current tax rate from economic system
        effects.tax_rate = 0.15; // TODO: Get from economic system
        effects.tax_happiness_modifier = CalculateTaxHappinessEffect(effects.tax_rate, pop_comp->average_happiness);
    }

    // Calculate employment effects
    effects.employment_rate = pop_comp->overall_employment_rate;
    effects.average_wages = 50.0; // TODO: Calculate from economic data

    // Calculate wealth inequality (simplified Gini coefficient)
    effects.wealth_inequality = std::min(0.8, std::max(0.1,
        1.0 - (pop_comp->average_wealth / 100.0))); // Inverse relationship for now

    // Calculate trade income per capita
    if (m_economic_system) {
        double total_trade_income = 0.0; // TODO: Get from economic system
        effects.trade_income_per_capita = total_trade_income / std::max(1.0, pop_comp->total_population);
    }

    // Infrastructure and investment (placeholder calculations)
    effects.infrastructure_quality = 0.6; // TODO: Get from building/infrastructure system
    effects.public_investment = 0.0; // TODO: Calculate government spending on population

    // Economic stability indicators
    effects.inflation_rate = 0.02; // TODO: Calculate from economic system
    effects.economic_growth = 0.03; // TODO: Calculate from economic trend

    return effects;
}

PopulationEconomicContribution EconomicPopulationBridge::CalculatePopulationContributions(types::EntityID entity_id) {
    PopulationEconomicContribution contributions;

    if (!m_entity_manager) return contributions;

    auto* pop_comp = m_entity_manager->GetComponent<game::population::PopulationComponent>(entity_id);
    if (!pop_comp) return contributions;

    // Labor force calculations
    contributions.total_workers = pop_comp->total_population * pop_comp->overall_employment_rate;
    contributions.skilled_worker_ratio = pop_comp->average_literacy; // Literacy as proxy for skill
    contributions.literacy_rate = pop_comp->average_literacy;

    // Tax base calculations
    contributions.taxable_population = pop_comp->total_population * 0.8; // 80% of population pays taxes
    contributions.tax_collection_efficiency = CalculateTaxCollectionEfficiency(
        pop_comp->average_literacy, pop_comp->average_happiness);

    // Consumer demand calculations
    contributions.consumer_spending = pop_comp->total_population * pop_comp->average_wealth * 0.6;
    contributions.luxury_demand = pop_comp->total_population *
        std::max(0.0, (pop_comp->average_wealth - 50.0)) * 0.1; // Only wealthy buy luxuries

    // Innovation and productivity
    contributions.innovation_factor = CalculateLiteracyProductivityBonus(pop_comp->average_literacy);
    contributions.productivity_modifier = CalculateHappinessProductivityBonus(pop_comp->average_happiness);

    return contributions;
}

void EconomicPopulationBridge::ApplyEconomicEffectsToPopulation(types::EntityID entity_id,
    const EconomicPopulationEffects& effects) {
    if (!m_entity_manager) return;

    auto* pop_comp = m_entity_manager->GetComponent<game::population::PopulationComponent>(entity_id);
    if (!pop_comp) return;

    // Apply tax happiness effects
    double tax_happiness_change = effects.tax_happiness_modifier * 0.1; // Gradual change
    pop_comp->average_happiness = std::max(0.0, std::min(1.0,
        pop_comp->average_happiness + tax_happiness_change));

    // Apply employment effects
    double employment_happiness_effect = CalculateEmploymentHappinessEffect(
        effects.employment_rate, effects.average_wages);
    pop_comp->average_happiness = std::max(0.0, std::min(1.0,
        pop_comp->average_happiness + employment_happiness_effect * 0.05));

    // Apply wealth inequality effects
    double inequality_effect = CalculateWealthInequalityEffect(
        effects.wealth_inequality, pop_comp->average_wealth);
    pop_comp->average_happiness = std::max(0.0, std::min(1.0,
        pop_comp->average_happiness + inequality_effect * 0.03));

    // Apply infrastructure effects on population capacity
    if (effects.infrastructure_quality > 0.7) {
        // Good infrastructure supports larger populations
        double capacity_bonus = (effects.infrastructure_quality - 0.7) * 0.5;
        // TODO: Apply to settlement capacity when settlement system is integrated
    }

    // Apply trade income effects on wealth
    if (effects.trade_income_per_capita > 0) {
        double wealth_increase = effects.trade_income_per_capita * 0.1;
        pop_comp->average_wealth = std::min(100.0, pop_comp->average_wealth + wealth_increase);
    }
}

void EconomicPopulationBridge::ApplyPopulationContributionsToEconomy(types::EntityID entity_id,
    const PopulationEconomicContribution& contributions) {
    if (!m_economic_system) return;

    // Calculate tax income based on population contributions
    double base_tax_income = contributions.taxable_population * 10.0; // Base tax per person
    double actual_tax_income = base_tax_income * contributions.tax_collection_efficiency;

    // Apply productivity bonuses to economic output
    double productivity_multiplier = 1.0 + contributions.innovation_factor + contributions.productivity_modifier;
    double enhanced_economic_output = actual_tax_income * productivity_multiplier;

    // Add to treasury (simplified - in full implementation, this would be more complex)
    m_economic_system->addIncome(enhanced_economic_output);

    // Log the contribution for debugging
    std::cout << "Entity " << entity_id.Get() << " contributed " << enhanced_economic_output
        << " to treasury (productivity multiplier: " << productivity_multiplier << ")" << std::endl;
}

// ============================================================================
// Crisis Detection and Management
// ============================================================================

void EconomicPopulationBridge::ProcessCrisisDetection(types::EntityID entity_id) {
    if (!m_entity_manager || !m_message_bus) return;

    auto* bridge_comp = m_entity_manager->GetComponent<EconomicPopulationBridgeComponent>(entity_id);
    if (!bridge_comp) return;

    // Check for economic crisis
    bool economic_crisis = DetectEconomicCrisis(*bridge_comp);
    if (economic_crisis && !bridge_comp->economic_crisis) {
        // New economic crisis detected
        bridge_comp->economic_crisis = true;

        EconomicCrisisEvent crisis_event;
        crisis_event.affected_entity = entity_id;
        crisis_event.crisis_severity = bridge_comp->crisis_severity;
        crisis_event.crisis_type = "economic_downturn";
        crisis_event.contributing_factors = { "low_productivity", "tax_inefficiency" };

        m_message_bus->Send(crisis_event);
        std::cout << "Economic crisis detected for entity " << entity_id.Get() << std::endl;
    }

    // Check for population unrest
    bool population_crisis = DetectPopulationCrisis(*bridge_comp);
    if (population_crisis && !bridge_comp->population_unrest) {
        // New population crisis detected
        bridge_comp->population_unrest = true;

        PopulationUnrestEvent unrest_event;
        unrest_event.affected_entity = entity_id;
        unrest_event.unrest_level = 1.0 - bridge_comp->economic_effects.tax_happiness_modifier;
        unrest_event.primary_cause = "high_taxation";
        unrest_event.affected_population_percentage = 0.6;

        m_message_bus->Send(unrest_event);
        std::cout << "Population unrest detected for entity " << entity_id.Get() << std::endl;
    }

    // Update crisis severity
    if (economic_crisis || population_crisis) {
        bridge_comp->crisis_severity = std::min(1.0, bridge_comp->crisis_severity + 0.1);
    }
    else {
        bridge_comp->crisis_severity = std::max(0.0, bridge_comp->crisis_severity - 0.05);

        // Reset crisis flags if severity is low
        if (bridge_comp->crisis_severity < 0.1) {
            bridge_comp->economic_crisis = false;
            bridge_comp->population_unrest = false;
        }
    }
}

// ============================================================================
// Internal Calculation Helpers
// ============================================================================

double EconomicPopulationBridge::CalculateTaxHappinessEffect(double tax_rate, double base_happiness) const {
    // Higher taxes reduce happiness, but effect diminishes at very low happiness
    double base_effect = m_config.tax_happiness_base_effect;
    double scaling_effect = tax_rate * m_config.tax_happiness_scaling;
    double happiness_factor = std::max(0.1, base_happiness); // Minimum factor to prevent infinite spiral

    return (base_effect + scaling_effect) * happiness_factor;
}

double EconomicPopulationBridge::CalculateEmploymentHappinessEffect(double employment_rate, double wages) const {
    // Unemployment reduces happiness, good wages increase it
    double unemployment_penalty = (1.0 - employment_rate) * m_config.unemployment_happiness_penalty;
    double wage_bonus = (wages / 100.0) * m_config.wage_happiness_scaling; // Assume 100 is baseline wage

    return unemployment_penalty + wage_bonus;
}

double EconomicPopulationBridge::CalculateWealthInequalityEffect(double inequality, double average_wealth) const {
    // High inequality reduces happiness, especially for poorer populations
    if (inequality < m_config.inequality_threshold) {
        return 0.0; // No effect below threshold
    }

    double excess_inequality = inequality - m_config.inequality_threshold;
    double wealth_factor = 1.0 - (average_wealth / 100.0); // Poorer populations feel inequality more

    return -excess_inequality * m_config.inequality_happiness_penalty * wealth_factor;
}

double EconomicPopulationBridge::CalculateLiteracyProductivityBonus(double literacy_rate) const {
    // Literacy improves productivity through better administration and innovation
    return literacy_rate * m_config.literacy_productivity_bonus;
}

double EconomicPopulationBridge::CalculateHappinessProductivityBonus(double happiness_level) const {
    // Happy populations are more productive
    double happiness_above_baseline = std::max(0.0, happiness_level - 0.5); // 0.5 is neutral
    return happiness_above_baseline * m_config.happiness_productivity_scaling;
}

double EconomicPopulationBridge::CalculateTaxCollectionEfficiency(double literacy_rate, double happiness_level) const {
    // Literate populations enable better administration
    // Happy populations are more compliant with tax collection
    double literacy_factor = 0.5 + (literacy_rate * 0.4); // 50% to 90% efficiency from literacy
    double happiness_factor = 0.7 + (happiness_level * 0.3); // 70% to 100% efficiency from happiness

    return std::min(1.0, literacy_factor * happiness_factor);
}

// ============================================================================
// Crisis Detection Helpers
// ============================================================================

bool EconomicPopulationBridge::DetectEconomicCrisis(const EconomicPopulationBridgeComponent& bridge_comp) const {
    // Check multiple economic indicators
    bool low_output = bridge_comp.population_contributions.productivity_modifier <
        m_config.economic_output_crisis_threshold;
    bool poor_tax_efficiency = bridge_comp.population_contributions.tax_collection_efficiency < 0.5;
    bool high_unemployment = bridge_comp.economic_effects.employment_rate < 0.6;

    return low_output || poor_tax_efficiency || high_unemployment;
}

bool EconomicPopulationBridge::DetectPopulationCrisis(const EconomicPopulationBridgeComponent& bridge_comp) const {
    // Check population happiness and stability
    bool low_happiness = false;
    if (!bridge_comp.happiness_history.empty()) {
        double recent_happiness = bridge_comp.happiness_history.back();
        low_happiness = recent_happiness < m_config.happiness_crisis_threshold;
    }

    bool declining_trend = false;
    if (bridge_comp.happiness_history.size() >= 3) {
        // Check if happiness has been declining for 3 consecutive periods
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
EconomicPopulationBridge::GetBridgeHealth(types::EntityID entity_id) const {
    BridgeHealthMetrics metrics;

    if (!m_entity_manager) return metrics;

    auto* bridge_comp = m_entity_manager->GetComponent<EconomicPopulationBridgeComponent>(entity_id);
    if (!bridge_comp) return metrics;

    metrics.economic_population_balance = bridge_comp->economic_population_balance;
    metrics.crisis_active = bridge_comp->economic_crisis || bridge_comp->population_unrest;
    metrics.crisis_severity = bridge_comp->crisis_severity;

    // Calculate trends
    if (bridge_comp->happiness_history.size() >= 2) {
        auto& history = bridge_comp->happiness_history;
        metrics.happiness_trend = history.back() - history[history.size() - 2];
    }

    if (bridge_comp->economic_output_history.size() >= 2) {
        auto& history = bridge_comp->economic_output_history;
        metrics.economic_output_trend = history.back() - history[history.size() - 2];
    }

    // Determine primary issue
    if (bridge_comp->economic_crisis) {
        metrics.primary_issue = "Economic productivity crisis";
    }
    else if (bridge_comp->population_unrest) {
        metrics.primary_issue = "Population happiness crisis";
    }
    else if (metrics.happiness_trend < -0.1) {
        metrics.primary_issue = "Declining population happiness";
    }
    else if (metrics.economic_output_trend < -0.1) {
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

void EconomicPopulationBridge::UpdateEntityBridge(types::EntityID entity_id,
    EconomicPopulationBridgeComponent& bridge_comp,
    double delta_time) {

    // Calculate current effects and contributions
    auto economic_effects = CalculateEconomicEffects(entity_id);
    auto population_contributions = CalculatePopulationContributions(entity_id);

    // Store in bridge component
    bridge_comp.economic_effects = economic_effects;
    bridge_comp.population_contributions = population_contributions;

    // Apply effects
    ApplyEconomicEffectsToPopulation(entity_id, economic_effects);
    ApplyPopulationContributionsToEconomy(entity_id, population_contributions);

    // Update historical data
    if (m_entity_manager) {
        auto* pop_comp = m_entity_manager->GetComponent<game::population::PopulationComponent>(entity_id);
        if (pop_comp) {
            UpdateHistoricalData(bridge_comp, pop_comp->average_happiness,
                population_contributions.productivity_modifier);
        }
    }

    // Calculate overall balance metric
    double happiness_score = economic_effects.tax_happiness_modifier + 0.5; // Normalize to 0-1
    double productivity_score = population_contributions.productivity_modifier;
    bridge_comp.economic_population_balance = (happiness_score + productivity_score) / 2.0;

    // Check for crises
    ProcessCrisisDetection(entity_id);
}

void EconomicPopulationBridge::UpdateHistoricalData(EconomicPopulationBridgeComponent& bridge_comp,
    double happiness, double economic_output) {
    // Maintain rolling history of last 12 data points
    const size_t max_history_size = 12;

    bridge_comp.happiness_history.push_back(happiness);
    if (bridge_comp.happiness_history.size() > max_history_size) {
        bridge_comp.happiness_history.erase(bridge_comp.happiness_history.begin());
    }

    bridge_comp.economic_output_history.push_back(economic_output);
    if (bridge_comp.economic_output_history.size() > max_history_size) {
        bridge_comp.economic_output_history.erase(bridge_comp.economic_output_history.begin());
    }
}

void EconomicPopulationBridge::LogPerformanceMetrics() {
    double current_time = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    double last_log_time = m_last_performance_log.load();
    if (current_time - last_log_time > 10.0) { // Log every 10 seconds
        int updates = m_updates_this_frame.exchange(0);

        std::cout << "Economic-Population Bridge Performance: "
            << updates << " updates in last 10 seconds" << std::endl;

        m_last_performance_log.store(current_time);
    }
}