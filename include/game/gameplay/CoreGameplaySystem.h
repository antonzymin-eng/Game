// ============================================================================
// Date/Time Created: Sunday, October 26, 2025 - 2:30 PM PST
// Intended Folder Location: include/game/gameplay/CoreGameplaySystem.h
// Mechanica Imperii - Core Gameplay System (Fixed with Complete Declarations)
// ============================================================================

#pragma once

#include "core/types/game_types.h"
#include "core/ECS/MessageBus.h"
#include "core/ECS/ISerializable.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <functional>
#include <random>

namespace game::gameplay {

    // Forward declarations
    class GameplayCoordinator;
    class DecisionConsequenceSystem;
    class DelegationSystem;
    class QuietPeriodManager;

    // ============================================================================
    // Configuration & Settings
    // ============================================================================

    enum class ComplexityLevel {
        SIMPLIFIED,
        INTERMEDIATE,
        REALISTIC,
        EXPERT
    };

    struct ComplexitySettings {
        ComplexityLevel overall_level = ComplexityLevel::INTERMEDIATE;
        std::unordered_map<types::SystemType, bool> simplified_systems;
        
        // Progressive unlock settings
        bool enable_progressive_unlock = true;
        int unlock_year_interval = 10;
        bool player_can_enable_early = true;

        // Delegation granularity
        bool allow_system_delegation = true;
        bool allow_regional_delegation = true;
        bool allow_task_delegation = true;

        // Quiet period management
        bool enable_quiet_period_acceleration = true;
        double max_acceleration_factor = 5.0;
        
        // Configurable thresholds
        int quiet_period_decision_threshold = 2;
        int quiet_period_event_threshold = 3;
        double escalation_performance_threshold = 0.3;
        
        // Memory management
        int max_stored_decisions = 100;
        int max_stored_consequences = 500;
        std::chrono::hours consequence_retention_time{24};
    };

    // ============================================================================
    // Decision System
    // ============================================================================

    enum class DecisionScope {
        LOCAL,
        REGIONAL,
        NATIONAL,
        HISTORIC
    };

    enum class ConsequenceSeverity {
        MINOR,
        MODERATE,
        MAJOR,
        CRITICAL
    };

    struct Decision {
        types::DecisionType type = types::DecisionType::INVALID;
        types::SystemType system = types::SystemType::INVALID;
        types::FunctionType function = types::FunctionType::INVALID;
        types::RegionType region = types::RegionType::INVALID;
        types::SituationType situation = types::SituationType::ROUTINE;

        std::string id;
        std::string title;
        std::string description;

        DecisionScope scope = DecisionScope::LOCAL;
        std::vector<std::string> choices;
        std::chrono::steady_clock::time_point created_time;
        bool urgent = false;
        
        // Importance weighting for escalation
        double importance_weight = 1.0;

        // Helper methods
        std::string GetSystemName() const;
        std::string GetFunctionName() const;
        std::string GetRegionName() const;
        bool IsEconomicDecision() const;
        bool IsMilitaryDecision() const;
        bool IsAdministrativeDecision() const;
        
        // FIXED: Add missing importance methods
        void SetImportanceWeight(double weight);
        double GetNormalizedImportance() const;
        bool IsHighImportance() const;
        bool IsLowImportance() const;
    };

    struct Consequence {
        std::string id;
        types::DecisionType source_decision_type;
        types::SystemType affected_system;

        DecisionScope scope;
        ConsequenceSeverity severity;
        ConsequenceSeverity escalated_severity;

        std::string description;
        std::unordered_map<std::string, double> stat_changes;
        std::vector<std::string> triggered_events;
        std::chrono::steady_clock::time_point when_occurred;
        
        // Duration for lasting effects
        std::chrono::hours duration{1};
        bool is_permanent = false;
        bool has_expired = false;

        // Helper methods
        bool AffectsSystem(types::SystemType system_type) const;
        bool RequiresPlayerAttention() const;
        bool IsExpired() const;
    };

    // Performance tracking with weighted averaging
    struct SystemPerformanceTracker {
        double current_performance = 0.5;
        double learning_rate = 0.2;
        int total_decisions = 0;
        std::chrono::steady_clock::time_point last_update;
        
        void UpdatePerformance(double new_quality);
        double GetPerformance() const { return current_performance; }
        bool HasSufficientData() const { return total_decisions >= 3; }
    };

    class DecisionConsequenceSystem : public game::core::ISerializable {
    private:
        friend class GameplayCoordinator;  // Allow access to m_system_performance
        
        std::vector<Decision> m_active_decisions;
        std::vector<Consequence> m_active_consequences;
        std::unordered_map<types::DecisionType, std::vector<types::DecisionType>> m_decision_relationships;
        
        // Performance tracking
        std::unordered_map<types::SystemType, SystemPerformanceTracker> m_system_performance;
        
        // Deterministic random generation
        std::mt19937 m_random_generator;
        bool m_deterministic_mode = false;
        
        // Configuration
        ComplexitySettings m_settings;

    public:
        DecisionConsequenceSystem(const ComplexitySettings& settings, 
                                uint32_t random_seed = 0);
        ~DecisionConsequenceSystem() = default;

        // Decision management
        void PresentDecision(const Decision& decision);
        void MakeDecision(const std::string& decision_id, const std::string& choice_id);
        
        // Memory management
        void CleanupOldDecisions();
        void CleanupExpiredConsequences();
        void Update(double delta_time);

        // Consequence escalation system
        void CheckEscalation(Consequence& consequence, const Decision& source_decision);
        void EscalateConsequence(Consequence& consequence, double escalation_factor);

        // Analysis
        double GetSystemPerformance(types::SystemType system) const;
        std::vector<Decision> GetDecisionsBySystem(types::SystemType system) const;
        std::vector<Consequence> GetConsequencesBySystem(types::SystemType system) const;

        // Getters
        const std::vector<Decision>& GetActiveDecisions() const { return m_active_decisions; }
        const std::vector<Consequence>& GetActiveConsequences() const { return m_active_consequences; }

        // Serialization support
        Json::Value Serialize(int version) const override;
        bool Deserialize(const Json::Value& data, int version) override;
        std::string GetSystemName() const override { return "DecisionConsequenceSystem"; }

    private:
        void UpdateSystemQuality(const Decision& decision, double quality);
        double CalculateChoiceQuality(const Decision& decision, const std::string& choice_id);
        Consequence GenerateConsequence(const Decision& decision, const std::string& choice_id, double quality);
        void GenerateStatChanges(Consequence& consequence, const Decision& decision, double quality);
        std::string GenerateConsequenceDescription(const Decision& decision, const std::string& choice_id, double quality);
        
        // Context-sensitive escalation
        bool ShouldEscalate(const Consequence& consequence, const Decision& source_decision) const;
        double CalculateEscalationFactor(const Decision& source_decision) const;
        
        // FIXED: Add missing serialization helpers
        Json::Value SerializeDecision(const Decision& decision) const;
        Decision DeserializeDecision(const Json::Value& data) const;
        Json::Value SerializeConsequence(const Consequence& consequence) const;
        Consequence DeserializeConsequence(const Json::Value& data) const;
    };

    // ============================================================================
    // Delegation System
    // ============================================================================

    enum class DelegationLevel {
        FULL_CONTROL,
        ADVISORY,
        SUPERVISED,
        AUTONOMOUS
    };

    struct DelegationRule {
        std::string id;
        std::string name;
        types::DelegationType type;
        DelegationLevel level;

        // Coverage definitions
        std::vector<types::SystemType> covered_systems;
        std::vector<types::RegionType> covered_regions;
        std::vector<types::FunctionType> covered_functions;
        std::vector<types::SituationType> covered_situations;

        // Conditions
        bool applies_during_war = true;
        bool applies_during_peace = true;
        bool applies_during_crisis = false;
        double minimum_treasury = 0.0;
        double maximum_threat_level = 1.0;

        // Council member assigned
        std::string assigned_council_member;
        double council_competence = 0.7;
        double council_loyalty = 0.8;
        
        // Track delegation performance
        SystemPerformanceTracker performance_tracker;

        // Helper methods
        bool CoversSystem(types::SystemType system) const;
        bool CoversFunction(types::FunctionType function) const;
        bool CoversRegion(types::RegionType region) const;
        bool CoversSituation(types::SituationType situation) const;
    };

    class DelegationSystem : public game::core::ISerializable {
    private:
        std::vector<DelegationRule> m_active_delegations;
        std::unordered_map<types::SystemType, std::function<void()>> m_automated_functions;
        ComplexitySettings m_settings;
        
        // Connection to consequence system
        DecisionConsequenceSystem* m_consequence_system = nullptr;

    public:
        DelegationSystem(const ComplexitySettings& settings);
        ~DelegationSystem() = default;

        // Connect to consequence system
        void SetConsequenceSystem(DecisionConsequenceSystem* consequence_system);

        // Set up delegation rules
        void CreateDelegationRule(const DelegationRule& rule);
        void SetupBeginnerDelegation();
        void SetupRegionalDelegation(const std::vector<types::RegionType>& distant_regions);
        void CreateSystemDelegation(types::SystemType system, const std::string& council_member);

        // Delegation checking
        bool ShouldDelegate(types::SystemType system, types::FunctionType function,
            types::RegionType region, types::SituationType situation);

        DelegationLevel GetDelegationLevel(types::SystemType system, types::FunctionType function,
            types::RegionType region);

        // Delegation execution
        void ExecuteDelegatedDecision(const Decision& decision);

        // Getters
        const std::vector<DelegationRule>& GetActiveDelegations() const { return m_active_delegations; }

        // Serialization support
        Json::Value Serialize(int version) const override;
        bool Deserialize(const Json::Value& data, int version) override;
        std::string GetSystemName() const override { return "DelegationSystem"; }

    private:
        bool MatchesDelegationRule(const DelegationRule& rule, types::SystemType system,
            types::FunctionType function, types::RegionType region,
            types::SituationType situation);
        double EvaluateDelegationQuality(const DelegationRule& rule, const Decision& decision);
        
        // Helper methods for delegation decision-making
        bool IsRoutineDecision(types::SystemType system, types::FunctionType function,
            types::SituationType situation);
        void GenerateDelegatedConsequence(const Decision& decision, const DelegationRule& rule);
        double CalculateDelegationEffectiveness(const DelegationRule& rule) const;
    };

    // ============================================================================
    // Quiet Period Manager
    // ============================================================================

    struct QuietPeriodMetrics {
        int pending_decisions = 0;
        int ongoing_events = 0;
        double player_activity_score = 0.0;
        std::chrono::steady_clock::time_point last_player_action;
        
        bool is_quiet_period = false;
        double current_acceleration = 1.0;
    };

    class QuietPeriodManager : public game::core::ISerializable {
    private:
        QuietPeriodMetrics m_metrics;
        ComplexitySettings m_settings;
        
        std::chrono::steady_clock::time_point m_last_check_time;

    public:
        QuietPeriodManager(const ComplexitySettings& settings);
        ~QuietPeriodManager() = default;

        // Update quiet period detection
        void Update(int pending_decisions, int ongoing_events);
        void RecordPlayerAction();

        // Acceleration management
        double GetCurrentAcceleration() const { return m_metrics.current_acceleration; }
        bool IsQuietPeriod() const { return m_metrics.is_quiet_period; }

        // Getters
        const QuietPeriodMetrics& GetMetrics() const { return m_metrics; }

        // Serialization support
        Json::Value Serialize(int version) const override;
        bool Deserialize(const Json::Value& data, int version) override;
        std::string GetSystemName() const override { return "QuietPeriodManager"; }

    private:
        bool CheckQuietPeriodConditions() const;
        double CalculateAccelerationFactor() const;
    };

    // ============================================================================
    // Gameplay Metrics
    // ============================================================================
    
    struct GameplayMetrics {
        int total_decisions_processed = 0;
        int delegated_decisions = 0;
        int player_decisions = 0;
        int escalated_consequences = 0;
        double average_decision_quality = 0.5;
        std::chrono::steady_clock::time_point last_reset;
        
        void Reset();
    };

    // ============================================================================
    // Gameplay Coordinator
    // ============================================================================

    class GameplayCoordinator : public game::core::ISerializable {
    private:
        DecisionConsequenceSystem m_decision_system;
        DelegationSystem m_delegation_system;
        QuietPeriodManager m_quiet_period_manager;
        
        ComplexitySettings m_settings;
        ::core::ecs::MessageBus* m_message_bus = nullptr;
        GameplayMetrics m_metrics;

    public:
        GameplayCoordinator(const ComplexitySettings& settings, 
                          ::core::ecs::MessageBus* message_bus,
                          uint32_t random_seed = 0);
        ~GameplayCoordinator() = default;

        // Update loop
        void Update(double delta_time);

        // Decision flow
        void PresentDecision(const Decision& decision);
        void MakePlayerDecision(const std::string& decision_id, const std::string& choice_id);
        bool PresentDecisionToPlayer(const Decision& decision);
        void HandleDelegatedDecision(const Decision& decision);

        // Configuration
        void ApplyComplexitySettings(const ComplexitySettings& new_settings);
        void UpdateComplexitySettings(const ComplexitySettings& new_settings);
        void EnableSystemComplexity(types::SystemType system, bool enable);
        const ComplexitySettings& GetSettings() const { return m_settings; }
        
        // Metrics
        GameplayMetrics GetMetrics() const;
        void ResetMetrics();

        // Subsystem access
        DecisionConsequenceSystem& GetDecisionSystem() { return m_decision_system; }
        DelegationSystem& GetDelegationSystem() { return m_delegation_system; }
        QuietPeriodManager& GetQuietPeriodManager() { return m_quiet_period_manager; }

        // Serialization support
        Json::Value Serialize(int version) const override;
        bool Deserialize(const Json::Value& data, int version) override;
        std::string GetSystemName() const override { return "GameplayCoordinator"; }

    private:
        void ProcessDecisionFlow(const Decision& decision);
        void SetupInitialDelegation();
        void UpdateGameSystems(double delta_time);
        void UpdateMetrics();
        int CountOngoingEvents() const;
        std::string GetCouncilMemberForSystem(types::SystemType system) const;
        void CreateSystemDelegation(types::SystemType system, const std::string& council_member);
        types::SystemType ExtractSystemFromDecision(const Decision& decision);
        types::FunctionType ExtractFunctionFromDecision(const Decision& decision);
        types::RegionType ExtractRegionFromDecision(const Decision& decision);
    };

} // namespace game::gameplay
