// Created: October 31, 2025
// Location: include/game/economy/TechnologyEconomicBridge.h
// Technology-Economic Bridge System - Bidirectional Integration

#pragma once

#include "core/ECS/EntityManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/threading/ThreadedSystemManager.h"
#include "core/ECS/ISerializable.h"
#include "game/technology/TechnologySystem.h"
#include "game/economy/EconomicSystem.h"
#include "game/config/GameConfig.h"
#include "core/types/game_types.h"
#include "utils/PlatformCompat.h"

#include <vector>
#include <deque>
#include <unordered_map>
#include <string>
#include <atomic>

namespace mechanica {
namespace integration {

// Use global namespace for ECS types to avoid ambiguity
using EntityManager = ::core::ecs::EntityManager;

// ============================================================================
// Technology-Economic Integration Data Structures
// ============================================================================

struct TechnologyEconomicEffects {
    // Production and efficiency multipliers
    double production_efficiency = 1.0;      // Agricultural + Craft tech
    double trade_efficiency = 1.0;           // Naval + Craft tech
    double tax_efficiency = 1.0;             // Administrative tech
    double infrastructure_multiplier = 1.0;  // All tech categories

    // Market and innovation effects
    double market_sophistication = 0.5;      // Academic + Craft tech
    double innovation_rate_modifier = 1.0;   // Academic tech
    double knowledge_transmission_rate = 0.5; // Academic tech + infrastructure

    // Military-economic effects
    double military_maintenance_efficiency = 1.0; // Military tech reduces costs
    double fortification_cost_modifier = 1.0;     // Military tech reduces construction costs

    // Technology implementation costs
    double total_implementation_cost = 0.0;
    double monthly_research_cost = 0.0;
    double infrastructure_upgrade_cost = 0.0;
};

struct EconomicTechnologyContribution {
    // Research funding and capacity
    double research_budget = 0.0;            // Treasury allocation
    double research_budget_percentage = 0.0; // % of income allocated
    double total_research_capacity = 0.0;    // Infrastructure + funding

    // Infrastructure that supports research
    double research_infrastructure_count = 0.0; // Universities, libraries, etc.
    double research_infrastructure_quality = 0.0;

    // Economic factors affecting innovation
    double trade_network_bonus = 0.0;        // Trade routes bring knowledge
    double wealth_innovation_bonus = 0.0;    // Wealthy provinces innovate more
    double economic_stability_modifier = 1.0; // Stability enables long-term research

    // Resource availability
    double scholar_funding = 0.0;            // Ability to hire scholars
    double workshop_funding = 0.0;           // Ability to support craftsmen
    double manuscript_production = 0.0;      // Book production capacity

    // Investment capacity
    double infrastructure_investment = 0.0;  // Monthly investment in research facilities
    double innovation_investment = 0.0;      // Investment in innovation
};

// ============================================================================
// Bridge Component
// ============================================================================

struct TechnologyEconomicBridgeComponent {
    TechnologyEconomicEffects technology_effects;
    EconomicTechnologyContribution economic_contributions;

    // Historical tracking (HIGH-008 FIX: use deque for efficient history management)
    std::deque<double> technology_level_history;
    std::deque<double> research_investment_history;
    std::deque<double> economic_impact_history;

    // Balance and crisis metrics
    double tech_economic_balance = 0.5;      // 0 = economy limited, 1 = tech limited
    bool research_funding_crisis = false;    // Not enough money for research
    bool implementation_cost_crisis = false; // Can't afford to implement tech
    bool brain_drain_active = false;         // Scholars leaving due to poor funding

    double crisis_severity = 0.0;
    double last_update_time = 0.0;

    // Efficiency metrics
    double research_roi = 0.0;               // Return on research investment
    double technology_utilization = 0.0;     // % of discovered tech actually used

    // IComponent interface
    std::string Serialize() const { return "{}"; }
    bool Deserialize(const std::string& data) { return true; }
};

// ============================================================================
// Event Messages
// ============================================================================

struct TechnologyBreakthroughEconomicImpact : public core::ecs::IMessage {
    game::types::EntityID affected_entity;
    game::technology::TechnologyType technology;
    double economic_impact;          // Immediate economic boost
    double efficiency_gain;          // Ongoing efficiency improvement
    double implementation_cost;      // Cost to implement
    std::type_index GetTypeIndex() const override {
        return typeid(TechnologyBreakthroughEconomicImpact);
    }
    ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
};

struct ResearchFundingCrisis : public core::ecs::IMessage {
    game::types::EntityID affected_entity;
    double funding_shortfall;        // How much money is missing
    double research_slowdown;        // Research speed reduction factor
    std::vector<game::technology::TechnologyType> affected_technologies;
    std::string crisis_cause;        // "treasury_empty", "war_spending", etc.
    std::type_index GetTypeIndex() const override {
        return typeid(ResearchFundingCrisis);
    }
    ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
};

struct BrainDrainEvent : public core::ecs::IMessage {
    game::types::EntityID affected_entity;
    int scholars_lost;
    int inventors_lost;
    double innovation_rate_penalty;
    std::string cause;               // "poor_funding", "better_opportunities"
    std::type_index GetTypeIndex() const override {
        return typeid(BrainDrainEvent);
    }
    ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
};

struct TechnologyImplementationComplete : public core::ecs::IMessage {
    game::types::EntityID affected_entity;
    game::technology::TechnologyType technology;
    double total_cost;
    double efficiency_bonus;
    std::vector<std::string> economic_benefits;
    std::type_index GetTypeIndex() const override {
        return typeid(TechnologyImplementationComplete);
    }
    ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
};

// ============================================================================
// Main Bridge System
// ============================================================================

class TechnologyEconomicBridge : public game::core::ISerializable {
public:
    TechnologyEconomicBridge();
    virtual ~TechnologyEconomicBridge() = default;

    // System lifecycle
    void Initialize();
    void Update(EntityManager& entities,
                ::core::threading::ThreadSafeMessageBus& message_bus,
                double delta_time);
    void Shutdown();

    // Threading interface
    core::threading::ThreadingStrategy GetThreadingStrategy() const;

    // ISerializable interface
    Json::Value Serialize(int version) const override;
    bool Deserialize(const Json::Value& data, int version) override;
    std::string GetSystemName() const override;

    // Core calculation methods
    TechnologyEconomicEffects CalculateTechnologyEffects(game::types::EntityID entity_id);
    EconomicTechnologyContribution CalculateEconomicContributions(game::types::EntityID entity_id);

    void ApplyTechnologyEffectsToEconomy(game::types::EntityID entity_id,
                                         const TechnologyEconomicEffects& effects);
    void ApplyEconomicContributionsToTechnology(game::types::EntityID entity_id,
                                                const EconomicTechnologyContribution& contributions);

    // Crisis detection
    void ProcessCrisisDetection(game::types::EntityID entity_id);

    // System configuration
    void SetTechnologySystem(game::technology::TechnologySystem* tech_system);
    void SetEconomicSystem(game::economy::EconomicSystem* economic_system);

    // Public metrics interface
    struct BridgeHealthMetrics {
        double tech_economic_balance = 0.0;
        bool crisis_active = false;
        double crisis_severity = 0.0;
        double research_investment_trend = 0.0;
        double technology_impact_trend = 0.0;
        double research_roi = 0.0;
        std::string primary_issue;
    };

    BridgeHealthMetrics GetBridgeHealth(game::types::EntityID entity_id) const;

private:
    // Configuration structure
    struct BridgeConfig {
        // Update intervals
        double bridge_update_interval = 1.0;

        // Technology effect multipliers
        double agricultural_tech_production_bonus = 0.15;    // +15% per major tech
        double craft_tech_production_bonus = 0.20;           // +20% per major tech
        double naval_tech_trade_bonus = 0.10;                // +10% per major tech
        double admin_tech_tax_bonus = 0.12;                  // +12% per major tech
        double academic_tech_innovation_bonus = 0.25;        // +25% per major tech
        double military_tech_maintenance_reduction = 0.08;   // -8% per major tech

        // Economic contribution parameters
        double research_budget_base_percentage = 0.05;       // 5% of income default
        double research_budget_wealthy_bonus = 0.03;         // +3% if wealthy
        double trade_knowledge_bonus_per_route = 0.02;       // +2% per route
        double stability_research_threshold = 0.6;           // Need 60% stability

        // Research infrastructure costs
        double university_monthly_cost = 50.0;
        double library_monthly_cost = 20.0;
        double workshop_monthly_cost = 30.0;
        double scholar_salary = 10.0;

        // Implementation costs
        double implementation_cost_multiplier = 100.0;       // Base cost per tech level
        double implementation_time_months = 12.0;            // Default 1 year

        // Crisis thresholds
        double funding_crisis_threshold = 0.3;               // <30% of needed funding
        double implementation_crisis_threshold = 0.5;        // <50% of implementation cost
        double brain_drain_threshold = 0.4;                  // <40% of scholar funding

        // ROI calculation
        double roi_calculation_period = 12.0;                // Months for ROI calc
        double min_roi_for_investment = 0.15;                // 15% minimum ROI

        // Crisis management
        double crisis_severity_increase = 0.1;
        double crisis_severity_decrease = 0.05;
        double crisis_reset_threshold = 0.1;

        // Technology levels (for categorization)
        int tech_level_primitive = 0;
        int tech_level_early = 3;
        int tech_level_intermediate = 7;
        int tech_level_advanced = 12;

        // History tracking
        int max_history_size = 12;
        double performance_log_interval = 10.0;
    };

    // Internal calculation helpers - Technology effects
    double CalculateProductionEfficiency(game::types::EntityID entity_id) const;
    double CalculateTradeEfficiency(game::types::EntityID entity_id) const;
    double CalculateTaxEfficiency(game::types::EntityID entity_id) const;
    double CalculateInfrastructureMultiplier(game::types::EntityID entity_id) const;
    double CalculateMarketSophistication(game::types::EntityID entity_id) const;
    double CalculateKnowledgeTransmission(game::types::EntityID entity_id) const;

    // Internal calculation helpers - Economic contributions
    double CalculateResearchBudget(game::types::EntityID entity_id) const;
    double CalculateResearchInfrastructure(game::types::EntityID entity_id) const;
    double CalculateTradeNetworkBonus(game::types::EntityID entity_id) const;
    double CalculateWealthInnovationBonus(game::types::EntityID entity_id) const;
    double CalculateEconomicStability(game::types::EntityID entity_id) const;

    // Technology level analysis
    int GetTechnologyLevel(game::types::EntityID entity_id,
                          game::technology::TechnologyCategory category) const;
    int GetOverallTechnologyLevel(game::types::EntityID entity_id) const;

    // Crisis detection helpers
    bool DetectResearchFundingCrisis(const TechnologyEconomicBridgeComponent& bridge_comp) const;
    bool DetectImplementationCrisis(const TechnologyEconomicBridgeComponent& bridge_comp) const;
    bool DetectBrainDrain(const TechnologyEconomicBridgeComponent& bridge_comp) const;

    // Update helpers
    void UpdateEntityBridge(game::types::EntityID entity_id,
                           TechnologyEconomicBridgeComponent& bridge_comp,
                           double delta_time);
    void UpdateHistoricalData(TechnologyEconomicBridgeComponent& bridge_comp,
                              double tech_level,
                              double research_investment,
                              double economic_impact);
    void CalculateROI(TechnologyEconomicBridgeComponent& bridge_comp) const;
    void LogPerformanceMetrics();

    // System references
    EntityManager* m_entity_manager = nullptr;
    ::core::threading::ThreadSafeMessageBus* m_message_bus = nullptr;
    game::technology::TechnologySystem* m_technology_system = nullptr;
    game::economy::EconomicSystem* m_economic_system = nullptr;

    // Configuration
    BridgeConfig m_config;

    // Performance tracking
    std::atomic<int> m_updates_this_frame{0};
    std::atomic<double> m_last_performance_log{0.0};
};

} // namespace integration
} // namespace mechanica
