// ============================================================================
// AdministrativeSystem.h - Administrative System Management
// Strategic Rebuild: October 21, 2025 - Following PopulationSystem pattern
// Location: include/game/administration/AdministrativeSystem.h
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "core/ECS/ISystem.h"
#include "core/threading/ThreadedSystemManager.h"
#include "game/administration/AdministrativeComponents.h"
#include "core/types/game_types.h"

#include <vector>
#include <string>
#include <memory>

namespace game::administration {

    // Forward declaration
    struct AdministrativeSystemConfig;

    // ============================================================================
    // Administrative System Configuration
    // ============================================================================

    struct AdministrativeSystemConfig {
        // Update frequencies
        double monthly_update_interval = 30.0; // 30 days in-game

        // Administrative efficiency parameters
        double base_efficiency = 0.7;
        double min_efficiency = 0.1;
        double max_efficiency = 1.0;
        
        // Corruption parameters
        double corruption_base_rate = 0.05;
        double corruption_threshold = 0.7; // When officials become corrupt
        double corruption_penalty_efficiency = 0.15; // -15% efficiency per corrupt official
        
        // Reform costs and benefits
        double reform_cost_multiplier = 1.0;
        double reform_efficiency_gain = 0.05; // +5% efficiency per reform
        double reform_corruption_reduction = 0.1; // -10% corruption per reform
        
        // Official management
        double competence_drift_rate = 0.01; // Monthly competence change
        double satisfaction_decay_rate = 0.02; // Monthly satisfaction decay
        double loyalty_bonus_per_year = 0.05; // Loyalty increases with tenure
        double experience_threshold_months = 12; // Months to gain experience trait
        
        // Salary costs (monthly)
        double tax_collector_salary = 50.0;
        double trade_minister_salary = 75.0;
        double military_governor_salary = 100.0;
        double court_advisor_salary = 80.0;
        double provincial_governor_salary = 120.0;
        double judge_salary = 90.0;
        double scribe_salary = 30.0;
        double customs_officer_salary = 60.0;
        
        // Bureaucracy costs
        double clerk_monthly_salary = 10.0;
        double bureaucracy_expansion_cost = 500.0; // Per clerk hired
        double record_keeping_improvement_cost = 1000.0; // Base cost
        
        // Efficiency bonuses from traits
        double efficient_trait_bonus = 0.15; // +15% efficiency
        double experienced_trait_bonus = 0.10; // +10% competence
        double corrupt_trait_penalty = 0.20; // -20% efficiency
        double loyal_trait_bonus = 0.05; // +5% loyalty
        
        // Population/culture modifiers (for future integration)
        double literacy_efficiency_multiplier = 0.5; // Literacy impact
        double cultural_acceptance_threshold = 0.6; // Below this = efficiency penalty
        double realm_stability_multiplier = 0.3; // Stability impact on efficiency
    };

    // ============================================================================
    // AdministrativeSystem - Strategic Rebuild Following PopulationSystem Pattern
    // ============================================================================

    class AdministrativeSystem : public game::core::ISystem {
    public:
        explicit AdministrativeSystem(::core::ecs::ComponentAccessManager& access_manager,
                                     ::core::ecs::MessageBus& message_bus);
        
        virtual ~AdministrativeSystem() = default;

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

        // Administrative management interface
        void CreateAdministrativeComponents(game::types::EntityID entity_id);
        void ProcessMonthlyUpdate(game::types::EntityID entity_id);
        
        // Official management
        bool AppointOfficial(game::types::EntityID entity_id, OfficialType type, const std::string& name);
        bool DismissOfficial(game::types::EntityID entity_id, uint32_t official_id);
        
        // Efficiency calculations
        double GetAdministrativeEfficiency(game::types::EntityID entity_id) const;
        double GetTaxCollectionRate(game::types::EntityID entity_id) const;
        double GetBureaucraticEfficiency(game::types::EntityID entity_id) const;
        
        // Governance operations
        void UpdateGovernanceType(game::types::EntityID entity_id, GovernanceType new_type);
        void ProcessAdministrativeReforms(game::types::EntityID entity_id);
        
        // Bureaucracy operations
        void ExpandBureaucracy(game::types::EntityID entity_id, uint32_t additional_clerks);
        void ImproveRecordKeeping(game::types::EntityID entity_id, double investment);
        
        // Law system operations
        void EstablishCourt(game::types::EntityID entity_id);
        void AppointJudge(game::types::EntityID entity_id, const std::string& judge_name);
        void EnactLaw(game::types::EntityID entity_id, const std::string& law_description);

        // Configuration access
        const AdministrativeSystemConfig& GetConfiguration() const;

    private:
        // Core dependencies
        ::core::ecs::ComponentAccessManager& m_access_manager;
        ::core::ecs::MessageBus& m_message_bus;

        // System state
        bool m_initialized = false;
        AdministrativeSystemConfig m_config;

        // Timing
        float m_accumulated_time = 0.0f;
        float m_monthly_timer = 0.0f;

        // System initialization
        void LoadConfiguration();
        void SubscribeToEvents();

        // Update processing
        void ProcessRegularUpdates(float delta_time);
        void ProcessMonthlyUpdates(float delta_time);

        // Event handlers
        void OnAdminAppointment(const AdminAppointmentEvent& event);
        void OnAdminDismissal(const AdminDismissalEvent& event);
        void OnAdminCorruption(const AdminCorruptionEvent& event);
        void OnAdminReform(const AdminReformEvent& event);

        // Internal methods
        void CalculateEfficiency(game::types::EntityID entity_id);
        void ProcessCorruption(game::types::EntityID entity_id);
        void UpdateSalaries(game::types::EntityID entity_id);
        void GenerateAdministrativeEvents(game::types::EntityID entity_id);
    };

} // namespace game::administration
