// Created: September 27, 2025 - 14:45 PST
// Location: include/game/economic/EconomicPopulationBridge.h
// Economic-Population Bridge System - Fixed Header with Full Compliance

#pragma once

#include "core/ECS/EntityManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/threading/ThreadedSystemManager.h"
#include "core/ECS/ISerializable.h"
#include "game/population/PopulationSystem.h"
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

// ============================================================================
// Economic-Population Integration Data Structures
// ============================================================================

struct EconomicPopulationEffects {
    double tax_rate = 0.0;
    double tax_happiness_modifier = 0.0;
    double employment_rate = 0.0;
    double average_wages = 0.0;
    double wealth_inequality = 0.0;
    double trade_income_per_capita = 0.0;
    double infrastructure_quality = 0.0;
    double public_investment = 0.0;
    double inflation_rate = 0.0;
    double economic_growth = 0.0;
};

struct PopulationEconomicContribution {
    double total_workers = 0.0;
    double skilled_worker_ratio = 0.0;
    double literacy_rate = 0.0;
    double taxable_population = 0.0;
    double tax_collection_efficiency = 0.0;
    double consumer_spending = 0.0;
    double luxury_demand = 0.0;
    double innovation_factor = 0.0;
    double productivity_modifier = 0.0;
};

// ============================================================================
// Bridge Component
// ============================================================================

struct EconomicPopulationBridgeComponent {
    EconomicPopulationEffects economic_effects;
    PopulationEconomicContribution population_contributions;

    // Historical tracking (HIGH-008 FIX: use deque for efficient history management)
    std::deque<double> happiness_history;
    std::deque<double> economic_output_history;
    
    double economic_population_balance = 0.5;
    bool economic_crisis = false;
    bool population_unrest = false;
    double crisis_severity = 0.0;
    double last_update_time = 0.0;

    // IComponent interface
    std::string Serialize() const { return "{}"; }
    bool Deserialize(const std::string& data) { return true; }
};

// ============================================================================
// Event Messages
// ============================================================================

struct EconomicCrisisEvent : public core::ecs::IMessage {
    game::types::EntityID affected_entity;
    double crisis_severity;
    std::string crisis_type;
    std::vector<std::string> contributing_factors;
    std::type_index GetTypeIndex() const override { return typeid(EconomicCrisisEvent); }
};

struct PopulationUnrestEvent : public core::ecs::IMessage {
    game::types::EntityID affected_entity;
    double unrest_level;
    std::string primary_cause;
    double affected_population_percentage;
    std::type_index GetTypeIndex() const override { return typeid(PopulationUnrestEvent); }
};

// ============================================================================
// Main Bridge System
// ============================================================================

class EconomicPopulationBridge : public game::core::ISerializable {
public:
    EconomicPopulationBridge();
    virtual ~EconomicPopulationBridge() = default;

    // System lifecycle
    void Initialize();
    void Update(core::ecs::EntityManager& entities,
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
    EconomicPopulationEffects CalculateEconomicEffects(game::types::EntityID entity_id);
    PopulationEconomicContribution CalculatePopulationContributions(game::types::EntityID entity_id);
    
    void ApplyEconomicEffectsToPopulation(game::types::EntityID entity_id,
                                          const EconomicPopulationEffects& effects);
    void ApplyPopulationContributionsToEconomy(game::types::EntityID entity_id,
                                               const PopulationEconomicContribution& contributions);

    // Crisis detection
    void ProcessCrisisDetection(game::types::EntityID entity_id);

    // System configuration
    void SetEconomicSystem(game::economy::EconomicSystem* economic_system);

    // Public metrics interface
    struct BridgeHealthMetrics {
        double economic_population_balance = 0.0;
        bool crisis_active = false;
        double crisis_severity = 0.0;
        double happiness_trend = 0.0;
        double economic_output_trend = 0.0;
        std::string primary_issue;
    };

    BridgeHealthMetrics GetBridgeHealth(game::types::EntityID entity_id) const;

private:
    // Configuration structure
    struct BridgeConfig {
        // Update intervals
        double bridge_update_interval = 1.0;
        
        // Tax happiness effects
        double tax_happiness_base_effect = -0.5;
        double tax_happiness_scaling = -0.3;
        
        // Employment effects
        double unemployment_happiness_penalty = -0.3;
        double wage_happiness_scaling = 0.2;
        
        // Inequality effects
        double inequality_threshold = 0.4;
        double inequality_happiness_penalty = -0.4;
        
        // Productivity bonuses
        double literacy_productivity_bonus = 0.3;
        double happiness_productivity_scaling = 0.2;
        
        // Crisis thresholds
        double economic_output_crisis_threshold = 0.3;
        double happiness_crisis_threshold = 0.3;
        
        // Economic parameters
        double default_tax_rate = 0.15;
        double default_wages = 50.0;
        double default_infrastructure_quality = 0.6;
        double default_inflation_rate = 0.02;
        double default_economic_growth = 0.03;
        
        // Population parameters
        double taxable_population_ratio = 0.8;
        double consumer_spending_multiplier = 0.6;
        double luxury_wealth_threshold = 50.0;
        double luxury_demand_multiplier = 0.1;
        
        // Tax collection
        double tax_collection_literacy_base = 0.5;
        double tax_collection_literacy_bonus = 0.4;
        double tax_collection_happiness_base = 0.7;
        double tax_collection_happiness_bonus = 0.3;
        
        // Infrastructure
        double infrastructure_good_threshold = 0.7;
        double infrastructure_capacity_bonus = 0.5;
        double wealth_increase_trade_multiplier = 0.1;
        
        // Crisis management
        double crisis_severity_increase = 0.1;
        double crisis_severity_decrease = 0.05;
        double crisis_reset_threshold = 0.1;
        
        // Employment thresholds
        double employment_crisis_threshold = 0.6;
        double tax_efficiency_crisis_threshold = 0.5;
        
        // Metrics
        double happiness_baseline = 0.5;
        double wealth_normalization = 100.0;
        
        // History tracking
        int max_history_size = 12;
        double performance_log_interval = 10.0;
    };

    // Internal calculation helpers
    double CalculateTaxHappinessEffect(double tax_rate, double base_happiness) const;
    double CalculateEmploymentHappinessEffect(double employment_rate, double wages) const;
    double CalculateWealthInequalityEffect(double inequality, double average_wealth) const;
    double CalculateLiteracyProductivityBonus(double literacy_rate) const;
    double CalculateHappinessProductivityBonus(double happiness_level) const;
    double CalculateTaxCollectionEfficiency(double literacy_rate, double happiness_level) const;

    // Crisis detection helpers
    bool DetectEconomicCrisis(const EconomicPopulationBridgeComponent& bridge_comp) const;
    bool DetectPopulationCrisis(const EconomicPopulationBridgeComponent& bridge_comp) const;

    // Update helpers
    void UpdateEntityBridge(game::types::EntityID entity_id,
                           EconomicPopulationBridgeComponent& bridge_comp,
                           double delta_time);
    void UpdateHistoricalData(EconomicPopulationBridgeComponent& bridge_comp,
                              double happiness,
                              double economic_output);
    void LogPerformanceMetrics();

    // System references
    core::ecs::EntityManager* m_entity_manager = nullptr;
    ::core::threading::ThreadSafeMessageBus* m_message_bus = nullptr;
    game::economy::EconomicSystem* m_economic_system = nullptr;

    // Configuration
    BridgeConfig m_config;

    // Performance tracking
    std::atomic<int> m_updates_this_frame{0};
    std::atomic<double> m_last_performance_log{0.0};
};

} // namespace integration
} // namespace mechanica
