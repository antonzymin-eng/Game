// ============================================================================
// Date/Time Created: September 27, 2025 - 3:30 PM PST
// Intended Folder Location: src/game/diplomacy/DiplomacySystem.cpp
// DiplomacySystem Implementation - FIXED: Lines 1-500
// ============================================================================

#include "game/diplomacy/DiplomacySystem.h"
#include "core/logging/Logger.h"
#include "game/config/GameConfig.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <chrono>

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

            // FIXED: Load durations from config
            auto& config = game::config::GameConfig::Instance();
            int duration_years = 10;
            
            switch (treaty_type) {
            case TreatyType::NON_AGGRESSION:
                duration_years = config.GetInt("diplomacy.non_aggression_duration_years", 5);
                break;
            case TreatyType::TRADE_AGREEMENT:
                duration_years = config.GetInt("diplomacy.trade_agreement_duration_years", 20);
                break;
            case TreatyType::ALLIANCE:
                duration_years = config.GetInt("diplomacy.alliance_duration_years", 25);
                break;
            case TreatyType::MARRIAGE_PACT:
                duration_years = config.GetInt("diplomacy.marriage_pact_duration_years", 50);
                break;
            default:
                duration_years = config.GetInt("diplomacy.default_treaty_duration_years", 10);
                break;
            }

            expiry_date = signed_date + std::chrono::hours(24 * 365 * duration_years);

            treaty_id = utils::TreatyTypeToString(treaty_type) + "_" +
                std::to_string(realm_a) + "_" + std::to_string(realm_b) + "_" +
                std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                    signed_date.time_since_epoch()).count());
        }

        bool Treaty::IsExpired() const {
            return std::chrono::system_clock::now() > expiry_date;
        }

        bool Treaty::IsBroken() const {
            auto& config = game::config::GameConfig::Instance();
            double min_compliance = config.GetDouble("diplomacy.treaty_compliance_threshold", 0.5);
            return compliance_a < min_compliance || compliance_b < min_compliance;
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

            // FIXED: Load marriage bonus from config
            auto& config = game::config::GameConfig::Instance();
            diplomatic_bonus = config.GetDouble("diplomacy.marriage_base_bonus", 20.0);

            marriage_id = "marriage_" + std::to_string(bride) + "_" + std::to_string(groom) + "_" +
                std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                    marriage_date.time_since_epoch()).count());
        }

        // ============================================================================

        DiplomaticProposal::DiplomaticProposal(types::EntityID from, types::EntityID to, DiplomaticAction action)
            : proposer(from), target(to), action_type(action) {

            proposed_date = std::chrono::system_clock::now();
            expiry_date = proposed_date + std::chrono::hours(24 * 30);

            proposal_id = utils::DiplomaticActionToString(action) + "_" +
                std::to_string(from) + "_" + std::to_string(to) + "_" +
                std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                    proposed_date.time_since_epoch()).count());
        }

        // ============================================================================
        // DiplomacySystem Implementation - FIXED: No duplicate Initialize()
        // ============================================================================

        DiplomacySystem::DiplomacySystem(::core::ecs::ComponentAccessManager& access_manager,
                                        ::core::ecs::MessageBus& message_bus)
            : m_access_manager(access_manager), m_message_bus(message_bus) {
        }

        void DiplomacySystem::Initialize() {
            if (m_initialized) {
                return;
            }

            ::core::logging::LogInfo("DiplomacySystem", "Initializing Diplomacy System");

            // TODO: Component registration handled by main application

            InitializeDiplomaticPersonalities();
            SubscribeToEvents();

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
                auto realms = GetAllRealms();

                for (auto realm_id : realms) {
                    UpdateDiplomaticRelationships(realm_id);
                    ProcessTreatyCompliance(realm_id);
                    ProcessDiplomaticDecay(realm_id, m_accumulated_time);
                }

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
            ::core::logging::LogInfo("DiplomacySystem", "Shutting down Diplomacy System");
            m_pending_proposals.clear();
            m_diplomatic_cooldowns.clear();
            m_initialized = false;
        }

        ::core::threading::ThreadingStrategy DiplomacySystem::GetThreadingStrategy() const {
            return ::core::threading::ThreadingStrategy::BACKGROUND_THREAD;
        }

        // ============================================================================
        // Diplomatic Actions
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
                return false;
            }

            if (proposer_diplomacy->IsAlliedWith(target)) {
                return false;
            }

            if (proposer_diplomacy->IsAtWarWith(target)) {
                return false;
            }

            DiplomaticProposal proposal(proposer, target, DiplomaticAction::PROPOSE_ALLIANCE);
            proposal.terms = terms;
            proposal.acceptance_chance = EvaluateAllianceProposal(proposal);

            m_pending_proposals.push_back(proposal);

            ::core::logging::LogInfo("DiplomacySystem",
                "Alliance proposed between " + std::to_string(proposer) +
                " and " + std::to_string(target));

            return true;
        }

        bool DiplomacySystem::ProposeTradeAgreement(types::EntityID proposer, types::EntityID target,
            double trade_bonus, int duration_years) {
            auto* entity_manager = m_access_manager.GetEntityManager();
            if (!entity_manager) return false;
            
            ::core::ecs::EntityID proposer_handle(static_cast<uint64_t>(proposer), 1);
            ::core::ecs::EntityID target_handle(static_cast<uint64_t>(target), 1);
            
            auto proposer_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(proposer_handle);
            auto target_diplomacy = entity_manager->GetComponent<DiplomacyComponent>(target_handle);

            if (!proposer_diplomacy || !target_diplomacy) {
                return false;
            }

            DiplomaticProposal proposal(proposer, target, DiplomaticAction::PROPOSE_TRADE);
            proposal.terms["trade_bonus"] = trade_bonus;
            proposal.terms["duration_years"] = static_cast<double>(duration_years);
            proposal.acceptance_chance = EvaluateTradeProposal(proposal);

            m_pending_proposals.push_back(proposal);

            ::core::logging::LogInfo("DiplomacySystem",
                "Trade agreement proposed between " + std::to_string(proposer) +
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

            if (!aggressor_diplomacy || !target_diplomacy) {
                return false;
            }

            if (aggressor_diplomacy->IsAtWarWith(target)) {
                return false;
            }

            aggressor_diplomacy->SetRelation(target, DiplomaticRelation::AT_WAR);
            target_diplomacy->SetRelation(aggressor, DiplomaticRelation::AT_WAR);

            auto aggressor_treaties = aggressor_diplomacy->GetTreatiesWith(target);
            for (auto* treaty : aggressor_treaties) {
                if (treaty->type == TreatyType::NON_AGGRESSION || treaty->type == TreatyType::TRADE_AGREEMENT) {
                    treaty->is_active = false;
                    HandleTreatyViolation(treaty->treaty_id, aggressor);
                }
            }

            HandleAllyActivation(aggressor, aggressor_diplomacy->GetMilitaryAllies());
            HandleAllyActivation(target, target_diplomacy->GetMilitaryAllies());

            // TODO: Implement proper message system for war declarations
            // messages::WarDeclared msg;
            // msg.aggressor = aggressor;
            // msg.defender = target;

            ::core::logging::LogInfo("DiplomacySystem",
                "War declared: " + std::to_string(aggressor) +
                " vs " + std::to_string(target));

            return true;
        }

        bool DiplomacySystem::SueForPeace(types::EntityID supplicant, types::EntityID victor,
            const std::unordered_map<std::string, double>& peace_terms) {
            auto& config = game::config::GameConfig::Instance();
            double base_war_weariness = config.GetDouble("diplomacy.base_war_weariness", 0.1);

            DiplomaticProposal proposal(supplicant, victor, DiplomaticAction::SUE_FOR_PEACE);
            proposal.terms = peace_terms;
            proposal.acceptance_chance = 0.5;

            m_pending_proposals.push_back(proposal);

            core::logging::LogInfo("DiplomacySystem",
                "Peace proposal from " + std::to_string(supplicant) +
                " to " + std::to_string(victor));

            return true;
        }

        bool DiplomacySystem::ArrangeMarriage(types::EntityID bride_realm, types::EntityID groom_realm, bool create_alliance) {
            auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("ArrangeMarriage");
            auto* bride_diplomacy = diplomacy_write.GetComponent(bride_realm);
            auto* groom_diplomacy = diplomacy_write.GetComponent(groom_realm);

            if (!bride_diplomacy || !groom_diplomacy) {
                return false;
            }

            if (!utils::IsValidMarriageCandidate(bride_realm, groom_realm)) {
                return false;
            }

            DynasticMarriage marriage(bride_realm, groom_realm);
            marriage.produces_alliance = create_alliance;
            marriage.diplomatic_bonus = utils::CalculateMarriageValue(bride_realm, groom_realm);

            bride_diplomacy->royal_marriages.push_back(marriage);
            groom_diplomacy->royal_marriages.push_back(marriage);

            ProcessMarriageEffects(marriage);

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

        void DiplomacySystem::ProcessMarriageEffects(const DynasticMarriage& marriage) {
            auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("ProcessMarriageEffects");
            auto* bride_diplomacy = diplomacy_write.GetComponent(marriage.bride_realm);
            auto* groom_diplomacy = diplomacy_write.GetComponent(marriage.groom_realm);

            if (bride_diplomacy && groom_diplomacy) {
                bride_diplomacy->ModifyOpinion(marriage.groom_realm, 
                    static_cast<int>(marriage.diplomatic_bonus), "Royal marriage");
                groom_diplomacy->ModifyOpinion(marriage.bride_realm, 
                    static_cast<int>(marriage.diplomatic_bonus), "Royal marriage");

                if (marriage.produces_alliance) {
                    bride_diplomacy->SetRelation(marriage.groom_realm, DiplomaticRelation::ALLIED);
                    groom_diplomacy->SetRelation(marriage.bride_realm, DiplomaticRelation::ALLIED);
                }
            }
        }
  bool DiplomacySystem::EstablishEmbassy(types::EntityID sender, types::EntityID host) {
            auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("EstablishEmbassy");
            auto* sender_diplomacy = diplomacy_write.GetComponent(sender);

            if (!sender_diplomacy) {
                return false;
            }

            if (sender_diplomacy->active_embassies.size() >= static_cast<size_t>(sender_diplomacy->embassy_capacity)) {
                return false;
            }

            auto it = std::find(sender_diplomacy->active_embassies.begin(), 
                               sender_diplomacy->active_embassies.end(), host);
            if (it != sender_diplomacy->active_embassies.end()) {
                return false;
            }

            sender_diplomacy->active_embassies.push_back(host);
            sender_diplomacy->ModifyOpinion(host, 5, "Embassy established");

            core::logging::LogInfo("DiplomacySystem",
                "Embassy established from " + std::to_string(sender) + " to " + std::to_string(host));

            return true;
        }

        void DiplomacySystem::RecallAmbassador(types::EntityID sender, types::EntityID host) {
            auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("RecallAmbassador");
            auto* sender_diplomacy = diplomacy_write.GetComponent(sender);

            if (!sender_diplomacy) {
                return;
            }

            auto it = std::find(sender_diplomacy->active_embassies.begin(), 
                               sender_diplomacy->active_embassies.end(), host);
            
            if (it != sender_diplomacy->active_embassies.end()) {
                sender_diplomacy->active_embassies.erase(it);
                sender_diplomacy->ModifyOpinion(host, -10, "Ambassador recalled");

                core::logging::LogInfo("DiplomacySystem",
                    "Ambassador recalled from " + std::to_string(host) + " by " + std::to_string(sender));
            }
        }

        void DiplomacySystem::SendDiplomaticGift(types::EntityID sender, types::EntityID recipient, double value) {
            auto& config = game::config::GameConfig::Instance();
            double opinion_per_gift_value = config.GetDouble("diplomacy.opinion_per_gift_value", 0.1);

            auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("SendDiplomaticGift");
            auto* sender_diplomacy = diplomacy_write.GetComponent(sender);

            if (!sender_diplomacy) {
                return;
            }

            int opinion_change = static_cast<int>(value * opinion_per_gift_value);
            sender_diplomacy->ModifyOpinion(recipient, opinion_change, "Diplomatic gift");

            core::logging::LogInfo("DiplomacySystem",
                "Diplomatic gift of " + std::to_string(value) + " sent from " + 
                std::to_string(sender) + " to " + std::to_string(recipient));
        }

        void DiplomacySystem::ProcessTreatyCompliance(types::EntityID realm_id) {
            auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("TreatyCompliance");
            auto* diplomacy = diplomacy_write.GetComponent(realm_id);

            if (!diplomacy) {
                return;
            }

            for (auto& treaty : diplomacy->active_treaties) {
                if (!treaty.is_active) continue;

                UpdateTreatyStatus(treaty);

                if (treaty.IsBroken()) {
                    HandleTreatyViolation(treaty.treaty_id, realm_id);
                }

                if (treaty.IsExpired()) {
                    treaty.is_active = false;
                    LogDiplomaticEvent(treaty.signatory_a, treaty.signatory_b,
                        "Treaty " + treaty.treaty_id + " expired");
                }
            }
        }

        void DiplomacySystem::UpdateTreatyStatus(Treaty& treaty) {
            auto& config = game::config::GameConfig::Instance();
            double compliance_decay_rate = config.GetDouble("diplomacy.compliance_decay_rate", 0.01);

            treaty.compliance_a = std::max(0.0, treaty.compliance_a - compliance_decay_rate);
            treaty.compliance_b = std::max(0.0, treaty.compliance_b - compliance_decay_rate);
        }

        void DiplomacySystem::HandleTreatyViolation(const std::string& treaty_id, types::EntityID violator) {
            messages::TreatyBroken msg;
            msg.treaty_id = treaty_id;
            msg.violator = violator;
            msg.violation_type = "Treaty compliance below threshold";

            auto& config = game::config::GameConfig::Instance();
            msg.diplomatic_damage = config.GetDouble("diplomacy.treaty_violation_penalty", 30.0);

            m_message_bus.SendMessage(core::ecs::Message::Create(msg));

            core::logging::LogInfo("DiplomacySystem", "Treaty violation: " + treaty_id);
        }

        void DiplomacySystem::UpdateDiplomaticRelationships(types::EntityID realm_id) {
            auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("UpdateRelationships");
            auto* diplomacy = diplomacy_write.GetComponent(realm_id);

            if (!diplomacy) {
                return;
            }

            auto& config = game::config::GameConfig::Instance();
            int friendly_threshold = config.GetInt("diplomacy.friendly_threshold", 80);
            int neutral_threshold = config.GetInt("diplomacy.neutral_threshold", 20);
            int hostile_threshold = config.GetInt("diplomacy.hostile_threshold", -50);

            for (auto& [other_realm, relationship] : diplomacy->relationships) {
                DiplomaticRelation old_relation = relationship.relation;

                if (relationship.opinion >= friendly_threshold && old_relation != DiplomaticRelation::ALLIED) {
                    relationship.relation = DiplomaticRelation::FRIENDLY;
                }
                else if (relationship.opinion >= neutral_threshold) {
                    relationship.relation = DiplomaticRelation::NEUTRAL;
                }
                else if (relationship.opinion >= hostile_threshold) {
                    relationship.relation = DiplomaticRelation::UNFRIENDLY;
                }
                else if (relationship.opinion < hostile_threshold && old_relation != DiplomaticRelation::AT_WAR) {
                    relationship.relation = DiplomaticRelation::HOSTILE;
                }

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
                double decay_amount = utils::CalculateOpinionDecay(relationship.opinion, time_delta);

                if (relationship.opinion > 0) {
                    relationship.opinion = std::max(0.0, relationship.opinion - decay_amount);
                }
                else if (relationship.opinion < 0) {
                    relationship.opinion = std::min(0.0, relationship.opinion + decay_amount);
                }

                auto time_since_contact = std::chrono::system_clock::now() - relationship.last_contact;
                auto days_since_contact = std::chrono::duration_cast<std::chrono::days>(time_since_contact).count();

                auto& config = game::config::GameConfig::Instance();
                int trust_decay_days = config.GetInt("diplomacy.trust_decay_days_threshold", 365);
                double trust_decay_rate = config.GetDouble("diplomacy.trust_decay_rate", 0.99);

                if (days_since_contact > trust_decay_days) {
                    relationship.trust *= trust_decay_rate;
                }
            }
        }

        void DiplomacySystem::CalculatePres
 void DiplomacySystem::CalculatePrestigeEffects(types::EntityID realm_id) {
            auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("CalculatePrestige");
            auto* diplomacy = diplomacy_write.GetComponent(realm_id);

            if (!diplomacy) {
                return;
            }

            auto& config = game::config::GameConfig::Instance();
            double prestige_from_allies = config.GetDouble("diplomacy.prestige_per_ally", 2.0);
            double prestige_decay_rate = config.GetDouble("diplomacy.prestige_decay_rate", 0.1);

            double ally_prestige = diplomacy->allies.size() * prestige_from_allies;
            diplomacy->prestige = std::max(0.0, diplomacy->prestige - prestige_decay_rate + ally_prestige);
        }

        void DiplomacySystem::ProcessAIDiplomacy(types::EntityID realm_id) {
            auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("AIDiplomacy");
            auto* diplomacy = diplomacy_read.GetComponent(realm_id);

            if (!diplomacy) {
                return;
            }

            GenerateAIDiplomaticActions(realm_id);

            for (auto& proposal : m_pending_proposals) {
                if (proposal.target == realm_id && proposal.is_pending) {
                    double acceptance_chance = EvaluateProposal(proposal);

                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_real_distribution<double> dist(0.0, 1.0);

                    if (dist(gen) < acceptance_chance) {
                        switch (proposal.action_type) {
                        case DiplomaticAction::PROPOSE_ALLIANCE:
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

            switch (diplomacy->personality) {
            case DiplomaticPersonality::AGGRESSIVE:
            {
                for (auto neighbor : GetNeighboringRealms(realm_id)) {
                    if (!diplomacy->IsAtWarWith(neighbor) &&
                        GetMilitaryStrengthRatio(realm_id, neighbor) > 1.5) {

                        CasusBelli cb = FindBestCasusBelli(realm_id, neighbor);
                        if (cb != CasusBelli::NONE) {
                            DeclareWar(realm_id, neighbor, cb);
                            break;
                        }
                    }
                }
            }
            break;

            case DiplomaticPersonality::DIPLOMATIC:
            {
                for (auto neighbor : GetNeighboringRealms(realm_id)) {
                    if (!diplomacy->IsAlliedWith(neighbor) &&
                        GetOpinion(realm_id, neighbor) > 40) {

                        std::unordered_map<std::string, double> terms;
                        terms["mutual_defense"] = 1.0;
                        ProposeAlliance(realm_id, neighbor, terms);
                        break;
                    }
                }
            }
            break;

            case DiplomaticPersonality::MERCHANT:
            {
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
            }
            break;

            default:
                break;
            }
        }

        void DiplomacySystem::ProcessWarDeclaration(types::EntityID aggressor, types::EntityID defender, CasusBelli cb) {
            DeclareWar(aggressor, defender, cb);
        }

        void DiplomacySystem::HandleAllyActivation(types::EntityID war_leader, const std::vector<types::EntityID>& allies) {
            auto& config = game::config::GameConfig::Instance();
            double ally_join_probability = config.GetDouble("diplomacy.ally_join_war_probability", 0.8);

            for (auto ally : allies) {
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<double> dist(0.0, 1.0);

                if (dist(gen) < ally_join_probability) {
                    core::logging::LogInfo("DiplomacySystem",
                        "Ally " + std::to_string(ally) + " joins " + std::to_string(war_leader) + " in war");
                }
            }
        }

        void DiplomacySystem::ProcessPeaceNegotiation(types::EntityID realm_a, types::EntityID realm_b) {
            auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("PeaceNegotiation");
            auto* diplomacy_a = diplomacy_write.GetComponent(realm_a);
            auto* diplomacy_b = diplomacy_write.GetComponent(realm_b);

            if (!diplomacy_a || !diplomacy_b) {
                return;
            }

            if (diplomacy_a->IsAtWarWith(realm_b)) {
                diplomacy_a->SetRelation(realm_b, DiplomaticRelation::NEUTRAL);
                diplomacy_b->SetRelation(realm_a, DiplomaticRelation::NEUTRAL);

                core::logging::LogInfo("DiplomacySystem",
                    "Peace established between " + std::to_string(realm_a) + " and " + std::to_string(realm_b));
            }
        }

        void DiplomacySystem::UpdateTradeRelations(types::EntityID realm_id) {
            auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("UpdateTradeRelations");
            auto* diplomacy = diplomacy_read.GetComponent(realm_id);

            if (!diplomacy) {
                return;
            }

            for (auto& [other_realm, relationship] : diplomacy->relationships) {
                relationship.trade_volume = CalculateTradeValue(realm_id, other_realm);
            }
        }

        double DiplomacySystem::CalculateTradeValue(types::EntityID realm_a, types::EntityID realm_b) {
            auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("CalculateTradeValue");
            auto* diplomacy_a = diplomacy_read.GetComponent(realm_a);

            if (!diplomacy_a) {
                return 0.0;
            }

            auto* relationship = diplomacy_a->GetRelationship(realm_b);
            if (!relationship) {
                return 0.0;
            }

            auto& config = game::config::GameConfig::Instance();
            double base_trade = config.GetDouble("diplomacy.base_trade_value", 50.0);
            double opinion_modifier = relationship->opinion / 100.0;

            return base_trade * (1.0 + opinion_modifier);
        }

        void DiplomacySystem::ProcessTradeDisputes(types::EntityID realm_id) {
            // Placeholder for trade dispute processing
        }

        void DiplomacySystem::ProcessDiplomaticIntelligence(types::EntityID realm_id) {
            // Placeholder for intelligence gathering
        }

        void DiplomacySystem::UpdateForeignRelationsKnowledge(types::EntityID realm_id) {
            // Placeholder for updating knowledge of foreign relations
        }

        std::vector<types::EntityID> DiplomacySystem::GetAllRealms() const {
            std::vector<types::EntityID> realms;

            for (types::EntityID id = 3000; id < 3010; ++id) {
                realms.push_back(id);
            }

            return realms;
        }

        std::vector<types::EntityID> DiplomacySystem::GetNeighboringRealms(types::EntityID realm_id) const {
            return GetBorderingRealms(realm_id);
        }

        std::vector<types::EntityID> DiplomacySystem::GetPotentialAllies(types::EntityID realm_id) const {
            std::vector<types::EntityID> potential_allies;
            auto all_realms = GetAllRealms();

            for (auto other : all_realms) {
                if (other != realm_id && GetOpinion(realm_id, other) > 20) {
                    potential_allies.push_back(other);
                }
            }

            return potential_allies;
        }

        std::vector<types::EntityID> DiplomacySystem::GetPotentialEnemies(types::EntityID realm_id) const {
            std::vector<types::EntityID> potential_enemies;
            auto all_realms = GetAllRealms();

            for (auto other : all_realms) {
                if (other != realm_id && GetOpinion(realm_id, other) < -20) {
                    potential_enemies.push_back(other);
                }
            }

            return potential_enemies;
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

        double DiplomacySystem::GetPrestige(types::EntityID realm_id) const {
            auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("GetPrestige");
            auto* diplomacy = diplomacy_read.GetComponent(realm_id);

            return diplomacy ? diplomacy->prestige : 0.0;
        }

        bool DiplomacySystem::AreAtWar(types::EntityID realm_a, types::EntityID realm_b) const {
            auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("AreAtWar");
            auto* diplomacy = diplomacy_read.GetComponent(realm_a);

            if (!diplomacy) {
                return false;
            }

            return diplomacy->IsAtWarWith(realm_b);
        }

        void DiplomacySystem::SetDiplomaticPersonality(types::EntityID realm_id, DiplomaticPersonality personality) {
            auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("SetPersonality");
            auto* diplomacy = diplomacy_write.GetComponent(realm_id);

            if (diplomacy) {
                diplomacy->personality = personality;
            }
        }

        void DiplomacySystem::SetWarWeariness(double base_war_weariness) {
            m_base_war_weariness = base_war_weariness;
        }

        void DiplomacySystem::SetDiplomaticSpeed(double speed_multiplier) {
            m_diplomatic_speed = speed_multiplier;
        }

        DiplomacyComponent* DiplomacySystem::GetDiplomacyComponent(types::EntityID realm_id) {
            auto diplomacy_write = m_access_manager.GetWriteAccess<DiplomacyComponent>("GetDiplomacyComponent");
            return diplomacy_write.GetComponent(realm_id);
        }

        const DiplomacyComponent* DiplomacySystem::GetDiplomacyComponent(types::EntityID realm_id) const {
            auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("GetDiplomacyComponent");
            return diplomacy_read.GetComponent(realm_id);
        }

        void DiplomacySystem::InitializeDiplomaticPersonalities() {
            core::logging::LogInfo("DiplomacySystem", "Initialized diplomatic personalities");
        }

        void DiplomacySystem::SubscribeToEvents() {
            // Subscribe to relevant events
        }

        double DiplomacySystem::CalculateBaseOpinion(types::EntityID realm_a, types::EntityID realm_b) const {
            return 0.0;
        }

        double DiplomacySystem::CalculateAllianceValue(types::EntityID realm_a, types::EntityID realm_b) const {
            return GetOpinion(realm_a, realm_b) / 100.0;
        }

        double DiplomacySystem::CalculateWarScore(types::EntityID realm_a, types::EntityID realm_b) const {
            return 0.0;
        }

        CasusBelli DiplomacySystem::FindBestCasusBelli(types::EntityID aggressor, types::EntityID target) const {
            auto diplomacy_read = m_access_manager.GetReadAccess<DiplomacyComponent>("FindCasusBelli");
            auto* diplomacy = diplomacy_read.GetComponent(aggressor);

            if (!diplomacy) {
                return CasusBelli::NONE;
            }

            if (!diplomacy->valid_war_goals.empty()) {
                return diplomacy->valid_war_goals[0];
            }

            return CasusBelli::BORDER_DISPUTE;
        }

        double DiplomacySystem::EvaluateProposal(const DiplomaticProposal& proposal) {
            switch (proposal.action_type) {
            case DiplomaticAction::PROPOSE_ALLIANCE:
                return EvaluateAllianceProposal(proposal);
            case DiplomaticAction::PROPOSE_TRADE:
                return EvaluateTradeProposal(proposal);
            default:
                return 0.5;
            }
        }

        double DiplomacySystem::EvaluateAllianceProposal(const DiplomaticProposal& proposal) const {
            auto proposer_diplomacy = GetDiplomacyComponent(proposal.proposer);
            auto target_diplomacy = GetDiplomacyComponent(proposal.target);

            if (!proposer_diplomacy || !target_diplomacy) {
                return 0.0;
            }

            double evaluation = 0.5;

            int opinion = GetOpinion(proposal.target, proposal.proposer);
            evaluation += opinion / 200.0;

            double strength_ratio = GetMilitaryStrengthRatio(proposal.proposer, proposal.target);
            if (strength_ratio > 1.0) {
                evaluation += 0.2;
            }

            auto proposer_enemies = proposer_diplomacy->GetWarEnemies();
            auto target_enemies = target_diplomacy->GetWarEnemies();

            for (auto enemy : proposer_enemies) {
                if (std::find(target_enemies.begin(), target_enemies.end(), enemy) != target_enemies.end()) {
                    evaluation += 0.3;
                }
            }

            return std::clamp(evaluation, 0.0, 1.0);
        }

        double DiplomacySystem::EvaluateTradeProposal(const DiplomaticProposal& proposal) const {
            double trade_value = CalculateTradeValue(proposal.proposer, proposal.target);
            return std::clamp(trade_value / 100.0, 0.0, 1.0);
        }

        double DiplomacySystem::EvaluateMarriageProposal(const DiplomaticProposal& proposal) const {
            return 0.5;
        }

        void DiplomacySystem::ApplyPersonalityToOpinion(types::EntityID realm_id, DiplomaticState& relationship) const {
            // Placeholder
        }

        double DiplomacySystem::GetPersonalityWarLikelihood(DiplomaticPersonality personality) const {
            switch (personality) {
            case DiplomaticPersonality::AGGRESSIVE: return 0.8;
            case DiplomaticPersonality::DIPLOMATIC: return 0.2;
            default: return 0.5;
            }
        }

        double DiplomacySystem::GetPersonalityTradePreference(DiplomaticPersonality personality) const {
            switch (personality) {
            case DiplomaticPersonality::MERCHANT: return 1.0;
            default: return 0.5;
            }
        }

        std::vector<types::EntityID> DiplomacySystem::GetBorderingRealms(types::EntityID realm_id) const {
            std::vector<types::EntityID> neighbors;
            neighbors.push_back(realm_id + 1);
            neighbors.push_back(realm_id - 1);
            return neighbors;
        }

        double DiplomacySystem::GetMilitaryStrengthRatio(types::EntityID realm_a, types::EntityID realm_b) const {
            return 1.0;
        }

        double DiplomacySystem::GetEconomicInterdependence(types::EntityID realm_a, types::EntityID realm_b) const {
            return 0.0;
        }

        void DiplomacySystem::OnWarEnded(const core::ecs::Message& message) {
            // Placeholder
        }

        void DiplomacySystem::OnTradeRouteEstablished(const core::ecs::Message& message) {
            // Placeholder
        }

        void DiplomacySystem::OnTechnologyDiscovered(const core::ecs::Message& message) {
            // Placeholder
        }

        void DiplomacySystem::OnReligiousConversion(const core::ecs::Message& message) {
            // Placeholder
        }

        void DiplomacySystem::LogDiplomaticEvent(types::EntityID realm_a, types::EntityID realm_b, const std::string& event) {
            core::logging::LogInfo("DiplomacySystem",
                "Diplomatic event between " + std::to_string(realm_a) +
                " and " + std::to_string(realm_b) + ": " + event);
        }

        void DiplomacySystem::ValidateDiplomaticState(types::EntityID realm_id) {
            // Placeholder
        }

        std::string DiplomacySystem::GenerateProposalId(types::EntityID proposer, types::EntityID target, DiplomaticAction action) {
            return utils::DiplomaticActionToString(action) + "_" + std::to_string(proposer) + "_" + std::to_string(target);
        }

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

            std::string DiplomaticPersonalityToString(DiplomaticPersonality personality) {
                switch (personality) {
                case DiplomaticPersonality::AGGRESSIVE: return "Aggressive";
                case DiplomaticPersonality::DIPLOMATIC: return "Diplomatic";
                case DiplomaticPersonality::ISOLATIONIST: return "Isolationist";
                case DiplomaticPersonality::OPPORTUNISTIC: return "Opportunistic";
                case DiplomaticPersonality::HONORABLE: return "Honorable";
                case DiplomaticPersonality::TREACHEROUS: return "Treacherous";
                case DiplomaticPersonality::MERCHANT: return "Merchant";
                case DiplomaticPersonality::RELIGIOUS: return "Religious";
                default: return "Unknown";
                }
            }

            double CalculateOpinionDecay(double current_opinion, float time_delta) {
                double decay_rate = 0.1 * time_delta;
                return std::abs(current_opinion) * decay_rate;
            }

            double CalculatePrestigeFromWar(bool victory, double enemy_prestige) {
                return victory ? enemy_prestige * 0.1 : -enemy_prestige * 0.05;
            }

            double CalculateDiplomaticDistance(types::EntityID realm_a, types::EntityID realm_b) {
                return 100.0;
            }

            bool IsOffensiveTreaty(TreatyType type) {
                return type == TreatyType::ALLIANCE;
            }

            bool IsEconomicTreaty(TreatyType type) {
                return type == TreatyType::TRADE_AGREEMENT;
            }

            bool RequiresMutualConsent(TreatyType type) {
                return type != TreatyType::TRIBUTE;
            }

            int GetTreatyDuration(TreatyType type) {
                switch (type) {
                case TreatyType::NON_AGGRESSION: return 5;
                case TreatyType::TRADE_AGREEMENT: return 20;
                case TreatyType::ALLIANCE: return 25;
                case TreatyType::MARRIAGE_PACT: return 50;
                default: return 10;
                }
            }

            bool AreNaturalAllies(types::EntityID realm_a, types::EntityID realm_b) {
                return false;
            }

            bool AreNaturalEnemies(types::EntityID realm_a, types::EntityID realm_b) {
                return false;
            }

            bool HaveSharedInterests(types::EntityID realm_a, types::EntityID realm_b) {
                return false;
            }

            bool IsValidCasusBelli(types::EntityID aggressor, types::EntityID target, CasusBelli cb) {
                return cb != CasusBelli::NONE;
            }

            double GetWarSupport(types::EntityID realm_id, CasusBelli cb) {
                return 0.5;
            }

            double GetWarWeariness(types::EntityID realm_id, int war_duration_months) {
                return war_duration_months * 0.01;
            }

            bool IsValidMarriageCandidate(types::EntityID bride_realm, types::EntityID groom_realm) {
                return bride_realm != groom_realm;
            }

            double CalculateMarriageValue(types::EntityID realm_a, types::EntityID realm_b) {
                return 25.0;
            }

            bool CreatesSuccessionClaim(const DynasticMarriage& marriage) {
                return marriage.inheritance_claim > 0.0;
            }
        }

    } // namespace diplomacy
} // namespace game
