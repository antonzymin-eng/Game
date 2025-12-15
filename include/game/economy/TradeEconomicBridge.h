// Created: October 30, 2025
// Location: include/game/economy/TradeEconomicBridge.h
// Trade-Economic Bridge System - Integrates TradeSystem with EconomicSystem

#pragma once

#include "core/ECS/EntityManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/threading/ThreadedSystemManager.h"
#include "core/ECS/ISerializable.h"
#include "game/trade/TradeSystem.h"
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
using IMessage = ::core::ecs::IMessage;
using MessagePriority = ::core::ecs::MessagePriority;
using ThreadSafeMessageBus = ::core::threading::ThreadSafeMessageBus;

// ============================================================================
// Trade-Economic Integration Data Structures
// ============================================================================

struct TradeEconomicEffects {
    double trade_route_income = 0.0;
    double import_export_balance = 0.0;
    double trade_hub_value = 0.0;
    double merchant_activity_level = 0.0;
    double trade_efficiency = 1.0;
    double market_price_index = 100.0;
    double trade_volume = 0.0;
    double customs_revenue = 0.0;
    double trade_profitability = 0.0;
    double international_trade_ratio = 0.0;
};

struct EconomicTradeContribution {
    double available_capital = 0.0;
    double tax_burden = 0.0;
    double infrastructure_quality = 0.0;
    double economic_stability = 1.0;
    double population_wealth = 0.0;
    double luxury_demand = 0.0;
    double investment_capacity = 0.0;
    double trade_subsidy = 0.0;
    double market_demand_modifier = 1.0;
    double credit_rating = 0.8;
};

// ============================================================================
// Bridge Component
// ============================================================================

struct TradeEconomicBridgeComponent {
    TradeEconomicEffects trade_effects;
    EconomicTradeContribution economic_contributions;

    // Historical tracking (HIGH-008 FIX: use deque for efficient history management)
    std::deque<double> trade_income_history;
    std::deque<double> economic_health_history;

    double trade_economic_balance = 0.5;
    bool trade_crisis = false;
    bool economic_crisis = false;
    double crisis_severity = 0.0;
    double last_update_time = 0.0;

    // IComponent interface
    std::string Serialize() const { return "{}"; }
    bool Deserialize(const std::string& data) { return true; }
};

// ============================================================================
// Event Messages
// ============================================================================

struct TradeCrisisEvent : public IMessage {
    game::types::EntityID affected_entity;
    double crisis_severity;
    std::string crisis_type;
    std::vector<std::string> contributing_factors;
    std::type_index GetTypeIndex() const override { return typeid(TradeCrisisEvent); }
    MessagePriority GetPriority() const override { return MessagePriority::NORMAL; }
};

struct TradeEconomicImbalanceEvent : public IMessage {
    game::types::EntityID affected_entity;
    double imbalance_level;
    std::string primary_cause;
    bool requires_intervention;
    std::type_index GetTypeIndex() const override { return typeid(TradeEconomicImbalanceEvent); }
    MessagePriority GetPriority() const override { return MessagePriority::NORMAL; }
};

// ============================================================================
// Main Bridge System
// ============================================================================

class TradeEconomicBridge : public game::core::ISerializable {
public:
    TradeEconomicBridge();
    virtual ~TradeEconomicBridge() = default;

    // System lifecycle
    void Initialize();
    void Update(EntityManager& entities,
                ThreadSafeMessageBus& message_bus,
                double delta_time);
    void Shutdown();

    // Threading interface
    core::threading::ThreadingStrategy GetThreadingStrategy() const;

    // ISerializable interface
    Json::Value Serialize(int version) const override;
    bool Deserialize(const Json::Value& data, int version) override;
    std::string GetSystemName() const override;

    // Core calculation methods
    TradeEconomicEffects CalculateTradeEffects(game::types::EntityID entity_id);
    EconomicTradeContribution CalculateEconomicContributions(game::types::EntityID entity_id);

    void ApplyTradeEffectsToEconomy(game::types::EntityID entity_id,
                                     const TradeEconomicEffects& effects);
    void ApplyEconomicContributionsToTrade(game::types::EntityID entity_id,
                                            const EconomicTradeContribution& contributions);

    // Crisis detection
    void ProcessCrisisDetection(game::types::EntityID entity_id);

    // System configuration
    void SetEntityManager(EntityManager* entity_manager);
    void SetMessageBus(ThreadSafeMessageBus* message_bus);
    void SetTradeSystem(game::trade::TradeSystem* trade_system);
    void SetEconomicSystem(game::economy::EconomicSystem* economic_system);

    // Public metrics interface
    struct BridgeHealthMetrics {
        double trade_economic_balance = 0.0;
        bool crisis_active = false;
        double crisis_severity = 0.0;
        double trade_income_trend = 0.0;
        double economic_health_trend = 0.0;
        std::string primary_issue;
    };

    BridgeHealthMetrics GetBridgeHealth(game::types::EntityID entity_id) const;

private:
    // Configuration structure
    struct BridgeConfig {
        // Update intervals
        double bridge_update_interval = 1.0;

        // Trade income effects
        double trade_income_to_treasury_ratio = 0.9;
        double customs_tax_rate = 0.05;
        double merchant_tax_rate = 0.02;

        // Economic effects on trade
        double low_treasury_trade_penalty = 0.3;
        double treasury_threshold_ratio = 0.2;
        double high_tax_trade_penalty = 0.4;
        double tax_threshold = 0.25;

        // Infrastructure synergy
        double infrastructure_trade_bonus = 0.5;
        double infrastructure_threshold = 0.7;
        double road_network_trade_multiplier = 1.5;

        // Market dynamics
        double price_volatility_threshold = 0.3;
        double demand_supply_imbalance_threshold = 0.4;
        double luxury_wealth_threshold = 100.0;
        double luxury_demand_multiplier = 0.15;

        // Crisis thresholds
        double trade_collapse_threshold = 0.3;
        double economic_instability_threshold = 0.3;
        double imbalance_threshold = 0.6;

        // Default values
        double default_trade_efficiency = 1.0;
        double default_economic_stability = 1.0;
        double default_infrastructure_quality = 0.6;
        double default_population_wealth = 50.0;

        // Investment and subsidy
        double infrastructure_investment_trade_ratio = 0.2;
        double trade_subsidy_effectiveness = 0.3;
        double capital_availability_multiplier = 0.1;

        // Balance calculations
        double balance_trade_weight = 0.5;
        double balance_economic_weight = 0.5;

        // Crisis management
        double crisis_severity_increase = 0.15;
        double crisis_severity_decrease = 0.05;
        double crisis_reset_threshold = 0.1;

        // History tracking
        int max_history_size = 12;
        double performance_log_interval = 10.0;
    };

    // Internal calculation helpers
    double CalculateTradeIncome(const TradeEconomicEffects& effects) const;
    double CalculateCustomsRevenue(const TradeEconomicEffects& effects) const;
    double CalculateTaxPenaltyOnTrade(double tax_rate) const;
    double CalculateTreasuryConstraint(double available_capital) const;
    double CalculateInfrastructureBonus(double infrastructure_quality) const;
    double CalculateLuxuryDemand(double population_wealth) const;
    double CalculateMarketDemandModifier(double economic_stability, double population_wealth) const;

    // Crisis detection helpers
    bool DetectTradeCrisis(const TradeEconomicBridgeComponent& bridge_comp) const;
    bool DetectEconomicCrisis(const TradeEconomicBridgeComponent& bridge_comp) const;
    bool DetectImbalance(const TradeEconomicBridgeComponent& bridge_comp) const;

    // Update helpers
    void UpdateEntityBridge(game::types::EntityID entity_id,
                           TradeEconomicBridgeComponent& bridge_comp,
                           double delta_time);
    void UpdateHistoricalData(TradeEconomicBridgeComponent& bridge_comp,
                              double trade_income,
                              double economic_health);
    void LogPerformanceMetrics();

    // System references
    EntityManager* m_entity_manager = nullptr;
    ThreadSafeMessageBus* m_message_bus = nullptr;
    game::trade::TradeSystem* m_trade_system = nullptr;
    game::economy::EconomicSystem* m_economic_system = nullptr;

    // Configuration
    BridgeConfig m_config;

    // Performance tracking
    std::atomic<int> m_updates_this_frame{0};
    std::atomic<double> m_last_performance_log{0.0};
};

} // namespace integration
} // namespace mechanica
