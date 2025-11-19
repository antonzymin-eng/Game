// ============================================================================
// CharacterRelationships.h - Character Marriage and Friendship System
// Created: November 15, 2025 - Phase 3: Influence System Integration
// Location: include/game/character/CharacterRelationships.h
// ============================================================================

#pragma once

#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <unordered_map>
#include <vector>
#include <chrono>
#include <optional>

namespace game {
namespace character {

// ============================================================================
// Marriage Types and Status
// ============================================================================

enum class MarriageType : uint8_t {
    NORMAL,           // Standard marriage
    MATRILINEAL,      // Children inherit mother's dynasty
    POLITICAL,        // Marriage arranged for alliance
    SECRET,           // Hidden marriage
    MORGANATIC,       // Lower-rank spouse, children don't inherit
    COUNT
};

struct Marriage {
    types::EntityID spouse{0};
    types::EntityID realm_of_spouse{0};  // Which realm the spouse rules/belongs to
    types::EntityID spouse_dynasty{0};   // Spouse's dynasty
    MarriageType type = MarriageType::NORMAL;
    std::chrono::system_clock::time_point marriage_date;
    bool is_alliance = false;  // Does this marriage create an alliance?
    std::vector<types::EntityID> children;

    Marriage() = default;
    Marriage(types::EntityID spouse_id, types::EntityID spouse_realm, types::EntityID dynasty)
        : spouse(spouse_id)
        , realm_of_spouse(spouse_realm)
        , spouse_dynasty(dynasty)
        , marriage_date(std::chrono::system_clock::now())
    {}
};

// ============================================================================
// Friendship/Relationship Strength
// ============================================================================

enum class RelationshipType : uint8_t {
    FRIEND,           // Close friend
    RIVAL,            // Personal rival
    LOVER,            // Romantic relationship
    MENTOR,           // This character is mentor
    STUDENT,          // This character is student
    BLOOD_BROTHER,    // Sworn brotherhood
    COUNT
};

struct CharacterRelationship {
    types::EntityID other_character{0};
    RelationshipType type = RelationshipType::FRIEND;
    int opinion = 0;  // -100 to +100
    double bond_strength = 0.0;  // 0.0 to 100.0
    std::chrono::system_clock::time_point established_date;

    // Decay/growth modifiers
    bool is_active = true;  // Relationships decay if not maintained
    std::chrono::system_clock::time_point last_interaction;

    CharacterRelationship() = default;
    CharacterRelationship(types::EntityID other, RelationshipType rel_type)
        : other_character(other)
        , type(rel_type)
        , established_date(std::chrono::system_clock::now())
        , last_interaction(std::chrono::system_clock::now())
    {}
};

// ============================================================================
// Character Relationships Component (ECS)
// ============================================================================

class CharacterRelationshipsComponent : public ::core::ecs::Component<CharacterRelationshipsComponent> {
public:
    types::EntityID character_id{0};

    // Marriages
    std::vector<Marriage> marriages;  // Current and past marriages
    types::EntityID current_spouse{0};  // Primary spouse

    // Friendships and relationships
    std::unordered_map<types::EntityID, CharacterRelationship> relationships;

    // Family ties (calculated from marriages)
    std::vector<types::EntityID> children;
    std::vector<types::EntityID> siblings;
    types::EntityID father{0};
    types::EntityID mother{0};

    CharacterRelationshipsComponent() = default;
    explicit CharacterRelationshipsComponent(types::EntityID char_id)
        : character_id(char_id)
    {}

    // ========================================================================
    // Marriage Management
    // ========================================================================

    /**
     * Add a new marriage for this character
     */
    void AddMarriage(types::EntityID spouse_id, types::EntityID spouse_realm,
                     types::EntityID spouse_dynasty, bool creates_alliance = false) {
        Marriage marriage(spouse_id, spouse_realm, spouse_dynasty);
        marriage.is_alliance = creates_alliance;
        marriages.push_back(marriage);
        current_spouse = spouse_id;
    }

    /**
     * Check if character is married to a specific person
     */
    bool IsMarriedTo(types::EntityID other_char) const {
        for (const auto& marriage : marriages) {
            if (marriage.spouse == other_char) {
                return true;
            }
        }
        return false;
    }

    /**
     * Get all current spouses (supports polygamy)
     */
    std::vector<types::EntityID> GetSpouses() const {
        std::vector<types::EntityID> spouses;
        for (const auto& marriage : marriages) {
            spouses.push_back(marriage.spouse);
        }
        return spouses;
    }

    /**
     * Check if this character has marriage ties to a specific realm
     */
    bool HasMarriageTiesTo(types::EntityID realm_id) const {
        for (const auto& marriage : marriages) {
            if (marriage.realm_of_spouse == realm_id) {
                return true;
            }
        }
        return false;
    }

    /**
     * Check if shares dynasty through marriage (spouse is same dynasty)
     */
    bool SharesDynastyThroughMarriage(types::EntityID dynasty_id) const {
        for (const auto& marriage : marriages) {
            if (marriage.spouse_dynasty == dynasty_id) {
                return true;
            }
        }
        return false;
    }

    // ========================================================================
    // Relationship Management
    // ========================================================================

    /**
     * Add or update a relationship with another character
     */
    void SetRelationship(types::EntityID other_char, RelationshipType type,
                        int opinion = 0, double bond = 0.0) {
        auto it = relationships.find(other_char);
        if (it != relationships.end()) {
            it->second.type = type;
            it->second.opinion = opinion;
            it->second.bond_strength = bond;
            it->second.last_interaction = std::chrono::system_clock::now();
        } else {
            CharacterRelationship rel(other_char, type);
            rel.opinion = opinion;
            rel.bond_strength = bond;
            relationships[other_char] = rel;
        }
    }

    /**
     * Get relationship with a specific character (std::nullopt if none exists)
     */
    std::optional<CharacterRelationship> GetRelationship(types::EntityID other_char) const {
        auto it = relationships.find(other_char);
        if (it != relationships.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * Check if this character is friends with another
     */
    bool IsFriendsWith(types::EntityID other_char) const {
        auto rel = GetRelationship(other_char);
        return rel.has_value() && rel->type == RelationshipType::FRIEND && rel->bond_strength >= 25.0;
    }

    /**
     * Get friendship bond strength with another character (0.0 if no friendship)
     */
    double GetFriendshipBondStrength(types::EntityID other_char) const {
        auto rel = GetRelationship(other_char);
        if (rel.has_value() && rel->type == RelationshipType::FRIEND) {
            return rel->bond_strength;
        }
        return 0.0;
    }

    /**
     * Get all friends of this character
     */
    std::vector<types::EntityID> GetFriends() const {
        std::vector<types::EntityID> friends;
        for (const auto& [char_id, rel] : relationships) {
            if (rel.type == RelationshipType::FRIEND && rel.bond_strength >= 25.0) {
                friends.push_back(char_id);
            }
        }
        return friends;
    }

    /**
     * Update relationship bond strength
     */
    void ModifyBondStrength(types::EntityID other_char, double delta) {
        auto it = relationships.find(other_char);
        if (it != relationships.end()) {
            it->second.bond_strength += delta;
            it->second.bond_strength = std::max(0.0, std::min(100.0, it->second.bond_strength));
            it->second.last_interaction = std::chrono::system_clock::now();
        }
    }

    // ========================================================================
    // Family Queries
    // ========================================================================

    /**
     * Check if shares a parent with another character
     */
    bool IsSiblingOf(types::EntityID other_char) const {
        return std::find(siblings.begin(), siblings.end(), other_char) != siblings.end();
    }

    /**
     * Check if is child of a specific character
     */
    bool IsChildOf(types::EntityID parent_id) const {
        return (father == parent_id || mother == parent_id);
    }

    /**
     * Add a child to this character
     */
    void AddChild(types::EntityID child_id) {
        if (std::find(children.begin(), children.end(), child_id) == children.end()) {
            children.push_back(child_id);
        }
    }
};

} // namespace character
} // namespace game
