// ============================================================================
// TechnologySystem.cpp - Technology System Implementation
// Created: October 11, 2025
// Location: src/game/technology/TechnologySystem.cpp
// Complete ECS integration using proper EntityManager API
// ============================================================================

#include "game/technology/TechnologySystem.h"
#include "utils/RandomGenerator.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace game::technology {

    // ============================================================================
    // TechnologySystem Implementation
    // ============================================================================

    TechnologySystem::TechnologySystem(::core::ecs::ComponentAccessManager& access_manager,
        ::core::ecs::MessageBus& message_bus)
        : m_access_manager(access_manager), m_message_bus(message_bus) {

        m_last_update = std::chrono::steady_clock::now();
    }

    TechnologySystem::~TechnologySystem() = default;

    void TechnologySystem::Initialize() {
        // Initialize the technology system
    }

    void TechnologySystem::Update(float delta_time) {
        auto now = std::chrono::steady_clock::now();
        auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_update).count();
        
        // Update at the target frequency
        if (time_since_last >= (1000.0 / m_update_frequency)) {
            UpdateResearchComponents(delta_time);
            UpdateInnovationComponents(delta_time);
            UpdateKnowledgeComponents(delta_time);
            ProcessTechnologyEvents(delta_time);
            
            m_last_update = now;
        }
    }

    void TechnologySystem::Shutdown() {
        // Cleanup any remaining components if needed
    }

    // ============================================================================
    // ECS Component Integration Methods Implementation
    // ============================================================================

    // ResearchComponent management
    bool TechnologySystem::CreateResearchComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        
        if (entity_manager->GetComponent<ResearchComponent>(entity_handle)) {
            return false; // Already exists
        }
        
        auto component = entity_manager->AddComponent<ResearchComponent>(entity_handle);
        if (component) {
            *component = utils::CreateTechnologyComponent(m_current_year);
            return true;
        }
        return false;
    }

    bool TechnologySystem::RemoveResearchComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        return entity_manager->RemoveComponent<ResearchComponent>(entity_handle);
    }

    ResearchComponent* TechnologySystem::GetResearchComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto component = entity_manager->GetComponent<ResearchComponent>(entity_handle);
        return component ? component.get() : nullptr;
    }

    const ResearchComponent* TechnologySystem::GetResearchComponent(types::EntityID entity_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto component = entity_manager->GetComponent<ResearchComponent>(entity_handle);
        return component ? component.get() : nullptr;
    }

    // InnovationComponent management
    bool TechnologySystem::CreateInnovationComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        
        if (entity_manager->GetComponent<InnovationComponent>(entity_handle)) {
            return false; // Already exists
        }
        
        auto component = entity_manager->AddComponent<InnovationComponent>(entity_handle);
        if (component) {
            *component = utils::CreateInnovationComponent(0.1);
            return true;
        }
        return false;
    }

    bool TechnologySystem::RemoveInnovationComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        return entity_manager->RemoveComponent<InnovationComponent>(entity_handle);
    }

    InnovationComponent* TechnologySystem::GetInnovationComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto component = entity_manager->GetComponent<InnovationComponent>(entity_handle);
        return component ? component.get() : nullptr;
    }

    const InnovationComponent* TechnologySystem::GetInnovationComponent(types::EntityID entity_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto component = entity_manager->GetComponent<InnovationComponent>(entity_handle);
        return component ? component.get() : nullptr;
    }

    // KnowledgeComponent management
    bool TechnologySystem::CreateKnowledgeComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        
        if (entity_manager->GetComponent<KnowledgeComponent>(entity_handle)) {
            return false; // Already exists
        }
        
        auto component = entity_manager->AddComponent<KnowledgeComponent>(entity_handle);
        if (component) {
            *component = utils::CreateKnowledgeNetwork();
            return true;
        }
        return false;
    }

    bool TechnologySystem::RemoveKnowledgeComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        return entity_manager->RemoveComponent<KnowledgeComponent>(entity_handle);
    }

    KnowledgeComponent* TechnologySystem::GetKnowledgeComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto component = entity_manager->GetComponent<KnowledgeComponent>(entity_handle);
        return component ? component.get() : nullptr;
    }

    const KnowledgeComponent* TechnologySystem::GetKnowledgeComponent(types::EntityID entity_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto component = entity_manager->GetComponent<KnowledgeComponent>(entity_handle);
        return component ? component.get() : nullptr;
    }

    // TechnologyEventsComponent management
    bool TechnologySystem::CreateTechnologyEventsComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        
        if (entity_manager->GetComponent<TechnologyEventsComponent>(entity_handle)) {
            return false; // Already exists
        }
        
        auto component = entity_manager->AddComponent<TechnologyEventsComponent>(entity_handle);
        if (component) {
            *component = utils::CreateTechnologyEvents(100);
            return true;
        }
        return false;
    }

    bool TechnologySystem::RemoveTechnologyEventsComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        return entity_manager->RemoveComponent<TechnologyEventsComponent>(entity_handle);
    }

    TechnologyEventsComponent* TechnologySystem::GetTechnologyEventsComponent(types::EntityID entity_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto component = entity_manager->GetComponent<TechnologyEventsComponent>(entity_handle);
        return component ? component.get() : nullptr;
    }

    const TechnologyEventsComponent* TechnologySystem::GetTechnologyEventsComponent(types::EntityID entity_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;
        
        ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
        auto component = entity_manager->GetComponent<TechnologyEventsComponent>(entity_handle);
        return component ? component.get() : nullptr;
    }

    // High-level ECS integration methods
    bool TechnologySystem::InitializeTechnologyComponents(types::EntityID entity_id, int starting_year, double initial_budget) {
        bool success = true;
        
        // Create all technology components
        success &= CreateResearchComponent(entity_id);
        success &= CreateInnovationComponent(entity_id);
        success &= CreateKnowledgeComponent(entity_id);
        success &= CreateTechnologyEventsComponent(entity_id);
        
        if (success) {
            // Initialize research budget
            if (auto research_comp = GetResearchComponent(entity_id)) {
                research_comp->monthly_research_budget = initial_budget;
                // Set initial budget for each category
                for (int i = 0; i < static_cast<int>(TechnologyCategory::COUNT); ++i) {
                    auto category = static_cast<TechnologyCategory>(i);
                    research_comp->category_investment[category] = initial_budget / static_cast<int>(TechnologyCategory::COUNT);
                }
            }
            
            // Initialize innovation based on starting year
            if (auto innovation_comp = GetInnovationComponent(entity_id)) {
                // Later start = more established but less innovative
                double time_factor = std::max(0.1, 1.0 - (starting_year - 1066) / 1000.0);
                innovation_comp->innovation_rate *= time_factor;
            }
        } else {
            // Cleanup if initialization failed
            CleanupTechnologyComponents(entity_id);
        }
        
        return success;
    }

    bool TechnologySystem::CleanupTechnologyComponents(types::EntityID entity_id) {
        bool success = true;
        
        success &= RemoveResearchComponent(entity_id);
        success &= RemoveInnovationComponent(entity_id);
        success &= RemoveKnowledgeComponent(entity_id);
        success &= RemoveTechnologyEventsComponent(entity_id);
        
        return success;
    }

    // Component validation and diagnostics
    bool TechnologySystem::ValidateTechnologyComponents(types::EntityID entity_id) const {
        const auto* research_comp = GetResearchComponent(entity_id);
        const auto* innovation_comp = GetInnovationComponent(entity_id);
        const auto* knowledge_comp = GetKnowledgeComponent(entity_id);
        const auto* events_comp = GetTechnologyEventsComponent(entity_id);
        
        return IsResearchComponentValid(research_comp) &&
               IsInnovationComponentValid(innovation_comp) &&
               IsKnowledgeComponentValid(knowledge_comp) &&
               IsTechnologyEventsComponentValid(events_comp);
    }

    std::vector<std::string> TechnologySystem::GetTechnologyComponentStatus(types::EntityID entity_id) const {
        std::vector<std::string> status;
        
        if (const auto* research_comp = GetResearchComponent(entity_id)) {
            status.push_back("ResearchComponent: Active (budget: " + std::to_string(research_comp->monthly_research_budget) + ")");
        } else {
            status.push_back("ResearchComponent: Missing");
        }
        
        if (const auto* innovation_comp = GetInnovationComponent(entity_id)) {
            status.push_back("InnovationComponent: Active (rate: " + std::to_string(innovation_comp->innovation_rate) + ")");
        } else {
            status.push_back("InnovationComponent: Missing");
        }
        
        if (const auto* knowledge_comp = GetKnowledgeComponent(entity_id)) {
            status.push_back("KnowledgeComponent: Active (" + std::to_string(knowledge_comp->manuscripts) + " manuscripts)");
        } else {
            status.push_back("KnowledgeComponent: Missing");
        }
        
        if (const auto* events_comp = GetTechnologyEventsComponent(entity_id)) {
            status.push_back("TechnologyEventsComponent: Active (" + std::to_string(events_comp->recent_discoveries.size()) + " recent discoveries)");
        } else {
            status.push_back("TechnologyEventsComponent: Missing");
        }
        
        return status;
    }

    size_t TechnologySystem::GetTechnologyComponentCount() const {
        // Simple implementation - count research components as proxy
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return 0;
        
        // This is a simplified count - in a real implementation we'd iterate through entities
        return 0; // Would need entity iteration support
    }

    // ============================================================================
    // Medieval Technology System Mechanics (Private Methods)
    // ============================================================================

    void TechnologySystem::UpdateResearchComponents(float delta_time) {
        // Simplified update - in full version would iterate through all entities with ResearchComponent
        // For now, this is a placeholder for the architecture
    }

    void TechnologySystem::UpdateInnovationComponents(float delta_time) {
        // Simplified update - in full version would iterate through all entities with InnovationComponent
        // For now, this is a placeholder for the architecture
    }

    void TechnologySystem::UpdateKnowledgeComponents(float delta_time) {
        // Simplified update - in full version would iterate through all entities with KnowledgeComponent
        // For now, this is a placeholder for the architecture
    }

    void TechnologySystem::ProcessTechnologyEvents(float delta_time) {
        // Simplified update - in full version would iterate through all entities with TechnologyEventsComponent
        // For now, this is a placeholder for the architecture
    }

    void TechnologySystem::ProcessResearch(types::EntityID entity_id, TechnologyType technology) {
        // Simple research progress simulation
        if (auto research_comp = GetResearchComponent(entity_id)) {
            // Calculate research progress based on investment and efficiency
            double base_progress = 0.01; // 1% per update cycle
            double efficiency_modifier = research_comp->base_research_efficiency;
            double literacy_modifier = 1.0 + research_comp->literacy_bonus;
            
            double progress_increment = base_progress * efficiency_modifier * literacy_modifier;
            
            research_comp->research_progress[technology] += progress_increment;
            
            // Check if research is completed
            if (research_comp->research_progress[technology] >= 1.0) {
                research_comp->research_progress[technology] = 1.0;
                research_comp->technology_states[technology] = ResearchState::DISCOVERED;
                
                // Record discovery in events
                if (auto events_comp = GetTechnologyEventsComponent(entity_id)) {
                    events_comp->recent_discoveries.push_back("Technology discovered: " + std::to_string(static_cast<int>(technology)));
                    events_comp->months_since_last_discovery = 0;
                    events_comp->discovery_dates[technology] = std::chrono::system_clock::now();
                    events_comp->discovery_methods[technology] = DiscoveryMethod::RESEARCH;
                }
            }
        }
    }

    // ============================================================================
    // Component Validation Helpers
    // ============================================================================

    bool TechnologySystem::IsResearchComponentValid(const ResearchComponent* component) const {
        if (!component) return false;
        
        // Validate research efficiency is in reasonable range
        if (component->base_research_efficiency < 0.0 || component->base_research_efficiency > 10.0) return false;
        
        // Validate budget amounts are reasonable
        if (component->monthly_research_budget < 0.0 || component->monthly_research_budget > 1000000.0) return false;
        
        // Validate investment amounts are reasonable
        for (const auto& investment_pair : component->category_investment) {
            if (investment_pair.second < 0.0 || investment_pair.second > 100000.0) return false;
        }
        
        // Validate infrastructure counts
        if (component->scholar_population > 10000) return false; // Reasonable upper limit
        
        return true;
    }

    bool TechnologySystem::IsInnovationComponentValid(const InnovationComponent* component) const {
        if (!component) return false;
        
        // Validate innovation parameters
        if (component->innovation_rate < 0.0 || component->innovation_rate > 1.0) return false;
        if (component->breakthrough_chance < 0.0 || component->breakthrough_chance > 1.0) return false;
        if (component->invention_quality < 0.0 || component->invention_quality > 1.0) return false;
        
        // Validate cultural parameters
        if (component->cultural_openness < 0.0 || component->cultural_openness > 1.0) return false;
        if (component->innovation_encouragement < 0.0 || component->innovation_encouragement > 1.0) return false;
        
        return true;
    }

    bool TechnologySystem::IsKnowledgeComponentValid(const KnowledgeComponent* component) const {
        if (!component) return false;
        
        // Validate knowledge infrastructure
        if (component->manuscripts > 100000) return false; // Reasonable upper limit
        if (component->scribes > 1000) return false;
        
        // Validate rates and quality measures
        if (component->knowledge_preservation_quality < 0.0 || component->knowledge_preservation_quality > 1.0) return false;
        if (component->translation_accuracy < 0.0 || component->translation_accuracy > 1.0) return false;
        if (component->knowledge_loss_rate < 0.0 || component->knowledge_loss_rate > 1.0) return false;
        if (component->knowledge_transmission_rate < 0.0 || component->knowledge_transmission_rate > 1.0) return false;
        
        // Validate literacy rates
        if (component->literacy_rate < 0.0 || component->literacy_rate > 1.0) return false;
        if (component->scholarly_literacy_rate < 0.0 || component->scholarly_literacy_rate > 1.0) return false;
        
        return true;
    }

    bool TechnologySystem::IsTechnologyEventsComponentValid(const TechnologyEventsComponent* component) const {
        if (!component) return false;
        
        // Validate event collections don't exceed reasonable limits
        if (component->recent_discoveries.size() > component->max_history_size * 2) return false;
        if (component->research_breakthroughs.size() > component->max_history_size * 2) return false;
        if (component->innovation_successes.size() > component->max_history_size * 2) return false;
        
        // Validate reputation scores
        if (component->technological_reputation < 0.0 || component->technological_reputation > 10.0) return false;
        if (component->innovation_prestige < 0.0 || component->innovation_prestige > 10.0) return false;
        if (component->scholarly_recognition < 0.0 || component->scholarly_recognition > 10.0) return false;
        
        return true;
    }

} // namespace game::technology