// ============================================================================
// DiplomacySystem_minimal.cpp - Clean Minimal Implementation
// Created: October 13, 2025 - ECS Integration
// Location: src/game/diplomacy/DiplomacySystem_minimal.cpp
// ============================================================================

#include "game/diplomacy/DiplomacySystem.h"
#include "game/military/MilitaryComponents.h"
#include "core/logging/Logger.h"
#include "game/config/GameConfig.h"
#include <jsoncpp/json/json.h>
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
        return ::core::threading::ThreadingStrategy::BACKGROUND_THREAD;
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
        auto diplomacy_component = entity_manager->AddComponent<DiplomacyComponent>(handle);
        if (diplomacy_component) {
            diplomacy_component->personality = DiplomaticPersonality::DIPLOMATIC;
            diplomacy_component->prestige = 0.0;
            diplomacy_component->diplomatic_reputation = 1.0;
        }

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

    // ============================================================================
    // Stub Implementations for Required Methods
    // ============================================================================

    bool DiplomacySystem::ProposeTradeAgreement(types::EntityID proposer, types::EntityID target,
        double trade_bonus, int duration_years) {
        
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;

        ::core::ecs::EntityID proposer_handle(static_cast<uint64_t>(proposer), 1);
        ::core::ecs::EntityID target_handle(static_cast<uint64_t>(target), 1);

        auto proposer_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(proposer_handle);
        auto target_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(target_handle);

        if (!proposer_diplomacy || !target_diplomacy) return false;

        // Create treaty
        Treaty treaty(TreatyType::TRADE_AGREEMENT, proposer, target);
        treaty.trade_bonus = trade_bonus;
        treaty.signed_date = std::chrono::system_clock::now();
        treaty.expiry_date = treaty.signed_date + std::chrono::hours(duration_years * 8760); // years to hours
        treaty.is_active = true;
        
        // Add terms
        treaty.terms["trade_bonus"] = trade_bonus;
        treaty.terms["duration_years"] = static_cast<double>(duration_years);

        // Store treaty in both realm's components
        proposer_diplomacy->AddTreaty(treaty);
        target_diplomacy->AddTreaty(treaty);

        // Improve relations using helper methods
        proposer_diplomacy->ModifyOpinion(target, 5, "Trade agreement signed");
        target_diplomacy->ModifyOpinion(proposer, 5, "Trade agreement signed");

        // Update trade volume
        auto* proposer_rel = proposer_diplomacy->GetRelationship(target);
        auto* target_rel = target_diplomacy->GetRelationship(proposer);
        if (proposer_rel) proposer_rel->trade_volume += trade_bonus;
        if (target_rel) target_rel->trade_volume += trade_bonus;

        LogDiplomaticEvent(proposer, target, "Trade agreement signed");
        return true;
    }

    bool DiplomacySystem::SueForPeace(types::EntityID supplicant, types::EntityID victor,
        const std::unordered_map<std::string, double>& peace_terms) {
        // TODO: Implement peace negotiation
        ::core::logging::LogInfo("DiplomacySystem", 
            "SueForPeace called - stub implementation");
        return false;
    }

    bool DiplomacySystem::ArrangeMarriage(types::EntityID bride_realm, types::EntityID groom_realm,
        bool create_alliance) {
        // TODO: Implement marriage arrangement
        ::core::logging::LogInfo("DiplomacySystem", 
            "ArrangeMarriage called - stub implementation");
        return false;
    }

    void DiplomacySystem::ProcessMarriageEffects(const DynasticMarriage& marriage) {
        // TODO: Process marriage diplomatic effects
        ::core::logging::LogInfo("DiplomacySystem", 
            "ProcessMarriageEffects called - stub implementation");
    }

    bool DiplomacySystem::EstablishEmbassy(types::EntityID sender, types::EntityID host) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;

        ::core::ecs::EntityID sender_handle(static_cast<uint64_t>(sender), 1);
        ::core::ecs::EntityID host_handle(static_cast<uint64_t>(host), 1);

        auto sender_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(sender_handle);
        auto host_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(host_handle);

        if (!sender_diplomacy || !host_diplomacy) {
            ::core::logging::LogError("DiplomacySystem", 
                "Cannot establish embassy - missing diplomacy components");
            return false;
        }

        // Use helper method to modify opinion
        sender_diplomacy->ModifyOpinion(host, 10, "Embassy established");
        host_diplomacy->ModifyOpinion(sender, 10, "Embassy received");

        // Get/create relationship and update trust
        auto* relationship = sender_diplomacy->GetRelationship(host);
        if (relationship) {
            relationship->trust += 0.05;  // +5% trust
            relationship->last_contact = std::chrono::system_clock::now();
        } else {
            // Create new relationship if it doesn't exist
            sender_diplomacy->relationships[host] = DiplomaticState(host);
            auto* new_rel = sender_diplomacy->GetRelationship(host);
            if (new_rel) {
                new_rel->trust = 0.55; // Base 0.5 + 0.05 bonus
                new_rel->last_contact = std::chrono::system_clock::now();
            }
        }

        LogDiplomaticEvent(sender, host, "Embassy established");
        return true;
    }

    void DiplomacySystem::RecallAmbassador(types::EntityID sender, types::EntityID host) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID sender_handle(static_cast<uint64_t>(sender), 1);
        auto sender_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(sender_handle);

        if (!sender_diplomacy) return;

        // Apply penalties using helper method and direct access
        sender_diplomacy->ModifyOpinion(host, -15, "Ambassador recalled");
        
        auto* relationship = sender_diplomacy->GetRelationship(host);
        if (relationship) {
            relationship->trust -= 0.1;  // -10% trust penalty
            relationship->diplomatic_incidents++;
        }

        LogDiplomaticEvent(sender, host, "Ambassador recalled");
    }

    void DiplomacySystem::SendDiplomaticGift(types::EntityID sender, types::EntityID recipient, double value) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID sender_handle(static_cast<uint64_t>(sender), 1);
        ::core::ecs::EntityID recipient_handle(static_cast<uint64_t>(recipient), 1);

        auto sender_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(sender_handle);
        auto recipient_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(recipient_handle);

        if (!sender_diplomacy || !recipient_diplomacy) return;

        // Calculate opinion bonus based on gift value (5-30 points)
        int opinion_bonus = static_cast<int>(std::clamp(value / 10.0, 5.0, 30.0));
        
        // Recipient gains opinion of sender
        recipient_diplomacy->ModifyOpinion(sender, opinion_bonus, "Diplomatic gift received");
        
        // Update relationship details
        auto* recipient_rel = recipient_diplomacy->GetRelationship(sender);
        if (recipient_rel) {
            recipient_rel->trust += 0.02;
            recipient_rel->last_contact = std::chrono::system_clock::now();
        } else {
            // Create relationship if it doesn't exist
            recipient_diplomacy->relationships[sender] = DiplomaticState(sender);
            auto* new_rel = recipient_diplomacy->GetRelationship(sender);
            if (new_rel) {
                new_rel->opinion = opinion_bonus;
                new_rel->trust = 0.52; // Base + small bonus
                new_rel->last_contact = std::chrono::system_clock::now();
            }
        }

        LogDiplomaticEvent(sender, recipient, 
            "Diplomatic gift sent (value: " + std::to_string(static_cast<int>(value)) + 
            ", opinion: +" + std::to_string(opinion_bonus) + ")");
    }

    void DiplomacySystem::ProcessTreatyCompliance(types::EntityID realm_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return;

        // Check compliance for all active treaties
        for (auto& treaty : diplomacy->active_treaties) {
            if (!treaty.is_active) continue;

            // Update treaty status (checks expiration)
            UpdateTreatyStatus(treaty);

            // Check if either party is violating terms
            bool realm_is_a = (treaty.signatory_a == realm_id);
            double& our_compliance = realm_is_a ? treaty.compliance_a : treaty.compliance_b;
            types::EntityID other_realm = realm_is_a ? treaty.signatory_b : treaty.signatory_a;

            // Compliance slowly decays (simulates minor infractions)
            our_compliance = std::max(0.0, our_compliance - 0.01);

            // Check for major violations
            if (our_compliance < 0.5) {
                HandleTreatyViolation(treaty.treaty_id, realm_id);
            }

            // Reward high compliance with opinion bonus
            if (our_compliance > 0.9) {
                diplomacy->ModifyOpinion(other_realm, 1, "Treaty compliance");
            }
        }
    }

    void DiplomacySystem::UpdateTreatyStatus(Treaty& treaty) {
        auto now = std::chrono::system_clock::now();

        // Check if treaty has expired
        if (now >= treaty.expiry_date) {
            if (treaty.is_active) {
                treaty.is_active = false;
                LogDiplomaticEvent(treaty.signatory_a, treaty.signatory_b, 
                    "Treaty expired: " + treaty.treaty_id);
            }
            return;
        }

        // Check if treaty is broken due to low compliance
        if (treaty.IsBroken()) {
            if (treaty.is_active) {
                treaty.is_active = false;
                LogDiplomaticEvent(treaty.signatory_a, treaty.signatory_b, 
                    "Treaty broken due to non-compliance: " + treaty.treaty_id);
            }
        }
    }

    void DiplomacySystem::HandleTreatyViolation(const std::string& treaty_id, types::EntityID violator) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID violator_handle(static_cast<uint64_t>(violator), 1);
        auto violator_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(violator_handle);

        if (!violator_diplomacy) return;

        // Find the treaty
        Treaty* violated_treaty = nullptr;
        for (auto& treaty : violator_diplomacy->active_treaties) {
            if (treaty.treaty_id == treaty_id) {
                violated_treaty = &treaty;
                break;
            }
        }

        if (!violated_treaty) return;

        // Determine the other party
        types::EntityID other_party = (violated_treaty->signatory_a == violator) ? 
            violated_treaty->signatory_b : violated_treaty->signatory_a;

        ::core::ecs::EntityID other_handle(static_cast<uint64_t>(other_party), 1);
        auto other_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(other_handle);

        if (!other_diplomacy) return;

        // Apply diplomatic penalties
        int opinion_penalty = -30; // Serious violation
        other_diplomacy->ModifyOpinion(violator, opinion_penalty, "Treaty violation");
        violator_diplomacy->diplomatic_reputation -= 0.1; // Reputation hit

        // Update trust
        auto* relationship = other_diplomacy->GetRelationship(violator);
        if (relationship) {
            relationship->trust = std::max(0.0, relationship->trust - 0.3);
            relationship->diplomatic_incidents++;
        }

        // Mark treaty as broken
        violated_treaty->is_active = false;

        LogDiplomaticEvent(violator, other_party, 
            "Treaty violation: " + treaty_id + " (opinion: " + std::to_string(opinion_penalty) + ")");
    }

    void DiplomacySystem::UpdateDiplomaticRelationships(types::EntityID realm_id) {
        // TODO: Update diplomatic relationships for a realm
        ::core::logging::LogInfo("DiplomacySystem", 
            "UpdateDiplomaticRelationships called - stub implementation");
    }

    void DiplomacySystem::ProcessDiplomaticDecay(types::EntityID realm_id, float time_delta) {
        // TODO: Process opinion decay over time
        ::core::logging::LogInfo("DiplomacySystem", 
            "ProcessDiplomaticDecay called - stub implementation");
    }

    void DiplomacySystem::CalculatePrestigeEffects(types::EntityID realm_id) {
        // TODO: Calculate prestige effects on diplomacy
        ::core::logging::LogInfo("DiplomacySystem", 
            "CalculatePrestigeEffects called - stub implementation");
    }

    void DiplomacySystem::ProcessAIDiplomacy(types::EntityID realm_id) {
        // TODO: Process AI diplomatic decisions
        ::core::logging::LogInfo("DiplomacySystem", 
            "ProcessAIDiplomacy called - stub implementation");
    }

    double DiplomacySystem::EvaluateProposal(const DiplomaticProposal& proposal) {
        // TODO: Evaluate diplomatic proposal acceptance chance
        return 0.5; // 50% acceptance for now
    }

    // ============================================================================
    // Additional Required Method Stubs
    // ============================================================================

    void DiplomacySystem::GenerateAIDiplomaticActions(types::EntityID realm_id) {
        // TODO: Generate AI diplomatic actions
    }

    void DiplomacySystem::ProcessWarDeclaration(types::EntityID aggressor, types::EntityID defender, CasusBelli cb) {
        // TODO: Process war declaration effects
    }

    void DiplomacySystem::HandleAllyActivation(types::EntityID war_leader, const std::vector<types::EntityID>& allies) {
        // TODO: Handle ally activation in war
    }

    void DiplomacySystem::ProcessPeaceNegotiation(types::EntityID realm_a, types::EntityID realm_b) {
        // TODO: Process peace negotiations
    }

    void DiplomacySystem::UpdateTradeRelations(types::EntityID realm_id) {
        // TODO: Update trade-based diplomatic relations
    }

    double DiplomacySystem::CalculateTradeValue(types::EntityID realm_a, types::EntityID realm_b) {
        return 0.0; // TODO: Calculate trade value between realms
    }

    void DiplomacySystem::ProcessTradeDisputes(types::EntityID realm_id) {
        // TODO: Process trade disputes
    }

    void DiplomacySystem::ProcessDiplomaticIntelligence(types::EntityID realm_id) {
        // TODO: Process diplomatic intelligence gathering
    }

    void DiplomacySystem::UpdateForeignRelationsKnowledge(types::EntityID realm_id) {
        // TODO: Update knowledge of foreign relations
    }

    std::vector<types::EntityID> DiplomacySystem::GetAllRealms() const {
        // TODO: Get all realm IDs from entity system
        return std::vector<types::EntityID>();
    }

    std::vector<types::EntityID> DiplomacySystem::GetNeighboringRealms(types::EntityID realm_id) const {
        // TODO: Get neighboring realms
        return std::vector<types::EntityID>();
    }

    std::vector<types::EntityID> DiplomacySystem::GetPotentialAllies(types::EntityID realm_id) const {
        // TODO: Calculate potential allies
        return std::vector<types::EntityID>();
    }

    std::vector<types::EntityID> DiplomacySystem::GetPotentialEnemies(types::EntityID realm_id) const {
        // TODO: Calculate potential enemies
        return std::vector<types::EntityID>();
    }

    DiplomaticRelation DiplomacySystem::GetRelation(types::EntityID realm_a, types::EntityID realm_b) const {
        auto diplomacy = GetDiplomacyComponent(realm_a);
        if (!diplomacy) return DiplomaticRelation::NEUTRAL;
        
        auto* relationship = diplomacy->GetRelationship(realm_b);
        return relationship ? relationship->relation : DiplomaticRelation::NEUTRAL;
    }

    int DiplomacySystem::GetOpinion(types::EntityID realm_a, types::EntityID realm_b) const {
        auto diplomacy = GetDiplomacyComponent(realm_a);
        if (!diplomacy) return 0;
        
        auto* relationship = diplomacy->GetRelationship(realm_b);
        return relationship ? relationship->opinion : 0;
    }

    double DiplomacySystem::GetPrestige(types::EntityID realm_id) const {
        auto diplomacy = GetDiplomacyComponent(realm_id);
        return diplomacy ? diplomacy->prestige : 0.0;
    }

    bool DiplomacySystem::AreAtWar(types::EntityID realm_a, types::EntityID realm_b) const {
        return GetRelation(realm_a, realm_b) == DiplomaticRelation::AT_WAR;
    }

    void DiplomacySystem::SetDiplomaticPersonality(types::EntityID realm_id, DiplomaticPersonality personality) {
        auto diplomacy_comp = GetDiplomacyComponent(realm_id);
        if (diplomacy_comp) {
            diplomacy_comp->personality = personality;
        }
    }

    void DiplomacySystem::SetWarWeariness(double base_war_weariness) {
        m_base_war_weariness = base_war_weariness;
    }

    void DiplomacySystem::SetDiplomaticSpeed(double speed_multiplier) {
        m_diplomatic_speed = speed_multiplier;
    }

    // ============================================================================
    // Private Method Stubs
    // ============================================================================

    void DiplomacySystem::InitializeDiplomaticPersonalities() {
        // TODO: Initialize diplomatic personalities for AI realms
    }

    void DiplomacySystem::SubscribeToEvents() {
        // TODO: Subscribe to relevant game events
    }

    double DiplomacySystem::CalculateBaseOpinion(types::EntityID realm_a, types::EntityID realm_b) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return 0.0;

        ::core::ecs::EntityID handle_a(static_cast<uint64_t>(realm_a), 1);
        ::core::ecs::EntityID handle_b(static_cast<uint64_t>(realm_b), 1);

        auto diplomacy_a = entity_manager->GetComponent<DiplomacyComponent>(handle_a);
        auto diplomacy_b = entity_manager->GetComponent<DiplomacyComponent>(handle_b);

        if (!diplomacy_a || !diplomacy_b) return 0.0;

        double base_opinion = 0.0;

        // Reputation effect (+/-20 based on diplomatic reputation)
        base_opinion += (diplomacy_b->diplomatic_reputation - 0.5) * 40.0;

        // Shared treaties bonus
        for (const auto& treaty : diplomacy_a->active_treaties) {
            if (!treaty.is_active) continue;
            if (treaty.signatory_a == realm_b || treaty.signatory_b == realm_b) {
                base_opinion += 5.0; // Direct treaty bonus
            }
        }

        // Trust effect (if relationship exists)
        auto* rel = diplomacy_a->GetRelationship(realm_b);
        if (rel) {
            base_opinion += rel->trust * 20.0; // Up to +20 for perfect trust
            
            // Recent incidents penalty
            if (rel->diplomatic_incidents > 0) {
                base_opinion -= rel->diplomatic_incidents * 5.0;
            }
        }

        return std::clamp(base_opinion, -100.0, 100.0);
    }

    double DiplomacySystem::CalculateAllianceValue(types::EntityID realm_a, types::EntityID realm_b) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return 0.5;

        ::core::ecs::EntityID handle_b(static_cast<uint64_t>(realm_b), 1);
        auto military = entity_manager->GetComponent<game::military::MilitaryComponent>(handle_b);
        auto diplomacy_b = entity_manager->GetComponent<DiplomacyComponent>(handle_b);

        double value = 0.5; // Base value

        // Military strength contribution (0.0 to 0.3)
        if (military) {
            uint32_t total_troops = military->GetTotalGarrisonStrength();
            value += std::min(0.3, static_cast<double>(total_troops) / 100000.0);
        }

        // Reputation contribution (0.0 to 0.2)
        if (diplomacy_b) {
            value += diplomacy_b->diplomatic_reputation * 0.2;
        }

        return std::clamp(value, 0.0, 1.0);
    }

    double DiplomacySystem::CalculateWarScore(types::EntityID realm_a, types::EntityID realm_b) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return 0.5;

        ::core::ecs::EntityID handle_a(static_cast<uint64_t>(realm_a), 1);
        ::core::ecs::EntityID handle_b(static_cast<uint64_t>(realm_b), 1);

        auto military_a = entity_manager->GetComponent<game::military::MilitaryComponent>(handle_a);
        auto military_b = entity_manager->GetComponent<game::military::MilitaryComponent>(handle_b);

        if (!military_a || !military_b) return 0.5;

        // Calculate total military strengths
        uint32_t troops_a = military_a->GetTotalGarrisonStrength();
        uint32_t troops_b = military_b->GetTotalGarrisonStrength();

        // Prevent division by zero
        if (troops_a + troops_b == 0) return 0.5;

        // War score based on relative strength (0.0 = realm_b winning, 1.0 = realm_a winning)
        double war_score = static_cast<double>(troops_a) / (troops_a + troops_b);

        // TODO: Factor in battles won/lost, occupied territory, war goal progress
        // This is a simplified implementation based only on military strength

        return std::clamp(war_score, 0.0, 1.0);
    }

    CasusBelli DiplomacySystem::FindBestCasusBelli(types::EntityID aggressor, types::EntityID target) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return CasusBelli::NONE;

        ::core::ecs::EntityID aggressor_handle(static_cast<uint64_t>(aggressor), 1);
        ::core::ecs::EntityID target_handle(static_cast<uint64_t>(target), 1);

        auto aggressor_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(aggressor_handle);
        auto target_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(target_handle);

        if (!aggressor_diplomacy || !target_diplomacy) return CasusBelli::NONE;

        // Check for valid casus belli in priority order

        // 1. Broken treaty (highest priority)
        if (aggressor_diplomacy->HasTreatyType(target, TreatyType::DEFENSIVE_LEAGUE) ||
            aggressor_diplomacy->HasTreatyType(target, TreatyType::NON_AGGRESSION)) {
            for (const auto& treaty : aggressor_diplomacy->active_treaties) {
                if ((treaty.signatory_a == target || treaty.signatory_b == target) && 
                    treaty.IsBroken()) {
                    return CasusBelli::BROKEN_TREATY;
                }
            }
        }

        // 2. Protection of ally
        for (const auto& treaty : aggressor_diplomacy->active_treaties) {
            if (treaty.type == TreatyType::ALLIANCE && treaty.is_active) {
                types::EntityID ally = (treaty.signatory_a == aggressor) ? 
                    treaty.signatory_b : treaty.signatory_a;
                
                // Check if ally is at war with target (would need WarComponent - simplified)
                auto* ally_rel = aggressor_diplomacy->GetRelationship(ally);
                if (ally_rel && ally_rel->opinion > 50) {
                    return CasusBelli::PROTECTION_OF_ALLY;
                }
            }
        }

        // 3. Insult to honor (if relationship is very bad)
        auto* relationship = aggressor_diplomacy->GetRelationship(target);
        if (relationship) {
            if (relationship->diplomatic_incidents > 3) {
                return CasusBelli::INSULT_TO_HONOR;
            }
            
            if (relationship->opinion < -50) {
                return CasusBelli::BORDER_DISPUTE;
            }
        }

        // 4. No valid casus belli
        return CasusBelli::NONE;
    }

    double DiplomacySystem::EvaluateAllianceProposal(const DiplomaticProposal& proposal) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return 0.5;

        ::core::ecs::EntityID proposer_handle(static_cast<uint64_t>(proposal.proposer), 1);
        ::core::ecs::EntityID target_handle(static_cast<uint64_t>(proposal.target), 1);

        auto target_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(target_handle);
        if (!target_diplomacy) return 0.5;

        double acceptance_chance = 0.5; // Base 50% chance

        // Opinion modifier (-0.5 to +0.5)
        auto* relationship = target_diplomacy->GetRelationship(proposal.proposer);
        if (relationship) {
            acceptance_chance += relationship->opinion / 200.0; // -100 to +100 becomes -0.5 to +0.5
            
            // Trust bonus (0.0 to +0.2)
            acceptance_chance += relationship->trust * 0.2;
            
            // Incidents penalty
            if (relationship->diplomatic_incidents > 0) {
                acceptance_chance -= relationship->diplomatic_incidents * 0.05;
            }
        }

        // Alliance value (0.0 to +0.3)
        double alliance_value = CalculateAllianceValue(proposal.target, proposal.proposer);
        acceptance_chance += (alliance_value - 0.5) * 0.6; // Normalized around 0.5

        // Existing treaties bonus (+0.1 for each positive treaty)
        if (target_diplomacy->HasTreatyType(proposal.proposer, TreatyType::TRADE_AGREEMENT)) {
            acceptance_chance += 0.1;
        }
        if (target_diplomacy->HasTreatyType(proposal.proposer, TreatyType::NON_AGGRESSION)) {
            acceptance_chance += 0.15;
        }

        return std::clamp(acceptance_chance, 0.0, 1.0);
    }

    double DiplomacySystem::EvaluateTradeProposal(const DiplomaticProposal& proposal) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return 0.5;

        ::core::ecs::EntityID proposer_handle(static_cast<uint64_t>(proposal.proposer), 1);
        ::core::ecs::EntityID target_handle(static_cast<uint64_t>(proposal.target), 1);

        auto target_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(target_handle);
        if (!target_diplomacy) return 0.5;

        double acceptance_chance = 0.6; // Trade is generally easier to accept

        // Opinion modifier (-0.3 to +0.3)
        auto* relationship = target_diplomacy->GetRelationship(proposal.proposer);
        if (relationship) {
            acceptance_chance += relationship->opinion / 333.0; // Smaller impact than alliance
            
            // Trust matters less for trade (+0.1 max)
            acceptance_chance += relationship->trust * 0.1;
        }

        // Reputation bonus (good traders get better acceptance)
        ::core::ecs::EntityID proposer_handle2(static_cast<uint64_t>(proposal.proposer), 1);
        auto proposer_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(proposer_handle2);
        if (proposer_diplomacy) {
            acceptance_chance += (proposer_diplomacy->diplomatic_reputation - 0.5) * 0.2;
        }

        // At war = trade rejected
        if (relationship && relationship->diplomatic_incidents > 5) {
            acceptance_chance = 0.1; // Very unlikely
        }

        return std::clamp(acceptance_chance, 0.0, 1.0);
    }

    double DiplomacySystem::EvaluateMarriageProposal(const DiplomaticProposal& proposal) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return 0.5;

        ::core::ecs::EntityID proposer_handle(static_cast<uint64_t>(proposal.proposer), 1);
        ::core::ecs::EntityID target_handle(static_cast<uint64_t>(proposal.target), 1);

        auto target_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(target_handle);
        if (!target_diplomacy) return 0.5;

        double acceptance_chance = 0.4; // Marriage requires good relations

        // Opinion is critical for marriage (-0.4 to +0.4)
        auto* relationship = target_diplomacy->GetRelationship(proposal.proposer);
        if (relationship) {
            acceptance_chance += relationship->opinion / 250.0;
            
            // Trust is very important (+0.3 max)
            acceptance_chance += relationship->trust * 0.3;
            
            // Incidents strongly discourage marriage
            if (relationship->diplomatic_incidents > 0) {
                acceptance_chance -= relationship->diplomatic_incidents * 0.1;
            }
        }

        // Existing alliances help marriage proposals (+0.2)
        if (target_diplomacy->HasTreatyType(proposal.proposer, TreatyType::ALLIANCE)) {
            acceptance_chance += 0.2;
        }

        // Reputation matters for dynasties (+0.1)
        ::core::ecs::EntityID proposer_handle2(static_cast<uint64_t>(proposal.proposer), 1);
        auto proposer_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(proposer_handle2);
        if (proposer_diplomacy) {
            acceptance_chance += (proposer_diplomacy->diplomatic_reputation - 0.5) * 0.2;
        }

        return std::clamp(acceptance_chance, 0.0, 1.0);
    }

    void DiplomacySystem::ApplyPersonalityToOpinion(types::EntityID realm_id, DiplomaticState& relationship) const {
        // TODO: Apply personality effects to opinion
    }

    double DiplomacySystem::GetPersonalityWarLikelihood(DiplomaticPersonality personality) const {
        // TODO: Get war likelihood based on personality
        switch (personality) {
            case DiplomaticPersonality::AGGRESSIVE: return 0.8;
            case DiplomaticPersonality::DIPLOMATIC: return 0.2;
            case DiplomaticPersonality::ISOLATIONIST: return 0.1;
            default: return 0.4;
        }
    }

    double DiplomacySystem::GetPersonalityTradePreference(DiplomaticPersonality personality) const {
        // TODO: Get trade preference based on personality
        switch (personality) {
            case DiplomaticPersonality::MERCHANT: return 0.9;
            case DiplomaticPersonality::ISOLATIONIST: return 0.1;
            default: return 0.5;
        }
    }

    std::vector<types::EntityID> DiplomacySystem::GetBorderingRealms(types::EntityID realm_id) const {
        // TODO: Get realms that border the given realm
        return std::vector<types::EntityID>();
    }

    void DiplomacySystem::LogDiplomaticEvent(types::EntityID realm_a, types::EntityID realm_b, 
                                             const std::string& event) {
        ::core::logging::LogInfo("DiplomacySystem", 
            "Diplomatic Event: Realm " + std::to_string(realm_a) + 
            " <-> Realm " + std::to_string(realm_b) + 
            ": " + event);
    }

    // ============================================================================
    // ISerializable Interface Implementation
    // ============================================================================
    // Note: Serialize/Deserialize implementations are in DiplomacySystemSerialization.cpp

    std::string DiplomacySystem::GetSystemName() const {
        return "DiplomacySystem";
    }

} // namespace game::diplomacy