// Created: October 30, 2025
// Location: include/game/military/MilitaryEconomicBridge.h
// Military-Economic Bridge System - Connects military operations with economy

#pragma once

#include "core/ECS/EntityManager.h"
#include "core/ECS/MessageBus.h"
#include "core/threading/ThreadedSystemManager.h"
#include "core/ECS/ISerializable.h"
#include "game/military/MilitarySystem.h"
#include "game/economy/EconomicSystem.h"
#include "game/trade/TradeSystem.h"
#include "game/config/GameConfig.h"
#include "core/types/game_types.h"
#include "utils/PlatformCompat.h"

#include <vector>
#include <unordered_map>
#include <string>
#include <atomic>

namespace mechanica {
namespace integration {

// ============================================================================
// Military-Economic Integration Data Structures
// ============================================================================

/**
 * MilitaryEconomicEffects - How military operations affect the economy
 */
struct MilitaryEconomicEffects {
    // Direct military costs
    double total_maintenance_cost = 0.0;        // Monthly upkeep for all units
    double recruitment_cost = 0.0;              // One-time cost for new units
    double equipment_cost = 0.0;                // Weapons, armor, ammunition
    double fortification_cost = 0.0;            // Building/maintaining defenses
    double supply_cost = 0.0;                   // Logistics and provisions

    // War impacts on economy
    double trade_disruption_cost = 0.0;         // Lost trade revenue
    double infrastructure_damage_cost = 0.0;    // War damage to economy
    double war_exhaustion_penalty = 0.0;        // Population productivity loss
    double military_employment = 0.0;           // Soldiers not in civilian economy

    // Revenue from military
    double loot_income = 0.0;                   // Plunder from conquests
    double tribute_income = 0.0;                // Payments from vassals
    double conquest_territory_value = 0.0;      // New provinces acquired
    double piracy_suppression_bonus = 0.0;      // Trade safety improvement
};

/**
 * EconomicMilitaryContribution - How economy affects military capability
 */
struct EconomicMilitaryContribution {
    // Budget constraints
    double available_military_budget = 0.0;     // Treasury allocation
    double budget_utilization_rate = 0.0;       // How much of budget is used
    double financial_sustainability = 1.0;      // Can afford current military?

    // Economic support
    double equipment_quality_modifier = 1.0;    // Better economy → better gear
    double supply_quality = 1.0;                // Provision quality
    double recruitment_capacity_modifier = 1.0; // Economic capacity for recruitment
    double mercenary_availability = 0.0;        // Can afford mercenaries?

    // Economic health impacts
    double war_support_capacity = 1.0;          // Economic ability to wage war
    double treasury_stability = 1.0;            // Financial reserve health
    double trade_revenue_for_military = 0.0;    // Trade income → military spending
};

// ============================================================================
// Bridge Component
// ============================================================================

struct MilitaryEconomicBridgeComponent {
    MilitaryEconomicEffects military_effects;
    EconomicMilitaryContribution economic_contributions;

    // Historical tracking
    std::vector<double> military_spending_history;
    std::vector<double> military_readiness_history;
    std::vector<double> treasury_balance_history;

    // State tracking
    double last_maintenance_payment = 0.0;
    double accumulated_debt = 0.0;
    double war_exhaustion = 0.0;
    double total_loot_collected = 0.0;

    // Crisis states
    bool budget_crisis = false;                 // Can't afford military
    bool unpaid_troops = false;                 // Soldiers haven't been paid
    bool supply_crisis = false;                 // Armies starving
    double crisis_severity = 0.0;

    // War state
    bool at_war = false;
    int months_at_war = 0;
    double war_economic_impact = 0.0;

    double last_update_time = 0.0;

    // IComponent interface
    std::string Serialize() const { return "{}"; }
    bool Deserialize(const std::string& data) { return true; }
};

// ============================================================================
// Event Messages
// ============================================================================

struct MilitaryBudgetCrisisEvent : public core::ecs::IMessage {
    game::types::EntityID affected_entity;
    double budget_shortfall;
    double monthly_deficit;
    std::vector<std::string> affected_units;
    bool troops_disbanded = false;
    std::type_index GetTypeIndex() const override { return typeid(MilitaryBudgetCrisisEvent); }
};

struct WarEconomicImpactEvent : public core::ecs::IMessage {
    game::types::EntityID affected_entity;
    double trade_losses;
    double infrastructure_damage;
    double total_economic_impact;
    int months_of_war;
    std::type_index GetTypeIndex() const override { return typeid(WarEconomicImpactEvent); }
};

struct ConquestLootEvent : public core::ecs::IMessage {
    game::types::EntityID conqueror_entity;
    game::types::EntityID conquered_entity;
    double loot_amount;
    double territory_value;
    std::string conquest_type; // "raid", "siege", "occupation"
    std::type_index GetTypeIndex() const override { return typeid(ConquestLootEvent); }
};

struct TradeDisruptionEvent : public core::ecs::IMessage {
    game::types::EntityID affected_entity;
    std::vector<game::types::EntityID> disrupted_routes;
    double revenue_loss;
    std::string disruption_cause; // "war", "piracy", "blockade"
    std::type_index GetTypeIndex() const override { return typeid(TradeDisruptionEvent); }
};

struct UnpaidTroopsEvent : public core::ecs::IMessage {
    game::types::EntityID affected_entity;
    int unpaid_months;
    double morale_penalty;
    double desertion_risk;
    bool rebellion_imminent = false;
    std::type_index GetTypeIndex() const override { return typeid(UnpaidTroopsEvent); }
};

// ============================================================================
// Main Bridge System
// ============================================================================

class MilitaryEconomicBridge : public game::core::ISerializable {
public:
    MilitaryEconomicBridge();
    virtual ~MilitaryEconomicBridge() = default;

    // System lifecycle
    void Initialize();
    void Update(core::ecs::EntityManager& entities,
                core::ecs::MessageBus& message_bus,
                double delta_time);
    void Shutdown();

    // Threading interface
    core::threading::ThreadingStrategy GetThreadingStrategy() const;

    // ISerializable interface
    Json::Value Serialize(int version) const override;
    bool Deserialize(const Json::Value& data, int version) override;
    std::string GetSystemName() const override;

    // Core calculation methods
    MilitaryEconomicEffects CalculateMilitaryEconomicEffects(game::types::EntityID entity_id);
    EconomicMilitaryContribution CalculateEconomicMilitaryContributions(game::types::EntityID entity_id);

    // Application methods - apply calculated effects
    void ApplyMilitaryEffectsToEconomy(game::types::EntityID entity_id,
                                       const MilitaryEconomicEffects& effects);
    void ApplyEconomicContributionsToMilitary(game::types::EntityID entity_id,
                                              const EconomicMilitaryContribution& contributions);

    // Monthly economic processing
    void ProcessMonthlyMaintenance(game::types::EntityID entity_id);
    void ProcessRecruitmentCosts(game::types::EntityID entity_id,
                                game::military::UnitType unit_type,
                                uint32_t quantity);
    void ProcessEquipmentPurchases(game::types::EntityID entity_id, double amount);
    void ProcessSupplyCosts(game::types::EntityID entity_id);

    // War economic impacts
    void ProcessWarEconomicImpact(game::types::EntityID entity_id, bool is_at_war);
    void ProcessTradeDisruption(game::types::EntityID entity_id,
                               const std::vector<game::types::EntityID>& hostile_neighbors);
    void CalculateWarExhaustion(game::types::EntityID entity_id, int months_at_war);

    // Conquest and loot
    void ProcessConquestLoot(game::types::EntityID conqueror_id,
                            game::types::EntityID conquered_id,
                            double loot_percentage = 0.3);
    void ProcessTerritoryCapture(game::types::EntityID conqueror_id,
                                game::types::EntityID new_territory_id);

    // Budget constraints and crisis management
    void CheckBudgetConstraints(game::types::EntityID entity_id);
    void ProcessUnpaidTroops(game::types::EntityID entity_id, int months_unpaid);
    void ProcessSupplyCrisis(game::types::EntityID entity_id);

    // Trade route safety (affects TradeSystem)
    double CalculateTradeRouteSafety(game::types::EntityID route_origin,
                                    game::types::EntityID route_destination);
    void ApplyMilitaryProtectionToTrade(game::types::EntityID entity_id);

    // Crisis detection
    void ProcessCrisisDetection(game::types::EntityID entity_id);

    // System configuration
    void SetMilitarySystem(game::military::MilitarySystem* military_system);
    void SetEconomicSystem(game::economy::EconomicSystem* economic_system);
    void SetTradeSystem(game::trade::TradeSystem* trade_system);

    // Public metrics interface
    struct BridgeHealthMetrics {
        double monthly_military_cost = 0.0;
        double budget_utilization = 0.0;
        double financial_sustainability = 1.0;
        bool crisis_active = false;
        double crisis_severity = 0.0;
        double war_exhaustion = 0.0;
        double accumulated_debt = 0.0;
        std::string primary_issue;
        bool can_afford_current_military = true;
    };

    BridgeHealthMetrics GetBridgeHealth(game::types::EntityID entity_id) const;

    // Budget queries
    bool CanAffordRecruitment(game::types::EntityID entity_id,
                             game::military::UnitType unit_type,
                             uint32_t quantity) const;
    double GetAvailableMilitaryBudget(game::types::EntityID entity_id) const;
    double GetMonthlyMilitaryCost(game::types::EntityID entity_id) const;

private:
    // Configuration structure
    struct BridgeConfig {
        // Update intervals
        double bridge_update_interval = 1.0;
        double maintenance_payment_interval = 30.0; // Monthly

        // Cost multipliers
        double maintenance_cost_multiplier = 1.0;
        double recruitment_cost_multiplier = 1.0;
        double equipment_cost_multiplier = 1.0;
        double supply_cost_per_soldier = 0.5;

        // Unit-specific costs (monthly maintenance per 1000 men)
        double levies_maintenance = 5.0;
        double infantry_maintenance = 10.0;
        double cavalry_maintenance = 25.0;
        double artillery_maintenance = 40.0;
        double naval_maintenance = 50.0;

        // Recruitment costs (per 1000 men)
        double levies_recruitment = 20.0;
        double infantry_recruitment = 50.0;
        double cavalry_recruitment = 150.0;
        double artillery_recruitment = 300.0;
        double naval_recruitment = 400.0;

        // War impact multipliers
        double war_trade_disruption_rate = 0.2;     // 20% trade loss during war
        double war_exhaustion_rate = 0.05;          // 5% per month
        double war_exhaustion_max = 1.0;            // 100% max exhaustion
        double war_productivity_penalty = 0.3;      // 30% productivity loss at max exhaustion

        // Conquest and loot
        double default_loot_percentage = 0.3;       // 30% of treasury
        double siege_loot_multiplier = 1.5;         // More loot from sieges
        double raid_loot_multiplier = 0.5;          // Less loot from raids
        double territory_capture_value_multiplier = 2.0; // Territory worth 2x annual income

        // Budget constraints
        double military_budget_default_percentage = 0.4; // 40% of income
        double budget_crisis_threshold = 1.2;       // Crisis if spending > 120% budget
        double unpaid_morale_penalty = 0.1;         // 10% morale loss per unpaid month
        double desertion_risk_base = 0.05;          // 5% base desertion risk
        double desertion_risk_per_unpaid_month = 0.1; // +10% per month

        // Supply crisis
        double supply_crisis_morale_penalty = 0.2;
        double supply_crisis_combat_penalty = 0.3;
        double supply_exhaustion_rate = 0.1;        // 10% strength loss per month

        // Trade route safety
        double military_strength_safety_multiplier = 0.001; // 1000 troops = +1.0 safety
        double piracy_base_risk = 0.1;
        double military_protection_radius = 2;      // Provinces

        // Crisis thresholds
        double budget_crisis_severity_threshold = 0.5;
        double unpaid_troops_crisis_months = 3;
        double supply_crisis_threshold = 0.3;       // 30% supply level

        // Economic modifiers
        double equipment_quality_wealth_factor = 0.0001; // Treasury → equipment quality
        double recruitment_capacity_income_factor = 0.01; // Income → recruitment capacity
        double treasury_stability_reserve_ratio = 3.0; // Reserves/monthly_expenses

        // History tracking
        int max_history_size = 12;
        double performance_log_interval = 10.0;
    };

    // Internal calculation helpers
    double CalculateUnitMaintenance(const game::military::MilitaryUnit& unit) const;
    double CalculateUnitRecruitmentCost(game::military::UnitType unit_type, uint32_t quantity) const;
    double CalculateTotalGarrisonMaintenance(game::types::EntityID entity_id) const;
    double CalculateSupplyConsumption(game::types::EntityID entity_id) const;
    double CalculateWarTradeDisruption(game::types::EntityID entity_id, int months_at_war) const;
    double CalculateLootAmount(game::types::EntityID target_id, const std::string& conquest_type) const;

    // Budget and constraint helpers
    bool CheckBudgetAvailable(game::types::EntityID entity_id, double cost) const;
    void DeductFromTreasury(game::types::EntityID entity_id, double amount);
    void AddToTreasury(game::types::EntityID entity_id, double amount);
    double GetCurrentTreasury(game::types::EntityID entity_id) const;
    double GetMonthlyIncome(game::types::EntityID entity_id) const;

    // Crisis detection helpers
    bool DetectBudgetCrisis(const MilitaryEconomicBridgeComponent& bridge_comp) const;
    bool DetectUnpaidTroopsCrisis(const MilitaryEconomicBridgeComponent& bridge_comp) const;
    bool DetectSupplyCrisis(game::types::EntityID entity_id) const;

    // Update helpers
    void UpdateEntityBridge(game::types::EntityID entity_id,
                           MilitaryEconomicBridgeComponent& bridge_comp,
                           double delta_time);
    void UpdateHistoricalData(MilitaryEconomicBridgeComponent& bridge_comp,
                             double military_spending,
                             double military_readiness,
                             double treasury_balance);
    void LogPerformanceMetrics();

    // System references
    core::ecs::EntityManager* m_entity_manager = nullptr;
    core::ecs::MessageBus* m_message_bus = nullptr;
    game::military::MilitarySystem* m_military_system = nullptr;
    game::economy::EconomicSystem* m_economic_system = nullptr;
    game::trade::TradeSystem* m_trade_system = nullptr;

    // Configuration
    BridgeConfig m_config;

    // Performance tracking
    std::atomic<int> m_updates_this_frame{0};
    std::atomic<double> m_last_performance_log{0.0};
    std::atomic<double> m_last_maintenance_payment{0.0};
};

} // namespace integration
} // namespace mechanica
