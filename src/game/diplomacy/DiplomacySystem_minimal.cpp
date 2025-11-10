// ============================================================================
// DiplomacySystem_minimal.cpp - Clean Minimal Implementation
// Created: October 13, 2025 - ECS Integration
// Location: src/game/diplomacy/DiplomacySystem_minimal.cpp
// ============================================================================

#include "game/diplomacy/DiplomacySystem.h"
#include "core/logging/Logger.h"
#include "game/config/GameConfig.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>

namespace game::diplomacy {

    // ============================================================================
    // DiplomacySystem Implementation - Clean Minimal Version
    // ============================================================================

    DiplomacySystem::DiplomacySystem(::core::ecs::ComponentAccessManager& access_manager,
                                    ::core::ecs::MessageBus& message_bus)
        : m_access_manager(access_manager), m_message_bus(message_bus), m_initialized(false) {
        ::core::logging::LogInfo("DiplomacySystem", "DiplomacySystem created");
    }

    void DiplomacySystem::Initialize() {
        if (m_initialized) {
            return;
        }

        ::core::logging::LogInfo("DiplomacySystem", "Initializing Diplomacy System");

        // Initialize diplomatic parameters from config
        auto& config = game::config::GameConfig::Instance();
        m_base_war_weariness = config.GetDouble("diplomacy.base_war_weariness", 0.1);
        m_diplomatic_speed = config.GetDouble("diplomacy.diplomatic_speed", 1.0);
        m_alliance_reliability = config.GetDouble("diplomacy.alliance_reliability", 0.8);
        
        m_update_interval = 1.0f; // Update every second
        m_monthly_timer = 0.0f;
        m_accumulated_time = 0.0f;

        m_initialized = true;
        ::core::logging::LogInfo("DiplomacySystem", "Diplomacy System initialized successfully");
    }

    void DiplomacySystem::Update(float delta_time) {
        if (!m_initialized) {
            return;
        }

        m_accumulated_time += delta_time;
        m_monthly_timer += delta_time;

        // Regular updates (every second)
        if (m_accumulated_time >= m_update_interval) {
            // Basic update - can add realm processing here later
            ProcessDiplomaticUpdates();
            m_accumulated_time = 0.0f;
        }

        // Monthly updates (simplified)
        if (m_monthly_timer >= 30.0f) { // 30 seconds = 1 month in game time
            ProcessMonthlyDiplomacy();
            m_monthly_timer = 0.0f;
        }
    }

    void DiplomacySystem::Shutdown() {
        ::core::logging::LogInfo("DiplomacySystem", "Shutting down Diplomacy System");
        m_pending_proposals.clear();
        m_initialized = false;
    }

    ::core::threading::ThreadingStrategy DiplomacySystem::GetThreadingStrategy() const {
        return ::core::threading::ThreadingStrategy::MAIN_THREAD;
    }

    // ============================================================================
    // Core Diplomatic Actions - Simplified Implementations
    // ============================================================================

    bool DiplomacySystem::ProposeAlliance(types::EntityID proposer, types::EntityID target,
        const std::unordered_map<std::string, double>& terms) {
        
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;
        
        ::core::ecs::EntityID proposer_handle(static_cast<uint64_t>(proposer), 1);
        ::core::ecs::EntityID target_handle(static_cast<uint64_t>(target), 1);
        
        auto proposer_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(proposer_handle);
        auto target_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(target_handle);

        if (!proposer_diplomacy || !target_diplomacy) {
            // Create components if they don't exist
            if (!proposer_diplomacy) {
                CreateDiplomacyComponent(proposer);
                proposer_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(proposer_handle);
            }
            if (!target_diplomacy) {
                CreateDiplomacyComponent(target);
                target_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(target_handle);
            }
        }

        if (!proposer_diplomacy || !target_diplomacy) {
            return false;
        }

        // Check if already allied
        if (proposer_diplomacy->IsAlliedWith(target)) {
            ::core::logging::LogInfo("DiplomacySystem", 
                "Alliance proposal rejected - already allied");
            return false;
        }

        // Check if at war
        if (proposer_diplomacy->IsAtWarWith(target)) {
            ::core::logging::LogInfo("DiplomacySystem", 
                "Alliance proposal rejected - currently at war");
            return false;
        }

        // Create and store proposal
        DiplomaticProposal proposal(proposer, target, DiplomaticAction::PROPOSE_ALLIANCE);
        proposal.terms = terms;
        m_pending_proposals.push_back(proposal);

        ::core::logging::LogInfo("DiplomacySystem",
            "Alliance proposed between " + std::to_string(proposer) +
            " and " + std::to_string(target));

        return true;
    }

    bool DiplomacySystem::DeclareWar(types::EntityID aggressor, types::EntityID target, CasusBelli casus_belli) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;
        
        ::core::ecs::EntityID aggressor_handle(static_cast<uint64_t>(aggressor), 1);
        ::core::ecs::EntityID target_handle(static_cast<uint64_t>(target), 1);
        
        auto aggressor_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(aggressor_handle);
        auto target_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(target_handle);

        // Create components if they don't exist
        if (!aggressor_diplomacy) {
            CreateDiplomacyComponent(aggressor);
            aggressor_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(aggressor_handle);
        }
        if (!target_diplomacy) {
            CreateDiplomacyComponent(target);
            target_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(target_handle);
        }

        if (!aggressor_diplomacy || !target_diplomacy) {
            return false;
        }

        // Check if already at war
        if (aggressor_diplomacy->IsAtWarWith(target)) {
            return false;
        }

        // Set war relationship
        aggressor_diplomacy->SetRelation(target, DiplomaticRelation::AT_WAR);
        target_diplomacy->SetRelation(aggressor, DiplomaticRelation::AT_WAR);

        // Modify opinions negatively
        aggressor_diplomacy->ModifyOpinion(target, -50, "War declaration");
        target_diplomacy->ModifyOpinion(aggressor, -50, "War declared on us");

        ::core::logging::LogInfo("DiplomacySystem",
            "War declared: " + std::to_string(aggressor) +
            " vs " + std::to_string(target));

        return true;
    }

    // ============================================================================
    // Component Management
    // ============================================================================

    void DiplomacySystem::CreateDiplomacyComponent(types::EntityID realm_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID handle(static_cast<uint64_t>(realm_id), 1);
        
        // Check if component already exists
        auto existing = entity_manager->GetComponent<DiplomacyComponent>(handle);
        if (existing) return;

        // Create new diplomacy component
        auto diplomacy_component = std::make_shared<DiplomacyComponent>();
        diplomacy_component->personality = DiplomaticPersonality::DIPLOMATIC;
        diplomacy_component->prestige = 0.0;
        diplomacy_component->diplomatic_reputation = 1.0;

        entity_manager->AddComponent(handle, diplomacy_component);

        ::core::logging::LogInfo("DiplomacySystem", 
            "Created DiplomacyComponent for realm " + std::to_string(realm_id));
    }

    DiplomacyComponent* DiplomacySystem::GetDiplomacyComponent(types::EntityID realm_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;

        ::core::ecs::EntityID handle(static_cast<uint64_t>(realm_id), 1);
        auto component = entity_manager->GetComponent<DiplomacyComponent>(handle);
        return component.get();
    }

    const DiplomacyComponent* DiplomacySystem::GetDiplomacyComponent(types::EntityID realm_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return nullptr;

        ::core::ecs::EntityID handle(static_cast<uint64_t>(realm_id), 1);
        auto component = entity_manager->GetComponent<DiplomacyComponent>(handle);
        return component.get();
    }

    // ============================================================================
    // Helper Methods
    // ============================================================================

    void DiplomacySystem::ProcessDiplomaticUpdates() {
        // Simplified update processing - can add realm iteration here
        // For now, just process pending proposals
        ProcessPendingProposals();
    }

    void DiplomacySystem::ProcessMonthlyDiplomacy() {
        // Monthly diplomatic processing - opinion decay, treaty updates, etc.
        ::core::logging::LogInfo("DiplomacySystem", 
            "Processing monthly diplomacy updates");
    }

    void DiplomacySystem::ProcessPendingProposals() {
        // Simple proposal processing - accept some proposals for testing
        for (auto& proposal : m_pending_proposals) {
            if (proposal.is_pending) {
                // Simple AI: accept alliance proposals with 50% chance
                if (proposal.action_type == DiplomaticAction::PROPOSE_ALLIANCE) {
                    static std::random_device rd;
                    static std::mt19937 gen(rd());
                    static std::uniform_real_distribution<> dis(0.0, 1.0);
                    
                    if (dis(gen) > 0.5) {
                        EstablishAlliance(proposal.proposer, proposal.target);
                        proposal.is_pending = false;
                        
                        ::core::logging::LogInfo("DiplomacySystem",
                            "Alliance accepted between " + std::to_string(proposal.proposer) +
                            " and " + std::to_string(proposal.target));
                    }
                }
            }
        }

        // Remove processed proposals
        m_pending_proposals.erase(
            std::remove_if(m_pending_proposals.begin(), m_pending_proposals.end(),
                [](const DiplomaticProposal& p) { return !p.is_pending; }),
            m_pending_proposals.end());
    }

    void DiplomacySystem::EstablishAlliance(types::EntityID realm_a, types::EntityID realm_b) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID handle_a(static_cast<uint64_t>(realm_a), 1);
        ::core::ecs::EntityID handle_b(static_cast<uint64_t>(realm_b), 1);
        
        auto diplomacy_a = entity_manager->GetComponent<DiplomacyComponent>(handle_a);
        auto diplomacy_b = entity_manager->GetComponent<DiplomacyComponent>(handle_b);

        if (diplomacy_a && diplomacy_b) {
            diplomacy_a->SetRelation(realm_b, DiplomaticRelation::ALLIED);
            diplomacy_b->SetRelation(realm_a, DiplomaticRelation::ALLIED);
            
            diplomacy_a->ModifyOpinion(realm_b, 20, "Alliance formed");
            diplomacy_b->ModifyOpinion(realm_a, 20, "Alliance formed");

            // Create alliance treaty
            Treaty alliance_treaty(TreatyType::ALLIANCE, realm_a, realm_b);
            diplomacy_a->AddTreaty(alliance_treaty);
            diplomacy_b->AddTreaty(alliance_treaty);
        }
    }

} // namespace game::diplomacy