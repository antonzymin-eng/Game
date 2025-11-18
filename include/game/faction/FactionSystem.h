// ============================================================================
// FactionSystem.h - Faction System Management
// Created: November 18, 2025 - Faction System Implementation
// Location: include/game/faction/FactionSystem.h
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/ECS/ISystem.h"
#include "core/threading/ThreadedSystemManager.h"
#include "game/faction/FactionComponents.h"
#include "core/types/game_types.h"

#include <vector>
#include <string>
#include <memory>
#include <random>

namespace game::faction {

    // Forward declaration
    struct FactionSystemConfig;

    // ============================================================================
    // Faction System Configuration
    // ============================================================================

    struct FactionSystemConfig {
        // Update frequencies
        double monthly_update_interval = 30.0; // 30 days in-game

        // Faction influence parameters
        double base_influence_decay = 0.01;    // Monthly decay if inactive
        double min_influence = 0.05;
        double max_influence = 0.95;

        // Loyalty parameters
        double base_loyalty = 0.7;
        double loyalty_decay_rate = 0.005;     // Monthly decay
        double loyalty_gain_from_concession = 0.15;
        double loyalty_loss_from_rejection = 0.20;

        // Satisfaction parameters
        double base_satisfaction = 0.6;
        double satisfaction_decay_rate = 0.01; // Monthly decay
        double satisfaction_from_demand_fulfilled = 0.20;
        double satisfaction_from_demand_rejected = -0.25;

        // Revolt mechanics
        double revolt_risk_threshold = 0.7;    // Above this = high revolt risk
        double revolt_base_chance = 0.05;      // 5% base chance per month when angry
        double revolt_loyalty_modifier = -2.0;  // Multiplier when loyalty is low
        double revolt_satisfaction_modifier = -1.5;

        // Demand generation
        double demand_base_rate = 0.1;         // 10% chance per faction per month
        double demand_rate_if_dissatisfied = 0.3; // 30% if satisfaction < 0.4
        double ultimatum_threshold = 0.3;      // Below this satisfaction = ultimatum

        // Power dynamics
        double power_shift_rate = 0.02;        // Monthly power redistribution
        double coalition_formation_threshold = 0.6; // Similarity needed
        double coalition_stability = 0.9;      // Monthly survival chance

        // Economic factors
        double wealth_influence_multiplier = 0.3;
        double economic_contribution_base = 100.0;

        // Faction-specific modifiers
        std::unordered_map<FactionType, double> faction_militancy;
        std::unordered_map<FactionType, double> faction_influence_base;
        std::unordered_map<FactionType, double> faction_cohesion_base;

        // Initialize faction-specific defaults
        void InitializeDefaults();
    };

    // ============================================================================
    // FactionSystem - Manages faction politics and dynamics
    // ============================================================================

    class FactionSystem : public game::core::ISystem {
    public:
        explicit FactionSystem(::core::ecs::ComponentAccessManager& access_manager,
                              ::core::threading::ThreadSafeMessageBus& message_bus);

        virtual ~FactionSystem() = default;

        // ISystem interface
        void Initialize() override;
        void Update(float delta_time) override;
        void Shutdown() override;

        // Threading configuration
        ::core::threading::ThreadingStrategy GetThreadingStrategy() const override;
        std::string GetThreadingRationale() const;

        // Serialization interface
        std::string GetSystemName() const override;
        Json::Value Serialize(int version) const override;
        bool Deserialize(const Json::Value& data, int version) override;

        // Faction initialization
        void InitializeFactions(game::types::EntityID entity_id);
        void CreateDefaultFactions(game::types::EntityID entity_id);

        // Faction management
        void AddFaction(game::types::EntityID entity_id, const FactionData& faction);
        void RemoveFaction(game::types::EntityID entity_id, FactionType type);
        FactionData* GetFaction(game::types::EntityID entity_id, FactionType type);

        // Influence and power
        void AdjustInfluence(game::types::EntityID entity_id, FactionType faction, double change, const std::string& reason);
        void AdjustLoyalty(game::types::EntityID entity_id, FactionType faction, double change, const std::string& reason);
        void AdjustSatisfaction(game::types::EntityID entity_id, FactionType faction, double change, const std::string& reason);
        void RecalculatePowerBalance(game::types::EntityID entity_id);

        // Demand management
        void GenerateDemand(game::types::EntityID entity_id, FactionType faction);
        void FulfillDemand(game::types::EntityID entity_id, FactionType faction, const std::string& demand_type);
        void RejectDemand(game::types::EntityID entity_id, FactionType faction, const std::string& demand_type);

        // Crisis and revolt management
        void CheckRevoltRisk(game::types::EntityID entity_id);
        void TriggerFactionRevolt(game::types::EntityID entity_id, FactionType faction);
        void ResolveRevolt(game::types::EntityID entity_id, FactionType faction, bool success);

        // Coalition management
        void FormCoalition(FactionType faction1, FactionType faction2, const std::string& reason);
        void DissolveCoalition(FactionType faction1, FactionType faction2, const std::string& reason);
        void UpdateCoalitions();

        // Queries and metrics
        double GetFactionInfluence(game::types::EntityID entity_id, FactionType faction) const;
        double GetFactionLoyalty(game::types::EntityID entity_id, FactionType faction) const;
        double GetFactionSatisfaction(game::types::EntityID entity_id, FactionType faction) const;
        double GetRevoltRisk(game::types::EntityID entity_id, FactionType faction) const;
        std::vector<FactionType> GetAngryFactions(game::types::EntityID entity_id) const;
        FactionType GetDominantFaction(game::types::EntityID entity_id) const;

        // National-level operations
        void UpdateNationalFactionMetrics(game::types::EntityID nation_id);
        void ProcessNationalDemands(game::types::EntityID nation_id);

        // Configuration access
        const FactionSystemConfig& GetConfiguration() const;
        void SetConfiguration(const FactionSystemConfig& config);

    private:
        // Core dependencies
        ::core::ecs::ComponentAccessManager& m_access_manager;
        ::core::threading::ThreadSafeMessageBus& m_message_bus;

        // System state
        bool m_initialized = false;
        FactionSystemConfig m_config;

        // Random number generation (for stochastic events)
        std::mt19937 m_rng;
        std::uniform_real_distribution<double> m_distribution;

        // Timing
        float m_accumulated_time = 0.0f;
        float m_monthly_timer = 0.0f;

        // System initialization
        void LoadConfiguration();
        void SubscribeToEvents();

        // Update processing
        void ProcessRegularUpdates(float delta_time);
        void ProcessMonthlyUpdates(float delta_time);
        void ProcessProvincialFactions(game::types::EntityID entity_id);

        // Faction dynamics
        void UpdateFactionInfluence(game::types::EntityID entity_id);
        void UpdateFactionLoyalty(game::types::EntityID entity_id);
        void UpdateFactionSatisfaction(game::types::EntityID entity_id);
        void UpdateFactionPower(game::types::EntityID entity_id);
        void UpdateFactionRelationships(game::types::EntityID entity_id);

        // Demand generation
        void ProcessDemandGeneration(game::types::EntityID entity_id);
        std::string GenerateDemandType(FactionType faction) const;
        std::string GenerateDemandDescription(FactionType faction, const std::string& demand_type) const;

        // Revolt mechanics
        bool ShouldRevolt(const FactionData& faction) const;
        double CalculateRevoltChance(const FactionData& faction) const;
        void ProcessRevoltAttempts(game::types::EntityID entity_id);

        // Power dynamics
        void RedistributePower(game::types::EntityID entity_id);
        void UpdateDominantFaction(game::types::EntityID entity_id);

        // Coalition mechanics
        bool ShouldFormCoalition(FactionType faction1, FactionType faction2) const;
        bool ShouldMaintainCoalition(FactionType faction1, FactionType faction2) const;
        double CalculateCoalitionCompatibility(FactionType faction1, FactionType faction2) const;

        // Event handlers
        void HandleAdministrativeEvent(const std::string& event_type, game::types::EntityID entity_id);
        void HandleEconomicChange(game::types::EntityID entity_id, double economic_change);
        void HandleMilitaryEvent(game::types::EntityID entity_id, bool is_victory);
        void HandlePolicyChange(game::types::EntityID entity_id, const std::string& policy_type);

        // Utility methods
        FactionData CreateDefaultFaction(FactionType type, FactionID id) const;
        std::string GetFactionName(FactionType type) const;
        double Clamp(double value, double min_val, double max_val) const;
        double GetRandomValue() const;
    };

} // namespace game::faction
