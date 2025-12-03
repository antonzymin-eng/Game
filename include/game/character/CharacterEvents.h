// ============================================================================
// CharacterEvents.h - Event definitions for character system
// Created: December 3, 2025
// Location: include/game/character/CharacterEvents.h
// ============================================================================

#pragma once

#include "core/types/game_types.h"
#include "game/character/CharacterLifeEvents.h"
#include "game/character/CharacterRelationships.h"
#include <string>
#include <cstdint>

namespace game {
namespace character {

// ============================================================================
// Character Lifecycle Events
// ============================================================================

/**
 * Published when a new character entity is created
 */
struct CharacterCreatedEvent {
    types::EntityID characterId;
    std::string name;
    uint32_t age;
    bool isHistorical;  // Loaded from historical data vs dynamically created

    CharacterCreatedEvent() = default;

    CharacterCreatedEvent(types::EntityID id, const std::string& n, uint32_t a, bool hist = false)
        : characterId(id)
        , name(n)
        , age(a)
        , isHistorical(hist)
    {}
};

/**
 * Published when a character dies
 */
struct CharacterDiedEvent {
    types::EntityID characterId;
    std::string name;
    LifeEventType deathType;
    uint32_t ageAtDeath;
    types::EntityID killer{0};  // 0 if natural causes

    CharacterDiedEvent() = default;

    CharacterDiedEvent(types::EntityID id, const std::string& n, LifeEventType dtype, uint32_t age)
        : characterId(id)
        , name(n)
        , deathType(dtype)
        , ageAtDeath(age)
    {}
};

/**
 * Published when a character comes of age
 */
struct CharacterCameOfAgeEvent {
    types::EntityID characterId;
    std::string name;
    uint32_t age;

    CharacterCameOfAgeEvent() = default;

    CharacterCameOfAgeEvent(types::EntityID id, const std::string& n, uint32_t a)
        : characterId(id)
        , name(n)
        , age(a)
    {}
};

// ============================================================================
// Character AI Events
// ============================================================================

/**
 * Request that AIDirector create an AI actor for this character
 * Published when important characters are created (rulers, council members)
 */
struct CharacterNeedsAIEvent {
    types::EntityID characterId;
    std::string name;
    bool isRuler;
    bool isCouncilMember;

    CharacterNeedsAIEvent() = default;

    CharacterNeedsAIEvent(types::EntityID id, const std::string& n, bool ruler = false, bool council = false)
        : characterId(id)
        , name(n)
        , isRuler(ruler)
        , isCouncilMember(council)
    {}
};

/**
 * Published when character's AI makes a decision
 */
struct CharacterDecisionEvent {
    types::EntityID characterId;
    std::string decisionType;  // "plot", "proposal", "relationship", "personal"
    std::string decisionDetails;

    CharacterDecisionEvent() = default;

    CharacterDecisionEvent(types::EntityID id, const std::string& type, const std::string& details)
        : characterId(id)
        , decisionType(type)
        , decisionDetails(details)
    {}
};

// ============================================================================
// Relationship Events
// ============================================================================

/**
 * Published when a relationship between two characters changes
 */
struct RelationshipChangedEvent {
    types::EntityID character1;
    types::EntityID character2;
    RelationshipType oldType;
    RelationshipType newType;
    float opinionDelta;  // Change in opinion (-100 to +100)

    RelationshipChangedEvent() = default;

    RelationshipChangedEvent(types::EntityID c1, types::EntityID c2,
                           RelationshipType old_t, RelationshipType new_t, float delta)
        : character1(c1)
        , character2(c2)
        , oldType(old_t)
        , newType(new_t)
        , opinionDelta(delta)
    {}
};

/**
 * Published when two characters marry
 */
struct CharacterMarriedEvent {
    types::EntityID character1;
    types::EntityID character2;
    std::string character1Name;
    std::string character2Name;
    MarriageType marriageType;
    bool createsAlliance;

    CharacterMarriedEvent() = default;

    CharacterMarriedEvent(types::EntityID c1, types::EntityID c2,
                         const std::string& n1, const std::string& n2,
                         MarriageType type, bool alliance)
        : character1(c1)
        , character2(c2)
        , character1Name(n1)
        , character2Name(n2)
        , marriageType(type)
        , createsAlliance(alliance)
    {}
};

/**
 * Published when a marriage ends (divorce or death)
 */
struct MarriageEndedEvent {
    types::EntityID character1;
    types::EntityID character2;
    std::string reason;  // "divorce", "death", "annulment"

    MarriageEndedEvent() = default;

    MarriageEndedEvent(types::EntityID c1, types::EntityID c2, const std::string& r)
        : character1(c1)
        , character2(c2)
        , reason(r)
    {}
};

/**
 * Published when a child is born to a character
 */
struct ChildBornEvent {
    types::EntityID parentId;
    types::EntityID childId;
    std::string parentName;
    std::string childName;
    bool isLegitimate;

    ChildBornEvent() = default;

    ChildBornEvent(types::EntityID parent, types::EntityID child,
                  const std::string& pname, const std::string& cname, bool legit)
        : parentId(parent)
        , childId(child)
        , parentName(pname)
        , childName(cname)
        , isLegitimate(legit)
    {}
};

// ============================================================================
// Education Events
// ============================================================================

/**
 * Published when character starts education
 */
struct EducationStartedEvent {
    types::EntityID characterId;
    types::EntityID tutorId;  // 0 if self-taught
    EducationFocus focus;

    EducationStartedEvent() = default;

    EducationStartedEvent(types::EntityID char_id, types::EntityID tutor, EducationFocus f)
        : characterId(char_id)
        , tutorId(tutor)
        , focus(f)
    {}
};

/**
 * Published when character completes education
 */
struct EducationCompletedEvent {
    types::EntityID characterId;
    EducationQuality quality;
    EducationFocus focus;
    std::vector<std::string> traitsGained;

    EducationCompletedEvent() = default;

    EducationCompletedEvent(types::EntityID char_id, EducationQuality qual, EducationFocus f)
        : characterId(char_id)
        , quality(qual)
        , focus(f)
    {}
};

/**
 * Published when character gains a skill level
 */
struct SkillLevelUpEvent {
    types::EntityID characterId;
    EducationFocus skill;
    uint8_t oldLevel;
    uint8_t newLevel;

    SkillLevelUpEvent() = default;

    SkillLevelUpEvent(types::EntityID char_id, EducationFocus s, uint8_t old_lvl, uint8_t new_lvl)
        : characterId(char_id)
        , skill(s)
        , oldLevel(old_lvl)
        , newLevel(new_lvl)
    {}
};

// ============================================================================
// Trait Events
// ============================================================================

/**
 * Published when character gains a trait
 */
struct TraitGainedEvent {
    types::EntityID characterId;
    std::string traitId;
    std::string traitName;
    bool isTemporary;

    TraitGainedEvent() = default;

    TraitGainedEvent(types::EntityID char_id, const std::string& id,
                    const std::string& name, bool temp = false)
        : characterId(char_id)
        , traitId(id)
        , traitName(name)
        , isTemporary(temp)
    {}
};

/**
 * Published when character loses a trait
 */
struct TraitLostEvent {
    types::EntityID characterId;
    std::string traitId;
    std::string reason;  // "expired", "removed", "replaced"

    TraitLostEvent() = default;

    TraitLostEvent(types::EntityID char_id, const std::string& id, const std::string& r)
        : characterId(char_id)
        , traitId(id)
        , reason(r)
    {}
};

// ============================================================================
// Title/Position Events
// ============================================================================

/**
 * Published when character gains a title
 */
struct TitleGainedEvent {
    types::EntityID characterId;
    types::EntityID titleId;
    std::string titleName;
    bool isPrimaryTitle;

    TitleGainedEvent() = default;

    TitleGainedEvent(types::EntityID char_id, types::EntityID title,
                    const std::string& name, bool primary)
        : characterId(char_id)
        , titleId(title)
        , titleName(name)
        , isPrimaryTitle(primary)
    {}
};

/**
 * Published when character loses a title
 */
struct TitleLostEvent {
    types::EntityID characterId;
    types::EntityID titleId;
    std::string titleName;
    std::string reason;  // "usurped", "inherited", "revoked", "destroyed"

    TitleLostEvent() = default;

    TitleLostEvent(types::EntityID char_id, types::EntityID title,
                  const std::string& name, const std::string& r)
        : characterId(char_id)
        , titleId(title)
        , titleName(name)
        , reason(r)
    {}
};

// ============================================================================
// Life Event Published
// ============================================================================

/**
 * Published when a major life event occurs for a character
 * Wraps the LifeEvent struct for message bus propagation
 */
struct CharacterLifeEventOccurred {
    types::EntityID characterId;
    LifeEvent event;

    CharacterLifeEventOccurred() = default;

    CharacterLifeEventOccurred(types::EntityID char_id, const LifeEvent& e)
        : characterId(char_id)
        , event(e)
    {}
};

} // namespace character
} // namespace game
