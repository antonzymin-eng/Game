// ============================================================================
// DiplomacyMessages.h - Message Bus Event Types for Diplomacy System
// Created: November 18, 2025 - Event-Driven Diplomacy Architecture
// Location: include/game/diplomacy/DiplomacyMessages.h
// ============================================================================

#pragma once

#include "core/ECS/MessageBus.h"
#include "core/types/game_types.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include <string>
#include <typeindex>

namespace game::diplomacy::messages {

    // ========================================================================
    // War and Conflict Events
    // ========================================================================

    struct WarDeclaredMessage : public ::core::ecs::IMessage {
        types::EntityID aggressor;
        types::EntityID defender;
        CasusBelli casus_belli;
        int aggressor_opinion_of_defender;
        int defender_opinion_of_aggressor;

        WarDeclaredMessage(types::EntityID agg, types::EntityID def, CasusBelli cb)
            : aggressor(agg), defender(def), casus_belli(cb)
            , aggressor_opinion_of_defender(0), defender_opinion_of_aggressor(0) {}

        std::type_index GetTypeIndex() const override { return typeid(WarDeclaredMessage); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    struct WarEndedMessage : public ::core::ecs::IMessage {
        types::EntityID victor;
        types::EntityID defeated;
        bool white_peace;
        double war_score;

        WarEndedMessage(types::EntityID vic, types::EntityID def, bool white = false, double score = 0.0)
            : victor(vic), defeated(def), white_peace(white), war_score(score) {}

        std::type_index GetTypeIndex() const override { return typeid(WarEndedMessage); }
    };

    struct AllyActivatedMessage : public ::core::ecs::IMessage {
        types::EntityID ally;
        types::EntityID war_leader;
        types::EntityID enemy;
        bool accepted;

        AllyActivatedMessage(types::EntityID a, types::EntityID wl, types::EntityID e, bool acc)
            : ally(a), war_leader(wl), enemy(e), accepted(acc) {}

        std::type_index GetTypeIndex() const override { return typeid(AllyActivatedMessage); }
    };

    // ========================================================================
    // Treaty and Alliance Events
    // ========================================================================

    struct TreatySignedMessage : public ::core::ecs::IMessage {
        types::EntityID signatory_a;
        types::EntityID signatory_b;
        TreatyType treaty_type;
        std::string treaty_id;
        bool is_secret;
        double secrecy_level;

        TreatySignedMessage(types::EntityID a, types::EntityID b, TreatyType type,
                           const std::string& id = "", bool secret = false, double secrecy = 0.0)
            : signatory_a(a), signatory_b(b), treaty_type(type)
            , treaty_id(id), is_secret(secret), secrecy_level(secrecy) {}

        std::type_index GetTypeIndex() const override { return typeid(TreatySignedMessage); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    struct TreatyViolatedMessage : public ::core::ecs::IMessage {
        types::EntityID violator;
        types::EntityID victim;
        TreatyType treaty_type;
        std::string treaty_id;
        double severity;  // 0.0 = minor, 1.0 = major breach

        TreatyViolatedMessage(types::EntityID viol, types::EntityID vict, TreatyType type,
                             const std::string& id = "", double sev = 0.5)
            : violator(viol), victim(vict), treaty_type(type), treaty_id(id), severity(sev) {}

        std::type_index GetTypeIndex() const override { return typeid(TreatyViolatedMessage); }
    };

    struct AllianceFormedMessage : public ::core::ecs::IMessage {
        types::EntityID realm_a;
        types::EntityID realm_b;
        bool defensive_only;
        std::string alliance_name;

        AllianceFormedMessage(types::EntityID a, types::EntityID b, bool defensive = false,
                             const std::string& name = "")
            : realm_a(a), realm_b(b), defensive_only(defensive), alliance_name(name) {}

        std::type_index GetTypeIndex() const override { return typeid(AllianceFormedMessage); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    struct AllianceBrokenMessage : public ::core::ecs::IMessage {
        types::EntityID breaker;
        types::EntityID former_ally;
        std::string reason;
        bool betrayal;  // true if broken during war

        AllianceBrokenMessage(types::EntityID br, types::EntityID ally,
                             const std::string& r = "", bool betr = false)
            : breaker(br), former_ally(ally), reason(r), betrayal(betr) {}

        std::type_index GetTypeIndex() const override { return typeid(AllianceBrokenMessage); }
    };

    // ========================================================================
    // Secret Diplomacy Events
    // ========================================================================

    struct SecretTreatyRevealedMessage : public ::core::ecs::IMessage {
        types::EntityID discoverer;
        types::EntityID signatory_a;
        types::EntityID signatory_b;
        TreatyType treaty_type;
        std::string treaty_id;
        double impact;  // Diplomatic impact of the discovery

        SecretTreatyRevealedMessage(types::EntityID disc, types::EntityID a, types::EntityID b,
                                   TreatyType type, const std::string& id = "", double imp = 0.5)
            : discoverer(disc), signatory_a(a), signatory_b(b)
            , treaty_type(type), treaty_id(id), impact(imp) {}

        std::type_index GetTypeIndex() const override { return typeid(SecretTreatyRevealedMessage); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    // ========================================================================
    // Opinion and Relationship Events
    // ========================================================================

    struct OpinionChangedMessage : public ::core::ecs::IMessage {
        types::EntityID realm;
        types::EntityID target;
        int old_opinion;
        int new_opinion;
        std::string reason;

        OpinionChangedMessage(types::EntityID r, types::EntityID t, int old_op, int new_op,
                             const std::string& reas = "")
            : realm(r), target(t), old_opinion(old_op), new_opinion(new_op), reason(reas) {}

        std::type_index GetTypeIndex() const override { return typeid(OpinionChangedMessage); }
    };

    struct RelationshipChangedMessage : public ::core::ecs::IMessage {
        types::EntityID realm_a;
        types::EntityID realm_b;
        DiplomaticRelation old_relation;
        DiplomaticRelation new_relation;

        RelationshipChangedMessage(types::EntityID a, types::EntityID b,
                                  DiplomaticRelation old_rel, DiplomaticRelation new_rel)
            : realm_a(a), realm_b(b), old_relation(old_rel), new_relation(new_rel) {}

        std::type_index GetTypeIndex() const override { return typeid(RelationshipChangedMessage); }
    };

    // ========================================================================
    // Diplomatic Actions
    // ========================================================================

    struct DiplomaticProposalMessage : public ::core::ecs::IMessage {
        types::EntityID proposer;
        types::EntityID target;
        DiplomaticAction action_type;
        std::string proposal_id;
        double ai_acceptance_chance;

        DiplomaticProposalMessage(types::EntityID prop, types::EntityID targ,
                                 DiplomaticAction action, const std::string& id = "",
                                 double chance = 0.5)
            : proposer(prop), target(targ), action_type(action)
            , proposal_id(id), ai_acceptance_chance(chance) {}

        std::type_index GetTypeIndex() const override { return typeid(DiplomaticProposalMessage); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    struct ProposalAcceptedMessage : public ::core::ecs::IMessage {
        types::EntityID proposer;
        types::EntityID accepter;
        DiplomaticAction action_type;
        std::string proposal_id;

        ProposalAcceptedMessage(types::EntityID prop, types::EntityID acc,
                               DiplomaticAction action, const std::string& id = "")
            : proposer(prop), accepter(acc), action_type(action), proposal_id(id) {}

        std::type_index GetTypeIndex() const override { return typeid(ProposalAcceptedMessage); }
    };

    struct ProposalRejectedMessage : public ::core::ecs::IMessage {
        types::EntityID proposer;
        types::EntityID rejecter;
        DiplomaticAction action_type;
        std::string proposal_id;
        std::string rejection_reason;

        ProposalRejectedMessage(types::EntityID prop, types::EntityID rej,
                               DiplomaticAction action, const std::string& id = "",
                               const std::string& reason = "")
            : proposer(prop), rejecter(rej), action_type(action)
            , proposal_id(id), rejection_reason(reason) {}

        std::type_index GetTypeIndex() const override { return typeid(ProposalRejectedMessage); }
    };

    // ========================================================================
    // Marriage and Dynasty Events
    // ========================================================================

    struct MarriageArrangedMessage : public ::core::ecs::IMessage {
        types::EntityID bride_realm;
        types::EntityID groom_realm;
        types::EntityID bride_character;
        types::EntityID groom_character;
        bool creates_alliance;
        double inheritance_claim;

        MarriageArrangedMessage(types::EntityID br, types::EntityID gr,
                               types::EntityID bc = 0, types::EntityID gc = 0,
                               bool alliance = false, double claim = 0.0)
            : bride_realm(br), groom_realm(gr), bride_character(bc), groom_character(gc)
            , creates_alliance(alliance), inheritance_claim(claim) {}

        std::type_index GetTypeIndex() const override { return typeid(MarriageArrangedMessage); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    // ========================================================================
    // Memory and Milestone Events
    // ========================================================================

    struct MilestoneAchievedMessage : public ::core::ecs::IMessage {
        types::EntityID realm_a;
        types::EntityID realm_b;
        int milestone_type;  // MilestoneType enum value
        int opinion_bonus;
        std::string milestone_name;

        MilestoneAchievedMessage(types::EntityID a, types::EntityID b, int type,
                                int bonus = 0, const std::string& name = "")
            : realm_a(a), realm_b(b), milestone_type(type)
            , opinion_bonus(bonus), milestone_name(name) {}

        std::type_index GetTypeIndex() const override { return typeid(MilestoneAchievedMessage); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    struct DiplomaticMemoryEventMessage : public ::core::ecs::IMessage {
        types::EntityID realm;
        types::EntityID other_realm;
        int event_type;  // EventType enum value
        double severity;
        std::string description;

        DiplomaticMemoryEventMessage(types::EntityID r, types::EntityID other, int type,
                                    double sev = 0.5, const std::string& desc = "")
            : realm(r), other_realm(other), event_type(type), severity(sev), description(desc) {}

        std::type_index GetTypeIndex() const override { return typeid(DiplomaticMemoryEventMessage); }
    };

    // ========================================================================
    // Embassy and Communication Events
    // ========================================================================

    struct EmbassyEstablishedMessage : public ::core::ecs::IMessage {
        types::EntityID sender;
        types::EntityID host;

        EmbassyEstablishedMessage(types::EntityID s, types::EntityID h)
            : sender(s), host(h) {}

        std::type_index GetTypeIndex() const override { return typeid(EmbassyEstablishedMessage); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    struct EmbassyClosedMessage : public ::core::ecs::IMessage {
        types::EntityID sender;
        types::EntityID former_host;
        bool expelled;  // true if kicked out, false if voluntarily closed

        EmbassyClosedMessage(types::EntityID s, types::EntityID h, bool exp = false)
            : sender(s), former_host(h), expelled(exp) {}

        std::type_index GetTypeIndex() const override { return typeid(EmbassyClosedMessage); }
    };

    struct DiplomaticGiftSentMessage : public ::core::ecs::IMessage {
        types::EntityID sender;
        types::EntityID recipient;
        double gift_value;
        int opinion_gain;

        DiplomaticGiftSentMessage(types::EntityID s, types::EntityID r, double val = 0.0, int gain = 0)
            : sender(s), recipient(r), gift_value(val), opinion_gain(gain) {}

        std::type_index GetTypeIndex() const override { return typeid(DiplomaticGiftSentMessage); }
    };

    // ========================================================================
    // Trade Events
    // ========================================================================

    struct TradeAgreementSignedMessage : public ::core::ecs::IMessage {
        types::EntityID realm_a;
        types::EntityID realm_b;
        double trade_bonus;
        int duration_years;

        TradeAgreementSignedMessage(types::EntityID a, types::EntityID b,
                                   double bonus = 0.0, int duration = 10)
            : realm_a(a), realm_b(b), trade_bonus(bonus), duration_years(duration) {}

        std::type_index GetTypeIndex() const override { return typeid(TradeAgreementSignedMessage); }
        ::core::ecs::MessagePriority GetPriority() const override { return ::core::ecs::MessagePriority::NORMAL; }
    };

    struct TradeAgreementBrokenMessage : public ::core::ecs::IMessage {
        types::EntityID breaker;
        types::EntityID partner;
        std::string reason;

        TradeAgreementBrokenMessage(types::EntityID br, types::EntityID part,
                                   const std::string& r = "")
            : breaker(br), partner(part), reason(r) {}

        std::type_index GetTypeIndex() const override { return typeid(TradeAgreementBrokenMessage); }
    };

} // namespace game::diplomacy::messages
