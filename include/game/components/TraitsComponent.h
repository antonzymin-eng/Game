// Created: November 19, 2025
// Location: include/game/components/TraitsComponent.h
// Purpose: Character traits system with trait effects

#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>

namespace game {
namespace character {

// ============================================================================
// Trait Categories
// ============================================================================

enum class TraitCategory : uint8_t {
    PERSONALITY,    // Brave, Ambitious, Cruel, Kind, etc.
    EDUCATION,      // Educated, Scholarly, Illiterate, etc.
    LIFESTYLE,      // Drunkard, Temperate, Gluttonous, etc.
    PHYSICAL,       // Strong, Weak, Beautiful, Ugly, etc.
    MENTAL,         // Genius, Quick, Slow, Imbecile, etc.
    HEALTH,         // Wounded, Ill, Maimed, etc.
    FAME,           // Famous, Renowned, Legendary, etc.
    RELIGIOUS,      // Pious, Zealous, Cynical, Heretic, etc.
    REPUTATION,     // Honorable, Dishonorable, Treacherous, etc.
    COUNT
};

// ============================================================================
// Trait Definition
// ============================================================================

struct Trait {
    std::string id;
    std::string name;
    std::string description;
    TraitCategory category = TraitCategory::PERSONALITY;

    // Attribute modifiers
    int8_t diplomacy_modifier = 0;
    int8_t martial_modifier = 0;
    int8_t stewardship_modifier = 0;
    int8_t intrigue_modifier = 0;
    int8_t learning_modifier = 0;

    // AI personality modifiers
    float ambition_modifier = 0.0f;      // -1.0 to +1.0
    float loyalty_modifier = 0.0f;
    float honor_modifier = 0.0f;
    float greed_modifier = 0.0f;
    float boldness_modifier = 0.0f;
    float compassion_modifier = 0.0f;

    // Other effects
    float health_modifier = 0.0f;        // -100 to +100
    float prestige_modifier = 0.0f;      // Monthly modifier
    float fertility_modifier = 0.0f;     // -1.0 to +1.0
    float opinion_modifier = 0.0f;       // General opinion from others

    // Trait properties
    bool is_genetic = false;             // Can be inherited
    bool is_congenital = false;          // Present from birth
    bool is_incurable = false;           // Cannot be removed
    bool is_hidden = false;              // Not visible to others
    int level = 1;                       // For tiered traits (e.g., Scarred I, II, III)

    // Opposites (mutually exclusive traits)
    std::vector<std::string> opposite_traits;

    Trait() = default;
    Trait(const std::string& trait_id, const std::string& trait_name)
        : id(trait_id), name(trait_name) {}
};

// ============================================================================
// Active Character Trait
// ============================================================================

struct ActiveTrait {
    std::string trait_id;
    std::chrono::system_clock::time_point acquired_date;
    bool is_temporary = false;
    std::chrono::system_clock::time_point expiry_date; // For temporary traits

    ActiveTrait() = default;
    ActiveTrait(const std::string& id)
        : trait_id(id)
        , acquired_date(std::chrono::system_clock::now())
    {}
};

// ============================================================================
// Traits Component (ECS)
// ============================================================================

class TraitsComponent : public game::core::Component<TraitsComponent> {
public:
    TraitsComponent() = default;
    ~TraitsComponent() = default;

    // Active traits on this character
    std::vector<ActiveTrait> active_traits;

    // Trait modifiers cache (for performance)
    struct TraitModifiers {
        int8_t total_diplomacy = 0;
        int8_t total_martial = 0;
        int8_t total_stewardship = 0;
        int8_t total_intrigue = 0;
        int8_t total_learning = 0;

        float total_ambition = 0.0f;
        float total_loyalty = 0.0f;
        float total_honor = 0.0f;
        float total_greed = 0.0f;
        float total_boldness = 0.0f;
        float total_compassion = 0.0f;

        float total_health = 0.0f;
        float total_prestige = 0.0f;
        float total_fertility = 0.0f;
        float total_opinion = 0.0f;

        bool needs_recalculation = true;
    } cached_modifiers;

    // ========================================================================
    // Trait Management
    // ========================================================================

    /**
     * Add a trait to this character
     * Returns true if trait was added, false if blocked by opposite trait
     */
    bool AddTrait(const std::string& trait_id, const Trait* trait_def = nullptr);

    /**
     * Remove a trait from this character
     */
    bool RemoveTrait(const std::string& trait_id);

    /**
     * Check if character has a specific trait
     */
    bool HasTrait(const std::string& trait_id) const;

    /**
     * Get all traits in a category
     */
    std::vector<std::string> GetTraitsByCategory(TraitCategory category) const;

    /**
     * Check if character has any trait from a list
     */
    bool HasAnyTrait(const std::vector<std::string>& trait_ids) const;

    /**
     * Get count of traits in a category
     */
    size_t GetTraitCount(TraitCategory category) const;

    /**
     * Add a temporary trait that expires after duration
     */
    void AddTemporaryTrait(const std::string& trait_id,
                          std::chrono::hours duration,
                          const Trait* trait_def = nullptr);

    /**
     * Remove expired temporary traits
     */
    void RemoveExpiredTraits();

    // ========================================================================
    // Modifier Calculation
    // ========================================================================

    /**
     * Recalculate all trait modifiers (call after adding/removing traits)
     */
    void RecalculateModifiers(const std::unordered_map<std::string, Trait>& trait_database);

    /**
     * Get cached modifiers (recalculates if needed)
     */
    const TraitModifiers& GetModifiers(const std::unordered_map<std::string, Trait>& trait_database);

    /**
     * Mark modifiers as needing recalculation
     */
    void InvalidateModifiers() { cached_modifiers.needs_recalculation = true; }

    // ========================================================================
    // Component Interface
    // ========================================================================

    std::unique_ptr<game::core::IComponent> Clone() const override {
        auto clone = std::make_unique<TraitsComponent>();
        clone->active_traits = active_traits;
        clone->cached_modifiers = cached_modifiers;
        return clone;
    }
};

// ============================================================================
// Trait Database (Singleton)
// ============================================================================

class TraitDatabase {
public:
    static TraitDatabase& Instance() {
        static TraitDatabase instance;
        return instance;
    }

    // Prevent copying
    TraitDatabase(const TraitDatabase&) = delete;
    TraitDatabase& operator=(const TraitDatabase&) = delete;

    /**
     * Load traits from JSON file
     */
    bool LoadTraits(const std::string& filepath);

    /**
     * Get trait definition by ID
     */
    const Trait* GetTrait(const std::string& trait_id) const;

    /**
     * Get all traits in a category
     */
    std::vector<const Trait*> GetTraitsByCategory(TraitCategory category) const;

    /**
     * Check if two traits are incompatible
     */
    bool AreTraitsIncompatible(const std::string& trait1, const std::string& trait2) const;

    /**
     * Get all trait definitions
     */
    const std::unordered_map<std::string, Trait>& GetAllTraits() const {
        return m_traits;
    }

private:
    TraitDatabase() { InitializeDefaultTraits(); }

    void InitializeDefaultTraits();

    std::unordered_map<std::string, Trait> m_traits;
};

// ============================================================================
// Predefined Common Traits
// ============================================================================

namespace CommonTraits {
    // Personality traits
    constexpr const char* BRAVE = "brave";
    constexpr const char* CRAVEN = "craven";
    constexpr const char* AMBITIOUS = "ambitious";
    constexpr const char* CONTENT = "content";
    constexpr const char* CRUEL = "cruel";
    constexpr const char* KIND = "kind";
    constexpr const char* GREEDY = "greedy";
    constexpr const char* GENEROUS = "generous";
    constexpr const char* HONEST = "honest";
    constexpr const char* DECEITFUL = "deceitful";

    // Education
    constexpr const char* GENIUS = "genius";
    constexpr const char* QUICK = "quick";
    constexpr const char* SLOW = "slow";
    constexpr const char* IMBECILE = "imbecile";
    constexpr const char* SCHOLARLY = "scholarly";
    constexpr const char* ILLITERATE = "illiterate";

    // Physical
    constexpr const char* STRONG = "strong";
    constexpr const char* WEAK = "weak";
    constexpr const char* ATTRACTIVE = "attractive";
    constexpr const char* UGLY = "ugly";
    constexpr const char* TALL = "tall";
    constexpr const char* DWARF = "dwarf";

    // Health
    constexpr const char* WOUNDED = "wounded";
    constexpr const char* MAIMED = "maimed";
    constexpr const char* ILL = "ill";
    constexpr const char* INFIRM = "infirm";
    constexpr const char* SCARRED = "scarred";

    // Fame
    constexpr const char* FAMOUS = "famous";
    constexpr const char* RENOWNED = "renowned";
    constexpr const char* LEGENDARY = "legendary";

    // Religious
    constexpr const char* PIOUS = "pious";
    constexpr const char* ZEALOUS = "zealous";
    constexpr const char* CYNICAL = "cynical";
    constexpr const char* HERETIC = "heretic";

    // Reputation
    constexpr const char* HONORABLE = "honorable";
    constexpr const char* DISHONORABLE = "dishonorable";
    constexpr const char* TREACHEROUS = "treacherous";
    constexpr const char* JUST = "just";
}

} // namespace character
} // namespace game
