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

        // Administrative parameters
        double base_efficiency = 0.7;
        double corruption_base_rate = 0.05;
        double reform_cost_multiplier = 1.0;

        // Bureaucracy costs
        int clerk_monthly_salary = 10;
        int official_monthly_salary = 50;
        int judge_monthly_salary = 75;

        // Efficiency thresholds
        double min_efficiency = 0.1;
        double max_efficiency = 1.0;
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

        // Internal methods
        void CalculateEfficiency(game::types::EntityID entity_id);
        void ProcessCorruption(game::types::EntityID entity_id);
        void UpdateSalaries(game::types::EntityID entity_id);
        void GenerateAdministrativeEvents(game::types::EntityID entity_id);
    };

} // namespace game::administration
