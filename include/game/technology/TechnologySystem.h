// ============================================================================
// TechnologySystem.h - Technology System with ECS Integration
// Created: October 11, 2025
// Location: include/game/technology/TechnologySystem.h
// Complete technology system implementation
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include "core/types/game_types.h"
#include "game/technology/TechnologyComponents.h"

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <random>

namespace game::technology {

    // Forward declarations
    struct ResearchComponent;
    struct InnovationComponent;
    struct KnowledgeComponent;
    struct TechnologyEventsComponent;

    // ============================================================================
    // Simple Technology System - ECS Integration
    // ============================================================================

    class TechnologySystem {
    private:
        ::core::ecs::ComponentAccessManager& m_access_manager;
        ::core::threading::ThreadSafeMessageBus& m_message_bus;

        // Update timing
        std::chrono::steady_clock::time_point m_last_update;
        double m_update_frequency = 1.0;

        // Current game year
        int m_current_year = 1066;

        // Random number generation
        std::random_device m_random_device;
        std::mt19937 m_random_generator;

        // Helper for random numbers
        double GetRandomDouble(double min = 0.0, double max = 1.0);

    public:
        explicit TechnologySystem(::core::ecs::ComponentAccessManager& access_manager,
            ::core::threading::ThreadSafeMessageBus& message_bus);
        ~TechnologySystem();

        // Basic system interface
        void Initialize();
        void Update(float delta_time);
        void Shutdown();

        std::string GetSystemName() const { return "TechnologySystem"; }
        bool CanRunInParallel() const { return true; }
        double GetTargetUpdateRate() const { return m_update_frequency; }

        // ============================================================================
        // ECS Component Integration Methods
        // ============================================================================

        // ResearchComponent management
        bool CreateResearchComponent(types::EntityID entity_id);
        bool RemoveResearchComponent(types::EntityID entity_id);
        ResearchComponent* GetResearchComponent(types::EntityID entity_id);
        const ResearchComponent* GetResearchComponent(types::EntityID entity_id) const;
        
        // InnovationComponent management
        bool CreateInnovationComponent(types::EntityID entity_id);
        bool RemoveInnovationComponent(types::EntityID entity_id);
        InnovationComponent* GetInnovationComponent(types::EntityID entity_id);
        const InnovationComponent* GetInnovationComponent(types::EntityID entity_id) const;
        
        // KnowledgeComponent management
        bool CreateKnowledgeComponent(types::EntityID entity_id);
        bool RemoveKnowledgeComponent(types::EntityID entity_id);
        KnowledgeComponent* GetKnowledgeComponent(types::EntityID entity_id);
        const KnowledgeComponent* GetKnowledgeComponent(types::EntityID entity_id) const;
        
        // TechnologyEventsComponent management
        bool CreateTechnologyEventsComponent(types::EntityID entity_id);
        bool RemoveTechnologyEventsComponent(types::EntityID entity_id);
        TechnologyEventsComponent* GetTechnologyEventsComponent(types::EntityID entity_id);
        const TechnologyEventsComponent* GetTechnologyEventsComponent(types::EntityID entity_id) const;

        // High-level ECS integration methods
        bool InitializeTechnologyComponents(types::EntityID entity_id, int starting_year = 1066, double initial_budget = 100.0);
        bool CleanupTechnologyComponents(types::EntityID entity_id);
        
        // Component validation and diagnostics
        bool ValidateTechnologyComponents(types::EntityID entity_id) const;
        std::vector<std::string> GetTechnologyComponentStatus(types::EntityID entity_id) const;
        size_t GetTechnologyComponentCount() const;

        // Prerequisites validation
        bool CheckTechnologyPrerequisites(types::EntityID entity_id, TechnologyType technology) const;
        std::vector<TechnologyType> GetMissingPrerequisites(types::EntityID entity_id, TechnologyType technology) const;

    private:
        // Component validation helpers
        bool IsResearchComponentValid(const ResearchComponent* component) const;
        bool IsInnovationComponentValid(const InnovationComponent* component) const;
        bool IsKnowledgeComponentValid(const KnowledgeComponent* component) const;
        bool IsTechnologyEventsComponentValid(const TechnologyEventsComponent* component) const;

        // Breakthrough effects
        void ApplyBreakthroughEffects(types::EntityID entity_id, InnovationComponent* innovation_comp);

        // Component update methods
        void UpdateResearchComponents(float delta_time);
        void UpdateInnovationComponents(float delta_time);
        void UpdateKnowledgeComponents(float delta_time);
        void ProcessTechnologyEvents(float delta_time);

        // Technology processing
        void ProcessResearch(types::EntityID entity_id, TechnologyType technology);
    };

} // namespace game::technology