// Created: October 30, 2025
// Location: src/game/military/MilitaryEconomicBridge.cpp
// Military-Economic Bridge System Implementation

#include "game/military/MilitaryEconomicBridge.h"
#include "game/military/MilitaryComponents.h"
#include "game/economy/EconomicComponents.h"
#include "game/trade/TradeSystem.h"
#include "core/logging/Logger.h"
#include <algorithm>
#include <cmath>

namespace mechanica {
namespace integration {

// Helper to convert between EntityID types
static inline core::ecs::EntityID ToECSEntityID(game::types::EntityID id) {
    core::ecs::EntityID ecs_id;
    ecs_id.id = id;
    return ecs_id;
}

// ============================================================================
// Constructor & Lifecycle
// ============================================================================

MilitaryEconomicBridge::MilitaryEconomicBridge() {
    CORE_LOG_INFO("MilitaryEconomicBridge", "Constructed");
}

void MilitaryEconomicBridge::Initialize() {
    CORE_LOG_INFO("MilitaryEconomicBridge", "Initializing...");
    m_last_maintenance_payment.store(0.0);
    CORE_LOG_INFO("MilitaryEconomicBridge", "Initialized successfully");
}

void MilitaryEconomicBridge::Update(core::ecs::EntityManager& entities,
                                    ::core::threading::ThreadSafeMessageBus& message_bus,
                                    double delta_time) {
    if (!m_military_system || !m_economic_system) {
        return;
    }

    m_entity_manager = &entities;
    m_message_bus = &message_bus;

    // Get all entities with military and economic components
    auto military_entities = entities.GetEntitiesWithComponent<game::military::MilitaryComponent>();

    for (auto entity_id : military_entities) {
        // Get or create bridge component
        auto bridge_comp = entities.GetComponent<MilitaryEconomicBridgeComponent>(entity_id);
        if (!bridge_comp) {
            bridge_comp = entities.AddComponent<MilitaryEconomicBridgeComponent>(entity_id);
        }

        if (bridge_comp) {
            // Convert core::ecs::EntityID to game::types::EntityID for UpdateEntityBridge
            UpdateEntityBridge(entity_id.id, *bridge_comp, delta_time);
        }
    }

    m_updates_this_frame++;
}

void MilitaryEconomicBridge::Shutdown() {
    CORE_LOG_INFO("MilitaryEconomicBridge", "Shutting down");
    m_entity_manager = nullptr;
    m_message_bus = nullptr;
    m_military_system = nullptr;
    m_economic_system = nullptr;
    m_trade_system = nullptr;
}

core::threading::ThreadingStrategy MilitaryEconomicBridge::GetThreadingStrategy() const {
    return core::threading::ThreadingStrategy::MAIN_THREAD; // Treasury updates must be synchronous
}

// ============================================================================
// ISerializable Interface
// ============================================================================

Json::Value MilitaryEconomicBridge::Serialize(int version) const {
    Json::Value root;
    root["system_name"] = "MilitaryEconomicBridge";
    root["version"] = version;
    root["last_maintenance_payment"] = m_last_maintenance_payment.load();
    root["updates_this_frame"] = m_updates_this_frame.load();
    return root;
}

bool MilitaryEconomicBridge::Deserialize(const Json::Value& data, int version) {
    if (data.isMember("last_maintenance_payment")) {
        m_last_maintenance_payment.store(data["last_maintenance_payment"].asDouble());
    }
    return true;
}

std::string MilitaryEconomicBridge::GetSystemName() const {
    return "MilitaryEconomicBridge";
}

// ============================================================================
// System Configuration
// ============================================================================

void MilitaryEconomicBridge::SetMilitarySystem(game::military::MilitarySystem* military_system) {
    m_military_system = military_system;
    CORE_LOG_INFO("MilitaryEconomicBridge", "MilitarySystem reference set");
}

void MilitaryEconomicBridge::SetEconomicSystem(game::economy::EconomicSystem* economic_system) {
    m_economic_system = economic_system;
    CORE_LOG_INFO("MilitaryEconomicBridge", "EconomicSystem reference set");
}

void MilitaryEconomicBridge::SetTradeSystem(game::trade::TradeSystem* trade_system) {
    m_trade_system = trade_system;
    CORE_LOG_INFO("MilitaryEconomicBridge", "TradeSystem reference set");
}

// ============================================================================
// Core Calculation Methods
// ============================================================================

MilitaryEconomicEffects MilitaryEconomicBridge::CalculateMilitaryEconomicEffects(
    game::types::EntityID entity_id) {

    MilitaryEconomicEffects effects;

    if (!m_entity_manager || !m_military_system) {
        return effects;
    }

    auto ecs_id = ToECSEntityID(entity_id);
    auto military_comp = m_entity_manager->GetComponent<game::military::MilitaryComponent>(ecs_id);
    auto bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ecs_id);

    if (!military_comp) {
        return effects;
    }

    // Calculate maintenance costs
    effects.total_maintenance_cost = CalculateTotalGarrisonMaintenance(entity_id);

    // Calculate equipment costs (from recent spending)
    if (military_comp) {
        effects.equipment_cost = military_comp->equipment_spending;
        effects.recruitment_cost = military_comp->recruitment_spending;
    }

    // Calculate supply costs
    effects.supply_cost = CalculateSupplyConsumption(entity_id);

    // War impacts
    if (bridge_comp && bridge_comp->at_war) {
        effects.trade_disruption_cost = CalculateWarTradeDisruption(entity_id, bridge_comp->months_at_war);
        effects.war_exhaustion_penalty = bridge_comp->war_exhaustion * m_config.war_productivity_penalty;
    }

    // Calculate military employment (soldiers not in civilian workforce)
    double total_soldiers = 0.0;
    for (const auto& unit : military_comp->garrison_units) {
        total_soldiers += unit.current_strength;
    }
    effects.military_employment = total_soldiers;

    // Revenue from conquests (if any)
    if (bridge_comp) {
        effects.loot_income = bridge_comp->total_loot_collected;
        effects.tribute_income = 0.0; // Can be extended for vassals
    }

    return effects;
}

EconomicMilitaryContribution MilitaryEconomicBridge::CalculateEconomicMilitaryContributions(
    game::types::EntityID entity_id) {

    EconomicMilitaryContribution contribution;

    if (!m_entity_manager || !m_economic_system) {
        return contribution;
    }

    auto ecs_id = ToECSEntityID(entity_id);
    auto economic_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(ecs_id);
    auto treasury_comp = m_entity_manager->GetComponent<game::economy::TreasuryComponent>(ecs_id);

    if (!economic_comp && !treasury_comp) {
        return contribution;
    }

    // Calculate available military budget (% of monthly income)
    int monthly_income = economic_comp ? economic_comp->monthly_income : 0;
    int treasury = economic_comp ? economic_comp->treasury : (treasury_comp ? treasury_comp->gold_reserves : 0);

    contribution.available_military_budget = monthly_income * m_config.military_budget_default_percentage;

    // Calculate budget utilization
    double current_military_cost = GetMonthlyMilitaryCost(entity_id);
    if (contribution.available_military_budget > 0) {
        contribution.budget_utilization_rate = current_military_cost / contribution.available_military_budget;
    }

    // Financial sustainability (can we afford this military?)
    if (current_military_cost > 0) {
        contribution.financial_sustainability = std::min(1.0,
            contribution.available_military_budget / current_military_cost);
    } else {
        contribution.financial_sustainability = 1.0;
    }

    // Equipment quality modifier (wealthier = better equipment)
    contribution.equipment_quality_modifier = 1.0 + (treasury * m_config.equipment_quality_wealth_factor);
    contribution.equipment_quality_modifier = std::min(2.0, contribution.equipment_quality_modifier);

    // Supply quality
    contribution.supply_quality = (treasury > 500) ? 1.0 : (treasury / 500.0);

    // Recruitment capacity modifier
    contribution.recruitment_capacity_modifier = 1.0 + (monthly_income * m_config.recruitment_capacity_income_factor);

    // Mercenary availability (need good treasury)
    contribution.mercenary_availability = (treasury > 1000) ? 1.0 : 0.0;

    // War support capacity
    double monthly_expenses = economic_comp ? economic_comp->monthly_expenses : 0;
    if (monthly_income > monthly_expenses) {
        contribution.war_support_capacity = (monthly_income - monthly_expenses) /
                                           std::max(1.0, static_cast<double>(monthly_income));
    } else {
        contribution.war_support_capacity = 0.1; // Minimal support if in deficit
    }

    // Treasury stability
    double reserve_ratio = (monthly_expenses > 0) ?
                          (treasury / (monthly_expenses * m_config.treasury_stability_reserve_ratio)) : 1.0;
    contribution.treasury_stability = std::min(1.0, reserve_ratio);

    // Trade revenue for military
    contribution.trade_revenue_for_military = economic_comp ? economic_comp->trade_income * 0.4 : 0.0;

    return contribution;
}

// ============================================================================
// Application Methods
// ============================================================================

void MilitaryEconomicBridge::ApplyMilitaryEffectsToEconomy(
    game::types::EntityID entity_id,
    const MilitaryEconomicEffects& effects) {

    if (!m_entity_manager || !m_economic_system) {
        return;
    }

    auto economic_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(ToECSEntityID(entity_id));
    auto treasury_comp = m_entity_manager->GetComponent<game::economy::TreasuryComponent>(ToECSEntityID(entity_id));

    if (!economic_comp && !treasury_comp) {
        return;
    }

    // Calculate total military expenses
    double total_military_expenses = effects.total_maintenance_cost +
                                    effects.equipment_cost +
                                    effects.supply_cost +
                                    effects.fortification_cost;

    // Update economic component
    if (economic_comp) {
        economic_comp->monthly_expenses += static_cast<int>(total_military_expenses);

        // Add war-related costs
        if (effects.trade_disruption_cost > 0) {
            economic_comp->trade_income -= static_cast<int>(effects.trade_disruption_cost);
        }

        // Add loot income
        if (effects.loot_income > 0) {
            economic_comp->monthly_income += static_cast<int>(effects.loot_income);
            economic_comp->treasury += static_cast<int>(effects.loot_income);
        }

        // Add tribute income
        if (effects.tribute_income > 0) {
            economic_comp->tribute_income += static_cast<int>(effects.tribute_income);
        }
    }

    // Update treasury component
    if (treasury_comp) {
        treasury_comp->military_expenses = static_cast<int>(total_military_expenses);
    }
}

void MilitaryEconomicBridge::ApplyEconomicContributionsToMilitary(
    game::types::EntityID entity_id,
    const EconomicMilitaryContribution& contributions) {

    if (!m_entity_manager || !m_military_system) {
        return;
    }

    auto military_comp = m_entity_manager->GetComponent<game::military::MilitaryComponent>(ToECSEntityID(entity_id));

    if (!military_comp) {
        return;
    }

    // Update military budget
    military_comp->military_budget = contributions.available_military_budget;

    // Apply equipment quality modifiers to all units
    for (auto& unit : military_comp->garrison_units) {
        unit.equipment_quality = std::min(1.0,
            unit.equipment_quality * contributions.equipment_quality_modifier);
    }

    // Apply supply quality
    // (This would affect ArmyComponent supply levels if we iterate armies)

    // Update recruitment capacity based on economic contribution
    military_comp->recruitment_capacity = static_cast<uint32_t>(
        military_comp->recruitment_capacity * contributions.recruitment_capacity_modifier);
}

// ============================================================================
// Monthly Economic Processing
// ============================================================================

void MilitaryEconomicBridge::ProcessMonthlyMaintenance(game::types::EntityID entity_id) {
    if (!m_entity_manager || !m_military_system || !m_economic_system) {
        return;
    }

    auto bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
    if (!bridge_comp) {
        m_entity_manager->AddComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
        bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
    }

    // Calculate total maintenance cost
    double maintenance_cost = CalculateTotalGarrisonMaintenance(entity_id);

    // Try to deduct from treasury
    if (CheckBudgetAvailable(entity_id, maintenance_cost)) {
        DeductFromTreasury(entity_id, maintenance_cost);
        bridge_comp->last_maintenance_payment = maintenance_cost;
        bridge_comp->unpaid_troops = false;

        CORE_LOG_DEBUG("MilitaryEconomicBridge", "Entity " + std::to_string(entity_id) +
                                    " paid military maintenance: " + std::to_string(maintenance_cost));
    } else {
        // Can't afford maintenance - crisis!
        bridge_comp->unpaid_troops = true;

        // Check debt limit before accumulating (HIGH-007 FIX)
        if (bridge_comp->accumulated_debt + maintenance_cost > m_config.max_accumulated_debt) {
            // BANKRUPTCY!
            CORE_LOG_ERROR("MilitaryEconomicBridge", "Entity " + std::to_string(entity_id) +
                          " BANKRUPTCY! Debt limit exceeded: " +
                          std::to_string(bridge_comp->accumulated_debt + maintenance_cost) +
                          " > " + std::to_string(m_config.max_accumulated_debt));

            // Trigger bankruptcy event
            if (m_message_bus) {
                BankruptcyEvent bankruptcy_event;
                bankruptcy_event.affected_entity = entity_id;
                bankruptcy_event.total_debt = bridge_comp->accumulated_debt + maintenance_cost;
                bankruptcy_event.max_debt_limit = m_config.max_accumulated_debt;
                bankruptcy_event.consequences.push_back("Military forces disbanded");
                bankruptcy_event.consequences.push_back("Severe economic penalties");
                bankruptcy_event.consequences.push_back("Loss of territory possible");
                bankruptcy_event.military_disbanded = true;
                m_message_bus->Publish(std::make_shared<BankruptcyEvent>(bankruptcy_event));
            }

            // Cap debt at maximum
            bridge_comp->accumulated_debt = m_config.max_accumulated_debt;
            bridge_comp->crisis_severity = 1.0; // Maximum crisis

        } else {
            // Accumulate debt within limits
            bridge_comp->accumulated_debt += maintenance_cost;

            CORE_LOG_WARN("MilitaryEconomicBridge", "Entity " + std::to_string(entity_id) +
                                          " CANNOT afford military maintenance! Cost: " +
                                          std::to_string(maintenance_cost) +
                                          " | Total debt: " + std::to_string(bridge_comp->accumulated_debt));

            // Trigger unpaid troops event
            UnpaidTroopsEvent event;
            event.affected_entity = entity_id;
            event.unpaid_months = 1; // Increment in actual implementation
            event.morale_penalty = m_config.unpaid_morale_penalty;
            event.desertion_risk = m_config.desertion_risk_base + m_config.desertion_risk_per_unpaid_month;

            if (m_message_bus) {
                m_message_bus->Publish(std::make_shared<UnpaidTroopsEvent>(event));
            }
        }
    }
}

void MilitaryEconomicBridge::ProcessRecruitmentCosts(
    game::types::EntityID entity_id,
    game::military::UnitType unit_type,
    uint32_t quantity) {

    if (!m_entity_manager || !m_economic_system) {
        return;
    }

    double recruitment_cost = CalculateUnitRecruitmentCost(unit_type, quantity);

    if (CheckBudgetAvailable(entity_id, recruitment_cost)) {
        DeductFromTreasury(entity_id, recruitment_cost);

        auto military_comp = m_entity_manager->GetComponent<game::military::MilitaryComponent>(ToECSEntityID(entity_id));
        if (military_comp) {
            military_comp->recruitment_spending += recruitment_cost;
        }

        CORE_LOG_INFO("MilitaryEconomicBridge", "Entity " + std::to_string(entity_id) +
                                   " recruited " + std::to_string(quantity) +
                                   " units for " + std::to_string(recruitment_cost));
    } else {
        CORE_LOG_WARN("MilitaryEconomicBridge", "Entity " + std::to_string(entity_id) +
                                      " CANNOT afford recruitment! Cost: " +
                                      std::to_string(recruitment_cost));
    }
}

void MilitaryEconomicBridge::ProcessEquipmentPurchases(game::types::EntityID entity_id, double amount) {
    if (CheckBudgetAvailable(entity_id, amount)) {
        DeductFromTreasury(entity_id, amount);

        auto military_comp = m_entity_manager->GetComponent<game::military::MilitaryComponent>(ToECSEntityID(entity_id));
        if (military_comp) {
            military_comp->equipment_spending += amount;
        }
    }
}

void MilitaryEconomicBridge::ProcessSupplyCosts(game::types::EntityID entity_id) {
    double supply_cost = CalculateSupplyConsumption(entity_id);

    if (CheckBudgetAvailable(entity_id, supply_cost)) {
        DeductFromTreasury(entity_id, supply_cost);
    } else {
        // Supply crisis!
        ProcessSupplyCrisis(entity_id);
    }
}

// ============================================================================
// War Economic Impacts
// ============================================================================

void MilitaryEconomicBridge::ProcessWarEconomicImpact(game::types::EntityID entity_id, bool is_at_war) {
    if (!m_entity_manager) return;

    auto bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
    if (!bridge_comp) {
        m_entity_manager->AddComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
        bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
    }

    bridge_comp->at_war = is_at_war;

    if (is_at_war) {
        bridge_comp->months_at_war++;
        CalculateWarExhaustion(entity_id, bridge_comp->months_at_war);

        // Calculate war economic impact
        double trade_losses = CalculateWarTradeDisruption(entity_id, bridge_comp->months_at_war);
        bridge_comp->war_economic_impact = trade_losses;

        // Trigger war economic impact event
        if (m_message_bus && bridge_comp->months_at_war % 3 == 0) { // Every 3 months
            WarEconomicImpactEvent event;
            event.affected_entity = entity_id;
            event.trade_losses = trade_losses;
            event.infrastructure_damage = 0.0; // Can be extended
            event.total_economic_impact = trade_losses;
            event.months_of_war = bridge_comp->months_at_war;
            m_message_bus->Publish(std::make_shared<WarEconomicImpactEvent>(event));
        }
    } else {
        // War ended - reset
        bridge_comp->months_at_war = 0;
        bridge_comp->war_exhaustion = std::max(0.0, bridge_comp->war_exhaustion - 0.1);
    }
}

void MilitaryEconomicBridge::ProcessTradeDisruption(
    game::types::EntityID entity_id,
    const std::vector<game::types::EntityID>& hostile_neighbors) {

    if (!m_entity_manager || !m_trade_system) return;

    // Calculate trade routes passing through hostile territory
    auto trade_comp = m_entity_manager->GetComponent<game::trade::TradeHubComponent>(ToECSEntityID(entity_id));
    if (!trade_comp) return;

    // TODO: TradeHubComponent doesn't have outgoing_routes field
    // Need to query TradeSystem for routes instead
    std::vector<game::types::EntityID> disrupted_routes;
    double total_revenue_loss = 0.0;

    // Temporarily disabled - API mismatch
    // for (const auto& route : trade_comp->outgoing_routes) {
    //     for (auto hostile_id : hostile_neighbors) {
    //         if (route.destination_province == hostile_id) {
    //             disrupted_routes.push_back(route.destination_province);
    //             total_revenue_loss += route.trade_value * m_config.war_trade_disruption_rate;
    //         }
    //     }
    // }

    if (!disrupted_routes.empty() && m_message_bus) {
        TradeDisruptionEvent event;
        event.affected_entity = entity_id;
        event.disrupted_routes = disrupted_routes;
        event.revenue_loss = total_revenue_loss;
        event.disruption_cause = "war";
        m_message_bus->Publish(std::make_shared<TradeDisruptionEvent>(event));
    }
}

void MilitaryEconomicBridge::CalculateWarExhaustion(game::types::EntityID entity_id, int months_at_war) {
    if (!m_entity_manager) return;

    auto bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
    if (!bridge_comp) return;

    // War exhaustion increases over time
    bridge_comp->war_exhaustion = std::min(m_config.war_exhaustion_max,
        months_at_war * m_config.war_exhaustion_rate);
}

// ============================================================================
// Conquest and Loot
// ============================================================================

void MilitaryEconomicBridge::ProcessConquestLoot(
    game::types::EntityID conqueror_id,
    game::types::EntityID conquered_id,
    double loot_percentage) {

    if (!m_entity_manager || !m_economic_system) return;

    double loot_amount = CalculateLootAmount(conquered_id, "siege");

    // Transfer loot to conqueror
    AddToTreasury(conqueror_id, loot_amount);
    DeductFromTreasury(conquered_id, loot_amount);

    // Update bridge component
    auto bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(conqueror_id));
    if (bridge_comp) {
        bridge_comp->total_loot_collected += loot_amount;
    }

    // Trigger loot event
    if (m_message_bus) {
        ConquestLootEvent event;
        event.conqueror_entity = conqueror_id;
        event.conquered_entity = conquered_id;
        event.loot_amount = loot_amount;
        event.territory_value = 0.0; // Calculated separately
        event.conquest_type = "siege";
        m_message_bus->Publish(std::make_shared<ConquestLootEvent>(event));
    }

    CORE_LOG_INFO("MilitaryEconomicBridge", "Entity " + std::to_string(conqueror_id) +
                               " looted " + std::to_string(loot_amount) +
                               " from entity " + std::to_string(conquered_id));
}

void MilitaryEconomicBridge::ProcessTerritoryCapture(
    game::types::EntityID conqueror_id,
    game::types::EntityID new_territory_id) {

    if (!m_entity_manager || !m_economic_system) return;

    // Calculate territory value (based on monthly income)
    double monthly_income = GetMonthlyIncome(new_territory_id);
    double territory_value = monthly_income * m_config.territory_capture_value_multiplier;

    // This would trigger province ownership transfer in the main game system
    // For now, just log it
    CORE_LOG_INFO("MilitaryEconomicBridge", "Entity " + std::to_string(conqueror_id) +
                               " captured territory " + std::to_string(new_territory_id) +
                               " worth " + std::to_string(territory_value));
}

// ============================================================================
// Budget Constraints and Crisis Management
// ============================================================================

void MilitaryEconomicBridge::CheckBudgetConstraints(game::types::EntityID entity_id) {
    if (!m_entity_manager) return;

    auto bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
    if (!bridge_comp) return;

    double monthly_cost = GetMonthlyMilitaryCost(entity_id);
    double available_budget = GetAvailableMilitaryBudget(entity_id);

    if (monthly_cost > available_budget * m_config.budget_crisis_threshold) {
        bridge_comp->budget_crisis = true;

        if (m_message_bus) {
            MilitaryBudgetCrisisEvent event;
            event.affected_entity = entity_id;
            event.budget_shortfall = monthly_cost - available_budget;
            event.monthly_deficit = monthly_cost;
            m_message_bus->Publish(std::make_shared<MilitaryBudgetCrisisEvent>(event));
        }

        CORE_LOG_WARN("MilitaryEconomicBridge", "Entity " + std::to_string(entity_id) +
                                      " in BUDGET CRISIS! Cost: " + std::to_string(monthly_cost) +
                                      " Budget: " + std::to_string(available_budget));
    } else {
        bridge_comp->budget_crisis = false;
    }
}

void MilitaryEconomicBridge::ProcessUnpaidTroops(game::types::EntityID entity_id, int months_unpaid) {
    if (!m_entity_manager || !m_military_system) return;

    auto military_comp = m_entity_manager->GetComponent<game::military::MilitaryComponent>(ToECSEntityID(entity_id));
    if (!military_comp) return;

    // Apply morale penalties to all units
    double morale_penalty = months_unpaid * m_config.unpaid_morale_penalty;
    double desertion_risk = m_config.desertion_risk_base +
                           (months_unpaid * m_config.desertion_risk_per_unpaid_month);

    for (auto& unit : military_comp->garrison_units) {
        // Reduce morale (this would need to be added to MilitaryUnit struct)
        // For now, reduce strength to simulate desertion
        if (desertion_risk > 0.5) {
            unit.current_strength = static_cast<uint32_t>(
                unit.current_strength * (1.0 - desertion_risk * 0.1));
        }
    }

    CORE_LOG_WARN("MilitaryEconomicBridge", "Entity " + std::to_string(entity_id) +
                                  " has unpaid troops for " + std::to_string(months_unpaid) +
                                  " months! Desertion risk: " + std::to_string(desertion_risk));
}

void MilitaryEconomicBridge::ProcessSupplyCrisis(game::types::EntityID entity_id) {
    if (!m_entity_manager) return;

    auto bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
    if (!bridge_comp) return;

    bridge_comp->supply_crisis = true;

    CORE_LOG_WARN("MilitaryEconomicBridge", "Entity " + std::to_string(entity_id) + " in SUPPLY CRISIS!");
}

// ============================================================================
// Trade Route Safety
// ============================================================================

double MilitaryEconomicBridge::CalculateTradeRouteSafety(
    game::types::EntityID route_origin,
    game::types::EntityID route_destination) {

    if (!m_entity_manager || !m_military_system) {
        return 0.5; // Default safety
    }

    // Get military strength of origin province
    double origin_strength = m_military_system->GetTotalMilitaryStrength(route_origin);

    // Base safety from military strength
    double safety = m_config.military_strength_safety_multiplier * origin_strength;

    // Cap between 0 and 1
    safety = std::max(0.0, std::min(1.0, safety));

    // TODO: TradeHubComponent doesn't have piracy_risk field
    // Reduce by piracy risk
    // auto trade_comp = m_entity_manager->GetComponent<game::trade::TradeHubComponent>(ToECSEntityID(route_origin));
    // if (trade_comp) {
    //     safety *= (1.0 - trade_comp->piracy_risk);
    // }

    return safety;
}

void MilitaryEconomicBridge::ApplyMilitaryProtectionToTrade(game::types::EntityID entity_id) {
    if (!m_entity_manager || !m_trade_system) return;

    auto trade_comp = m_entity_manager->GetComponent<game::trade::TradeHubComponent>(ToECSEntityID(entity_id));
    if (!trade_comp) return;

    // TODO: TradeHubComponent doesn't have piracy_risk field
    // double military_strength = m_military_system ? m_military_system->GetTotalMilitaryStrength(entity_id) : 0.0;
    // double piracy_reduction = military_strength * m_config.military_strength_safety_multiplier * 0.1;
    // trade_comp->piracy_risk = std::max(0.0, trade_comp->piracy_risk - piracy_reduction);
}

// ============================================================================
// Crisis Detection
// ============================================================================

void MilitaryEconomicBridge::ProcessCrisisDetection(game::types::EntityID entity_id) {
    if (!m_entity_manager) return;

    auto bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
    if (!bridge_comp) return;

    bool budget_crisis = DetectBudgetCrisis(*bridge_comp);
    bool unpaid_crisis = DetectUnpaidTroopsCrisis(*bridge_comp);
    bool supply_crisis = DetectSupplyCrisis(entity_id);

    // Update crisis severity
    if (budget_crisis || unpaid_crisis || supply_crisis) {
        bridge_comp->crisis_severity = std::min(1.0, bridge_comp->crisis_severity + 0.1);
    } else {
        bridge_comp->crisis_severity = std::max(0.0, bridge_comp->crisis_severity - 0.05);
    }
}

// ============================================================================
// Public Metrics Interface
// ============================================================================

MilitaryEconomicBridge::BridgeHealthMetrics
MilitaryEconomicBridge::GetBridgeHealth(game::types::EntityID entity_id) const {

    BridgeHealthMetrics metrics;

    if (!m_entity_manager) {
        return metrics;
    }

    auto bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
    if (!bridge_comp) {
        return metrics;
    }

    metrics.monthly_military_cost = GetMonthlyMilitaryCost(entity_id);
    metrics.budget_utilization = bridge_comp->economic_contributions.budget_utilization_rate;
    metrics.financial_sustainability = bridge_comp->economic_contributions.financial_sustainability;
    metrics.crisis_active = bridge_comp->budget_crisis || bridge_comp->unpaid_troops || bridge_comp->supply_crisis;
    metrics.crisis_severity = bridge_comp->crisis_severity;
    metrics.war_exhaustion = bridge_comp->war_exhaustion;
    metrics.accumulated_debt = bridge_comp->accumulated_debt;
    metrics.can_afford_current_military = bridge_comp->economic_contributions.financial_sustainability >= 1.0;

    if (bridge_comp->budget_crisis) {
        metrics.primary_issue = "Budget Crisis";
    } else if (bridge_comp->unpaid_troops) {
        metrics.primary_issue = "Unpaid Troops";
    } else if (bridge_comp->supply_crisis) {
        metrics.primary_issue = "Supply Crisis";
    } else {
        metrics.primary_issue = "Healthy";
    }

    return metrics;
}

// ============================================================================
// Budget Queries
// ============================================================================

bool MilitaryEconomicBridge::CanAffordRecruitment(
    game::types::EntityID entity_id,
    game::military::UnitType unit_type,
    uint32_t quantity) const {

    double cost = CalculateUnitRecruitmentCost(unit_type, quantity);
    return CheckBudgetAvailable(entity_id, cost);
}

double MilitaryEconomicBridge::GetAvailableMilitaryBudget(game::types::EntityID entity_id) const {
    if (!m_entity_manager) return 0.0;

    auto bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
    if (bridge_comp) {
        return bridge_comp->economic_contributions.available_military_budget;
    }

    // Calculate on the fly if no bridge component
    double monthly_income = GetMonthlyIncome(entity_id);
    return monthly_income * m_config.military_budget_default_percentage;
}

double MilitaryEconomicBridge::GetMonthlyMilitaryCost(game::types::EntityID entity_id) const {
    if (!m_entity_manager) return 0.0;

    auto bridge_comp = m_entity_manager->GetComponent<MilitaryEconomicBridgeComponent>(ToECSEntityID(entity_id));
    if (bridge_comp) {
        return bridge_comp->military_effects.total_maintenance_cost +
               bridge_comp->military_effects.supply_cost;
    }

    return CalculateTotalGarrisonMaintenance(entity_id);
}

// ============================================================================
// Internal Calculation Helpers
// ============================================================================

double MilitaryEconomicBridge::CalculateUnitMaintenance(const game::military::MilitaryUnit& unit) const {
    // Get base maintenance cost based on unit type
    double base_cost = 0.0;

    switch (unit.type) {
        case game::military::UnitType::LEVIES:
            base_cost = m_config.levies_maintenance;
            break;
        case game::military::UnitType::SPEARMEN:
        case game::military::UnitType::SWORDSMEN:
        case game::military::UnitType::PIKEMEN:
            base_cost = m_config.infantry_maintenance;
            break;
        case game::military::UnitType::LIGHT_CAVALRY:
        case game::military::UnitType::HEAVY_CAVALRY:
            base_cost = m_config.cavalry_maintenance;
            break;
        case game::military::UnitType::CANNONS:
        case game::military::UnitType::TREBUCHETS:
            base_cost = m_config.artillery_maintenance;
            break;
        case game::military::UnitType::GALLEYS:
        case game::military::UnitType::WAR_GALLEONS:
            base_cost = m_config.naval_maintenance;
            break;
        default:
            base_cost = m_config.infantry_maintenance;
    }

    // Scale by actual unit strength (base cost is per 1000 men)
    double cost = base_cost * (unit.current_strength / 1000.0);

    // Apply maintenance cost multiplier
    cost *= m_config.maintenance_cost_multiplier;

    return cost;
}

double MilitaryEconomicBridge::CalculateUnitRecruitmentCost(
    game::military::UnitType unit_type,
    uint32_t quantity) const {

    double base_cost = 0.0;

    switch (unit_type) {
        case game::military::UnitType::LEVIES:
            base_cost = m_config.levies_recruitment;
            break;
        case game::military::UnitType::SPEARMEN:
        case game::military::UnitType::SWORDSMEN:
        case game::military::UnitType::PIKEMEN:
            base_cost = m_config.infantry_recruitment;
            break;
        case game::military::UnitType::LIGHT_CAVALRY:
        case game::military::UnitType::HEAVY_CAVALRY:
            base_cost = m_config.cavalry_recruitment;
            break;
        case game::military::UnitType::CANNONS:
        case game::military::UnitType::TREBUCHETS:
            base_cost = m_config.artillery_recruitment;
            break;
        case game::military::UnitType::GALLEYS:
        case game::military::UnitType::WAR_GALLEONS:
            base_cost = m_config.naval_recruitment;
            break;
        default:
            base_cost = m_config.infantry_recruitment;
    }

    return base_cost * (quantity / 1000.0) * m_config.recruitment_cost_multiplier;
}

double MilitaryEconomicBridge::CalculateTotalGarrisonMaintenance(game::types::EntityID entity_id) const {
    if (!m_entity_manager || !m_military_system) {
        return 0.0;
    }

    auto military_comp = m_entity_manager->GetComponent<game::military::MilitaryComponent>(ToECSEntityID(entity_id));
    if (!military_comp) {
        return 0.0;
    }

    double total_maintenance = 0.0;
    for (const auto& unit : military_comp->garrison_units) {
        total_maintenance += CalculateUnitMaintenance(unit);
    }

    return total_maintenance;
}

double MilitaryEconomicBridge::CalculateSupplyConsumption(game::types::EntityID entity_id) const {
    if (!m_entity_manager) {
        return 0.0;
    }

    auto military_comp = m_entity_manager->GetComponent<game::military::MilitaryComponent>(ToECSEntityID(entity_id));
    if (!military_comp) {
        return 0.0;
    }

    double total_soldiers = 0.0;
    for (const auto& unit : military_comp->garrison_units) {
        total_soldiers += unit.current_strength;
    }

    return total_soldiers * m_config.supply_cost_per_soldier;
}

double MilitaryEconomicBridge::CalculateWarTradeDisruption(
    game::types::EntityID entity_id,
    int months_at_war) const {

    if (!m_entity_manager) {
        return 0.0;
    }

    auto economic_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(ToECSEntityID(entity_id));
    if (!economic_comp) {
        return 0.0;
    }

    // Trade income loss during war
    double trade_loss = economic_comp->trade_income * m_config.war_trade_disruption_rate;

    // Increases with war duration
    double duration_multiplier = 1.0 + (months_at_war * 0.05);
    return trade_loss * duration_multiplier;
}

double MilitaryEconomicBridge::CalculateLootAmount(
    game::types::EntityID target_id,
    const std::string& conquest_type) const {

    double treasury = GetCurrentTreasury(target_id);
    double loot_percentage = m_config.default_loot_percentage;

    if (conquest_type == "siege") {
        loot_percentage *= m_config.siege_loot_multiplier;
    } else if (conquest_type == "raid") {
        loot_percentage *= m_config.raid_loot_multiplier;
    }

    return treasury * loot_percentage;
}

// ============================================================================
// Budget and Constraint Helpers
// ============================================================================

bool MilitaryEconomicBridge::CheckBudgetAvailable(game::types::EntityID entity_id, double cost) const {
    double current_treasury = GetCurrentTreasury(entity_id);
    return current_treasury >= cost;
}

void MilitaryEconomicBridge::DeductFromTreasury(game::types::EntityID entity_id, double amount) {
    if (!m_economic_system) return;

    // Use EconomicSystem API instead of direct mutation (CRITICAL FIX)
    bool success = m_economic_system->SpendMoney(entity_id, static_cast<int>(amount));

    if (!success) {
        CORE_LOG_WARN("MilitaryEconomicBridge",
                     "Failed to deduct " + std::to_string(amount) +
                     " from entity " + std::to_string(entity_id) +
                     " treasury (insufficient funds or below minimum)");
    }

    // Update TreasuryComponent if it exists (for tracking)
    if (m_entity_manager && success) {
        auto treasury_comp = m_entity_manager->GetComponent<game::economy::TreasuryComponent>(ToECSEntityID(entity_id));
        if (treasury_comp) {
            treasury_comp->military_expenses += static_cast<int>(amount);
        }
    }
}

void MilitaryEconomicBridge::AddToTreasury(game::types::EntityID entity_id, double amount) {
    if (!m_economic_system) return;

    // Use EconomicSystem API instead of direct mutation (CRITICAL FIX)
    m_economic_system->AddMoney(entity_id, static_cast<int>(amount));

    CORE_LOG_DEBUG("MilitaryEconomicBridge",
                  "Added " + std::to_string(amount) +
                  " to entity " + std::to_string(entity_id) + " treasury");

    // Update TreasuryComponent if it exists (for tracking)
    if (m_entity_manager) {
        auto treasury_comp = m_entity_manager->GetComponent<game::economy::TreasuryComponent>(ToECSEntityID(entity_id));
        if (treasury_comp) {
            treasury_comp->gold_reserves += static_cast<int>(amount);
        }
    }
}

double MilitaryEconomicBridge::GetCurrentTreasury(game::types::EntityID entity_id) const {
    if (!m_entity_manager) return 0.0;

    auto economic_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(ToECSEntityID(entity_id));
    if (economic_comp) {
        return economic_comp->treasury;
    }

    auto treasury_comp = m_entity_manager->GetComponent<game::economy::TreasuryComponent>(ToECSEntityID(entity_id));
    if (treasury_comp) {
        return treasury_comp->gold_reserves;
    }

    return 0.0;
}

double MilitaryEconomicBridge::GetMonthlyIncome(game::types::EntityID entity_id) const {
    if (!m_entity_manager) return 0.0;

    auto economic_comp = m_entity_manager->GetComponent<game::economy::EconomicComponent>(ToECSEntityID(entity_id));
    if (economic_comp) {
        return economic_comp->monthly_income;
    }

    return 0.0;
}

// ============================================================================
// Crisis Detection Helpers
// ============================================================================

bool MilitaryEconomicBridge::DetectBudgetCrisis(const MilitaryEconomicBridgeComponent& bridge_comp) const {
    return bridge_comp.economic_contributions.budget_utilization_rate > m_config.budget_crisis_threshold;
}

bool MilitaryEconomicBridge::DetectUnpaidTroopsCrisis(const MilitaryEconomicBridgeComponent& bridge_comp) const {
    return bridge_comp.unpaid_troops;
}

bool MilitaryEconomicBridge::DetectSupplyCrisis(game::types::EntityID entity_id) const {
    if (!m_entity_manager) return false;

    // Check if any armies are low on supplies
    auto military_comp = m_entity_manager->GetComponent<game::military::MilitaryComponent>(ToECSEntityID(entity_id));
    if (!military_comp) return false;

    // Check garrison supply levels
    for (const auto& unit : military_comp->garrison_units) {
        if (unit.supply_level < m_config.supply_crisis_threshold) {
            return true;
        }
    }

    return false;
}

// ============================================================================
// Update Helpers
// ============================================================================

void MilitaryEconomicBridge::UpdateEntityBridge(
    game::types::EntityID entity_id,
    MilitaryEconomicBridgeComponent& bridge_comp,
    double delta_time) {

    bridge_comp.last_update_time += delta_time;

    // Only update at intervals
    if (bridge_comp.last_update_time < m_config.bridge_update_interval) {
        return;
    }

    bridge_comp.last_update_time = 0.0;

    // Calculate effects and contributions
    bridge_comp.military_effects = CalculateMilitaryEconomicEffects(entity_id);
    bridge_comp.economic_contributions = CalculateEconomicMilitaryContributions(entity_id);

    // Apply effects
    ApplyMilitaryEffectsToEconomy(entity_id, bridge_comp.military_effects);
    ApplyEconomicContributionsToMilitary(entity_id, bridge_comp.economic_contributions);

    // Check for crises
    ProcessCrisisDetection(entity_id);
    CheckBudgetConstraints(entity_id);

    // Update historical data
    UpdateHistoricalData(bridge_comp,
                        bridge_comp.military_effects.total_maintenance_cost,
                        bridge_comp.economic_contributions.financial_sustainability,
                        GetCurrentTreasury(entity_id));
}

void MilitaryEconomicBridge::UpdateHistoricalData(
    MilitaryEconomicBridgeComponent& bridge_comp,
    double military_spending,
    double military_readiness,
    double treasury_balance) {

    bridge_comp.military_spending_history.push_back(military_spending);
    bridge_comp.military_readiness_history.push_back(military_readiness);
    bridge_comp.treasury_balance_history.push_back(treasury_balance);

    // Limit history size (HIGH-008 FIX: use pop_front() for O(1) removal)
    if (bridge_comp.military_spending_history.size() > static_cast<size_t>(m_config.max_history_size)) {
        bridge_comp.military_spending_history.pop_front();
        bridge_comp.military_readiness_history.pop_front();
        bridge_comp.treasury_balance_history.pop_front();
    }
}

void MilitaryEconomicBridge::LogPerformanceMetrics() {
    // Placeholder for performance logging
}

} // namespace integration
} // namespace mechanica
