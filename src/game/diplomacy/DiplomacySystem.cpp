#include "diplomacysystem.h"
#include "../core/logging.h"
#include <algorithm>
#include <cmath>
#include <random>

namespace game {
    namespace diplomacy {

        // ============================================================================
        // DiplomaticState Implementation
        // ============================================================================

        DiplomaticState::DiplomaticState(types::EntityID realm) : other_realm(realm) {
            last_contact = std::chrono::system_clock::now();
        }

        // ============================================================================
        // Treaty Implementation
        // ============================================================================

        Treaty::Treaty(TreatyType treaty_type, types::EntityID realm_a, types::EntityID realm_b)
            : type(treaty_type), signatory_a(realm_a), signatory_b(realm_b) {

            signed_date = std::chrono::system_clock::now();

            // Set default expiry based on treaty type
            auto duration = std::chrono::years(10); // Default 10 years
            switch (treaty_type) {
            case TreatyType::NON_AGGRESSION:
                duration = std::chrono::years(5);
                break;
            case TreatyType::TRADE_AGREEMENT:
                duration = std::chrono::years(20);
                break;
            case TreatyType::ALLIANCE:
                duration = std::chrono::years(25);
                break;
            case TreatyType::MARRIAGE_PACT:
                duration = std::chrono::years(50); // Generational
                break;
            default:
                break;
            }

            expiry_date = signed_date + duration;

            // Generate unique treaty ID
            treaty_id = utils::TreatyTypeToString(treaty_type) + "_" +
                std::to_string(realm_a) + "_" + std::to_string(realm_b) + "_" +
                std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                    signed_date.time_since_epoch()).count());
        }

        bool Treaty::IsExpired() const {
            return std::chrono::system_clock::now() > expiry_date;
        }

        bool Treaty::IsBroken() const {
            return compliance_a < 0.5 || compliance_b < 0.5;
        }

        double Treaty::GetOverallCompliance() const {
            return (compliance_a + compliance_b) / 2.0;
        }

        // ============================================================================
        // DynasticMarriage Implementation
        // ============================================================================

        DynasticMarriage::DynasticMarriage(types::EntityID bride, types::EntityID groom)
            : bride_realm(bride), groom_realm(groom) {

            marriage_date = std::chrono::system_clock::now();

            // Generate unique marriage ID
            marriage_id = "marriage_" + std::to_string(bride) + "_" + std::to_string(groom) + "_" +
                std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                    marriage_date.time_since_epoch()).count());
        }

        // ============================================================================
        // DiplomacyComponent Implementation
        // ============================================================================

        DiplomaticState* DiplomacyComponent::GetRelationship(types::EntityID other_realm) {
            auto it = relationships.find(other_realm);
            if (it != relationships.end()) {
                return &it->second;
            }

            // Create new relationship if it doesn't exist
            relationships[other_realm] = DiplomaticState(other_realm);
            return &relationships[other_realm];
        }

        const DiplomaticState* DiplomacyComponent::GetRelationship(types::EntityID other_realm) const {
            auto it = relationships.find(other_realm);
            return (it != relationships.end()) ? &it->second : nullptr;
        }

        void DiplomacyComponent::SetRelation(types::EntityID other_realm, DiplomaticRelation relation) {
            auto* state = GetRelationship(other_realm);
            if (state) {
                state->relation = relation;

                // Update allies/enemies lists
                auto ally_it = std::find(allies.begin(), allies.end(), other_realm);
                auto enemy_it = std::find(enemies.begin(), enemies.end(), other_realm);

                if (relation == DiplomaticRelation::ALLIED && ally_it == allies.end()) {
                    allies.push_back(other_realm);
                    if (enemy_it != enemies.end()) {
                        enemies.erase(enemy_it);
                    }
                }
                else if (relation == DiplomaticRelation::AT_WAR && enemy_it == enemies.end()) {
                    enemies.push_back(other_realm);
                    if (ally_it != allies.end()) {
                        allies.erase(ally_it);
                    }
                }
            }
        }

        void DiplomacyComponent::ModifyOpinion(types::EntityID other_realm, int opinion_change, const std::string& reason) {
            auto* state = GetRelationship(other_realm);
            if (state) {
                state->opinion += opinion_change;
                state->opinion = std::clamp(state->opinion, -100, 100);
                state->recent_actions.push_back(reason + " (" +
                    (opinion_change > 0 ? "+" : "") + std::to_string(opinion_change) + ")");

                // Keep only recent actions (last 10)
                if (state->recent_actions.size() > 10) {
                    state->recent_actions.erase(state->recent_actions.begin());
                }
            }
        }

        void DiplomacyComponent::AddTreaty(const Treaty& treaty) {
            active_treaties.push_back(treaty);
        }

        void DiplomacyComponent::RemoveTreaty(const std::string& treaty_id) {
            active_treaties.erase(
                std::remove_if(active_treaties.begin(), active_treaties.end(),
                    [&treaty_id](const Treaty& t) { return t.treaty_id == treaty_id; }),
                active_treaties.end());
        }

        std::vector<Treaty*> DiplomacyComponent::GetTreatiesWith(types::EntityID other_realm) {
            std::vector<Treaty*> treaties;
            for (auto& treaty : active_treaties) {
                if (treaty.signatory_a == other_realm || treaty.signatory_b == other_realm) {
                    treaties.push_back(&treaty);
                }
            }
            return treaties;
        }

        bool DiplomacyComponent::HasTreatyType(types::EntityID other_realm, TreatyType type) const {
            for (const auto& treaty : active_treaties) {
                if ((treaty.signatory_a == other_realm || treaty.signatory_b == other_realm) &&
                    treaty.type == type && treaty.is_active) {
                    return true;
                }
            }
            return false;
        }

        bool DiplomacyComponent::IsAtWar() const {
            return !enemies.empty();
        }

        bool DiplomacyComponent::IsAtWarWith(types::EntityID other_realm) const {
            return std::find(enemies.begin(), enemies.end(), other_realm) != enemies.end();
        }

        bool DiplomacyComponent::IsAlliedWith(types::EntityID other_realm) const {
            return std::find(allies.begin(), allies.end(), other_realm) != allies.end();
        }

        std::vector<types::EntityID> DiplomacyComponent::GetWarEnemies() const {
            return enemies;
        }

        std::vector<types::EntityID> DiplomacyComponent::GetMilitaryAllies() const {
            return allies;
        }

        // ============================================================================
        // DiplomaticProposal Implementation
        // ============================================================================

        DiplomaticProposal::DiplomaticProposal(types::EntityID from, types::EntityID to, DiplomaticAction action)
            : proposer(from), target(to), action_type(action) {

            proposed_date = std::chrono::system_clock::now();
            expiry_date = proposed_date + std::chrono::days(30); // 30 day expiry

            proposal_id = utils::DiplomaticActionToString(action) + "_" +
                std::to_string(from) + "_" + std::to_string(to) + "_" +
                std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                    proposed_date.time_since_epoch()).count());
        }

        // ============================================================================
        // DiplomacySystem Implementation
        // ============================================================================

        DiplomacySystem::DiplomacySystem(core::ecs::ComponentAccessManager& access_manager,
            core::ecs::MessageBus& message_bus)
            : m_access_manager(access_manager), m_message_bus(message_bus) {
        }

        void DiplomacySystem::Initialize() {
            if (m_initialized) {
                return;
            }

            core::logging::LogInfo("DiplomacySystem", "Initializing Diplomacy System");

            void DiplomacySystem::Initialize() {
                if (m_initialized) {
                    return;
                }

                core::logging::LogInfo("DiplomacySystem", "Initializing Diplomacy System");

                InitializeDiplomaticPersonalities();
                SubscribeToEvents();

                m_initialized = true;
                core::logging::LogInfo("DiplomacySystem", "Diplomacy System initialized successfully");
            }

            void DiplomacySystem::Update(float delta_time) {
                if (!m_initialized) {
                    return;
                }

                m_accumulated_time += delta_time;
                m_monthly_timer += delta_time;

                // Regular updates (every second)
                if (m_accumulated_time >= m_update_interval) {
                    auto realms = GetAllRealms();

                    for (auto realm_id : realms) {
                        UpdateDiplomaticRelationships(realm_id);
                        ProcessTreatyCompliance(realm_id);
                        ProcessDiplomaticDecay(realm_id, m_accumulated_time);
                    }

                    // Process pending proposals
                    for (auto& proposal : m_pending_proposals) {
                        if (proposal.is_pending) {
                            proposal.acceptance_chance = EvaluateProposal(proposal);
                        }
                    }

                    m_accumulated_time = 0.0f;
                }

                // Monthly diplomatic processing
                if (m_monthly_timer >= 30.0f) {
                    auto realms = GetAllRealms();

                    for (auto realm_id : realms) {
                        CalculatePrestigeEffects(realm_id);
                        ProcessAIDiplomacy(realm_id);
                        UpdateTradeRelations(realm_id);
                        ProcessDiplomaticIntelligence(realm_id);
                    }

                    // Clean up expired proposals
                    m_pending_proposals.erase(
                        std::remove_if(m_pending_proposals.begin(), m_pending_proposals.end(),
                            [](const DiplomaticProposal& p) {
                                return std::chrono::system_clock::now() > p.expiry_date;
                            }),
                        m_pending_proposals.end());

                    m_monthly_timer = 0.0f;
                }
            }

            void DiplomacySystem::Shutdown() {
                core::logging::LogInfo("DiplomacySystem", "Shutting down Diplomacy System");
                m_pending_proposals.clear();
                m_diplomatic_cooldowns.clear();
                m_initialized = false;
            }

            // ============================================================================
            // Diplomatic Actions
            // ============================================================================

            bool DiplomacySystem::ProposeAlliance(types::EntityID proposer, types::EntityID target,
                const std::unordered_map<std::string, double>&terms) {
                auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("ProposeAlliance");
                auto* proposer_diplomacy = diplomacy_read.GetComponent(proposer);
                auto* target_diplomacy = diplomacy_read.GetComponent(target);

                if (!proposer_diplomacy || !target_diplomacy) {
                    return false;
                }

                // Check if already allied
                if (proposer_diplomacy->IsAlliedWith(target)) {
                    return false;
                }

                // Check if at war
                if (proposer_diplomacy->IsAtWarWith(target)) {
                    return false;
                }

                // Create proposal
                DiplomaticProposal proposal(proposer, target, DiplomaticAction::PROPOSE_ALLIANCE);
                proposal.terms = terms;
                proposal.acceptance_chance = EvaluateAllianceProposal(proposal);

                m_pending_proposals.push_back(proposal);

                core::logging::LogInfo("DiplomacySystem",
                    "Alliance proposed between " + std::to_string(proposer) +
                    " and " + std::to_string(target));

                return true;
            }

            bool DiplomacySystem::DeclareWar(types::EntityID aggressor, types::EntityID target, CasusBelli casus_belli) {
                auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("DeclareWar");
                auto* aggressor_diplomacy = diplomacy_write.GetComponent(aggressor);
                auto* target_diplomacy = diplomacy_write.GetComponent(target);

                if (!aggressor_diplomacy || !target_diplomacy) {
                    return false;
                }

                // Check if already at war
                if (aggressor_diplomacy->IsAtWarWith(target)) {
                    return false;
                }

                // Set war state
                aggressor_diplomacy->SetRelation(target, DiplomaticRelation::AT_WAR);
                target_diplomacy->SetRelation(aggressor, DiplomaticRelation::AT_WAR);

                // Break existing treaties
                auto aggressor_treaties = aggressor_diplomacy->GetTreatiesWith(target);
                for (auto* treaty : aggressor_treaties) {
                    if (treaty->type == TreatyType::NON_AGGRESSION || treaty->type == TreatyType::TRADE_AGREEMENT) {
                        treaty->is_active = false;
                        HandleTreatyViolation(treaty->treaty_id, aggressor);
                    }
                }

                // Activate allies
                HandleAllyActivation(aggressor, aggressor_diplomacy->GetMilitaryAllies());
                HandleAllyActivation(target, target_diplomacy->GetMilitaryAllies());

                // Send war declaration message
                messages::WarDeclared msg;
                msg.aggressor = aggressor;
                msg.defender = target;
                msg.casus_belli = casus_belli;
                msg.war_name = "War of " + std::to_string(aggressor) + " vs " + std::to_string(target);
                msg.aggressor_allies = aggressor_diplomacy->GetMilitaryAllies();
                msg.defender_allies = target_diplomacy->GetMilitaryAllies();

                m_message_bus.SendMessage(core::ecs::Message::Create(msg));

                core::logging::LogInfo("DiplomacySystem",
                    "War declared: " + std::to_string(aggressor) +
                    " vs " + std::to_string(target) +
                    " (CB: " + utils::CasusBelliToString(casus_belli) + ")");

                return true;
            }

            bool DiplomacySystem::ArrangeMarriage(types::EntityID bride_realm, types::EntityID groom_realm, bool create_alliance) {
                auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("ArrangeMarriage");
                auto* bride_diplomacy = diplomacy_write.GetComponent(bride_realm);
                auto* groom_diplomacy = diplomacy_write.GetComponent(groom_realm);

                if (!bride_diplomacy || !groom_diplomacy) {
                    return false;
                }

                // Check if valid marriage candidate
                if (!utils::IsValidMarriageCandidate(bride_realm, groom_realm)) {
                    return false;
                }

                // Create marriage
                DynasticMarriage marriage(bride_realm, groom_realm);
                marriage.produces_alliance = create_alliance;
                marriage.diplomatic_bonus = utils::CalculateMarriageValue(bride_realm, groom_realm);

                bride_diplomacy->royal_marriages.push_back(marriage);
                groom_diplomacy->royal_marriages.push_back(marriage);

                // Apply diplomatic effects
                ProcessMarriageEffects(marriage);

                // Send marriage message
                messages::MarriageArranged msg;
                msg.marriage = marriage;
                msg.diplomatic_impact = marriage.diplomatic_bonus;
                msg.marriage_story = "Marriage arranged between realms " +
                    std::to_string(bride_realm) + " and " + std::to_string(groom_realm);
                msg.creates_alliance = create_alliance;

                m_message_bus.SendMessage(core::ecs::Message::Create(msg));

                core::logging::LogInfo("DiplomacySystem",
                    "Marriage arranged between " + std::to_string(bride_realm) +
                    " and " + std::to_string(groom_realm));

                return true;
            }

            // ============================================================================
            // Treaty and Relationship Management
            // ============================================================================

            void DiplomacySystem::ProcessTreatyCompliance(types::EntityID realm_id) {
                auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("TreatyCompliance");
                auto* diplomacy = diplomacy_write.GetComponent(realm_id);

                if (!diplomacy) {
                    return;
                }

                for (auto& treaty : diplomacy->active_treaties) {
                    if (!treaty.is_active) continue;

                    UpdateTreatyStatus(treaty);

                    // Check for violations
                    if (treaty.IsBroken()) {
                        HandleTreatyViolation(treaty.treaty_id, realm_id);
                    }

                    // Check for expiry
                    if (treaty.IsExpired()) {
                        treaty.is_active = false;
                        LogDiplomaticEvent(treaty.signatory_a, treaty.signatory_b,
                            "Treaty " + treaty.treaty_id + " expired");
                    }
                }
            }

            void DiplomacySystem::UpdateDiplomaticRelationships(types::EntityID realm_id) {
                auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("UpdateRelationships");
                auto* diplomacy = diplomacy_write.GetComponent(realm_id);

                if (!diplomacy) {
                    return;
                }

                for (auto& [other_realm, relationship] : diplomacy->relationships) {
                    // Update relationship based on opinion
                    DiplomaticRelation old_relation = relationship.relation;

                    if (relationship.opinion >= 80 && old_relation != DiplomaticRelation::ALLIED) {
                        relationship.relation = DiplomaticRelation::FRIENDLY;
                    }
                    else if (relationship.opinion >= 20) {
                        relationship.relation = DiplomaticRelation::NEUTRAL;
                    }
                    else if (relationship.opinion >= -50) {
                        relationship.relation = DiplomaticRelation::UNFRIENDLY;
                    }
                    else if (relationship.opinion < -50 && old_relation != DiplomaticRelation::AT_WAR) {
                        relationship.relation = DiplomaticRelation::HOSTILE;
                    }

                    // Send message if relation changed
                    if (old_relation != relationship.relation) {
                        messages::DiplomaticRelationChanged msg;
                        msg.realm_a = realm_id;
                        msg.realm_b = other_realm;
                        msg.old_relation = old_relation;
                        msg.new_relation = relationship.relation;
                        msg.reason = "Opinion change";

                        m_message_bus.SendMessage(core::ecs::Message::Create(msg));
                    }
                }
            }

            void DiplomacySystem::ProcessDiplomaticDecay(types::EntityID realm_id, float time_delta) {
                auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("DiplomaticDecay");
                auto* diplomacy = diplomacy_write.GetComponent(realm_id);

                if (!diplomacy) {
                    return;
                }

                for (auto& [other_realm, relationship] : diplomacy->relationships) {
                    // Opinion decays toward neutral over time
                    double decay_amount = utils::CalculateOpinionDecay(relationship.opinion, time_delta);

                    if (relationship.opinion > 0) {
                        relationship.opinion = std::max(0.0, relationship.opinion - decay_amount);
                    }
                    else if (relationship.opinion < 0) {
                        relationship.opinion = std::min(0.0, relationship.opinion + decay_amount);
                    }

                    // Trust decays slowly without interaction
                    auto time_since_contact = std::chrono::system_clock::now() - relationship.last_contact;
                    auto days_since_contact = std::chrono::duration_cast<std::chrono::days>(time_since_contact).count();

                    if (days_since_contact > 365) { // More than a year
                        relationship.trust *= 0.99; // 1% monthly decay
                    }
                }
            }

            // ============================================================================
            // AI Diplomacy
            // ============================================================================

            void DiplomacySystem::ProcessAIDiplomacy(types::EntityID realm_id) {
                auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("AIDiplomacy");
                auto* diplomacy = diplomacy_read.GetComponent(realm_id);

                if (!diplomacy) {
                    return;
                }

                // Generate AI diplomatic actions based on personality and situation
                GenerateAIDiplomaticActions(realm_id);

                // Process pending proposals
                for (auto& proposal : m_pending_proposals) {
                    if (proposal.target == realm_id && proposal.is_pending) {
                        double acceptance_chance = EvaluateProposal(proposal);

                        // Simple acceptance threshold
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_real_distribution<double> dist(0.0, 1.0);

                        if (dist(gen) < acceptance_chance) {
                            // Accept proposal
                            switch (proposal.action_type) {
                            case DiplomaticAction::PROPOSE_ALLIANCE:
                                // Create alliance treaty
                            {
                                Treaty alliance_treaty(TreatyType::ALLIANCE, proposal.proposer, proposal.target);
                                alliance_treaty.terms = proposal.terms;

                                auto proposer_diplomacy = GetDiplomacyComponent(proposal.proposer);
                                auto target_diplomacy = GetDiplomacyComponent(proposal.target);

                                if (proposer_diplomacy && target_diplomacy) {
                                    proposer_diplomacy->AddTreaty(alliance_treaty);
                                    target_diplomacy->AddTreaty(alliance_treaty);
                                    proposer_diplomacy->SetRelation(proposal.target, DiplomaticRelation::ALLIED);
                                    target_diplomacy->SetRelation(proposal.proposer, DiplomaticRelation::ALLIED);
                                }
                            }
                            break;
                            // Handle other proposal types...
                            default:
                                break;
                            }

                            proposal.is_pending = false;

                            core::logging::LogInfo("DiplomacySystem",
                                "Proposal accepted: " + utils::DiplomaticActionToString(proposal.action_type));
                        }
                    }
                }
            }

            void DiplomacySystem::GenerateAIDiplomaticActions(types::EntityID realm_id) {
                auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("GenerateAIActions");
                auto* diplomacy = diplomacy_read.GetComponent(realm_id);

                if (!diplomacy) {
                    return;
                }

                // AI behavior based on personality
                switch (diplomacy->personality) {
                case DiplomaticPersonality::AGGRESSIVE:
                    // Look for weak neighbors to declare war on
                    for (auto neighbor : GetNeighboringRealms(realm_id)) {
                        if (!diplomacy->IsAtWarWith(neighbor) &&
                            GetMilitaryStrengthRatio(realm_id, neighbor) > 1.5) {

                            CasusBelli cb = FindBestCasusBelli(realm_id, neighbor);
                            if (cb != CasusBelli::NONE) {
                                DeclareWar(realm_id, neighbor, cb);
                                break; // Only one war at a time
                            }
                        }
                    }
                    break;

                case DiplomaticPersonality::DIPLOMATIC:
                    // Seek alliances with friendly neighbors
                    for (auto neighbor : GetNeighboringRealms(realm_id)) {
                        if (!diplomacy->IsAlliedWith(neighbor) &&
                            GetOpinion(realm_id, neighbor) > 40) {

                            std::unordered_map<std::string, double> terms;
                            terms["mutual_defense"] = 1.0;
                            ProposeAlliance(realm_id, neighbor, terms);
                            break; // One proposal at a time
                        }
                    }
                    break;

                case DiplomaticPersonality::MERCHANT:
                    // Focus on trade agreements
                    for (auto potential_partner : GetAllRealms()) {
                        if (potential_partner != realm_id &&
                            !diplomacy->HasTreatyType(potential_partner, TreatyType::TRADE_AGREEMENT)) {

                            double trade_value = CalculateTradeValue(realm_id, potential_partner);
                            if (trade_value > 50.0) {
                                ProposeTradeAgreement(realm_id, potential_partner, 0.2, 10);
                                break;
                            }
                        }
                    }
                    break;

                default:
                    // Default behavior - maintain neutrality
                    break;
                }
            }

            // ============================================================================
            // Query Methods
            // ============================================================================

            std::vector<types::EntityID> DiplomacySystem::GetAllRealms() const {
                std::vector<types::EntityID> realms;

                // For demonstration, return a range of entity IDs
                // In a real implementation, this would query the entity manager
                for (types::EntityID id = 3000; id < 3010; ++id) {
                    realms.push_back(id);
                }

                return realms;
            }

            std::vector<types::EntityID> DiplomacySystem::GetNeighboringRealms(types::EntityID realm_id) const {
                // This would use actual geographic data to find border neighbors
                // For now, return a simplified neighboring relationship
                return GetBorderingRealms(realm_id);
            }

            DiplomaticRelation DiplomacySystem::GetRelation(types::EntityID realm_a, types::EntityID realm_b) const {
                auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("GetRelation");
                auto* diplomacy = diplomacy_read.GetComponent(realm_a);

                if (!diplomacy) {
                    return DiplomaticRelation::NEUTRAL;
                }

                auto* relationship = diplomacy->GetRelationship(realm_b);
                return relationship ? relationship->relation : DiplomaticRelation::NEUTRAL;
            }

            int DiplomacySystem::GetOpinion(types::EntityID realm_a, types::EntityID realm_b) const {
                auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("GetOpinion");
                auto* diplomacy = diplomacy_read.GetComponent(realm_a);

                if (!diplomacy) {
                    return 0;
                }

                auto* relationship = diplomacy->GetRelationship(realm_b);
                return relationship ? relationship->opinion : 0;
            }

            DiplomacyComponent* DiplomacySystem::GetDiplomacyComponent(types::EntityID realm_id) {
                auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("GetDiplomacyComponent");
                return diplomacy_write.GetComponent(realm_id);
            }

            const DiplomacyComponent* DiplomacySystem::GetDiplomacyComponent(types::EntityID realm_id) const {
                auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("GetDiplomacyComponent");
                return diplomacy_read.GetComponent(realm_id);
            }

            // ============================================================================
            // Helper Methods
            // ============================================================================

            void DiplomacySystem::InitializeDiplomaticPersonalities() {
                // This would initialize AI personalities for different realms
                core::logging::LogInfo("DiplomacySystem", "Initialized diplomatic personalities");
            }

            void DiplomacySystem::SubscribeToEvents() {
                // Subscribe to war-related events
                // Subscribe to trade events
                // Subscribe to technology discoveries that affect diplomacy
            }

            double DiplomacySystem::EvaluateProposal(const DiplomaticProposal & proposal) {
                switch (proposal.action_type) {
                case DiplomaticAction::PROPOSE_ALLIANCE:
                    return EvaluateAllianceProposal(proposal);
                case DiplomaticAction::PROPOSE_TRADE:
                    return EvaluateTradeProposal(proposal);
                default:
                    return 0.5; // Neutral evaluation
                }
            }

            double DiplomacySystem::EvaluateAllianceProposal(const DiplomaticProposal & proposal) const {
                auto proposer_diplomacy = GetDiplomacyComponent(proposal.proposer);
                auto target_diplomacy = GetDiplomacyComponent(proposal.target);

                if (!proposer_diplomacy || !target_diplomacy) {
                    return 0.0;
                }

                double evaluation = 0.5; // Base neutral

                // Opinion affects acceptance
                int opinion = GetOpinion(proposal.target, proposal.proposer);
                evaluation += opinion / 200.0; // Scale -100:100 to -0.5:0.5

                // Military strength ratio affects desirability
                double strength_ratio = GetMilitaryStrengthRatio(proposal.proposer, proposal.target);
                if (strength_ratio > 1.0) {
                    evaluation += 0.2; // Allying with stronger party is good
                }

                // Common enemies increase alliance value
                auto proposer_enemies = proposer_diplomacy->GetWarEnemies();
                auto target_enemies = target_diplomacy->GetWarEnemies();

                for (auto enemy : proposer_enemies) {
                    if (std::find(target_enemies.begin(), target_enemies.end(), enemy) != target_enemies.end()) {
                        evaluation += 0.3; // Shared enemies strongly favor alliance
                    }
                }

                return std::clamp(evaluation, 0.0, 1.0);
            }

            void DiplomacySystem::LogDiplomaticEvent(types::EntityID realm_a, types::EntityID realm_b, const std::string & event) {
                core::logging::LogInfo("DiplomacySystem",
                    "Diplomatic event between " + std::to_string(realm_a) +
                    " and " + std::to_string(realm_b) + ": " + event);
            }

            // ============================================================================
            // Utility Functions Implementation
            // ============================================================================

            namespace utils {
                std::string DiplomaticRelationToString(DiplomaticRelation relation) {
                    switch (relation) {
                    case DiplomaticRelation::ALLIED: return "Allied";
                    case DiplomaticRelation::FRIENDLY: return "Friendly";
                    case DiplomaticRelation::NEUTRAL: return "Neutral";
                    case DiplomaticRelation::UNFRIENDLY: return "Unfriendly";
                    case DiplomaticRelation::HOSTILE: return "Hostile";
                    case DiplomaticRelation::AT_WAR: return "At War";
                    default: return "Unknown";
                    }
                }

                std::string TreatyTypeToString(TreatyType type) {
                    switch (type) {
                    case TreatyType::ALLIANCE: return "Alliance";
                    case TreatyType::TRADE_AGREEMENT: return "Trade Agreement";
                    case TreatyType::NON_AGGRESSION: return "Non-Aggression Pact";
                    case TreatyType::MARRIAGE_PACT: return "Marriage Pact";
                    case TreatyType::TRIBUTE: return "Tribute";
                    case TreatyType::BORDER_AGREEMENT: return "Border Agreement";
                    case TreatyType::MILITARY_ACCESS: return "Military Access";
                    case TreatyType::DEFENSIVE_LEAGUE: return "Defensive League";
                    default: return "Unknown Treaty";
                    }
                }

                std::string DiplomaticActionToString(DiplomaticAction action) {
                    switch (action) {
                    case DiplomaticAction::PROPOSE_ALLIANCE: return "Propose Alliance";
                    case DiplomaticAction::PROPOSE_TRADE: return "Propose Trade";
                    case DiplomaticAction::DECLARE_WAR: return "Declare War";
                    case DiplomaticAction::SUE_FOR_PEACE: return "Sue for Peace";
                    case DiplomaticAction::SEND_GIFT: return "Send Gift";
                    case DiplomaticAction::ARRANGE_MARRIAGE: return "Arrange Marriage";
                    default: return "Unknown Action";
                    }
                }

                std::string CasusBelliToString(CasusBelli cb) {
                    switch (cb) {
                    case CasusBelli::BORDER_DISPUTE: return "Border Dispute";
                    case CasusBelli::TRADE_INTERFERENCE: return "Trade Interference";
                    case CasusBelli::DYNASTIC_CLAIM: return "Dynastic Claim";
                    case CasusBelli::RELIGIOUS_CONFLICT: return "Religious Conflict";
                    case CasusBelli::INSULT_TO_HONOR: return "Insult to Honor";
                    case CasusBelli::BROKEN_TREATY: return "Broken Treaty";
                    case CasusBelli::PROTECTION_OF_ALLY: return "Protection of Ally";
                    default: return "No Justification";
                    }
                }

                double CalculateOpinionDecay(double current_opinion, float time_delta) {
                    // Opinion decays toward 0 over time
                    double decay_rate = 0.1 * time_delta; // Adjust decay speed
                    return std::abs(current_opinion) * decay_rate;
                }

                bool IsValidMarriageCandidate(types::EntityID bride_realm, types::EntityID groom_realm) {
                    // Basic validation - can't marry yourself
                    return bride_realm != groom_realm;
                }

                double CalculateMarriageValue(types::EntityID realm_a, types::EntityID realm_b) {
                    // Simplified marriage value calculation
                    return 25.0; // Base diplomatic bonus
                }
            }

        } // namespace diplomacy
    } // namespace game