#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <unordered_map>
#include <vector>
#include <deque>
#include <string>
#include <chrono>
#include <chrono>

namespace game::diplomacy {

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
    // Diplomatic Data Structures
    // ============================================================================

    struct DiplomaticState {
        game::types::EntityID other_realm;
        DiplomaticRelation relation = DiplomaticRelation::NEUTRAL;

        int opinion = 0;
        double trust = 0.5;
        double prestige_difference = 0.0;

        std::deque<std::string> recent_actions; // Changed from vector to deque for O(1) front removal
        std::chrono::system_clock::time_point last_contact;
        int diplomatic_incidents = 0;

        double trade_volume = 0.0;
        double economic_dependency = 0.0;

        bool military_access = false;
        bool has_common_enemies = false;
        bool has_border_tensions = false;

        // Cooldown tracking to prevent action spam
        std::unordered_map<DiplomaticAction, std::chrono::system_clock::time_point> action_cooldowns;
        std::chrono::system_clock::time_point last_major_action;

        // Long-term diplomatic memory - rolling average of historical opinions
        std::deque<int> opinion_history;  // Stores recent opinion values for rolling average
        double historical_opinion_average = 0.0;  // Rolling average of past opinions

        // ENHANCED MEMORY SYSTEM

        // Weighted opinion calculation
        struct OpinionModifier {
            std::string source;              // What caused this modifier
            int value;                       // Opinion impact
            double weight = 1.0;             // Current weight (decays)
            bool is_permanent = false;
            std::chrono::system_clock::time_point created;

            int GetCurrentValue() const {
                return is_permanent ? value : static_cast<int>(value * weight);
            }
        };

        std::vector<OpinionModifier> opinion_modifiers;

        // Enhanced historical tracking
        struct HistoricalOpinionData {
            std::deque<int> monthly_opinions;       // Last 120 months (10 years)
            std::deque<int> yearly_opinions;        // Last 100 years

            double short_term_average = 0.0;        // 1 year average
            double medium_term_average = 0.0;       // 10 year average
            double long_term_average = 0.0;         // 50+ year average

            int highest_ever = 0;
            int lowest_ever = 0;
            std::chrono::system_clock::time_point best_relations_date;
            std::chrono::system_clock::time_point worst_relations_date;
        };

        HistoricalOpinionData historical_data;

        DiplomaticState() = default;
        explicit DiplomaticState(game::types::EntityID realm);

        // Cooldown helper methods
        bool IsActionOnCooldown(DiplomaticAction action) const;
        void SetActionCooldown(DiplomaticAction action, int cooldown_days = 0);
        int GetRemainingCooldownDays(DiplomaticAction action) const;

        // Decay helper methods for passive drift toward neutral
        void ApplyOpinionDecay(float time_delta, int neutral_baseline = 0);
        void ApplyTrustDecay(float time_delta, double neutral_baseline = 0.5);

        // Long-term memory helper - updates rolling opinion average
        void UpdateOpinionHistory(int new_opinion);
        double GetHistoricalOpinionAverage() const { return historical_opinion_average; }

        // New memory integration methods
        void AddOpinionModifier(const std::string& source, int value, bool permanent = false);
        void RemoveOpinionModifier(const std::string& source);
        int CalculateTotalOpinion() const;  // Sum all modifiers
        void UpdateHistoricalData(int current_opinion, bool is_monthly = false, bool is_yearly = false);
        void ApplyModifierDecay(float months_elapsed = 1.0f);
    };

    struct Treaty {
        std::string treaty_id;
        TreatyType type;
        game::types::EntityID signatory_a;
        game::types::EntityID signatory_b;

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
        Treaty(TreatyType treaty_type, game::types::EntityID realm_a, game::types::EntityID realm_b);

        bool IsExpired() const;
        bool IsBroken() const;
        double GetOverallCompliance() const;
    };

    struct DynasticMarriage {
        std::string marriage_id;
        game::types::EntityID bride_realm;
        game::types::EntityID groom_realm;
        game::types::EntityID bride_character;
        game::types::EntityID groom_character;

        double diplomatic_bonus = 20.0;
        double inheritance_claim = 0.0;
        bool produces_alliance = false;

        std::chrono::system_clock::time_point marriage_date;
        bool is_active = true;
        std::vector<game::types::EntityID> children;

        DynasticMarriage() = default;
        DynasticMarriage(game::types::EntityID bride, game::types::EntityID groom);
    };

    struct DiplomaticProposal {
        std::string proposal_id;
        game::types::EntityID proposer;
        game::types::EntityID target;
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
        DiplomaticProposal(game::types::EntityID from, game::types::EntityID to, DiplomaticAction action);
    };

    // ============================================================================
    // ECS Components
    // ============================================================================

    struct DiplomacyComponent : public game::core::Component<DiplomacyComponent> {
        // Core diplomatic relationships
        std::unordered_map<game::types::EntityID, DiplomaticState> relationships;
        
        // Active treaties
        std::vector<Treaty> active_treaties;
        
        // Dynastic marriages
        std::vector<DynasticMarriage> marriages;
        
        // Quick access lists
        std::vector<game::types::EntityID> allies;
        std::vector<game::types::EntityID> enemies;
        
        // Diplomatic personality and settings
        DiplomaticPersonality personality = DiplomaticPersonality::DIPLOMATIC;
        double prestige = 0.0;
        double diplomatic_reputation = 1.0;
        
        // War and conflict tracking
        double war_weariness = 0.0; // 0.0 to 1.0, increases during wars
        
        std::string GetComponentTypeName() const override {
            return "DiplomacyComponent";
        }
        
        // Helper methods (implemented in DiplomacyComponents.cpp)
        DiplomaticState* GetRelationship(types::EntityID other_realm);
        const DiplomaticState* GetRelationship(types::EntityID other_realm) const;
        void SetRelation(types::EntityID other_realm, DiplomaticRelation relation);
        void ModifyOpinion(types::EntityID other_realm, int opinion_change, const std::string& reason);
        void AddTreaty(const Treaty& treaty);
        void RemoveTreaty(const std::string& treaty_id);
        void BreakTreaty(types::EntityID other_realm, TreatyType type);
        std::vector<Treaty*> GetTreatiesWith(types::EntityID other_realm);
        bool HasTreatyType(types::EntityID other_realm, TreatyType type) const;
        bool IsAtWar() const;
        bool IsAtWarWith(types::EntityID other_realm) const;
        bool IsAlliedWith(types::EntityID other_realm) const;
        std::vector<types::EntityID> GetWarEnemies() const;
        std::vector<types::EntityID> GetMilitaryAllies() const;
    };

    struct TreatyComponent : public game::core::Component<TreatyComponent> {
        TreatyType treaty_type = TreatyType::ALLIANCE;
        game::types::EntityID participant_1{ 0 };
        game::types::EntityID participant_2{ 0 };
        
        std::chrono::system_clock::time_point start_date;
        std::chrono::system_clock::time_point end_date;
        
        double compliance_rate = 1.0;
        bool is_active = true;
        
        // Treaty-specific parameters
        std::unordered_map<std::string, double> parameters;
        
        std::string GetComponentTypeName() const override {
            return "TreatyComponent";
        }
    };

    struct DiplomaticActionComponent : public game::core::Component<DiplomaticActionComponent> {
        game::types::EntityID actor{ 0 };
        game::types::EntityID target{ 0 };
        
        std::string action_type;
        std::string description;
        
        std::chrono::system_clock::time_point timestamp;
        double impact_value = 0.0;
        
        bool is_resolved = false;
        
        std::string GetComponentTypeName() const override {
            return "DiplomacyComponent";
        }

        // Component methods
        DiplomaticState* GetRelationship(game::types::EntityID other_realm);
        const DiplomaticState* GetRelationship(game::types::EntityID other_realm) const;
        void SetRelation(game::types::EntityID other_realm, DiplomaticRelation relation);
        void ModifyOpinion(game::types::EntityID other_realm, int opinion_change, const std::string& reason);
        void AddTreaty(const Treaty& treaty);
        void RemoveTreaty(const std::string& treaty_id);
        std::vector<Treaty*> GetTreatiesWith(game::types::EntityID other_realm);
        bool HasTreatyType(game::types::EntityID other_realm, TreatyType type) const;
        bool IsAtWar() const;
        bool IsAtWarWith(game::types::EntityID other_realm) const;
        bool IsAlliedWith(game::types::EntityID other_realm) const;
        std::vector<game::types::EntityID> GetWarEnemies() const;
        std::vector<game::types::EntityID> GetMilitaryAllies() const;
    };

} // namespace game::diplomacy