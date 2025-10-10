// ============================================================================
// Date/Time Created: September 27, 2025 - 3:45 PM PST
// Intended Folder Location: include/game/diplomacy/DiplomacySystem.h
// DiplomacySystem Header - FIXED: Added ISerializable and GetThreadingStrategy
// ============================================================================

#pragma once

#include "core/ECS/IComponent.h"
#include "core/ECS/ISystem.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "core/ECS/ISerializable.h"
#include "core/threading/ThreadingStrategy.h"
#include "core/types.h"
#include "population/comprehensive_population_system.h"
#include "../province/enhanced_province_system.h"
#include "../military/military_system.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

namespace game {
    namespace diplomacy {

        // ============================================================================
        // Diplomacy Type Definitions
        // ============================================================================

        enum class DiplomaticRelation {
            ALLIED = 0,
            FRIENDLY,
            NEUTRAL,
            UNFRIENDLY,
            HOSTILE,
            AT_WAR,
            COUNT
        };

        enum class TreatyType {
            ALLIANCE = 0,
            TRADE_AGREEMENT,
            NON_AGGRESSION,
            MARRIAGE_PACT,
            TRIBUTE,
            BORDER_AGREEMENT,
            MILITARY_ACCESS,
            DEFENSIVE_LEAGUE,
            COUNT
        };

        enum class DiplomaticAction {
            PROPOSE_ALLIANCE = 0,
            PROPOSE_TRADE,
            DECLARE_WAR,
            SUE_FOR_PEACE,
            SEND_GIFT,
            DEMAND_TRIBUTE,
            ARRANGE_MARRIAGE,
            ESTABLISH_EMBASSY,
            RECALL_AMBASSADOR,
            ISSUE_ULTIMATUM,
            MEDIATE_CONFLICT,
            GUARANTEE_INDEPENDENCE,
            COUNT
        };

        enum class CasusBelli {
            NONE = 0,
            BORDER_DISPUTE,
            TRADE_INTERFERENCE,
            DYNASTIC_CLAIM,
            RELIGIOUS_CONFLICT,
            INSULT_TO_HONOR,
            BROKEN_TREATY,
            PROTECTION_OF_ALLY,
            LIBERATION_WAR,
            COUNT
        };

        enum class DiplomaticPersonality {
            AGGRESSIVE = 0,
            DIPLOMATIC,
            ISOLATIONIST,
            OPPORTUNISTIC,
            HONORABLE,
            TREACHEROUS,
            MERCHANT,
            RELIGIOUS,
            COUNT
        };

        // ============================================================================
        // Diplomatic Relationship Data
        // ============================================================================

        struct DiplomaticState {
            types::EntityID other_realm;
            DiplomaticRelation relation = DiplomaticRelation::NEUTRAL;

            int opinion = 0;
            double trust = 0.5;
            double prestige_difference = 0.0;

            std::vector<std::string> recent_actions;
            std::chrono::system_clock::time_point last_contact;
            int diplomatic_incidents = 0;

            double trade_volume = 0.0;
            double economic_dependency = 0.0;

            bool military_access = false;
            bool has_common_enemies = false;
            bool has_border_tensions = false;

            DiplomaticState() = default;
            explicit DiplomaticState(types::EntityID realm);
        };

        struct Treaty {
            std::string treaty_id;
            TreatyType type;
            types::EntityID signatory_a;
            types::EntityID signatory_b;

            std::unordered_map<std::string, double> terms;
            std::vector<std::string> conditions;

            std::chrono::system_clock::time_point signed_date;
            std::chrono::system_clock::time_point expiry_date;
            bool is_active = true;
            double compliance_a = 1.0;
            double compliance_b = 1.0;

            double tribute_amount = 0.0;
            double trade_bonus = 0.0;

            Treaty() = default;
            Treaty(TreatyType treaty_type, types::EntityID realm_a, types::EntityID realm_b);

            bool IsExpired() const;
            bool IsBroken() const;
            double GetOverallCompliance() const;
        };

        struct DynasticMarriage {
            std::string marriage_id;
            types::EntityID bride_realm;
            types::EntityID groom_realm;
            types::EntityID bride_character;
            types::EntityID groom_character;

            double diplomatic_bonus = 20.0;
            double inheritance_claim = 0.0;
            bool produces_alliance = false;

            std::chrono::system_clock::time_point marriage_date;
            bool is_active = true;
            std::vector<types::EntityID> children;

            DynasticMarriage() = default;
            DynasticMarriage(types::EntityID bride, types::EntityID groom);
        };

        // ============================================================================
        // Diplomacy Components
        // ============================================================================

        struct DiplomacyComponent : public core::ecs::IComponent<DiplomacyComponent> {
            std::unordered_map<types::EntityID, DiplomaticState> relationships;

            std::vector<Treaty> active_treaties;
            std::vector<DynasticMarriage> royal_marriages;

            DiplomaticPersonality personality = DiplomaticPersonality::DIPLOMATIC;
            double diplomatic_reputation = 0.5;
            double prestige = 50.0;

            double diplomatic_power = 100.0;
            int embassy_capacity = 3;
            std::vector<types::EntityID> active_embassies;

            std::vector<types::EntityID> enemies;
            std::vector<types::EntityID> allies;
            std::vector<CasusBelli> valid_war_goals;

            std::unordered_map<types::EntityID, double> favor_owed;
            std::unordered_map<types::EntityID, int> broken_promises;

            DiplomacyComponent() = default;

            DiplomaticState* GetRelationship(types::EntityID other_realm);
            const DiplomaticState* GetRelationship(types::EntityID other_realm) const;
            void SetRelation(types::EntityID other_realm, DiplomaticRelation relation);
            void ModifyOpinion(types::EntityID other_realm, int opinion_change, const std::string& reason);

            void AddTreaty(const Treaty& treaty);
            void RemoveTreaty(const std::string& treaty_id);
            std::vector<Treaty*> GetTreatiesWith(types::EntityID other_realm);
            bool HasTreatyType(types::EntityID other_realm, TreatyType type) const;

            bool IsAtWar() const;
            bool IsAtWarWith(types::EntityID other_realm) const;
            bool IsAlliedWith(types::EntityID other_realm) const;
            std::vector<types::EntityID> GetWarEnemies() const;
            std::vector<types::EntityID> GetMilitaryAllies() const;
        };

        struct DiplomaticProposal {
            std::string proposal_id;
            types::EntityID proposer;
            types::EntityID target;
            DiplomaticAction action_type;

            std::unordered_map<std::string, double> terms;
            std::vector<std::string> conditions;
            std::string message;

            std::chrono::system_clock::time_point proposed_date;
            std::chrono::system_clock::time_point expiry_date;
            bool is_pending = true;

            double ai_evaluation = 0.0;
            double acceptance_chance = 0.0;

            DiplomaticProposal() = default;
            DiplomaticProposal(types::EntityID from, types::EntityID to, DiplomaticAction action);
        };

        // ============================================================================
        // Diplomacy Events and Messages
        // ============================================================================

        namespace messages {

            struct DiplomaticRelationChanged {
                types::EntityID realm_a;
                types::EntityID realm_b;
                DiplomaticRelation old_relation;
                DiplomaticRelation new_relation;
                std::string reason;
            };

            struct TreatySigned {
                Treaty treaty;
                std::string negotiation_summary;
                double realm_a_satisfaction;
                double realm_b_satisfaction;
            };

            struct TreatyBroken {
                std::string treaty_id;
                types::EntityID violator;
                types::EntityID victim;
                std::string violation_type;
                double diplomatic_damage;
            };

            struct WarDeclared {
                types::EntityID aggressor;
                types::EntityID defender;
                CasusBelli casus_belli;
                std::string war_name;
                std::vector<types::EntityID> aggressor_allies;
                std::vector<types::EntityID> defender_allies;
            };

            struct DiplomaticIncident {
                types::EntityID involved_realm_a;
                types::EntityID involved_realm_b;
                std::string incident_type;
                int opinion_impact;
                double prestige_impact;
                std::string description;
            };

            struct MarriageArranged {
                DynasticMarriage marriage;
                double diplomatic_impact;
                std::string marriage_story;
                bool creates_alliance;
            };

            struct DiplomaticMission {
                types::EntityID sender;
                types::EntityID recipient;
                std::string mission_type;
                bool mission_success;
                std::string mission_result;
                double diplomatic_cost;
            };

        } // namespace messages

        // ============================================================================
        // Diplomacy System - FIXED: Added ISerializable and GetThreadingStrategy
        // ============================================================================

        class DiplomacySystem : public core::ecs::ISystem, 
                                public core::save::ISerializable {
        public:
            DiplomacySystem(core::ecs::ComponentAccessManager& access_manager,
                core::ecs::MessageBus& message_bus);
            virtual ~DiplomacySystem() = default;

            // ISystem interface
            void Initialize() override;
            void Update(float delta_time) override;
            void Shutdown() override;

            // FIXED: Added GetThreadingStrategy
            core::threading::ThreadingStrategy GetThreadingStrategy() const;

            // FIXED: ISerializable interface
            Json::Value Serialize(int version) const override;
            bool Deserialize(const Json::Value& data, int version) override;
            std::string GetSystemName() const override { return "DiplomacySystem"; }

            // Diplomatic actions
            bool ProposeAlliance(types::EntityID proposer, types::EntityID target,
                const std::unordered_map<std::string, double>& terms);
            bool ProposeTradeAgreement(types::EntityID proposer, types::EntityID target,
                double trade_bonus, int duration_years);
            bool DeclareWar(types::EntityID aggressor, types::EntityID target, CasusBelli casus_belli);
            bool SueForPeace(types::EntityID supplicant, types::EntityID victor,
                const std::unordered_map<std::string, double>& peace_terms);

            // Marriage diplomacy
            bool ArrangeMarriage(types::EntityID bride_realm, types::EntityID groom_realm,
                bool create_alliance = false);
            void ProcessMarriageEffects(const DynasticMarriage& marriage);

            // Embassy and communication
            bool EstablishEmbassy(types::EntityID sender, types::EntityID host);
            void RecallAmbassador(types::EntityID sender, types::EntityID host);
            void SendDiplomaticGift(types::EntityID sender, types::EntityID recipient, double value);

            // Treaty management
            void ProcessTreatyCompliance(types::EntityID realm_id);
            void UpdateTreatyStatus(Treaty& treaty);
            void HandleTreatyViolation(const std::string& treaty_id, types::EntityID violator);

            // Relationship dynamics
            void UpdateDiplomaticRelationships(types::EntityID realm_id);
            void ProcessDiplomaticDecay(types::EntityID realm_id, float time_delta);
            void CalculatePrestigeEffects(types::EntityID realm_id);

            // AI diplomacy
            void ProcessAIDiplomacy(types::EntityID realm_id);
            double EvaluateProposal(const DiplomaticProposal& proposal);
            void GenerateAIDiplomaticActions(types::EntityID realm_id);

            // War and peace
            void ProcessWarDeclaration(types::EntityID aggressor, types::EntityID defender, CasusBelli cb);
            void HandleAllyActivation(types::EntityID war_leader, const std::vector<types::EntityID>& allies);
            void ProcessPeaceNegotiation(types::EntityID realm_a, types::EntityID realm_b);

            // Trade integration
            void UpdateTradeRelations(types::EntityID realm_id);
            double CalculateTradeValue(types::EntityID realm_a, types::EntityID realm_b);
            void ProcessTradeDisputes(types::EntityID realm_id);

            // Information and espionage
            void ProcessDiplomaticIntelligence(types::EntityID realm_id);
            void UpdateForeignRelationsKnowledge(types::EntityID realm_id);

            // Query methods
            std::vector<types::EntityID> GetAllRealms() const;
            std::vector<types::EntityID> GetNeighboringRealms(types::EntityID realm_id) const;
            std::vector<types::EntityID> GetPotentialAllies(types::EntityID realm_id) const;
            std::vector<types::EntityID> GetPotentialEnemies(types::EntityID realm_id) const;

            // Relationship queries
            DiplomaticRelation GetRelation(types::EntityID realm_a, types::EntityID realm_b) const;
            int GetOpinion(types::EntityID realm_a, types::EntityID realm_b) const;
            double GetPrestige(types::EntityID realm_id) const;
            bool AreAtWar(types::EntityID realm_a, types::EntityID realm_b) const;

            // Configuration
            void SetDiplomaticPersonality(types::EntityID realm_id, DiplomaticPersonality personality);
            void SetWarWeariness(double base_war_weariness);
            void SetDiplomaticSpeed(double speed_multiplier);

            // Data access
            DiplomacyComponent* GetDiplomacyComponent(types::EntityID realm_id);
            const DiplomacyComponent* GetDiplomacyComponent(types::EntityID realm_id) const;

        private:
            core::ecs::ComponentAccessManager& m_access_manager;
            core::ecs::MessageBus& m_message_bus;

            bool m_initialized = false;
            float m_accumulated_time = 0.0f;
            float m_update_interval = 1.0f;
            float m_monthly_timer = 0.0f;

            double m_base_war_weariness = 0.1;
            double m_diplomatic_speed = 1.0;
            double m_alliance_reliability = 0.8;

            std::vector<DiplomaticProposal> m_pending_proposals;
            std::unordered_map<std::string, std::chrono::system_clock::time_point> m_diplomatic_cooldowns;

            void InitializeDiplomaticPersonalities();
            void SubscribeToEvents();

            double CalculateBaseOpinion(types::EntityID realm_a, types::EntityID realm_b) const;
            double CalculateAllianceValue(types::EntityID realm_a, types::EntityID realm_b) const;
            double CalculateWarScore(types::EntityID realm_a, types::EntityID realm_b) const;
            CasusBelli FindBestCasusBelli(types::EntityID aggressor, types::EntityID target) const;

            double EvaluateAllianceProposal(const DiplomaticProposal& proposal) const;
            double EvaluateTradeProposal(const DiplomaticProposal& proposal) const;
            double EvaluateMarriageProposal(const DiplomaticProposal& proposal) const;

            void ApplyPersonalityToOpinion(types::EntityID realm_id, DiplomaticState& relationship) const;
            double GetPersonalityWarLikelihood(DiplomaticPersonality personality) const;
            double GetPersonalityTradePreference(DiplomaticPersonality personality) const;

            std::vector<types::EntityID> GetBorderingRealms(types::EntityID realm_id) const;
            double GetMilitaryStrengthRatio(types::EntityID realm_a, types::EntityID realm_b) const;
            double GetEconomicInterdependence(types::EntityID realm_a, types::EntityID realm_b) const;

            void OnWarEnded(const core::ecs::Message& message);
            void OnTradeRouteEstablished(const core::ecs::Message& message);
            void OnTechnologyDiscovered(const core::ecs::Message& message);
            void OnReligiousConversion(const core::ecs::Message& message);

            void LogDiplomaticEvent(types::EntityID realm_a, types::EntityID realm_b, const std::string& event);
            void ValidateDiplomaticState(types::EntityID realm_id);
            std::string GenerateProposalId(types::EntityID proposer, types::EntityID target, DiplomaticAction action);
        };

        // ============================================================================
        // Diplomatic Utility Functions
        // ============================================================================

        namespace utils {
            std::string DiplomaticRelationToString(DiplomaticRelation relation);
            std::string TreatyTypeToString(TreatyType type);
            std::string DiplomaticActionToString(DiplomaticAction action);
            std::string CasusBelliToString(CasusBelli cb);
            std::string DiplomaticPersonalityToString(DiplomaticPersonality personality);

            double CalculateOpinionDecay(double current_opinion, float time_delta);
            double CalculatePrestigeFromWar(bool victory, double enemy_prestige);
            double CalculateDiplomaticDistance(types::EntityID realm_a, types::EntityID realm_b);

            bool IsOffensiveTreaty(TreatyType type);
            bool IsEconomicTreaty(TreatyType type);
            bool RequiresMutualConsent(TreatyType type);
            int GetTreatyDuration(TreatyType type);

            bool AreNaturalAllies(types::EntityID realm_a, types::EntityID realm_b);
            bool AreNaturalEnemies(types::EntityID realm_a, types::EntityID realm_b);
            bool HaveSharedInterests(types::EntityID realm_a, types::EntityID realm_b);

            bool IsValidCasusBelli(types::EntityID aggressor, types::EntityID target, CasusBelli cb);
            double GetWarSupport(types::EntityID realm_id, CasusBelli cb);
            double GetWarWeariness(types::EntityID realm_id, int war_duration_months);

            bool IsValidMarriageCandidate(types::EntityID bride_realm, types::EntityID groom_realm);
            double CalculateMarriageValue(types::EntityID realm_a, types::EntityID realm_b);
            bool CreatesSuccessionClaim(const DynasticMarriage& marriage);
        }

    } // namespace diplomacy
} // namespace game
