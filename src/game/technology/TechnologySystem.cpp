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
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        // Get all entities with ResearchComponent
        auto entities_with_research = entity_manager->GetEntitiesWithComponent<ResearchComponent>();

        for (const auto& entity_handle : entities_with_research) {
            types::EntityID entity_id = static_cast<types::EntityID>(entity_handle.id);
            auto research_comp = GetResearchComponent(entity_id);
            if (!research_comp) continue;

            // Update research progress for active technologies
            for (auto& [tech_type, state] : research_comp->technology_states) {
                if (state == ResearchState::RESEARCHING) {
                    // Calculate research progress based on budget and efficiency
                    double base_progress = 0.001 * delta_time; // 0.1% per second baseline

                    // Apply efficiency modifiers
                    double efficiency = research_comp->base_research_efficiency;
                    efficiency *= (1.0 + research_comp->literacy_bonus);
                    efficiency *= (1.0 + research_comp->trade_network_bonus);
                    efficiency *= (1.0 + research_comp->stability_bonus);

                    // Apply focus bonus if this is the focused technology
                    if (research_comp->current_focus == tech_type) {
                        efficiency *= (1.0 + research_comp->focus_bonus);
                    }

                    // Apply budget availability (if budget is insufficient, research slows)
                    double budget_factor = std::min(1.0, research_comp->monthly_research_budget / 100.0);
                    efficiency *= budget_factor;

                    // Update progress
                    double progress_increment = base_progress * efficiency;
                    research_comp->research_progress[tech_type] += progress_increment;

                    // Check if research is completed
                    if (research_comp->research_progress[tech_type] >= 1.0) {
                        research_comp->research_progress[tech_type] = 1.0;
                        research_comp->technology_states[tech_type] = ResearchState::DISCOVERED;

                        // Publish discovery event via message bus
                        // TODO: Create and publish TechnologyDiscoveryEvent
                    }
                }
                else if (state == ResearchState::IMPLEMENTING) {
                    // Gradually implement discovered technology
                    double implementation_rate = 0.0005 * delta_time; // Slower than research
                    research_comp->implementation_level[tech_type] += implementation_rate;

                    if (research_comp->implementation_level[tech_type] >= 1.0) {
                        research_comp->implementation_level[tech_type] = 1.0;
                        research_comp->technology_states[tech_type] = ResearchState::IMPLEMENTED;
                    }
                }
            }

            // Update total research investment tracking
            research_comp->total_research_investment += research_comp->monthly_research_budget * (delta_time / 30.0);
        }
    }

    void TechnologySystem::UpdateInnovationComponents(float delta_time) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        // Get all entities with InnovationComponent
        auto entities_with_innovation = entity_manager->GetEntitiesWithComponent<InnovationComponent>();

        for (const auto& entity_handle : entities_with_innovation) {
            types::EntityID entity_id = static_cast<types::EntityID>(entity_handle.id);
            auto innovation_comp = GetInnovationComponent(entity_id);
            if (!innovation_comp) continue;

            // Calculate innovation chance based on various factors
            double innovation_chance = innovation_comp->innovation_rate * delta_time / 100.0;

            // Apply innovation environment modifiers
            innovation_chance *= innovation_comp->cultural_openness;
            innovation_chance *= innovation_comp->innovation_encouragement;
            innovation_chance *= (1.0 - innovation_comp->guild_resistance);
            innovation_chance *= (1.0 - innovation_comp->religious_restriction);

            // Boost from patronage
            innovation_chance *= (1.0 + innovation_comp->royal_patronage + innovation_comp->merchant_funding);

            // Check for innovation events (simplified random check)
            double random_roll = static_cast<double>(rand()) / RAND_MAX;
            if (random_roll < innovation_chance) {
                // Record innovation attempt
                innovation_comp->innovation_attempts.push_back("Innovation attempt at time " + std::to_string(delta_time));

                // Check for breakthrough
                double breakthrough_roll = static_cast<double>(rand()) / RAND_MAX;
                if (breakthrough_roll < innovation_comp->breakthrough_chance) {
                    // Breakthrough occurred - would trigger research acceleration
                    // TODO: Implement breakthrough effects
                }
            }

            // Update innovator counts based on funding and environment
            // Innovators grow slowly with good conditions
            if (innovation_comp->innovation_encouragement > 0.6 &&
                innovation_comp->cultural_openness > 0.5) {
                double growth_rate = 0.001 * delta_time;
                innovation_comp->scholar_innovators += static_cast<uint32_t>(
                    innovation_comp->scholar_innovators * growth_rate);
                innovation_comp->craftsmen_innovators += static_cast<uint32_t>(
                    innovation_comp->craftsmen_innovators * growth_rate);
            }

            // Limit history sizes to prevent unbounded growth
            if (innovation_comp->recent_discoveries.size() > 100) {
                innovation_comp->recent_discoveries.erase(innovation_comp->recent_discoveries.begin());
            }
            if (innovation_comp->innovation_attempts.size() > 100) {
                innovation_comp->innovation_attempts.erase(innovation_comp->innovation_attempts.begin());
            }
            if (innovation_comp->failed_experiments.size() > 100) {
                innovation_comp->failed_experiments.erase(innovation_comp->failed_experiments.begin());
            }
        }
    }

    void TechnologySystem::UpdateKnowledgeComponents(float delta_time) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        // Get all entities with KnowledgeComponent
        auto entities_with_knowledge = entity_manager->GetEntitiesWithComponent<KnowledgeComponent>();

        for (const auto& entity_handle : entities_with_knowledge) {
            types::EntityID entity_id = static_cast<types::EntityID>(entity_handle.id);
            auto knowledge_comp = GetKnowledgeComponent(entity_id);
            if (!knowledge_comp) continue;

            // Process knowledge decay
            double decay_amount = knowledge_comp->knowledge_loss_rate * delta_time / 30.0; // Monthly rate
            for (auto& [tech_type, knowledge_level] : knowledge_comp->specific_knowledge) {
                knowledge_level *= (1.0 - decay_amount);

                // Preserve knowledge if preservation quality is high
                if (knowledge_comp->knowledge_preservation_quality > 0.7) {
                    knowledge_level = std::max(knowledge_level, 0.5); // Keep at least 50%
                }
            }

            // Process manuscript production
            double monthly_production = knowledge_comp->book_production_capacity * delta_time / 30.0;
            double manuscripts_produced = monthly_production * knowledge_comp->scribes;
            knowledge_comp->manuscripts += static_cast<uint32_t>(manuscripts_produced);

            // Manuscripts also decay over time due to wear
            double manuscript_decay = knowledge_comp->manuscripts *
                (1.0 - knowledge_comp->manuscript_durability) * delta_time / 365.0; // Annual decay
            knowledge_comp->manuscripts -= static_cast<uint32_t>(manuscript_decay);

            // Process knowledge transmission to connected entities
            for (const auto& [connected_entity, connection_strength] : knowledge_comp->knowledge_connections) {
                auto target_knowledge = GetKnowledgeComponent(connected_entity);
                if (!target_knowledge) continue;

                // Transfer knowledge based on transmission rate and connection strength
                double transmission_amount = knowledge_comp->knowledge_transmission_rate *
                    connection_strength * delta_time / 30.0;

                // Transfer specific knowledge
                for (const auto& [tech_type, our_knowledge] : knowledge_comp->specific_knowledge) {
                    double& target_knowledge_level = target_knowledge->specific_knowledge[tech_type];
                    double knowledge_gap = our_knowledge - target_knowledge_level;

                    if (knowledge_gap > 0) {
                        // Transfer some of our knowledge
                        double transfer = knowledge_gap * transmission_amount *
                            target_knowledge->cultural_knowledge_absorption;
                        target_knowledge_level += transfer * target_knowledge->foreign_knowledge_acceptance;
                    }
                }
            }

            // Update category knowledge depth based on specific technologies
            for (int i = 0; i < static_cast<int>(TechnologyCategory::COUNT); ++i) {
                auto category = static_cast<TechnologyCategory>(i);
                double total_knowledge = 0.0;
                int count = 0;

                for (const auto& [tech_type, knowledge_level] : knowledge_comp->specific_knowledge) {
                    // Simple heuristic: check if technology belongs to category
                    int tech_id = static_cast<int>(tech_type);
                    int category_base = i * 100 + 1000;
                    if (tech_id >= category_base && tech_id < category_base + 100) {
                        total_knowledge += knowledge_level;
                        count++;
                    }
                }

                if (count > 0) {
                    knowledge_comp->knowledge_depth[category] = total_knowledge / count;
                }
            }

            // Limit history sizes
            if (knowledge_comp->knowledge_acquisitions.size() > knowledge_comp->translation_projects.size() + 100) {
                knowledge_comp->knowledge_acquisitions.erase(knowledge_comp->knowledge_acquisitions.begin());
            }
            if (knowledge_comp->knowledge_losses.size() > 100) {
                knowledge_comp->knowledge_losses.erase(knowledge_comp->knowledge_losses.begin());
            }
            if (knowledge_comp->translation_projects.size() > 100) {
                knowledge_comp->translation_projects.erase(knowledge_comp->translation_projects.begin());
            }
        }
    }

    void TechnologySystem::ProcessTechnologyEvents(float delta_time) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        // Get all entities with TechnologyEventsComponent
        auto entities_with_events = entity_manager->GetEntitiesWithComponent<TechnologyEventsComponent>();

        for (const auto& entity_handle : entities_with_events) {
            types::EntityID entity_id = static_cast<types::EntityID>(entity_handle.id);
            auto events_comp = GetTechnologyEventsComponent(entity_id);
            if (!events_comp) continue;

            // Update time counters
            double months_elapsed = delta_time / 30.0; // Approximate days to months
            events_comp->months_since_last_discovery += static_cast<uint32_t>(months_elapsed);
            events_comp->months_since_last_innovation += static_cast<uint32_t>(months_elapsed);
            events_comp->months_since_last_breakthrough += static_cast<uint32_t>(months_elapsed);

            // Check for new technology discoveries
            auto research_comp = GetResearchComponent(entity_id);
            if (research_comp) {
                for (const auto& [tech_type, state] : research_comp->technology_states) {
                    // Check if this is a new discovery
                    if (state == ResearchState::DISCOVERED &&
                        events_comp->discovery_dates.find(tech_type) == events_comp->discovery_dates.end()) {

                        // Record the discovery
                        events_comp->discovery_dates[tech_type] = std::chrono::system_clock::now();
                        events_comp->discovery_methods[tech_type] = DiscoveryMethod::RESEARCH;
                        events_comp->discovery_investments[tech_type] = research_comp->research_progress[tech_type];

                        // Add to recent discoveries
                        events_comp->recent_discoveries.push_back(
                            "Technology discovered: " + std::to_string(static_cast<int>(tech_type)));

                        // Add to research breakthroughs
                        events_comp->research_breakthroughs.push_back(
                            "Research breakthrough: " + std::to_string(static_cast<int>(tech_type)));

                        // Reset discovery counter
                        events_comp->months_since_last_discovery = 0;

                        // Increase technological reputation
                        events_comp->technological_reputation += 0.1;
                        events_comp->technological_reputation = std::min(events_comp->technological_reputation, 10.0);

                        // Increase scholarly recognition
                        events_comp->scholarly_recognition += 0.05;
                        events_comp->scholarly_recognition = std::min(events_comp->scholarly_recognition, 10.0);
                    }

                    // Check for technology implementation completion
                    if (state == ResearchState::IMPLEMENTED &&
                        std::find(events_comp->technology_adoptions.begin(),
                                events_comp->technology_adoptions.end(),
                                "Implemented: " + std::to_string(static_cast<int>(tech_type)))
                        == events_comp->technology_adoptions.end()) {

                        // Record the adoption
                        events_comp->technology_adoptions.push_back(
                            "Implemented: " + std::to_string(static_cast<int>(tech_type)));

                        // Add to improvements
                        events_comp->technology_improvements.push_back(
                            "Technology fully integrated: " + std::to_string(static_cast<int>(tech_type)));

                        // Increase innovation prestige
                        events_comp->innovation_prestige += 0.08;
                        events_comp->innovation_prestige = std::min(events_comp->innovation_prestige, 10.0);
                    }
                }

                // Track active research projects
                events_comp->active_research_projects.clear();
                for (const auto& [tech_type, state] : research_comp->technology_states) {
                    if (state == ResearchState::RESEARCHING) {
                        events_comp->active_research_projects.push_back(
                            "Researching: " + std::to_string(static_cast<int>(tech_type)));
                    }
                }
            }

            // Update monthly progress history
            if (research_comp) {
                for (const auto& [tech_type, progress] : research_comp->research_progress) {
                    events_comp->monthly_progress_history[tech_type] = progress;
                }
            }

            // Decay reputation over time if no new discoveries
            if (events_comp->months_since_last_discovery > 12) {
                events_comp->technological_reputation *= 0.99; // Slow decay
            }
            if (events_comp->months_since_last_innovation > 12) {
                events_comp->innovation_prestige *= 0.99;
            }

            // Limit history sizes to prevent unbounded growth
            if (events_comp->recent_discoveries.size() > events_comp->max_history_size) {
                events_comp->recent_discoveries.erase(events_comp->recent_discoveries.begin());
            }
            if (events_comp->research_breakthroughs.size() > events_comp->max_history_size) {
                events_comp->research_breakthroughs.erase(events_comp->research_breakthroughs.begin());
            }
            if (events_comp->innovation_successes.size() > events_comp->max_history_size) {
                events_comp->innovation_successes.erase(events_comp->innovation_successes.begin());
            }
            if (events_comp->technology_adoptions.size() > events_comp->max_history_size) {
                events_comp->technology_adoptions.erase(events_comp->technology_adoptions.begin());
            }
            if (events_comp->implementation_challenges.size() > events_comp->max_history_size) {
                events_comp->implementation_challenges.erase(events_comp->implementation_challenges.begin());
            }
            if (events_comp->research_failures.size() > events_comp->max_history_size) {
                events_comp->research_failures.erase(events_comp->research_failures.begin());
            }
        }
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