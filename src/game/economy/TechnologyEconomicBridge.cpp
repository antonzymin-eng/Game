// Created: October 31, 2025
// Location: src/game/economy/TechnologyEconomicBridge.cpp
// Technology-Economic Bridge System - Implementation

#include "../../../include/game/economy/TechnologyEconomicBridge.h"
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

TechnologyEconomicBridge::TechnologyEconomicBridge() {
    // Load configuration from GameConfig
    auto& config = game::config::GameConfig::Instance();

    m_config.bridge_update_interval = config.GetDouble("tech_economic_bridge.update_interval_days", 1.0);

    // Technology effect multipliers
    m_config.agricultural_tech_production_bonus = config.GetDouble("tech_economic_bridge.agricultural_production_bonus", 0.15);
    m_config.craft_tech_production_bonus = config.GetDouble("tech_economic_bridge.craft_production_bonus", 0.20);
    m_config.naval_tech_trade_bonus = config.GetDouble("tech_economic_bridge.naval_trade_bonus", 0.10);
    m_config.admin_tech_tax_bonus = config.GetDouble("tech_economic_bridge.admin_tax_bonus", 0.12);
    m_config.academic_tech_innovation_bonus = config.GetDouble("tech_economic_bridge.academic_innovation_bonus", 0.25);
    m_config.military_tech_maintenance_reduction = config.GetDouble("tech_economic_bridge.military_maintenance_reduction", 0.08);

    // Economic contribution parameters
    m_config.research_budget_base_percentage = config.GetDouble("tech_economic_bridge.research_budget_base", 0.05);
    m_config.research_budget_wealthy_bonus = config.GetDouble("tech_economic_bridge.research_budget_wealthy_bonus", 0.03);
    m_config.trade_knowledge_bonus_per_route = config.GetDouble("tech_economic_bridge.trade_knowledge_bonus", 0.02);
    m_config.stability_research_threshold = config.GetDouble("tech_economic_bridge.stability_threshold", 0.6);

    // Research infrastructure costs
    m_config.university_monthly_cost = config.GetDouble("tech_economic_bridge.university_cost", 50.0);
    m_config.library_monthly_cost = config.GetDouble("tech_economic_bridge.library_cost", 20.0);
    m_config.workshop_monthly_cost = config.GetDouble("tech_economic_bridge.workshop_cost", 30.0);
    m_config.scholar_salary = config.GetDouble("tech_economic_bridge.scholar_salary", 10.0);

    // Implementation costs
    m_config.implementation_cost_multiplier = config.GetDouble("tech_economic_bridge.implementation_cost_multiplier", 100.0);
    m_config.implementation_time_months = config.GetDouble("tech_economic_bridge.implementation_time_months", 12.0);

    // Crisis thresholds
    m_config.funding_crisis_threshold = config.GetDouble("tech_economic_bridge.funding_crisis_threshold", 0.3);
    m_config.implementation_crisis_threshold = config.GetDouble("tech_economic_bridge.implementation_crisis_threshold", 0.5);
    m_config.brain_drain_threshold = config.GetDouble("tech_economic_bridge.brain_drain_threshold", 0.4);

    // ROI calculation
    m_config.roi_calculation_period = config.GetDouble("tech_economic_bridge.roi_calculation_period", 12.0);
    m_config.min_roi_for_investment = config.GetDouble("tech_economic_bridge.min_roi_threshold", 0.15);

    // Crisis management
    m_config.crisis_severity_increase = config.GetDouble("tech_economic_bridge.crisis_severity_increase", 0.1);
    m_config.crisis_severity_decrease = config.GetDouble("tech_economic_bridge.crisis_severity_decrease", 0.05);
    m_config.crisis_reset_threshold = config.GetDouble("tech_economic_bridge.crisis_reset_threshold", 0.1);

    // Technology levels
    m_config.tech_level_primitive = config.GetInt("tech_economic_bridge.tech_level_primitive", 0);
    m_config.tech_level_early = config.GetInt("tech_economic_bridge.tech_level_early", 3);
    m_config.tech_level_intermediate = config.GetInt("tech_economic_bridge.tech_level_intermediate", 7);
    m_config.tech_level_advanced = config.GetInt("tech_economic_bridge.tech_level_advanced", 12);

    // History tracking
    m_config.max_history_size = config.GetInt("tech_economic_bridge.max_history_size", 12);
    m_config.performance_log_interval = config.GetDouble("tech_economic_bridge.performance_log_interval", 10.0);
}

// ============================================================================
// System Lifecycle Implementation
// ============================================================================

void TechnologyEconomicBridge::Initialize() {
    CORE_STREAM_INFO("TechnologyEconomicBridge") << "Initializing Technology-Economic Bridge System...";

    if (!m_entity_manager) {
        throw std::runtime_error("TechnologyEconomicBridge: EntityManager not set");
    }

    if (!m_message_bus) {
        throw std::runtime_error("TechnologyEconomicBridge: MessageBus not set");
    }

    if (!m_technology_system) {
        CORE_STREAM_INFO("TechnologyEconomicBridge") << "Warning: TechnologySystem not set - some features will be limited";
    }

    if (!m_economic_system) {
        CORE_STREAM_INFO("TechnologyEconomicBridge") << "Warning: EconomicSystem not set - some features will be limited";
    }

    m_updates_this_frame.store(0);
    m_last_performance_log.store(0.0);

    CORE_STREAM_INFO("TechnologyEconomicBridge") << "Technology-Economic Bridge System initialized successfully";
}

void TechnologyEconomicBridge::Update(core::ecs::EntityManager& entities,
    ::core::threading::ThreadSafeMessageBus& message_bus,
    double delta_time) {

    m_entity_manager = &entities;
    m_message_bus = &message_bus;

    m_updates_this_frame.fetch_add(1);

    // Get all entities with research component (provinces that can research)
    auto entities_with_research = entities.GetEntitiesWithComponent<game::technology::ResearchComponent>();

    for (auto entity_id : entities_with_research) {
        auto bridge_comp = entities.GetComponent<TechnologyEconomicBridgeComponent>(entity_id);
        if (!bridge_comp) {
            bridge_comp = entities.AddComponent<TechnologyEconomicBridgeComponent>(entity_id);
            CORE_STREAM_INFO("TechnologyEconomicBridge") << "Created tech-economic bridge component for entity " << entity_id.id;
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

void TechnologyEconomicBridge::Shutdown() {
    m_entity_manager = nullptr;
    m_message_bus = nullptr;
    m_technology_system = nullptr;
    m_economic_system = nullptr;

    CORE_STREAM_INFO("TechnologyEconomicBridge") << "Technology-Economic Bridge System shut down";
}

// ============================================================================
// Threading Interface
// ============================================================================

core::threading::ThreadingStrategy TechnologyEconomicBridge::GetThreadingStrategy() const {
    return core::threading::ThreadingStrategy::THREAD_POOL;
}

// ============================================================================
// Serialization Interface
// ============================================================================

Json::Value TechnologyEconomicBridge::Serialize(int version) const {
    Json::Value root;
    root["system_name"] = GetSystemName();
    root["version"] = version;
    return root;
}

bool TechnologyEconomicBridge::Deserialize(const Json::Value& data, int version) {
    return true;
}

std::string TechnologyEconomicBridge::GetSystemName() const {
    return "TechnologyEconomicBridge";
}

// ============================================================================
// Core Bridge Calculation Methods
// ============================================================================

TechnologyEconomicEffects TechnologyEconomicBridge::CalculateTechnologyEffects(game::types::EntityID entity_id) {
    TechnologyEconomicEffects effects;

    if (!m_entity_manager) return effects;

    auto research_comp = m_entity_manager->GetComponent<game::technology::ResearchComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!research_comp) return effects;

    // Calculate production efficiency from Agricultural and Craft technologies
    effects.production_efficiency = CalculateProductionEfficiency(entity_id);

    // Calculate trade efficiency from Naval and Craft technologies
    effects.trade_efficiency = CalculateTradeEfficiency(entity_id);

    // Calculate tax efficiency from Administrative technologies
    effects.tax_efficiency = CalculateTaxEfficiency(entity_id);

    // Calculate infrastructure multiplier from all tech categories
    effects.infrastructure_multiplier = CalculateInfrastructureMultiplier(entity_id);

    // Calculate market sophistication from Academic and Craft tech
    effects.market_sophistication = CalculateMarketSophistication(entity_id);

    // Calculate knowledge transmission rate
    effects.knowledge_transmission_rate = CalculateKnowledgeTransmission(entity_id);

    // Calculate military-economic effects
    int military_tech_level = GetTechnologyLevel(entity_id, game::technology::TechnologyCategory::MILITARY);
    effects.military_maintenance_efficiency = 1.0 + (military_tech_level * m_config.military_tech_maintenance_reduction);
    effects.fortification_cost_modifier = 1.0 - (military_tech_level * 0.05);  // 5% reduction per level

    // Calculate implementation costs
    int overall_tech_level = GetOverallTechnologyLevel(entity_id);
    effects.total_implementation_cost = overall_tech_level * m_config.implementation_cost_multiplier;

    // Monthly research cost = infrastructure maintenance
    effects.monthly_research_cost =
        (research_comp->universities * m_config.university_monthly_cost) +
        (research_comp->libraries * m_config.library_monthly_cost) +
        (research_comp->workshops * m_config.workshop_monthly_cost);

    effects.infrastructure_upgrade_cost = effects.total_implementation_cost * 0.1;  // 10% of implementation cost

    return effects;
}

EconomicTechnologyContribution TechnologyEconomicBridge::CalculateEconomicContributions(game::types::EntityID entity_id) {
    EconomicTechnologyContribution contributions;

    if (!m_entity_manager) return contributions;

    contributions.research_budget = CalculateResearchBudget(entity_id);
    contributions.research_infrastructure_count = CalculateResearchInfrastructure(entity_id);
    contributions.trade_network_bonus = CalculateTradeNetworkBonus(entity_id);
    contributions.wealth_innovation_bonus = CalculateWealthInnovationBonus(entity_id);
    contributions.economic_stability_modifier = CalculateEconomicStability(entity_id);

    // Calculate research capacity
    contributions.total_research_capacity = contributions.research_budget *
        contributions.economic_stability_modifier *
        (1.0 + contributions.trade_network_bonus);

    // Calculate infrastructure quality (0-1 scale)
    contributions.research_infrastructure_quality = std::min(1.0,
        contributions.research_infrastructure_count / 10.0);

    // Calculate funding for scholars and workshops
    contributions.scholar_funding = contributions.research_budget * 0.4;  // 40% goes to scholars
    contributions.workshop_funding = contributions.research_budget * 0.3;  // 30% goes to workshops
    contributions.manuscript_production = contributions.research_budget * 0.2;  // 20% for books

    // Calculate investment capacity
    contributions.infrastructure_investment = contributions.research_budget * 0.1;  // 10% for infrastructure
    contributions.innovation_investment = contributions.scholar_funding + contributions.workshop_funding;

    // Calculate research budget percentage
    if (m_economic_system) {
        auto econ_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(
            core::ecs::EntityID(entity_id, 1));
        if (econ_comp && econ_comp->treasury > 0) {
            double total_income = econ_comp->tax_income + econ_comp->trade_income;
            if (total_income > 0) {
                contributions.research_budget_percentage = contributions.research_budget / total_income;
            }
        }
    }

    return contributions;
}

void TechnologyEconomicBridge::ApplyTechnologyEffectsToEconomy(game::types::EntityID entity_id,
    const TechnologyEconomicEffects& effects) {
    if (!m_entity_manager || !m_economic_system) return;

    auto econ_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!econ_comp) return;

    // Apply production efficiency to resource production
    for (auto& [resource_type, production] : econ_comp->resource_production) {
        production *= effects.production_efficiency;
    }

    // Apply trade efficiency
    econ_comp->trade_income *= effects.trade_efficiency;

    // Apply tax efficiency
    econ_comp->tax_collection_efficiency *= effects.tax_efficiency;

    // Apply infrastructure multiplier
    econ_comp->infrastructure_quality *= effects.infrastructure_multiplier;

    // Deduct monthly research costs from treasury
    if (effects.monthly_research_cost > 0) {
        m_economic_system->SpendMoney(entity_id, static_cast<int>(effects.monthly_research_cost));
    }

    CORE_STREAM_INFO("TechnologyEconomicBridge") << "Applied tech effects to entity " << entity_id
        << ": prod=" << effects.production_efficiency
        << ", trade=" << effects.trade_efficiency
        << ", tax=" << effects.tax_efficiency;
}

void TechnologyEconomicBridge::ApplyEconomicContributionsToTechnology(game::types::EntityID entity_id,
    const EconomicTechnologyContribution& contributions) {
    if (!m_entity_manager || !m_technology_system) return;

    auto research_comp = m_entity_manager->GetComponent<game::technology::ResearchComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!research_comp) return;

    // Apply research budget to monthly research allocations
    double total_budget = contributions.research_budget;
    if (total_budget > 0) {
        // Update the overall monthly research budget
        research_comp->monthly_research_budget = total_budget;

        // Distribute budget across categories based on current priorities
        double total_priority = 0.0;
        for (const auto& [category, budget] : research_comp->category_investment) {
            total_priority += budget;
        }

        if (total_priority > 0) {
            for (auto& [category, budget] : research_comp->category_investment) {
                double category_percentage = budget / total_priority;
                budget = total_budget * category_percentage;
            }
        }
    }

    // Apply trade network bonus to research efficiency
    research_comp->trade_network_bonus = contributions.trade_network_bonus;

    // Apply economic stability to research progress
    research_comp->stability_bonus =
        (contributions.economic_stability_modifier - 1.0) * 0.5;  // Half the stability effect

    CORE_STREAM_INFO("TechnologyEconomicBridge") << "Applied economic contributions to entity " << entity_id
        << ": budget=" << contributions.research_budget
        << ", trade_bonus=" << contributions.trade_network_bonus;
}

// ============================================================================
// System Configuration
// ============================================================================

void TechnologyEconomicBridge::SetTechnologySystem(game::technology::TechnologySystem* tech_system) {
    m_technology_system = tech_system;
}

void TechnologyEconomicBridge::SetEconomicSystem(game::economy::EconomicSystem* economic_system) {
    m_economic_system = economic_system;
}

// ============================================================================
// Crisis Detection
// ============================================================================

void TechnologyEconomicBridge::ProcessCrisisDetection(game::types::EntityID entity_id) {
    if (!m_entity_manager || !m_message_bus) return;

    auto bridge_comp = m_entity_manager->GetComponent<TechnologyEconomicBridgeComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!bridge_comp) return;

    bool funding_crisis = DetectResearchFundingCrisis(*bridge_comp);
    if (funding_crisis && !bridge_comp->research_funding_crisis) {
        bridge_comp->research_funding_crisis = true;

        ResearchFundingCrisis crisis_event;
        crisis_event.affected_entity = entity_id;
        crisis_event.funding_shortfall = bridge_comp->technology_effects.monthly_research_cost -
            bridge_comp->economic_contributions.research_budget;
        crisis_event.research_slowdown = 1.0 - m_config.funding_crisis_threshold;
        crisis_event.crisis_cause = "insufficient_treasury";

        m_message_bus->Publish(crisis_event);
        CORE_STREAM_INFO("TechnologyEconomicBridge") << "Research funding crisis detected for entity " << entity_id;
    }

    bool implementation_crisis = DetectImplementationCrisis(*bridge_comp);
    if (implementation_crisis && !bridge_comp->implementation_cost_crisis) {
        bridge_comp->implementation_cost_crisis = true;
        CORE_STREAM_INFO("TechnologyEconomicBridge") << "Technology implementation crisis detected for entity " << entity_id;
    }

    bool brain_drain = DetectBrainDrain(*bridge_comp);
    if (brain_drain && !bridge_comp->brain_drain_active) {
        bridge_comp->brain_drain_active = true;

        BrainDrainEvent drain_event;
        drain_event.affected_entity = entity_id;
        drain_event.scholars_lost = 5;  // Simplified
        drain_event.inventors_lost = 2;
        drain_event.innovation_rate_penalty = 0.3;
        drain_event.cause = "poor_funding";

        m_message_bus->Publish(drain_event);
        CORE_STREAM_INFO("TechnologyEconomicBridge") << "Brain drain event detected for entity " << entity_id;
    }

    if (funding_crisis || implementation_crisis || brain_drain) {
        bridge_comp->crisis_severity = std::min(1.0,
            bridge_comp->crisis_severity + m_config.crisis_severity_increase);
    }
    else {
        bridge_comp->crisis_severity = std::max(0.0,
            bridge_comp->crisis_severity - m_config.crisis_severity_decrease);

        if (bridge_comp->crisis_severity < m_config.crisis_reset_threshold) {
            bridge_comp->research_funding_crisis = false;
            bridge_comp->implementation_cost_crisis = false;
            bridge_comp->brain_drain_active = false;
        }
    }
}

// ============================================================================
// Internal Calculation Helpers - Technology Effects
// ============================================================================

double TechnologyEconomicBridge::CalculateProductionEfficiency(game::types::EntityID entity_id) const {
    int agricultural_level = GetTechnologyLevel(entity_id, game::technology::TechnologyCategory::AGRICULTURAL);
    int craft_level = GetTechnologyLevel(entity_id, game::technology::TechnologyCategory::CRAFT);

    double agricultural_bonus = agricultural_level * m_config.agricultural_tech_production_bonus;
    double craft_bonus = craft_level * m_config.craft_tech_production_bonus;

    return 1.0 + agricultural_bonus + craft_bonus;
}

double TechnologyEconomicBridge::CalculateTradeEfficiency(game::types::EntityID entity_id) const {
    int naval_level = GetTechnologyLevel(entity_id, game::technology::TechnologyCategory::NAVAL);
    int craft_level = GetTechnologyLevel(entity_id, game::technology::TechnologyCategory::CRAFT);

    double naval_bonus = naval_level * m_config.naval_tech_trade_bonus;
    double craft_bonus = craft_level * 0.05;  // Craft tech helps trade goods quality

    return 1.0 + naval_bonus + craft_bonus;
}

double TechnologyEconomicBridge::CalculateTaxEfficiency(game::types::EntityID entity_id) const {
    int admin_level = GetTechnologyLevel(entity_id, game::technology::TechnologyCategory::ADMINISTRATIVE);

    double admin_bonus = admin_level * m_config.admin_tech_tax_bonus;

    return 1.0 + admin_bonus;
}

double TechnologyEconomicBridge::CalculateInfrastructureMultiplier(game::types::EntityID entity_id) const {
    int overall_level = GetOverallTechnologyLevel(entity_id);

    // All tech categories contribute to infrastructure
    return 1.0 + (overall_level * 0.03);  // 3% per overall tech level
}

double TechnologyEconomicBridge::CalculateMarketSophistication(game::types::EntityID entity_id) const {
    int academic_level = GetTechnologyLevel(entity_id, game::technology::TechnologyCategory::ACADEMIC);
    int craft_level = GetTechnologyLevel(entity_id, game::technology::TechnologyCategory::CRAFT);

    double sophistication = 0.3 + (academic_level * 0.05) + (craft_level * 0.03);
    return std::min(1.0, sophistication);
}

double TechnologyEconomicBridge::CalculateKnowledgeTransmission(game::types::EntityID entity_id) const {
    if (!m_entity_manager) return 0.5;

    auto knowledge_comp = m_entity_manager->GetComponent<game::technology::KnowledgeComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!knowledge_comp) return 0.5;

    return knowledge_comp->knowledge_transmission_rate;
}

// ============================================================================
// Internal Calculation Helpers - Economic Contributions
// ============================================================================

double TechnologyEconomicBridge::CalculateResearchBudget(game::types::EntityID entity_id) const {
    if (!m_entity_manager || !m_economic_system) return 0.0;

    auto econ_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!econ_comp) return 0.0;

    double total_income = econ_comp->tax_income + econ_comp->trade_income;
    double base_budget = total_income * m_config.research_budget_base_percentage;

    // Wealthy provinces invest more in research
    if (econ_comp->treasury > 5000) {
        base_budget += total_income * m_config.research_budget_wealthy_bonus;
    }

    return base_budget;
}

double TechnologyEconomicBridge::CalculateResearchInfrastructure(game::types::EntityID entity_id) const {
    if (!m_entity_manager) return 0.0;

    auto research_comp = m_entity_manager->GetComponent<game::technology::ResearchComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!research_comp) return 0.0;

    return research_comp->universities +
           research_comp->libraries +
           research_comp->workshops;
}

double TechnologyEconomicBridge::CalculateTradeNetworkBonus(game::types::EntityID entity_id) const {
    if (!m_entity_manager) return 0.0;

    auto trade_comp = m_entity_manager->GetComponent<game::economy::TradeComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!trade_comp) return 0.0;

    int total_routes = trade_comp->outgoing_routes.size() + trade_comp->incoming_routes.size();
    return total_routes * m_config.trade_knowledge_bonus_per_route;
}

double TechnologyEconomicBridge::CalculateWealthInnovationBonus(game::types::EntityID entity_id) const {
    if (!m_entity_manager) return 0.0;

    auto econ_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!econ_comp) return 0.0;

    // Wealthier provinces have more resources for innovation
    double wealth_factor = std::min(1.0, econ_comp->treasury / 10000.0);
    return wealth_factor * 0.5;  // Up to 50% bonus
}

double TechnologyEconomicBridge::CalculateEconomicStability(game::types::EntityID entity_id) const {
    if (!m_entity_manager) return 1.0;

    auto econ_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!econ_comp) return 1.0;

    // Stability based on economic indicators
    double stability = 1.0;

    // Negative effects from high inflation
    if (econ_comp->inflation_rate > 0.05) {
        stability -= 0.2;
    }

    // Negative effects from negative growth
    if (econ_comp->economic_growth < 0) {
        stability -= 0.3;
    }

    return std::max(0.5, stability);
}

// ============================================================================
// Technology Level Analysis
// ============================================================================

int TechnologyEconomicBridge::GetTechnologyLevel(game::types::EntityID entity_id,
    game::technology::TechnologyCategory category) const {
    if (!m_entity_manager) return 0;

    auto research_comp = m_entity_manager->GetComponent<game::technology::ResearchComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!research_comp) return 0;

    int count = 0;
    for (const auto& [tech_type, state] : research_comp->technology_states) {
        // Check if technology belongs to the category and is implemented
        if (state == game::technology::ResearchState::IMPLEMENTED) {
            // Simple heuristic: count implemented technologies in range
            int tech_id = static_cast<int>(tech_type);
            int category_base = static_cast<int>(category) * 100 + 1000;
            if (tech_id >= category_base && tech_id < category_base + 100) {
                count++;
            }
        }
    }

    return count;
}

int TechnologyEconomicBridge::GetOverallTechnologyLevel(game::types::EntityID entity_id) const {
    if (!m_entity_manager) return 0;

    auto research_comp = m_entity_manager->GetComponent<game::technology::ResearchComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!research_comp) return 0;

    int count = 0;
    for (const auto& [tech_type, state] : research_comp->technology_states) {
        if (state == game::technology::ResearchState::IMPLEMENTED) {
            count++;
        }
    }

    return count;
}

// ============================================================================
// Crisis Detection Helpers
// ============================================================================

bool TechnologyEconomicBridge::DetectResearchFundingCrisis(const TechnologyEconomicBridgeComponent& bridge_comp) const {
    double required_funding = bridge_comp.technology_effects.monthly_research_cost;
    double available_funding = bridge_comp.economic_contributions.research_budget;

    if (required_funding == 0) return false;

    double funding_ratio = available_funding / required_funding;
    return funding_ratio < m_config.funding_crisis_threshold;
}

bool TechnologyEconomicBridge::DetectImplementationCrisis(const TechnologyEconomicBridgeComponent& bridge_comp) const {
    double implementation_cost = bridge_comp.technology_effects.total_implementation_cost;
    double available_budget = bridge_comp.economic_contributions.research_budget * 12.0;  // Annual budget

    if (implementation_cost == 0) return false;

    double affordability = available_budget / implementation_cost;
    return affordability < m_config.implementation_crisis_threshold;
}

bool TechnologyEconomicBridge::DetectBrainDrain(const TechnologyEconomicBridgeComponent& bridge_comp) const {
    double scholar_funding = bridge_comp.economic_contributions.scholar_funding;
    double required_funding = bridge_comp.technology_effects.monthly_research_cost * 0.4;  // 40% should go to scholars

    if (required_funding == 0) return false;

    double funding_ratio = scholar_funding / required_funding;
    return funding_ratio < m_config.brain_drain_threshold;
}

// ============================================================================
// Update Helpers
// ============================================================================

void TechnologyEconomicBridge::UpdateEntityBridge(game::types::EntityID entity_id,
    TechnologyEconomicBridgeComponent& bridge_comp,
    double delta_time) {

    auto tech_effects = CalculateTechnologyEffects(entity_id);
    auto economic_contributions = CalculateEconomicContributions(entity_id);

    bridge_comp.technology_effects = tech_effects;
    bridge_comp.economic_contributions = economic_contributions;

    ApplyTechnologyEffectsToEconomy(entity_id, tech_effects);
    ApplyEconomicContributionsToTechnology(entity_id, economic_contributions);

    int tech_level = GetOverallTechnologyLevel(entity_id);
    UpdateHistoricalData(bridge_comp, tech_level,
        economic_contributions.research_budget,
        tech_effects.production_efficiency);

    CalculateROI(bridge_comp);

    double tech_score = std::min(1.0, tech_level / 20.0);  // Normalize to 0-1
    double economic_score = std::min(1.0, economic_contributions.research_budget / 500.0);
    bridge_comp.tech_economic_balance = (tech_score + economic_score) / 2.0;

    ProcessCrisisDetection(entity_id);
}

void TechnologyEconomicBridge::UpdateHistoricalData(TechnologyEconomicBridgeComponent& bridge_comp,
    double tech_level, double research_investment, double economic_impact) {
    bridge_comp.technology_level_history.push_back(tech_level);
    if (bridge_comp.technology_level_history.size() > static_cast<size_t>(m_config.max_history_size)) {
        bridge_comp.technology_level_history.pop_front();  // HIGH-008 FIX: O(1) instead of O(n)
    }

    bridge_comp.research_investment_history.push_back(research_investment);
    if (bridge_comp.research_investment_history.size() > static_cast<size_t>(m_config.max_history_size)) {
        bridge_comp.research_investment_history.pop_front();  // HIGH-008 FIX: O(1) instead of O(n)
    }

    bridge_comp.economic_impact_history.push_back(economic_impact);
    if (bridge_comp.economic_impact_history.size() > static_cast<size_t>(m_config.max_history_size)) {
        bridge_comp.economic_impact_history.pop_front();  // HIGH-008 FIX: O(1) instead of O(n)
    }
}

void TechnologyEconomicBridge::CalculateROI(TechnologyEconomicBridgeComponent& bridge_comp) const {
    if (bridge_comp.research_investment_history.empty() ||
        bridge_comp.economic_impact_history.empty()) {
        bridge_comp.research_roi = 0.0;
        return;
    }

    double total_investment = 0.0;
    for (double investment : bridge_comp.research_investment_history) {
        total_investment += investment;
    }

    double total_impact = 0.0;
    for (double impact : bridge_comp.economic_impact_history) {
        total_impact += (impact - 1.0);  // Impact is a multiplier, so subtract baseline
    }

    if (total_investment > 0) {
        bridge_comp.research_roi = (total_impact * 100.0) / total_investment;  // ROI as percentage
    }
}

void TechnologyEconomicBridge::LogPerformanceMetrics() {
    double current_time = std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

    double last_log_time = m_last_performance_log.load();
    if (current_time - last_log_time > m_config.performance_log_interval) {
        int updates = m_updates_this_frame.exchange(0);

        CORE_STREAM_INFO("TechnologyEconomicBridge") << "Technology-Economic Bridge Performance: "
            << updates << " updates in last " << m_config.performance_log_interval << " seconds";

        m_last_performance_log.store(current_time);
    }
}

// ============================================================================
// Public Interface Methods
// ============================================================================

TechnologyEconomicBridge::BridgeHealthMetrics
TechnologyEconomicBridge::GetBridgeHealth(game::types::EntityID entity_id) const {
    BridgeHealthMetrics metrics;

    if (!m_entity_manager) return metrics;

    auto bridge_comp = m_entity_manager->GetComponent<TechnologyEconomicBridgeComponent>(
        core::ecs::EntityID(entity_id, 1));
    if (!bridge_comp) return metrics;

    metrics.tech_economic_balance = bridge_comp->tech_economic_balance;
    metrics.crisis_active = bridge_comp->research_funding_crisis ||
                           bridge_comp->implementation_cost_crisis ||
                           bridge_comp->brain_drain_active;
    metrics.crisis_severity = bridge_comp->crisis_severity;
    metrics.research_roi = bridge_comp->research_roi;

    if (bridge_comp->research_investment_history.size() >= 2) {
        auto& history = bridge_comp->research_investment_history;
        metrics.research_investment_trend = history.back() - history[history.size() - 2];
    }

    if (bridge_comp->economic_impact_history.size() >= 2) {
        auto& history = bridge_comp->economic_impact_history;
        metrics.technology_impact_trend = history.back() - history[history.size() - 2];
    }

    if (bridge_comp->research_funding_crisis) {
        metrics.primary_issue = "Research funding crisis";
    }
    else if (bridge_comp->implementation_cost_crisis) {
        metrics.primary_issue = "Technology implementation crisis";
    }
    else if (bridge_comp->brain_drain_active) {
        metrics.primary_issue = "Scholar brain drain";
    }
    else if (metrics.research_investment_trend < -10.0) {
        metrics.primary_issue = "Declining research investment";
    }
    else if (bridge_comp->research_roi < m_config.min_roi_for_investment) {
        metrics.primary_issue = "Low research ROI";
    }
    else {
        metrics.primary_issue = "Stable";
    }

    return metrics;
}

} // namespace integration
} // namespace mechanica
