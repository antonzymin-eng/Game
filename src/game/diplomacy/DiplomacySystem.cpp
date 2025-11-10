// ============================================================================
// DiplomacySystem_minimal.cpp - Clean Minimal Implementation
// Created: October 13, 2025 - ECS Integration
// Location: src/game/diplomacy/DiplomacySystem_minimal.cpp
// ============================================================================

#include "game/diplomacy/DiplomacySystem.h"
#include "game/military/MilitaryComponents.h"
#include "core/logging/Logger.h"
#include "game/config/GameConfig.h"
#include "utils/PlatformCompat.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>

namespace game::diplomacy {

    // ============================================================================
    // DiplomacySystem Implementation - Clean Minimal Version
    // ============================================================================

    DiplomacySystem::DiplomacySystem(::core::ecs::ComponentAccessManager& access_manager,
                                    ::core::threading::ThreadSafeMessageBus& message_bus)
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
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;

        ::core::ecs::EntityID supplicant_handle(static_cast<uint64_t>(supplicant), 1);
        ::core::ecs::EntityID victor_handle(static_cast<uint64_t>(victor), 1);

        auto supplicant_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(supplicant_handle);
        auto victor_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(victor_handle);

        if (!supplicant_diplomacy || !victor_diplomacy) return false;

        // Check if realms are actually at war
        auto* supplicant_rel = supplicant_diplomacy->GetRelationship(victor);
        auto* victor_rel = victor_diplomacy->GetRelationship(supplicant);
        
        if (!supplicant_rel || !victor_rel || 
            supplicant_rel->relation != DiplomaticRelation::AT_WAR) {
            ::core::logging::LogWarning("DiplomacySystem", 
                "SueForPeace: Realms are not at war");
            return false;
        }

        // Calculate war score to determine if peace terms are reasonable
        double war_score = CalculateWarScore(victor, supplicant); // Higher = victor winning

        // Evaluate peace terms acceptability
        double total_cost = 0.0;
        for (const auto& [term, value] : peace_terms) {
            total_cost += value;
        }

        // Victor is more likely to accept if they're winning
        bool peace_acceptable = (war_score > 0.6 || total_cost > war_score * 100.0);

        if (!peace_acceptable) {
            ::core::logging::LogInfo("DiplomacySystem", 
                "Peace terms rejected - insufficient concessions");
            return false;
        }

        // Create peace treaty
        Treaty peace_treaty(TreatyType::NON_AGGRESSION, supplicant, victor);
        peace_treaty.signed_date = std::chrono::system_clock::now();
        peace_treaty.expiry_date = peace_treaty.signed_date + std::chrono::hours(8760 * 5); // 5 years
        peace_treaty.is_active = true;

        // Apply peace terms
        for (const auto& [term, value] : peace_terms) {
            peace_treaty.terms[term] = value;
            if (term == "tribute") {
                peace_treaty.tribute_amount = value;
            }
        }

        // End war status
        supplicant_rel->relation = DiplomaticRelation::UNFRIENDLY;
        victor_rel->relation = DiplomaticRelation::UNFRIENDLY;

        // Apply opinion changes based on peace terms
        int opinion_change = static_cast<int>(-total_cost / 10.0); // Negative for loser
        supplicant_diplomacy->ModifyOpinion(victor, opinion_change, "Peace treaty signed");
        victor_diplomacy->ModifyOpinion(supplicant, std::abs(opinion_change) / 2, "Peace accepted");

        // Store treaty
        supplicant_diplomacy->AddTreaty(peace_treaty);
        victor_diplomacy->AddTreaty(peace_treaty);

        // Reduce war weariness
        supplicant_diplomacy->war_weariness = std::max(0.0, supplicant_diplomacy->war_weariness - 0.3);
        victor_diplomacy->war_weariness = std::max(0.0, victor_diplomacy->war_weariness - 0.2);

        LogDiplomaticEvent(supplicant, victor, "Peace treaty signed - war ended");
        
        ::core::logging::LogInfo("DiplomacySystem", 
            "Peace successfully negotiated between realms");
        return true;
    }

    bool DiplomacySystem::ArrangeMarriage(types::EntityID bride_realm, types::EntityID groom_realm,
        bool create_alliance) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return false;

        ::core::ecs::EntityID bride_handle(static_cast<uint64_t>(bride_realm), 1);
        ::core::ecs::EntityID groom_handle(static_cast<uint64_t>(groom_realm), 1);

        auto bride_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(bride_handle);
        auto groom_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(groom_handle);

        if (!bride_diplomacy || !groom_diplomacy) return false;

        // Check if realms are already at war (marriages not allowed during war)
        auto* bride_rel = bride_diplomacy->GetRelationship(groom_realm);
        auto* groom_rel = groom_diplomacy->GetRelationship(bride_realm);

        if (bride_rel && bride_rel->relation == DiplomaticRelation::AT_WAR) {
            ::core::logging::LogWarning("DiplomacySystem", 
                "Cannot arrange marriage while at war");
            return false;
        }

        // Create dynastic marriage
        DynasticMarriage marriage(bride_realm, groom_realm);
        marriage.marriage_id = "marriage_" + std::to_string(bride_realm) + "_" + 
                               std::to_string(groom_realm);
        marriage.marriage_date = std::chrono::system_clock::now();
        marriage.is_active = true;
        marriage.diplomatic_bonus = 20.0;
        marriage.produces_alliance = create_alliance;

        // Note: Character IDs are stubs - full character system not implemented
        marriage.bride_character = types::EntityID(1000 + bride_realm);
        marriage.groom_character = types::EntityID(2000 + groom_realm);

        // Add marriage to both realms
        bride_diplomacy->marriages.push_back(marriage);
        groom_diplomacy->marriages.push_back(marriage);

        // Apply immediate diplomatic bonuses
        bride_diplomacy->ModifyOpinion(groom_realm, 20, "Royal marriage arranged");
        groom_diplomacy->ModifyOpinion(bride_realm, 20, "Royal marriage arranged");

        // Improve relations
        if (bride_rel) {
            bride_rel->trust += 0.15;
            bride_rel->trust = std::min(1.0, bride_rel->trust);
            if (bride_rel->relation == DiplomaticRelation::NEUTRAL) {
                bride_rel->relation = DiplomaticRelation::FRIENDLY;
            }
        }

        if (groom_rel) {
            groom_rel->trust += 0.15;
            groom_rel->trust = std::min(1.0, groom_rel->trust);
            if (groom_rel->relation == DiplomaticRelation::NEUTRAL) {
                groom_rel->relation = DiplomaticRelation::FRIENDLY;
            }
        }

        // Create alliance if requested
        if (create_alliance) {
            Treaty alliance(TreatyType::ALLIANCE, bride_realm, groom_realm);
            alliance.signed_date = std::chrono::system_clock::now();
            alliance.expiry_date = alliance.signed_date + std::chrono::hours(8760 * 20); // 20 years
            alliance.is_active = true;
            alliance.terms["marriage_alliance"] = 1.0;

            bride_diplomacy->AddTreaty(alliance);
            groom_diplomacy->AddTreaty(alliance);

            // Update relation to allied
            if (bride_rel) bride_rel->relation = DiplomaticRelation::ALLIED;
            if (groom_rel) groom_rel->relation = DiplomaticRelation::ALLIED;

            LogDiplomaticEvent(bride_realm, groom_realm, "Marriage alliance formed");
        } else {
            LogDiplomaticEvent(bride_realm, groom_realm, "Royal marriage arranged");
        }

        ::core::logging::LogInfo("DiplomacySystem", 
            "Marriage successfully arranged between realms");
        return true;
    }

    void DiplomacySystem::ProcessMarriageEffects(const DynasticMarriage& marriage) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        // Only process active marriages
        if (!marriage.is_active) return;

        ::core::ecs::EntityID bride_handle(static_cast<uint64_t>(marriage.bride_realm), 1);
        ::core::ecs::EntityID groom_handle(static_cast<uint64_t>(marriage.groom_realm), 1);

        auto bride_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(bride_handle);
        auto groom_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(groom_handle);

        if (!bride_diplomacy || !groom_diplomacy) return;

        // Apply ongoing diplomatic bonus (small monthly bonus)
        bride_diplomacy->ModifyOpinion(marriage.groom_realm, 1, "Royal marriage bond");
        groom_diplomacy->ModifyOpinion(marriage.bride_realm, 1, "Royal marriage bond");

        // Strengthen trust gradually
        auto* bride_rel = bride_diplomacy->GetRelationship(marriage.groom_realm);
        auto* groom_rel = groom_diplomacy->GetRelationship(marriage.bride_realm);

        if (bride_rel) {
            bride_rel->trust += 0.005; // +0.5% trust per processing
            bride_rel->trust = std::min(1.0, bride_rel->trust);
        }

        if (groom_rel) {
            groom_rel->trust += 0.005;
            groom_rel->trust = std::min(1.0, groom_rel->trust);
        }

        // Maintain alliance if marriage produces one
        if (marriage.produces_alliance) {
            // Ensure alliance treaty remains active
            bool has_active_alliance = false;
            for (const auto& treaty : bride_diplomacy->active_treaties) {
                if (treaty.type == TreatyType::ALLIANCE && treaty.is_active &&
                    (treaty.signatory_a == marriage.groom_realm || 
                     treaty.signatory_b == marriage.groom_realm)) {
                    has_active_alliance = true;
                    break;
                }
            }

            // Recreate alliance if it was somehow broken
            if (!has_active_alliance) {
                Treaty alliance(TreatyType::ALLIANCE, marriage.bride_realm, marriage.groom_realm);
                alliance.signed_date = std::chrono::system_clock::now();
                alliance.expiry_date = alliance.signed_date + std::chrono::hours(8760 * 20);
                alliance.is_active = true;
                alliance.terms["marriage_alliance"] = 1.0;

                bride_diplomacy->AddTreaty(alliance);
                groom_diplomacy->AddTreaty(alliance);
            }

            // Maintain allied relation status
            if (bride_rel && bride_rel->relation != DiplomaticRelation::ALLIED) {
                bride_rel->relation = DiplomaticRelation::ALLIED;
            }
            if (groom_rel && groom_rel->relation != DiplomaticRelation::ALLIED) {
                groom_rel->relation = DiplomaticRelation::ALLIED;
            }
        }

        // Process inheritance claims (simplified - would integrate with succession system)
        if (marriage.inheritance_claim > 0.0) {
            // TODO: Full inheritance system integration
            // For now, just maintain the claim value in the marriage structure
            ::core::logging::LogDebug("DiplomacySystem", 
                "Processing inheritance claim from marriage");
        }

        // Check for children (stub - full character system needed)
        // In a full implementation, children would strengthen the bond
        if (!marriage.children.empty()) {
            // Each child adds additional diplomatic weight
            int child_bonus = static_cast<int>(marriage.children.size());
            if (child_bonus > 0 && bride_rel) {
                // Already applied above, just a marker for future enhancement
            }
        }
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

    void DiplomacySystem::BreakTreatyBidirectional(types::EntityID realm_a, types::EntityID realm_b, TreatyType type) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID handle_a(static_cast<uint64_t>(realm_a), 1);
        ::core::ecs::EntityID handle_b(static_cast<uint64_t>(realm_b), 1);

        auto diplomacy_a = entity_manager->GetComponent<DiplomacyComponent>(handle_a);
        auto diplomacy_b = entity_manager->GetComponent<DiplomacyComponent>(handle_b);

        if (!diplomacy_a || !diplomacy_b) return;

        // Break treaty on both sides - BreakTreaty() now handles trust/opinion penalties
        diplomacy_a->BreakTreaty(realm_b, type);
        diplomacy_b->BreakTreaty(realm_a, type);

        // Log the event
        std::string treaty_name = "Unknown";
        switch(type) {
            case TreatyType::ALLIANCE: treaty_name = "Alliance"; break;
            case TreatyType::NON_AGGRESSION: treaty_name = "Non-Aggression Pact"; break;
            case TreatyType::TRADE_AGREEMENT: treaty_name = "Trade Agreement"; break;
            case TreatyType::DEFENSIVE_LEAGUE: treaty_name = "Defensive League"; break;
            default: treaty_name = "Treaty"; break;
        }

        LogDiplomaticEvent(realm_a, realm_b, treaty_name + " broken");
    }

    void DiplomacySystem::UpdateDiplomaticRelationships(types::EntityID realm_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return;

        // Update relationship status based on opinion and circumstances
        for (auto& [other_realm, state] : diplomacy->relationships) {
            // Skip if already at war
            if (state.relation == DiplomaticRelation::AT_WAR) continue;

            // Determine new relation based on opinion and treaties
            DiplomaticRelation new_relation = DiplomaticRelation::NEUTRAL;

            // Check for alliance treaty
            bool has_alliance = diplomacy->HasTreatyType(other_realm, TreatyType::ALLIANCE);
            if (has_alliance) {
                new_relation = DiplomaticRelation::ALLIED;
            }
            // Opinion-based relations
            else if (state.opinion >= 75) {
                new_relation = DiplomaticRelation::FRIENDLY;
            }
            else if (state.opinion >= 25) {
                new_relation = DiplomaticRelation::NEUTRAL;
            }
            else if (state.opinion >= -25) {
                new_relation = DiplomaticRelation::UNFRIENDLY;
            }
            else {
                new_relation = DiplomaticRelation::HOSTILE;
            }

            // Update relation if it changed
            if (new_relation != state.relation && state.relation != DiplomaticRelation::AT_WAR) {
                state.relation = new_relation;
            }

            // Update trust based on diplomatic incidents
            if (state.diplomatic_incidents > 5) {
                state.trust = std::max(0.0, state.trust - 0.01);
            }
            else if (state.diplomatic_incidents == 0 && state.trust < 1.0) {
                state.trust = std::min(1.0, state.trust + 0.005); // Slowly rebuild trust
            }

            // Update economic dependency
            if (state.trade_volume > 0.0) {
                state.economic_dependency = std::min(1.0, state.trade_volume / 10000.0);
            }
        }
    }

    void DiplomacySystem::ProcessDiplomaticDecay(types::EntityID realm_id, float time_delta) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return;

        // Process opinion decay over time
        for (auto& [other_realm, state] : diplomacy->relationships) {
            // Skip war relationships - they don't decay naturally
            if (state.relation == DiplomaticRelation::AT_WAR) continue;

            // Apply passive opinion decay toward neutral using new decay system
            state.ApplyOpinionDecay(time_delta, 0);
            
            // Apply trust decay toward neutral baseline (0.5)
            state.ApplyTrustDecay(time_delta, 0.5);

            // Recent actions fade over time
            if (!state.recent_actions.empty() && state.recent_actions.size() > 10) {
                state.recent_actions.pop_front(); // Remove oldest action
            }

            // Diplomatic incidents decay slowly
            if (state.diplomatic_incidents > 0 && time_delta > 0.5f) {
                state.diplomatic_incidents = std::max(0, state.diplomatic_incidents - 1);
            }

            // Border tensions ease over time if opinion improves
            if (state.has_border_tensions && state.opinion > -10) {
                state.has_border_tensions = false;
            }
        }

        // War weariness decays slowly in peacetime
        if (diplomacy->war_weariness > 0.0) {
            double weariness_decay = 0.05 * time_delta; // 5% per time unit
            diplomacy->war_weariness = std::max(0.0, diplomacy->war_weariness - weariness_decay);
        }
    }

    void DiplomacySystem::CalculatePrestigeEffects(types::EntityID realm_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return;

        // Base prestige from diplomatic reputation
        double base_prestige = diplomacy->diplomatic_reputation * 50.0;

        // Alliance count bonus
        int alliance_count = 0;
        for (const auto& treaty : diplomacy->active_treaties) {
            if (treaty.type == TreatyType::ALLIANCE && treaty.is_active) {
                alliance_count++;
            }
        }
        base_prestige += alliance_count * 10.0; // +10 prestige per alliance

        // Marriage prestige
        base_prestige += diplomacy->marriages.size() * 15.0; // +15 per royal marriage

        // War weariness penalty
        base_prestige -= diplomacy->war_weariness * 25.0; // Up to -25 prestige at max weariness

        // Hostile relationships penalty
        int hostile_count = 0;
        for (const auto& [other_realm, state] : diplomacy->relationships) {
            if (state.relation == DiplomaticRelation::HOSTILE || 
                state.relation == DiplomaticRelation::AT_WAR) {
                hostile_count++;
            }
        }
        base_prestige -= hostile_count * 5.0; // -5 prestige per enemy

        // Update prestige
        diplomacy->prestige = std::max(0.0, base_prestige);

        // Apply prestige effects to relationships
        for (auto& [other_realm, state] : diplomacy->relationships) {
            ::core::ecs::EntityID other_handle(static_cast<uint64_t>(other_realm), 1);
            auto other_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(other_handle);

            if (!other_diplomacy) continue;

            // Calculate prestige difference effect on opinion
            double prestige_diff = diplomacy->prestige - other_diplomacy->prestige;
            state.prestige_difference = prestige_diff;

            // High prestige realms get small opinion bonus from low prestige realms
            if (prestige_diff > 50.0) {
                // Low prestige realm respects high prestige realm (+1 to +5 opinion)
                int prestige_bonus = static_cast<int>(std::min(5.0, prestige_diff / 20.0));
                // This is applied as a modifier, not a permanent change
                state.prestige_difference = prestige_diff;
            }
        }
    }

    void DiplomacySystem::ProcessAIDiplomacy(types::EntityID realm_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return;

        // AI diplomatic decision-making based on personality and circumstances
        
        // 1. Evaluate current diplomatic situation
        int friendly_relations = 0;
        int hostile_relations = 0;
        int active_wars = 0;

        for (const auto& [other_realm, state] : diplomacy->relationships) {
            if (state.relation == DiplomaticRelation::FRIENDLY || 
                state.relation == DiplomaticRelation::ALLIED) {
                friendly_relations++;
            }
            else if (state.relation == DiplomaticRelation::HOSTILE) {
                hostile_relations++;
            }
            else if (state.relation == DiplomaticRelation::AT_WAR) {
                active_wars++;
            }
        }

        // 2. Personality-based decision making
        double war_likelihood = GetPersonalityWarLikelihood(diplomacy->personality);
        double trade_preference = GetPersonalityTradePreference(diplomacy->personality);

        // 3. Generate diplomatic actions based on personality and situation
        
        // Aggressive personalities seek wars when not overextended
        if (diplomacy->personality == DiplomaticPersonality::AGGRESSIVE && 
            active_wars < 2 && diplomacy->war_weariness < 0.5) {
            
            // Look for hostile targets
            for (const auto& [other_realm, state] : diplomacy->relationships) {
                if (state.relation == DiplomaticRelation::HOSTILE && state.opinion < -40) {
                    // Consider war declaration (would be triggered by external system)
                    CasusBelli cb = FindBestCasusBelli(realm_id, other_realm);
                    if (cb != CasusBelli::NONE) {
                        ::core::logging::LogDebug("DiplomacySystem", 
                            "AI considering war declaration");
                        // In full implementation, would generate war proposal
                    }
                    break; // One war consideration per update
                }
            }
        }

        // Diplomatic personalities seek alliances
        if (diplomacy->personality == DiplomaticPersonality::DIPLOMATIC && 
            friendly_relations < 3) {
            
            for (const auto& [other_realm, state] : diplomacy->relationships) {
                if (state.relation == DiplomaticRelation::FRIENDLY && 
                    state.opinion > 50 && !diplomacy->HasTreatyType(other_realm, TreatyType::ALLIANCE)) {
                    
                    // Propose alliance (would generate proposal)
                    ::core::logging::LogDebug("DiplomacySystem", 
                        "AI considering alliance proposal");
                    break;
                }
            }
        }

        // Merchant personalities seek trade agreements
        if (diplomacy->personality == DiplomaticPersonality::MERCHANT) {
            for (const auto& [other_realm, state] : diplomacy->relationships) {
                if ((state.relation == DiplomaticRelation::NEUTRAL || 
                     state.relation == DiplomaticRelation::FRIENDLY) &&
                    !diplomacy->HasTreatyType(other_realm, TreatyType::TRADE_AGREEMENT)) {
                    
                    ::core::logging::LogDebug("DiplomacySystem", 
                        "AI considering trade agreement");
                    break;
                }
            }
        }

        // Treacherous personalities may betray allies under certain conditions
        if (diplomacy->personality == DiplomaticPersonality::TREACHEROUS && 
            war_likelihood > 0.6) {
            
            for (const auto& [other_realm, state] : diplomacy->relationships) {
                if (state.relation == DiplomaticRelation::ALLIED && state.trust < 0.3) {
                    // Consider betrayal if trust is low
                    ::core::logging::LogDebug("DiplomacySystem", 
                        "AI considering alliance betrayal");
                    break;
                }
            }
        }

        // Honorable personalities maintain alliances and avoid unjust wars
        if (diplomacy->personality == DiplomaticPersonality::HONORABLE) {
            // Strengthen existing alliances
            for (auto& [other_realm, state] : diplomacy->relationships) {
                if (state.relation == DiplomaticRelation::ALLIED) {
                    // Small trust boost for honorable rulers
                    state.trust = std::min(1.0, state.trust + 0.01);
                }
            }
        }

        // Isolationist personalities avoid entanglements
        if (diplomacy->personality == DiplomaticPersonality::ISOLATIONIST) {
            // Try to break non-essential treaties if overcommitted
            if (diplomacy->active_treaties.size() > 2) {
                ::core::logging::LogDebug("DiplomacySystem", 
                    "Isolationist AI considering reducing treaties");
            }
        }
    }

    double DiplomacySystem::EvaluateProposal(const DiplomaticProposal& proposal) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return 0.0;

        ::core::ecs::EntityID proposer_handle(static_cast<uint64_t>(proposal.proposer), 1);
        ::core::ecs::EntityID target_handle(static_cast<uint64_t>(proposal.target), 1);

        auto proposer_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(proposer_handle);
        auto target_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(target_handle);

        if (!proposer_diplomacy || !target_diplomacy) return 0.0;

        // Base acceptance chance
        double acceptance = 0.5;

        // Get relationship state
        auto* relationship = target_diplomacy->GetRelationship(proposal.proposer);
        if (!relationship) {
            // No existing relationship - neutral starting point
            acceptance = 0.3;
        } else {
            // Opinion strongly affects acceptance (-100 to +100 mapped to 0.0 to 1.0)
            double opinion_factor = (relationship->opinion + 100.0) / 200.0;
            acceptance = opinion_factor * 0.6; // Opinion contributes 60%

            // Trust affects acceptance (0.0 to 1.0)
            acceptance += relationship->trust * 0.2; // Trust contributes 20%

            // Relation type modifier
            switch (relationship->relation) {
                case DiplomaticRelation::ALLIED:
                    acceptance += 0.3;
                    break;
                case DiplomaticRelation::FRIENDLY:
                    acceptance += 0.15;
                    break;
                case DiplomaticRelation::HOSTILE:
                    acceptance -= 0.3;
                    break;
                case DiplomaticRelation::AT_WAR:
                    acceptance = 0.0; // No proposals during war
                    break;
                default:
                    break;
            }
        }

        // Action-specific modifiers
        switch (proposal.action_type) {
            case DiplomaticAction::PROPOSE_ALLIANCE:
                acceptance *= EvaluateAllianceProposal(proposal);
                break;
            
            case DiplomaticAction::PROPOSE_TRADE:
                acceptance *= EvaluateTradeProposal(proposal);
                break;
            
            case DiplomaticAction::ARRANGE_MARRIAGE:
                acceptance *= EvaluateMarriageProposal(proposal);
                break;
            
            case DiplomaticAction::DECLARE_WAR:
                // Wars are not proposals - return 0
                acceptance = 0.0;
                break;
            
            case DiplomaticAction::SUE_FOR_PEACE:
                // Peace depends on war weariness
                if (target_diplomacy->war_weariness > 0.5) {
                    acceptance += 0.3;
                }
                break;
            
            case DiplomaticAction::SEND_GIFT:
                // Gifts are always accepted
                acceptance = 1.0;
                break;
            
            case DiplomaticAction::DEMAND_TRIBUTE:
                // Tributes are rarely accepted unless under duress
                acceptance *= 0.2;
                break;
            
            case DiplomaticAction::ESTABLISH_EMBASSY:
                // Embassies are usually accepted if relations aren't hostile
                if (relationship && relationship->relation != DiplomaticRelation::HOSTILE) {
                    acceptance = 0.8;
                }
                break;
            
            default:
                break;
        }

        // Personality modifiers
        switch (target_diplomacy->personality) {
            case DiplomaticPersonality::DIPLOMATIC:
                acceptance += 0.1; // More accepting of proposals
                break;
            case DiplomaticPersonality::AGGRESSIVE:
                if (proposal.action_type == DiplomaticAction::PROPOSE_ALLIANCE) {
                    acceptance -= 0.1; // Less interested in alliances
                }
                break;
            case DiplomaticPersonality::ISOLATIONIST:
                acceptance -= 0.2; // Generally avoids entanglements
                break;
            case DiplomaticPersonality::MERCHANT:
                if (proposal.action_type == DiplomaticAction::PROPOSE_TRADE) {
                    acceptance += 0.2; // Loves trade
                }
                break;
            case DiplomaticPersonality::TREACHEROUS:
                acceptance += 0.05; // Accepts proposals but may not honor them
                break;
            case DiplomaticPersonality::HONORABLE:
                if (relationship && relationship->trust < 0.3) {
                    acceptance -= 0.15; // Won't accept from untrustworthy partners
                }
                break;
            default:
                break;
        }

        // War weariness makes peace more acceptable
        if (proposal.action_type == DiplomaticAction::SUE_FOR_PEACE) {
            acceptance += target_diplomacy->war_weariness * 0.5;
        }

        // Prestige difference affects acceptance
        if (proposer_diplomacy->prestige > target_diplomacy->prestige + 30.0) {
            acceptance += 0.1; // More likely to accept from prestigious realms
        }

        return std::clamp(acceptance, 0.0, 1.0);
    }

    // ============================================================================
    // Additional Required Method Stubs
    // ============================================================================

    void DiplomacySystem::GenerateAIDiplomaticActions(types::EntityID realm_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return;

        // Generate automated diplomatic actions based on AI evaluation
        
        // Limit action generation - don't spam proposals
        static std::unordered_map<types::EntityID, int> action_cooldowns;
        if (action_cooldowns[realm_id] > 0) {
            action_cooldowns[realm_id]--;
            return;
        }

        // Find best diplomatic action to take
        std::vector<DiplomaticProposal> potential_actions;

        for (const auto& [other_realm, state] : diplomacy->relationships) {
            // Skip if at war or if target doesn't exist
            if (state.relation == DiplomaticRelation::AT_WAR) continue;

            ::core::ecs::EntityID other_handle(static_cast<uint64_t>(other_realm), 1);
            auto other_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(other_handle);
            if (!other_diplomacy) continue;

            // 1. Consider alliance proposals for friendly realms
            if (state.relation == DiplomaticRelation::FRIENDLY && 
                state.opinion > 50 && 
                !diplomacy->HasTreatyType(other_realm, TreatyType::ALLIANCE)) {
                
                DiplomaticProposal alliance_proposal(realm_id, other_realm, DiplomaticAction::PROPOSE_ALLIANCE);
                alliance_proposal.ai_evaluation = EvaluateAllianceProposal(alliance_proposal);
                
                if (alliance_proposal.ai_evaluation > 0.6) {
                    potential_actions.push_back(alliance_proposal);
                }
            }

            // 2. Consider trade agreements
            if ((state.relation == DiplomaticRelation::NEUTRAL || 
                 state.relation == DiplomaticRelation::FRIENDLY) &&
                !diplomacy->HasTreatyType(other_realm, TreatyType::TRADE_AGREEMENT) &&
                diplomacy->personality == DiplomaticPersonality::MERCHANT) {
                
                DiplomaticProposal trade_proposal(realm_id, other_realm, DiplomaticAction::PROPOSE_TRADE);
                trade_proposal.ai_evaluation = EvaluateTradeProposal(trade_proposal);
                
                if (trade_proposal.ai_evaluation > 0.5) {
                    potential_actions.push_back(trade_proposal);
                }
            }

            // 3. Consider marriage proposals for neutral/friendly realms
            if ((state.relation == DiplomaticRelation::NEUTRAL || 
                 state.relation == DiplomaticRelation::FRIENDLY) &&
                state.opinion > 25 &&
                diplomacy->marriages.size() < 3) { // Limit marriages
                
                DiplomaticProposal marriage_proposal(realm_id, other_realm, DiplomaticAction::ARRANGE_MARRIAGE);
                marriage_proposal.ai_evaluation = EvaluateMarriageProposal(marriage_proposal);
                
                if (marriage_proposal.ai_evaluation > 0.55) {
                    potential_actions.push_back(marriage_proposal);
                }
            }

            // 4. Consider sending gifts to improve relations
            if (state.relation == DiplomaticRelation::UNFRIENDLY && 
                state.opinion < 0 && state.opinion > -50) {
                
                DiplomaticProposal gift_proposal(realm_id, other_realm, DiplomaticAction::SEND_GIFT);
                gift_proposal.terms["value"] = 100.0;
                gift_proposal.ai_evaluation = 0.7; // Gifts are generally good
                
                if (diplomacy->personality == DiplomaticPersonality::DIPLOMATIC) {
                    potential_actions.push_back(gift_proposal);
                }
            }

            // 5. Consider establishing embassies
            if (state.relation == DiplomaticRelation::NEUTRAL && 
                state.opinion > -10) {
                
                DiplomaticProposal embassy_proposal(realm_id, other_realm, DiplomaticAction::ESTABLISH_EMBASSY);
                embassy_proposal.ai_evaluation = 0.65;
                potential_actions.push_back(embassy_proposal);
            }
        }

        // Select best action based on evaluation
        if (!potential_actions.empty()) {
            auto best_action = std::max_element(potential_actions.begin(), potential_actions.end(),
                [](const DiplomaticProposal& a, const DiplomaticProposal& b) {
                    return a.ai_evaluation < b.ai_evaluation;
                });

            if (best_action->ai_evaluation > 0.6) {
                // In a full implementation, this would actually execute the diplomatic action
                // For now, just log it
                std::string action_name = "Unknown";
                switch (best_action->action_type) {
                    case DiplomaticAction::PROPOSE_ALLIANCE: action_name = "Alliance"; break;
                    case DiplomaticAction::PROPOSE_TRADE: action_name = "Trade Agreement"; break;
                    case DiplomaticAction::ARRANGE_MARRIAGE: action_name = "Marriage"; break;
                    case DiplomaticAction::SEND_GIFT: action_name = "Gift"; break;
                    case DiplomaticAction::ESTABLISH_EMBASSY: action_name = "Embassy"; break;
                    default: break;
                }

                ::core::logging::LogInfo("DiplomacySystem", 
                    "AI generated diplomatic action: " + action_name + 
                    " (evaluation: " + std::to_string(best_action->ai_evaluation) + ")");

                // Add to pending proposals (if that system is active)
                m_pending_proposals.push_back(*best_action);

                // Set cooldown to avoid spamming actions
                action_cooldowns[realm_id] = 10; // 10 update cycles
            }
        }
    }

    void DiplomacySystem::ProcessWarDeclaration(types::EntityID aggressor, types::EntityID defender, CasusBelli cb) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID aggressor_handle(static_cast<uint64_t>(aggressor), 1);
        ::core::ecs::EntityID defender_handle(static_cast<uint64_t>(defender), 1);

        auto aggressor_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(aggressor_handle);
        auto defender_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(defender_handle);

        if (!aggressor_diplomacy || !defender_diplomacy) return;

        // Validate casus belli
        if (cb == CasusBelli::NONE) {
            ::core::logging::LogWarning("DiplomacySystem", 
                "War declaration without valid casus belli");
            // Apply heavy diplomatic penalties for unjust war
            aggressor_diplomacy->diplomatic_reputation -= 0.2;
        }

        // Set war status
        auto* aggressor_rel = aggressor_diplomacy->GetRelationship(defender);
        auto* defender_rel = defender_diplomacy->GetRelationship(aggressor);

        if (aggressor_rel) {
            aggressor_rel->relation = DiplomaticRelation::AT_WAR;
        } else {
            // Create relationship if it doesn't exist
            DiplomaticState new_state(defender);
            new_state.relation = DiplomaticRelation::AT_WAR;
            aggressor_diplomacy->relationships[defender] = new_state;
            aggressor_rel = &aggressor_diplomacy->relationships[defender];
        }

        if (defender_rel) {
            defender_rel->relation = DiplomaticRelation::AT_WAR;
        } else {
            DiplomaticState new_state(aggressor);
            new_state.relation = DiplomaticRelation::AT_WAR;
            defender_diplomacy->relationships[aggressor] = new_state;
            defender_rel = &defender_diplomacy->relationships[aggressor];
        }

        // Apply severe opinion penalties
        aggressor_diplomacy->ModifyOpinion(defender, -50, "Declared war");
        defender_diplomacy->ModifyOpinion(aggressor, -60, "Was attacked");

        // Destroy trust
        if (aggressor_rel) {
            aggressor_rel->trust = 0.0;
            aggressor_rel->diplomatic_incidents += 5;
        }
        if (defender_rel) {
            defender_rel->trust = 0.0;
            defender_rel->diplomatic_incidents += 5;
        }

        // Break incompatible treaties (trade, non-aggression) using bidirectional method
        BreakTreatyBidirectional(aggressor, defender, TreatyType::TRADE_AGREEMENT);
        BreakTreatyBidirectional(aggressor, defender, TreatyType::NON_AGGRESSION);

        // Increase war weariness
        aggressor_diplomacy->war_weariness += m_base_war_weariness;
        defender_diplomacy->war_weariness += m_base_war_weariness * 1.5; // Defender gets more

        // Update trade relations (war disrupts trade)
        if (aggressor_rel) aggressor_rel->trade_volume = 0.0;
        if (defender_rel) defender_rel->trade_volume = 0.0;

        // Log the war with casus belli
        std::string cb_string = "Unknown CB";
        switch (cb) {
            case CasusBelli::BORDER_DISPUTE: cb_string = "Border Dispute"; break;
            case CasusBelli::TRADE_INTERFERENCE: cb_string = "Trade Interference"; break;
            case CasusBelli::DYNASTIC_CLAIM: cb_string = "Dynastic Claim"; break;
            case CasusBelli::RELIGIOUS_CONFLICT: cb_string = "Religious Conflict"; break;
            case CasusBelli::INSULT_TO_HONOR: cb_string = "Insult to Honor"; break;
            case CasusBelli::BROKEN_TREATY: cb_string = "Broken Treaty"; break;
            case CasusBelli::PROTECTION_OF_ALLY: cb_string = "Protection of Ally"; break;
            case CasusBelli::LIBERATION_WAR: cb_string = "Liberation War"; break;
            default: break;
        }

        LogDiplomaticEvent(aggressor, defender, "War declared - CB: " + cb_string);
        
        ::core::logging::LogInfo("DiplomacySystem", 
            "War declared: Aggressor=" + std::to_string(aggressor) + 
            " Defender=" + std::to_string(defender) + " CB=" + cb_string);
    }

    void DiplomacySystem::HandleAllyActivation(types::EntityID war_leader, const std::vector<types::EntityID>& allies) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID leader_handle(static_cast<uint64_t>(war_leader), 1);
        auto leader_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(leader_handle);

        if (!leader_diplomacy) return;

        // Find who the war leader is at war with
        std::vector<types::EntityID> enemies;
        for (const auto& [other_realm, state] : leader_diplomacy->relationships) {
            if (state.relation == DiplomaticRelation::AT_WAR) {
                enemies.push_back(other_realm);
            }
        }

        if (enemies.empty()) {
            ::core::logging::LogWarning("DiplomacySystem", 
                "HandleAllyActivation: War leader not at war with anyone");
            return;
        }

        // Process each ally
        int allies_joined = 0;
        for (types::EntityID ally : allies) {
            ::core::ecs::EntityID ally_handle(static_cast<uint64_t>(ally), 1);
            auto ally_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(ally_handle);

            if (!ally_diplomacy) continue;

            // Check alliance reliability (based on trust, opinion, and personality)
            auto* ally_rel = leader_diplomacy->GetRelationship(ally);
            if (!ally_rel) continue;

            // Calculate reliability: base 80% + trust bonus - war weariness penalty
            double reliability = m_alliance_reliability + (ally_rel->trust * 0.15) - 
                               (ally_diplomacy->war_weariness * 0.3);
            
            // Opinion modifier
            if (ally_rel->opinion > 50) {
                reliability += 0.1;
            } else if (ally_rel->opinion < 0) {
                reliability -= 0.2;
            }

            reliability = std::clamp(reliability, 0.1, 0.95);

            // Simulate ally decision (in full game, this would be AI evaluation)
            // For now, use reliability threshold
            bool joins_war = (reliability > 0.6);

            if (joins_war) {
                // Ally joins the war against all enemies
                for (types::EntityID enemy : enemies) {
                    ::core::ecs::EntityID enemy_handle(static_cast<uint64_t>(enemy), 1);
                    auto enemy_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(enemy_handle);

                    if (!enemy_diplomacy) continue;

                    // Set war status between ally and enemy
                    auto* ally_enemy_rel = ally_diplomacy->GetRelationship(enemy);
                    if (ally_enemy_rel) {
                        ally_enemy_rel->relation = DiplomaticRelation::AT_WAR;
                    } else {
                        DiplomaticState new_state(enemy);
                        new_state.relation = DiplomaticRelation::AT_WAR;
                        ally_diplomacy->relationships[enemy] = new_state;
                    }

                    auto* enemy_ally_rel = enemy_diplomacy->GetRelationship(ally);
                    if (enemy_ally_rel) {
                        enemy_ally_rel->relation = DiplomaticRelation::AT_WAR;
                    } else {
                        DiplomaticState new_state(ally);
                        new_state.relation = DiplomaticRelation::AT_WAR;
                        enemy_diplomacy->relationships[ally] = new_state;
                    }

                    // Apply opinion penalties
                    ally_diplomacy->ModifyOpinion(enemy, -40, "Joined war against them");
                    enemy_diplomacy->ModifyOpinion(ally, -40, "Joined war");
                }

                // Improve relation with war leader for honoring alliance
                ally_diplomacy->ModifyOpinion(war_leader, 10, "Honored alliance");
                leader_diplomacy->ModifyOpinion(ally, 15, "Honored our call to arms");

                // Increase war weariness for ally
                ally_diplomacy->war_weariness += m_base_war_weariness * 0.8;

                allies_joined++;

                LogDiplomaticEvent(ally, war_leader, "Honored alliance - joined war");
            } else {
                // Ally refuses to join - damages alliance
                ally_diplomacy->ModifyOpinion(war_leader, -25, "Refused call to arms");
                leader_diplomacy->ModifyOpinion(ally, -30, "Broke alliance obligation");

                if (ally_rel) {
                    ally_rel->trust -= 0.3;
                    ally_rel->trust = std::max(0.0, ally_rel->trust);
                    ally_rel->diplomatic_incidents++;
                }

                // May break alliance treaty (bidirectional)
                BreakTreatyBidirectional(war_leader, ally, TreatyType::ALLIANCE);

                LogDiplomaticEvent(ally, war_leader, "Refused call to arms - alliance damaged");
            }
        }

        ::core::logging::LogInfo("DiplomacySystem", 
            "Ally activation complete: " + std::to_string(allies_joined) + 
            "/" + std::to_string(allies.size()) + " allies joined the war");
    }

    void DiplomacySystem::ProcessPeaceNegotiation(types::EntityID realm_a, types::EntityID realm_b) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID handle_a(static_cast<uint64_t>(realm_a), 1);
        ::core::ecs::EntityID handle_b(static_cast<uint64_t>(realm_b), 1);

        auto diplomacy_a = entity_manager->GetComponent<DiplomacyComponent>(handle_a);
        auto diplomacy_b = entity_manager->GetComponent<DiplomacyComponent>(handle_b);

        if (!diplomacy_a || !diplomacy_b) return;

        // Check if realms are at war
        auto* rel_a = diplomacy_a->GetRelationship(realm_b);
        auto* rel_b = diplomacy_b->GetRelationship(realm_a);

        if (!rel_a || !rel_b || rel_a->relation != DiplomaticRelation::AT_WAR) {
            ::core::logging::LogWarning("DiplomacySystem", 
                "ProcessPeaceNegotiation: Realms are not at war");
            return;
        }

        // Calculate war score to determine peace terms
        double war_score = CalculateWarScore(realm_a, realm_b);
        // war_score > 0.5 means realm_a is winning

        // Determine who is winning and by how much
        types::EntityID victor = (war_score > 0.5) ? realm_a : realm_b;
        types::EntityID loser = (war_score > 0.5) ? realm_b : realm_a;
        double victory_margin = std::abs(war_score - 0.5) * 2.0; // 0.0 to 1.0 scale

        // Check war weariness - higher weariness makes peace more likely
        double avg_weariness = (diplomacy_a->war_weariness + diplomacy_b->war_weariness) / 2.0;
        
        // Peace likelihood based on war weariness and victory margin
        // If one side is decisively winning (>0.7) OR both sides are exhausted (>0.5 weariness)
        bool peace_likely = (victory_margin > 0.4 || avg_weariness > 0.5);

        if (!peace_likely) {
            ::core::logging::LogDebug("DiplomacySystem", 
                "Peace negotiation ongoing - no agreement yet");
            return;
        }

        // Calculate peace terms based on war score
        std::unordered_map<std::string, double> peace_terms;
        
        if (victory_margin > 0.7) {
            // Decisive victory - harsh terms
            peace_terms["prestige_loss"] = 50.0 * victory_margin;
            peace_terms["tribute"] = 1000.0 * victory_margin;
            peace_terms["territory_concession"] = 2.0; // Number of provinces (stub)
        } else if (victory_margin > 0.4) {
            // Clear victory - moderate terms
            peace_terms["prestige_loss"] = 30.0 * victory_margin;
            peace_terms["tribute"] = 500.0 * victory_margin;
            peace_terms["territory_concession"] = 1.0;
        } else {
            // Close war - white peace (no major concessions)
            peace_terms["prestige_loss"] = 10.0;
            peace_terms["status_quo"] = 1.0;
        }

        // Apply peace terms through SueForPeace
        bool peace_successful = SueForPeace(loser, victor, peace_terms);

        if (peace_successful) {
            ::core::logging::LogInfo("DiplomacySystem", 
                "Peace negotiated successfully between realms");
            
            // Additional effects
            auto victor_diplomacy = (victor == realm_a) ? diplomacy_a : diplomacy_b;
            auto loser_diplomacy = (loser == realm_a) ? diplomacy_a : diplomacy_b;

            // Prestige effects
            if (victor_diplomacy) {
                victor_diplomacy->diplomatic_reputation += 0.1 * victory_margin;
            }
            if (loser_diplomacy) {
                loser_diplomacy->diplomatic_reputation -= 0.15 * victory_margin;
                loser_diplomacy->diplomatic_reputation = std::max(0.0, loser_diplomacy->diplomatic_reputation);
            }

            LogDiplomaticEvent(realm_a, realm_b, "Peace negotiations completed");
        } else {
            ::core::logging::LogDebug("DiplomacySystem", 
                "Peace negotiation failed - terms unacceptable");
        }
    }

    void DiplomacySystem::UpdateTradeRelations(types::EntityID realm_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return;

        // Update trade-based diplomatic relations for all partners
        for (auto& [other_realm, state] : diplomacy->relationships) {
            // Skip war relationships
            if (state.relation == DiplomaticRelation::AT_WAR) continue;

            // Calculate trade value with this realm
            double trade_value = CalculateTradeValue(realm_id, other_realm);
            
            // Update trade volume in relationship
            state.trade_volume = trade_value;

            // Calculate economic dependency (0.0 to 1.0)
            // High dependency means this realm relies heavily on trade with the partner
            state.economic_dependency = std::min(1.0, trade_value / 5000.0);

            // Trade improves relations gradually
            if (trade_value > 100.0) {
                // Small opinion bonus for active trade (+1 to +5 based on volume)
                int trade_bonus = static_cast<int>(std::min(5.0, trade_value / 500.0));
                
                // Don't spam opinion changes - only apply if significant
                if (trade_bonus > 0) {
                    state.opinion += trade_bonus;
                    state.opinion = std::clamp(state.opinion, -100, 100);
                }

                // Trade builds trust slowly (+0.1% to +0.5%)
                double trust_gain = std::min(0.005, trade_value / 100000.0);
                state.trust = std::min(1.0, state.trust + trust_gain);
            }

            // High economic dependency creates pressure to maintain good relations
            if (state.economic_dependency > 0.6) {
                // Very dependent realms avoid hostility
                if (state.relation == DiplomaticRelation::HOSTILE && state.opinion > -75) {
                    state.relation = DiplomaticRelation::UNFRIENDLY;
                }
            }

            // Check for trade agreements and ensure they're beneficial
            if (diplomacy->HasTreatyType(other_realm, TreatyType::TRADE_AGREEMENT)) {
                // Trade agreements boost trade volume by treaty bonus
                for (auto& treaty : diplomacy->active_treaties) {
                    if ((treaty.signatory_a == other_realm || treaty.signatory_b == other_realm) &&
                        treaty.type == TreatyType::TRADE_AGREEMENT && treaty.is_active) {
                        
                        // Apply trade agreement bonus
                        state.trade_volume += treaty.trade_bonus;
                        
                        // Improve opinion slightly for active trade agreement
                        if (state.opinion < 50) {
                            state.opinion = std::min(100, state.opinion + 1);
                        }
                        break;
                    }
                }
            }
        }

        // Log if realm has high trade dependence on any partner
        for (const auto& [other_realm, state] : diplomacy->relationships) {
            if (state.economic_dependency > 0.7) {
                ::core::logging::LogDebug("DiplomacySystem", 
                    "High economic dependency detected (>70%)");
                break;
            }
        }
    }

    double DiplomacySystem::CalculateTradeValue(types::EntityID realm_a, types::EntityID realm_b) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return 0.0;

        ::core::ecs::EntityID handle_a(static_cast<uint64_t>(realm_a), 1);
        ::core::ecs::EntityID handle_b(static_cast<uint64_t>(realm_b), 1);

        auto diplomacy_a = entity_manager->GetComponent<DiplomacyComponent>(handle_a);
        auto diplomacy_b = entity_manager->GetComponent<DiplomacyComponent>(handle_b);

        if (!diplomacy_a || !diplomacy_b) return 0.0;

        // Base trade value calculation
        double trade_value = 100.0; // Baseline trade

        // Get relationship state
        auto* relationship = diplomacy_a->GetRelationship(realm_b);
        if (!relationship) return 0.0;

        // Trade is heavily impacted by diplomatic relations
        switch (relationship->relation) {
            case DiplomaticRelation::ALLIED:
                trade_value *= 2.0; // Double trade with allies
                break;
            case DiplomaticRelation::FRIENDLY:
                trade_value *= 1.5; // +50% with friends
                break;
            case DiplomaticRelation::NEUTRAL:
                trade_value *= 1.0; // Base rate
                break;
            case DiplomaticRelation::UNFRIENDLY:
                trade_value *= 0.5; // -50% penalty
                break;
            case DiplomaticRelation::HOSTILE:
                trade_value *= 0.2; // Minimal trade
                break;
            case DiplomaticRelation::AT_WAR:
                return 0.0; // No trade during war
            default:
                break;
        }

        // Trade agreement bonus
        if (diplomacy_a->HasTreatyType(realm_b, TreatyType::TRADE_AGREEMENT)) {
            trade_value *= 1.5; // +50% from trade agreement
        }

        // Distance penalty (stub - would use province/map system)
        // For now, assume all realms have some distance penalty
        double distance_modifier = 0.8; // 20% penalty for distance
        trade_value *= distance_modifier;

        // Merchant personality bonus
        if (diplomacy_a->personality == DiplomaticPersonality::MERCHANT ||
            diplomacy_b->personality == DiplomaticPersonality::MERCHANT) {
            trade_value *= 1.3; // +30% if either is merchant-focused
        }

        // Opinion affects willingness to trade
        if (relationship->opinion > 50) {
            trade_value *= 1.2; // +20% with good relations
        } else if (relationship->opinion < -50) {
            trade_value *= 0.5; // -50% with bad relations
        }

        // Trust affects trade reliability
        trade_value *= (0.5 + relationship->trust * 0.5); // 50% to 100% based on trust

        // Economic system integration (stub)
        // In full implementation, would query economic components for:
        // - Population size (more people = more trade)
        // - Production capacity (more goods = more trade)
        // - Trade infrastructure (ports, roads)
        // - Resource availability (complementary resources = more trade)
        
        // For now, use a simple scaling factor
        trade_value *= 2.0; // Scale to reasonable values

        // Isolationist penalty
        if (diplomacy_a->personality == DiplomaticPersonality::ISOLATIONIST ||
            diplomacy_b->personality == DiplomaticPersonality::ISOLATIONIST) {
            trade_value *= 0.4; // -60% for isolationists
        }

        return std::max(0.0, trade_value);
    }

    void DiplomacySystem::ProcessTradeDisputes(types::EntityID realm_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return;

        // Process trade disputes with trading partners
        for (auto& [other_realm, state] : diplomacy->relationships) {
            // Only check trade partners
            if (state.trade_volume < 50.0) continue;

            ::core::ecs::EntityID other_handle(static_cast<uint64_t>(other_realm), 1);
            auto other_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(other_handle);
            if (!other_diplomacy) continue;

            // Check for conditions that create trade disputes
            bool dispute_occurred = false;
            std::string dispute_reason;

            // 1. Broken trade agreements
            if (diplomacy->HasTreatyType(other_realm, TreatyType::TRADE_AGREEMENT)) {
                for (const auto& treaty : diplomacy->active_treaties) {
                    if ((treaty.signatory_a == other_realm || treaty.signatory_b == other_realm) &&
                        treaty.type == TreatyType::TRADE_AGREEMENT) {
                        
                        // Check treaty compliance
                        if (treaty.IsBroken() || !treaty.is_active) {
                            dispute_occurred = true;
                            dispute_reason = "Broken trade agreement";
                            break;
                        }
                    }
                }
            }

            // 2. Sudden trade volume drop (indicates embargo or disruption)
            auto* reciprocal_rel = other_diplomacy->GetRelationship(realm_id);
            if (reciprocal_rel && reciprocal_rel->trade_volume > 0) {
                double expected_trade = CalculateTradeValue(realm_id, other_realm);
                if (state.trade_volume < expected_trade * 0.5) {
                    dispute_occurred = true;
                    dispute_reason = "Trade disruption";
                }
            }

            // 3. High economic dependency with deteriorating relations
            if (state.economic_dependency > 0.6 && state.opinion < -20) {
                dispute_occurred = true;
                dispute_reason = "Economic coercion";
            }

            // 4. Conflicting trade interests (both merchant personalities competing)
            if (diplomacy->personality == DiplomaticPersonality::MERCHANT &&
                other_diplomacy->personality == DiplomaticPersonality::MERCHANT &&
                state.relation == DiplomaticRelation::UNFRIENDLY) {
                dispute_occurred = true;
                dispute_reason = "Trade competition";
            }

            // Process dispute consequences
            if (dispute_occurred) {
                // Opinion penalty
                diplomacy->ModifyOpinion(other_realm, -10, "Trade dispute: " + dispute_reason);

                // Trust damage
                state.trust = std::max(0.0, state.trust - 0.1);

                // Increase diplomatic incidents
                state.diplomatic_incidents++;

                // May lead to trade embargo
                if (state.diplomatic_incidents > 3 && state.opinion < -40) {
                    // Reduce trade volume significantly
                    state.trade_volume *= 0.5;
                    
                    ::core::logging::LogInfo("DiplomacySystem", 
                        "Trade embargo imposed due to repeated disputes");
                }

                // May break trade agreements (bidirectional)
                if (state.opinion < -60 && diplomacy->HasTreatyType(other_realm, TreatyType::TRADE_AGREEMENT)) {
                    BreakTreatyBidirectional(realm_id, other_realm, TreatyType::TRADE_AGREEMENT);
                    
                    ::core::logging::LogInfo("DiplomacySystem", 
                        "Trade agreement broken due to severe disputes");
                }

                LogDiplomaticEvent(realm_id, other_realm, "Trade dispute: " + dispute_reason);
            }
        }
    }

    void DiplomacySystem::ProcessDiplomaticIntelligence(types::EntityID realm_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return;

        // Gather intelligence on other realms' diplomatic activities
        // Intelligence quality depends on relationship and embassies

        for (auto& [other_realm, state] : diplomacy->relationships) {
            ::core::ecs::EntityID other_handle(static_cast<uint64_t>(other_realm), 1);
            auto other_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(other_handle);

            if (!other_diplomacy) continue;

            // Calculate intelligence level (0.0 to 1.0)
            double intelligence_level = 0.3; // Base intelligence

            // Embassies provide intelligence
            // (Would check for actual embassy in full implementation)
            if (state.relation == DiplomaticRelation::ALLIED) {
                intelligence_level += 0.4; // Allies share information
            } else if (state.relation == DiplomaticRelation::FRIENDLY) {
                intelligence_level += 0.2; // Friends share some info
            }

            // High trust increases intelligence sharing
            intelligence_level += state.trust * 0.2;

            // Diplomatic personality affects intelligence gathering
            if (diplomacy->personality == DiplomaticPersonality::OPPORTUNISTIC) {
                intelligence_level += 0.1; // Better at gathering info
            }

            intelligence_level = std::clamp(intelligence_level, 0.0, 1.0);

            // Discover information based on intelligence level
            
            // 1. Learn about wars (always visible with >30% intelligence)
            if (intelligence_level > 0.3) {
                int war_count = 0;
                for (const auto& [third_realm, third_state] : other_diplomacy->relationships) {
                    if (third_state.relation == DiplomaticRelation::AT_WAR) {
                        war_count++;
                        
                        // Update our knowledge of this conflict
                        auto* our_third_rel = diplomacy->GetRelationship(third_realm);
                        if (our_third_rel) {
                            our_third_rel->has_common_enemies = 
                                (state.relation == DiplomaticRelation::HOSTILE);
                        }
                    }
                }

                if (war_count > 0 && intelligence_level > 0.5) {
                    ::core::logging::LogDebug("DiplomacySystem", 
                        "Intelligence: Discovered realm at war");
                }
            }

            // 2. Learn about alliances (>50% intelligence)
            if (intelligence_level > 0.5) {
                for (const auto& treaty : other_diplomacy->active_treaties) {
                    if (treaty.type == TreatyType::ALLIANCE && treaty.is_active) {
                        types::EntityID ally = (treaty.signatory_a == other_realm) ? 
                            treaty.signatory_b : treaty.signatory_a;

                        // Update our knowledge
                        auto* our_ally_rel = diplomacy->GetRelationship(ally);
                        if (our_ally_rel) {
                            // Note: other_realm is allied with ally
                            // This could affect our diplomatic calculations
                        }
                    }
                }
            }

            // 3. Detect military buildups (>60% intelligence)
            // Would integrate with military system in full implementation
            if (intelligence_level > 0.6) {
                // Check if realm is preparing for war
                if (other_diplomacy->war_weariness < 0.2) {
                    double war_likelihood = GetPersonalityWarLikelihood(other_diplomacy->personality);
                    
                    if (war_likelihood > 0.6) {
                        ::core::logging::LogDebug("DiplomacySystem", 
                            "Intelligence: Detected potential military threat");
                        
                        // Increase border tensions if we're nearby
                        if (state.relation == DiplomaticRelation::NEUTRAL ||
                            state.relation == DiplomaticRelation::UNFRIENDLY) {
                            state.has_border_tensions = true;
                        }
                    }
                }
            }

            // 4. Learn diplomatic reputation (>40% intelligence)
            if (intelligence_level > 0.4) {
                // Update prestige difference (already calculated elsewhere)
                // This represents awareness of the realm's standing
                state.prestige_difference = diplomacy->prestige - other_diplomacy->prestige;
            }

            // 5. Discover trade opportunities (>70% intelligence - Merchant focus)
            if (intelligence_level > 0.7 && 
                diplomacy->personality == DiplomaticPersonality::MERCHANT) {
                
                double potential_trade = CalculateTradeValue(realm_id, other_realm);
                if (potential_trade > state.trade_volume * 1.5) {
                    ::core::logging::LogDebug("DiplomacySystem", 
                        "Intelligence: Discovered lucrative trade opportunity");
                }
            }
        }

        // Counter-intelligence: Check if anyone is spying on us
        // Paranoid personalities are more aware of espionage
        if (diplomacy->personality == DiplomaticPersonality::TREACHEROUS ||
            diplomacy->personality == DiplomaticPersonality::AGGRESSIVE) {
            
            for (const auto& [other_realm, state] : diplomacy->relationships) {
                if (state.trust < 0.3 && state.diplomatic_incidents > 2) {
                    ::core::logging::LogDebug("DiplomacySystem", 
                        "Counter-intelligence: Suspected espionage activity");
                }
            }
        }
    }

    void DiplomacySystem::UpdateForeignRelationsKnowledge(types::EntityID realm_id) {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return;

        // Update knowledge of third-party relationships
        // This affects diplomatic decision-making ("the enemy of my enemy is my friend")

        for (auto& [other_realm, state] : diplomacy->relationships) {
            ::core::ecs::EntityID other_handle(static_cast<uint64_t>(other_realm), 1);
            auto other_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(other_handle);

            if (!other_diplomacy) continue;

            // Reset flags for this update cycle
            state.has_common_enemies = false;

            // Scan third-party relationships
            for (const auto& [third_realm, our_third_state] : diplomacy->relationships) {
                if (third_realm == other_realm) continue; // Skip self-reference

                // Check if other_realm has a relationship with third_realm
                auto* their_third_rel = other_diplomacy->GetRelationship(third_realm);
                if (!their_third_rel) continue;

                // 1. Detect common enemies
                if (our_third_state.relation == DiplomaticRelation::HOSTILE ||
                    our_third_state.relation == DiplomaticRelation::AT_WAR) {
                    
                    if (their_third_rel->relation == DiplomaticRelation::HOSTILE ||
                        their_third_rel->relation == DiplomaticRelation::AT_WAR) {
                        
                        // We both dislike third_realm - potential ally
                        state.has_common_enemies = true;
                        
                        // Small opinion bonus for shared enemies
                        if (state.opinion < 50) {
                            state.opinion = std::min(100, state.opinion + 5);
                        }
                    }
                }

                // 2. Detect if they're allied with our enemies
                if (our_third_state.relation == DiplomaticRelation::AT_WAR &&
                    their_third_rel->relation == DiplomaticRelation::ALLIED) {
                    
                    // They're allied with our enemy - makes them unfriendly
                    if (state.opinion > -25) {
                        state.opinion = std::max(-100, state.opinion - 15);
                    }

                    ::core::logging::LogDebug("DiplomacySystem", 
                        "Detected alliance with our enemy");
                }

                // 3. Detect if they're at war with our allies
                if (our_third_state.relation == DiplomaticRelation::ALLIED &&
                    their_third_rel->relation == DiplomaticRelation::AT_WAR) {
                    
                    // They're attacking our ally - hostile action
                    if (state.opinion > -50) {
                        state.opinion = std::max(-100, state.opinion - 20);
                    }

                    // May trigger alliance call-to-arms
                    ::core::logging::LogDebug("DiplomacySystem", 
                        "Detected attack on our ally");
                }

                // 4. Detect mutual friendships (network effect)
                if (our_third_state.relation == DiplomaticRelation::FRIENDLY &&
                    their_third_rel->relation == DiplomaticRelation::FRIENDLY) {
                    
                    // Mutual friend - small opinion boost
                    if (state.opinion < 75) {
                        state.opinion = std::min(100, state.opinion + 2);
                    }
                }

                // 5. Track trade networks
                if (our_third_state.trade_volume > 200.0 &&
                    their_third_rel->trade_volume > 200.0) {
                    
                    // We're both trading with third_realm - economic network
                    // This creates interdependence
                    if (diplomacy->personality == DiplomaticPersonality::MERCHANT) {
                        state.opinion = std::min(100, state.opinion + 3);
                    }
                }
            }

            // Update relationship based on third-party analysis
            
            // If we have common enemies and aren't hostile, improve relations
            if (state.has_common_enemies && 
                state.relation == DiplomaticRelation::NEUTRAL &&
                state.opinion > 20) {
                state.relation = DiplomaticRelation::FRIENDLY;
            }

            // Check for triangular alliance opportunities
            // (We're allied with A, they're allied with A, we should be allied)
            int mutual_allies = 0;
            for (const auto& our_ally_treaty : diplomacy->active_treaties) {
                if (our_ally_treaty.type != TreatyType::ALLIANCE || !our_ally_treaty.is_active) 
                    continue;

                types::EntityID our_ally = (our_ally_treaty.signatory_a == realm_id) ?
                    our_ally_treaty.signatory_b : our_ally_treaty.signatory_a;

                for (const auto& their_treaty : other_diplomacy->active_treaties) {
                    if (their_treaty.type != TreatyType::ALLIANCE || !their_treaty.is_active)
                        continue;

                    types::EntityID their_ally = (their_treaty.signatory_a == other_realm) ?
                        their_treaty.signatory_b : their_treaty.signatory_a;

                    if (our_ally == their_ally) {
                        mutual_allies++;
                    }
                }
            }

            // Multiple mutual allies suggests we should also be allied
            if (mutual_allies >= 2 && state.relation == DiplomaticRelation::FRIENDLY) {
                ::core::logging::LogDebug("DiplomacySystem", 
                    "Detected potential alliance network opportunity");
                
                // Boost opinion to make alliance more likely
                state.opinion = std::min(100, state.opinion + 10);
            }
        }

        // Log significant diplomatic network findings
        int common_enemy_count = 0;
        for (const auto& [other_realm, state] : diplomacy->relationships) {
            if (state.has_common_enemies) {
                common_enemy_count++;
            }
        }

        if (common_enemy_count > 2) {
            ::core::logging::LogDebug("DiplomacySystem", 
                "Found multiple potential allies with common enemies");
        }
    }

    std::vector<types::EntityID> DiplomacySystem::GetAllRealms() const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return std::vector<types::EntityID>();

        std::vector<types::EntityID> all_realms;

        // Query all entities with DiplomacyComponent
        // In full implementation, would use ECS query system
        // For now, iterate through known entity IDs

        // Typical realm IDs range (this is a stub - real implementation would query ECS)
        for (uint32_t i = 1; i <= 100; ++i) {
            ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(i), 1);
            auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(entity_handle);
            
            if (diplomacy) {
                all_realms.push_back(i);
            }
        }

        return all_realms;
    }

    std::vector<types::EntityID> DiplomacySystem::GetNeighboringRealms(types::EntityID realm_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return std::vector<types::EntityID>();

        std::vector<types::EntityID> neighbors;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return neighbors;

        // Get neighboring realms based on various criteria
        for (const auto& [other_realm, state] : diplomacy->relationships) {
            // Neighbors are realms with:
            // 1. Border tensions (implies geographic proximity)
            // 2. High trade volume (indicates proximity or good connections)
            // 3. Direct diplomatic relations (not neutral isolation)

            bool is_neighbor = false;

            // Border tensions indicate shared borders
            if (state.has_border_tensions) {
                is_neighbor = true;
            }

            // High trade volume suggests proximity
            if (state.trade_volume > 300.0) {
                is_neighbor = true;
            }

            // Active treaties indicate meaningful proximity
            if (state.relation == DiplomaticRelation::ALLIED ||
                state.relation == DiplomaticRelation::AT_WAR) {
                is_neighbor = true;
            }

            // Strong opinions (positive or negative) suggest proximity
            if (std::abs(state.opinion) > 40) {
                is_neighbor = true;
            }

            if (is_neighbor) {
                neighbors.push_back(other_realm);
            }
        }

        // In full implementation, would query province/map system for actual geographic neighbors
        // This is a diplomatic-relationship-based approximation

        return neighbors;
    }

    std::vector<types::EntityID> DiplomacySystem::GetPotentialAllies(types::EntityID realm_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return std::vector<types::EntityID>();

        std::vector<types::EntityID> potential_allies;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return potential_allies;

        // Identify potential alliance candidates based on multiple factors
        for (const auto& [other_realm, state] : diplomacy->relationships) {
            // Skip if already allied or at war
            if (state.relation == DiplomaticRelation::ALLIED ||
                state.relation == DiplomaticRelation::AT_WAR) {
                continue;
            }

            ::core::ecs::EntityID other_handle(static_cast<uint64_t>(other_realm), 1);
            auto other_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(other_handle);
            if (!other_diplomacy) continue;

            // Calculate alliance potential score (0.0 to 1.0)
            double alliance_score = 0.0;

            // 1. Opinion is critical (40% weight)
            if (state.opinion > 50) {
                alliance_score += 0.4 * (state.opinion - 50.0) / 50.0;
            }

            // 2. Trust matters (20% weight)
            alliance_score += 0.2 * state.trust;

            // 3. Common enemies (20% weight)
            if (state.has_common_enemies) {
                alliance_score += 0.2;
            }

            // 4. Friendly relations (10% weight)
            if (state.relation == DiplomaticRelation::FRIENDLY) {
                alliance_score += 0.1;
            }

            // 5. Personality compatibility (10% weight)
            if (diplomacy->personality == DiplomaticPersonality::DIPLOMATIC ||
                other_diplomacy->personality == DiplomaticPersonality::DIPLOMATIC) {
                alliance_score += 0.05;
            }
            if (diplomacy->personality == DiplomaticPersonality::HONORABLE &&
                other_diplomacy->personality == DiplomaticPersonality::HONORABLE) {
                alliance_score += 0.05; // Honorable rulers trust each other
            }

            // Negative factors
            if (diplomacy->personality == DiplomaticPersonality::ISOLATIONIST ||
                other_diplomacy->personality == DiplomaticPersonality::ISOLATIONIST) {
                alliance_score *= 0.5; // Isolationists avoid alliances
            }
            if (diplomacy->personality == DiplomaticPersonality::TREACHEROUS ||
                other_diplomacy->personality == DiplomaticPersonality::TREACHEROUS) {
                alliance_score *= 0.7; // Treacherous rulers are risky allies
            }

            // Consider potential allies if score > 0.5
            if (alliance_score > 0.5) {
                potential_allies.push_back(other_realm);
            }
        }

        // Sort by alliance potential (highest first)
        std::sort(potential_allies.begin(), potential_allies.end(),
            [this, &diplomacy](types::EntityID a, types::EntityID b) {
                auto* state_a = diplomacy->GetRelationship(a);
                auto* state_b = diplomacy->GetRelationship(b);
                if (!state_a || !state_b) return false;
                return state_a->opinion > state_b->opinion;
            });

        return potential_allies;
    }

    std::vector<types::EntityID> DiplomacySystem::GetPotentialEnemies(types::EntityID realm_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return std::vector<types::EntityID>();

        std::vector<types::EntityID> potential_enemies;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return potential_enemies;

        // Identify potential threats and enemies
        for (const auto& [other_realm, state] : diplomacy->relationships) {
            // Skip if already at war
            if (state.relation == DiplomaticRelation::AT_WAR) {
                continue;
            }

            ::core::ecs::EntityID other_handle(static_cast<uint64_t>(other_realm), 1);
            auto other_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(other_handle);
            if (!other_diplomacy) continue;

            // Calculate threat level (0.0 to 1.0)
            double threat_score = 0.0;

            // 1. Negative opinion is primary indicator (30% weight)
            if (state.opinion < 0) {
                threat_score += 0.3 * (std::abs(state.opinion) / 100.0);
            }

            // 2. Hostile/Unfriendly relations (20% weight)
            if (state.relation == DiplomaticRelation::HOSTILE) {
                threat_score += 0.2;
            } else if (state.relation == DiplomaticRelation::UNFRIENDLY) {
                threat_score += 0.1;
            }

            // 3. Low trust (15% weight)
            threat_score += 0.15 * (1.0 - state.trust);

            // 4. Diplomatic incidents (15% weight)
            threat_score += std::min(0.15, state.diplomatic_incidents * 0.03);

            // 5. Border tensions (10% weight)
            if (state.has_border_tensions) {
                threat_score += 0.1;
            }

            // 6. Enemy personality assessment (10% weight)
            double enemy_aggression = GetPersonalityWarLikelihood(other_diplomacy->personality);
            threat_score += 0.1 * enemy_aggression;

            // 7. Allied with our enemies (bonus threat)
            for (const auto& [our_enemy, our_enemy_state] : diplomacy->relationships) {
                if (our_enemy_state.relation == DiplomaticRelation::AT_WAR) {
                    auto* their_rel = other_diplomacy->GetRelationship(our_enemy);
                    if (their_rel && their_rel->relation == DiplomaticRelation::ALLIED) {
                        threat_score += 0.15; // Significant threat
                        break;
                    }
                }
            }

            // Consider as potential enemy if threat > 0.4
            if (threat_score > 0.4) {
                potential_enemies.push_back(other_realm);
            }
        }

        // Sort by threat level (highest first)
        std::sort(potential_enemies.begin(), potential_enemies.end(),
            [this, &diplomacy](types::EntityID a, types::EntityID b) {
                auto* state_a = diplomacy->GetRelationship(a);
                auto* state_b = diplomacy->GetRelationship(b);
                if (!state_a || !state_b) return false;
                
                // Lower opinion = higher threat
                return state_a->opinion < state_b->opinion;
            });

        return potential_enemies;
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
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        // This method would be called during game initialization to assign personalities
        // to AI-controlled realms. In a full implementation, this would query all realms
        // and assign personalities based on historical data, culture, or randomization.

        // For now, this serves as a placeholder for personality initialization logic
        // that would be integrated with the realm management system.

        // Personality distribution guidelines:
        // - AGGRESSIVE: 15% (warlike, expansion-focused)
        // - DIPLOMATIC: 20% (alliance-building, peaceful)
        // - ISOLATIONIST: 10% (avoids entanglements)
        // - OPPORTUNISTIC: 20% (pragmatic, flexible)
        // - HONORABLE: 15% (keeps promises, reliable)
        // - TREACHEROUS: 10% (unreliable, betrays easily)
        // - MERCHANT: 15% (trade-focused, economic)
        // - RELIGIOUS: 10% (ideology-driven)

        ::core::logging::LogInfo("DiplomacySystem", 
            "Diplomatic personalities initialized - ready for AI realm assignment");

        // In full implementation, would iterate through realms and assign:
        // for each realm:
        //     auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm);
        //     diplomacy->personality = SelectPersonalityForRealm(realm);
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
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return;

        // Apply personality-based opinion modifiers
        int opinion_modifier = 0;

        switch (diplomacy->personality) {
            case DiplomaticPersonality::AGGRESSIVE:
                // Aggressive realms respect strength, dislike weakness
                if (relationship.opinion > 50) {
                    opinion_modifier = -5; // Don't like strong friends much
                }
                if (relationship.relation == DiplomaticRelation::HOSTILE) {
                    opinion_modifier = -10; // Really dislike enemies
                }
                break;

            case DiplomaticPersonality::DIPLOMATIC:
                // Diplomatic realms like everyone a bit more
                if (relationship.relation != DiplomaticRelation::AT_WAR) {
                    opinion_modifier = +5;
                }
                if (relationship.relation == DiplomaticRelation::FRIENDLY) {
                    opinion_modifier = +10; // Extra bonus for friends
                }
                break;

            case DiplomaticPersonality::ISOLATIONIST:
                // Isolationists are neutral to everyone
                if (relationship.opinion > 25) {
                    opinion_modifier = -5; // Don't get too friendly
                }
                break;

            case DiplomaticPersonality::OPPORTUNISTIC:
                // Opportunists favor the strong
                if (relationship.prestige_difference < -30.0) {
                    opinion_modifier = +10; // Like prestigious realms
                }
                else if (relationship.prestige_difference > 30.0) {
                    opinion_modifier = -5; // Dismiss weak realms
                }
                break;

            case DiplomaticPersonality::HONORABLE:
                // Honorable realms value trust
                if (relationship.trust > 0.7) {
                    opinion_modifier = +15; // Big bonus for trustworthy partners
                }
                else if (relationship.trust < 0.3) {
                    opinion_modifier = -15; // Dislike untrustworthy ones
                }
                if (relationship.diplomatic_incidents > 0) {
                    opinion_modifier -= relationship.diplomatic_incidents * 5;
                }
                break;

            case DiplomaticPersonality::TREACHEROUS:
                // Treacherous realms are unpredictable
                // Small random-seeming modifiers (based on trust)
                opinion_modifier = static_cast<int>((1.0 - relationship.trust) * 10.0) - 5;
                break;

            case DiplomaticPersonality::MERCHANT:
                // Merchants favor trade partners
                if (relationship.trade_volume > 500.0) {
                    opinion_modifier = +15; // Love trading partners
                }
                if (relationship.economic_dependency > 0.5) {
                    opinion_modifier += +10; // Depend on trade
                }
                break;

            case DiplomaticPersonality::RELIGIOUS:
                // Religious realms have strong opinions
                // Would check religious alignment if that system existed
                // For now, slightly more extreme opinions
                if (relationship.opinion > 50) {
                    opinion_modifier = +10; // Like friends more
                }
                else if (relationship.opinion < -50) {
                    opinion_modifier = -10; // Hate enemies more
                }
                break;

            default:
                break;
        }

        // Apply the modifier (this is a temporary modifier, not permanent)
        // In a full implementation, this would be applied during opinion calculations
        relationship.opinion += opinion_modifier;
        relationship.opinion = std::clamp(relationship.opinion, -100, 100);
    }

    double DiplomacySystem::GetPersonalityWarLikelihood(DiplomaticPersonality personality) const {
        // Returns 0.0 to 1.0 representing how likely this personality is to start wars
        switch (personality) {
            case DiplomaticPersonality::AGGRESSIVE:
                return 0.8; // Very likely to start wars
            case DiplomaticPersonality::DIPLOMATIC:
                return 0.2; // Avoids war
            case DiplomaticPersonality::ISOLATIONIST:
                return 0.1; // Almost never starts wars
            case DiplomaticPersonality::OPPORTUNISTIC:
                return 0.6; // Will war if advantageous
            case DiplomaticPersonality::HONORABLE:
                return 0.3; // Only with valid casus belli
            case DiplomaticPersonality::TREACHEROUS:
                return 0.7; // Willing to betray and attack
            case DiplomaticPersonality::MERCHANT:
                return 0.3; // Prefers trade to war
            case DiplomaticPersonality::RELIGIOUS:
                return 0.5; // Moderate, depends on ideology
            default:
                return 0.4; // Default moderate
        }
    }

    double DiplomacySystem::GetPersonalityTradePreference(DiplomaticPersonality personality) const {
        // Returns 0.0 to 1.0 representing how much this personality values trade
        switch (personality) {
            case DiplomaticPersonality::AGGRESSIVE:
                return 0.3; // Conquest over trade
            case DiplomaticPersonality::DIPLOMATIC:
                return 0.7; // Values economic cooperation
            case DiplomaticPersonality::ISOLATIONIST:
                return 0.1; // Avoids trade entanglements
            case DiplomaticPersonality::OPPORTUNISTIC:
                return 0.6; // Trades when beneficial
            case DiplomaticPersonality::HONORABLE:
                return 0.6; // Reliable trade partner
            case DiplomaticPersonality::TREACHEROUS:
                return 0.5; // Trades but may manipulate
            case DiplomaticPersonality::MERCHANT:
                return 0.95; // Loves trade above all
            case DiplomaticPersonality::RELIGIOUS:
                return 0.4; // Moderate trade interest
            default:
                return 0.5; // Default moderate
        }
    }

    std::vector<types::EntityID> DiplomacySystem::GetBorderingRealms(types::EntityID realm_id) const {
        auto* entity_manager = m_access_manager.GetEntityManager();
        if (!entity_manager) return std::vector<types::EntityID>();

        std::vector<types::EntityID> bordering_realms;

        ::core::ecs::EntityID realm_handle(static_cast<uint64_t>(realm_id), 1);
        auto diplomacy = entity_manager->GetComponent<DiplomacyComponent>(realm_handle);

        if (!diplomacy) return bordering_realms;

        // Identify realms that share geographic borders
        // In full implementation, would query province/map system for actual borders
        // For now, use diplomatic indicators

        for (const auto& [other_realm, state] : diplomacy->relationships) {
            bool shares_border = false;

            // 1. Border tensions strongly indicate shared borders
            if (state.has_border_tensions) {
                shares_border = true;
            }

            // 2. Very high trade suggests proximity (likely neighbors)
            if (state.trade_volume > 500.0) {
                shares_border = true;
            }

            // 3. Recent wars or conflicts suggest proximity
            if (state.relation == DiplomaticRelation::AT_WAR ||
                (state.relation == DiplomaticRelation::HOSTILE && state.diplomatic_incidents > 2)) {
                shares_border = true;
            }

            // 4. Strong opinions (very positive or very negative) often indicate neighbors
            if (state.opinion < -60 || state.opinion > 60) {
                shares_border = true;
            }

            if (shares_border) {
                bordering_realms.push_back(other_realm);
            }
        }

        // Note: In full implementation with province system, would do something like:
        // auto provinces = GetProvincesForRealm(realm_id);
        // for (auto province : provinces) {
        //     for (auto neighbor_province : GetNeighboringProvinces(province)) {
        //         auto neighbor_realm = GetRealmOwningProvince(neighbor_province);
        //         if (neighbor_realm != realm_id) {
        //             bordering_realms.push_back(neighbor_realm);
        //         }
        //     }
        // }

        // Remove duplicates
        std::sort(bordering_realms.begin(), bordering_realms.end());
        bordering_realms.erase(std::unique(bordering_realms.begin(), bordering_realms.end()), 
                              bordering_realms.end());

        return bordering_realms;
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