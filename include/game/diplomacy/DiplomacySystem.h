// ============================================================================
// Date/Time Created: September 27, 2025 - 3:45 PM PST
// Intended Folder Location: include/game/diplomacy/DiplomacySystem.h
// DiplomacySystem Header - FIXED: Added ISerializable and GetThreadingStrategy
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "core/ECS/ISystem.h"
#include "core/threading/ThreadedSystemManager.h"
#include "core/threading/ThreadingTypes.h"
#include "core/types/game_types.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <chrono>

// Forward declarations
namespace Json {
    class Value;
}

namespace game {
    namespace diplomacy {

        // ============================================================================
        // Note: Data structures moved to DiplomacyComponents.h
        // ============================================================================

        // Note: Message structures moved to DiplomacyComponents.h

        // ============================================================================
        // Diplomacy System - FIXED: Added ISerializable and GetThreadingStrategy
        // ============================================================================

        class DiplomacySystem : public game::core::ISystem {
        public:
            explicit DiplomacySystem(::core::ecs::ComponentAccessManager& access_manager,
                                    ::core::ecs::MessageBus& message_bus);
            virtual ~DiplomacySystem() = default;

            // ISystem interface
            void Initialize() override;
            void Update(float delta_time) override;
            void Shutdown() override;

            // Threading configuration
            ::core::threading::ThreadingStrategy GetThreadingStrategy() const override;

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
            ::core::ecs::ComponentAccessManager& m_access_manager;
            ::core::ecs::MessageBus& m_message_bus;

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

            // Helper methods for the minimal implementation
            void CreateDiplomacyComponent(types::EntityID realm_id);
            void ProcessDiplomaticUpdates();
            void ProcessMonthlyDiplomacy();
            void ProcessPendingProposals();
            void EstablishAlliance(types::EntityID realm_a, types::EntityID realm_b);

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

            // Event Handlers (TODO: Implement proper message handling)

            void LogDiplomaticEvent(types::EntityID realm_a, types::EntityID realm_b, const std::string& event);
            void ValidateDiplomaticState(types::EntityID realm_id);
            std::string GenerateProposalId(types::EntityID proposer, types::EntityID target, DiplomaticAction action);

            // ISerializable interface
            Json::Value Serialize(int version) const override;
            bool Deserialize(const Json::Value& data, int version) override;
            std::string GetSystemName() const override;
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
