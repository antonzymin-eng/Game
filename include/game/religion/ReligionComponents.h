// ============================================================================
// ReligionComponents.h - Religion and Faith System
// Created: November 15, 2025 - Phase 3: Influence System Integration
// Location: include/game/religion/ReligionComponents.h
// ============================================================================

#pragma once

#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace game {
namespace religion {

// ============================================================================
// Faith/Religion Definitions
// ============================================================================

enum class ReligionGroup : uint8_t {
    CHRISTIAN,
    ISLAMIC,
    PAGAN,
    EASTERN,
    DHARMIC,
    ZOROASTRIAN,
    JEWISH,
    CUSTOM,
    COUNT
};

enum class DoctrineTenet : uint8_t {
    PLURALISM,          // Tolerant of other faiths
    FUNDAMENTALISM,     // Strict adherence required
    PROSELYTIZING,      // Actively converts others
    PACIFISM,           // Opposes violence
    MILITARISM,         // Embraces holy war
    THEOCRACY,          // Religious rule
    COUNT
};

// ============================================================================
// Faith Definition (Shared properties of a faith)
// ============================================================================

struct FaithDefinition {
    types::EntityID faith_id{0};
    std::string faith_name;
    std::string denomination;  // e.g., "Catholic", "Sunni", "Orthodox"
    ReligionGroup religion_group = ReligionGroup::CUSTOM;

    // Doctrines and tenets
    std::vector<DoctrineTenet> doctrines;

    // Religious authority
    types::EntityID religious_head{0};  // Character ID of religious leader
    bool has_religious_head = false;

    // Holy sites
    std::vector<types::EntityID> holy_sites;  // Province IDs of holy sites

    // Modifiers
    double conversion_resistance = 1.0;
    double religious_authority = 50.0;  // 0-100 strength of religious hierarchy
    double piety_gain_modifier = 1.0;

    FaithDefinition() = default;
    explicit FaithDefinition(const std::string& name)
        : faith_name(name)
    {}

    /**
     * Check if this faith is same as another
     */
    bool IsSameFaith(types::EntityID other_faith_id) const {
        return faith_id == other_faith_id;
    }

    /**
     * Check if this faith is same denomination (e.g., both Catholic)
     */
    bool IsSameDenomination(const FaithDefinition& other) const {
        return denomination == other.denomination;
    }

    /**
     * Check if this faith is same religion group (e.g., both Christian)
     */
    bool IsSameReligionGroup(const FaithDefinition& other) const {
        return religion_group == other.religion_group;
    }

    /**
     * Check if faith has a specific doctrine
     */
    bool HasDoctrine(DoctrineTenet doctrine) const {
        return std::find(doctrines.begin(), doctrines.end(), doctrine) != doctrines.end();
    }
};

// ============================================================================
// Character Religion Component (ECS)
// ============================================================================

class CharacterReligionComponent : public ::core::ecs::Component<CharacterReligionComponent> {
public:
    types::EntityID character_id{0};
    types::EntityID faith_id{0};

    // Piety and devotion
    double piety = 50.0;  // 0-100
    double devotion = 50.0;  // 0-100, how strictly they follow faith

    // Religious status
    bool is_clergy = false;
    bool is_religious_head = false;
    uint8_t clergy_rank = 0;  // 0 = layperson, 1-10 = clergy ranks

    // Holy sites controlled (as ruler)
    std::vector<types::EntityID> controlled_holy_sites;

    CharacterReligionComponent() = default;
    explicit CharacterReligionComponent(types::EntityID char_id, types::EntityID faith)
        : character_id(char_id)
        , faith_id(faith)
    {}

    /**
     * Check if character is same faith as another
     */
    bool IsSameFaith(types::EntityID other_faith_id) const {
        return faith_id == other_faith_id;
    }

    /**
     * Get religious authority (based on clergy status and devotion)
     */
    double GetReligiousAuthority() const {
        double authority = devotion;
        if (is_religious_head) {
            authority += 50.0;
        } else if (is_clergy) {
            authority += (clergy_rank * 3.0);
        }
        return std::min(100.0, authority);
    }

    /**
     * Modify piety
     */
    void ModifyPiety(double delta) {
        piety += delta;
        piety = std::max(0.0, std::min(100.0, piety));
    }
};

// ============================================================================
// Realm Religion Component (ECS)
// ============================================================================

class RealmReligionComponent : public ::core::ecs::Component<RealmReligionComponent> {
public:
    types::EntityID realm_id{0};
    types::EntityID state_faith{0};  // Official religion of realm

    // Religious tolerance
    double tolerance = 50.0;  // 0 = persecute heretics, 100 = full tolerance
    bool is_theocracy = false;

    // Religious demographics (faith_id -> percentage)
    std::unordered_map<types::EntityID, double> faith_demographics;

    // Holy sites owned
    std::vector<types::EntityID> owned_holy_sites;

    // Clergy loyalty to this realm
    double clergy_loyalty = 50.0;  // 0-100

    RealmReligionComponent() = default;
    explicit RealmReligionComponent(types::EntityID realm, types::EntityID faith)
        : realm_id(realm)
        , state_faith(faith)
    {
        faith_demographics[faith] = 100.0;  // Start 100% state faith
    }

    /**
     * Check if realm's state faith matches another faith
     */
    bool IsStateFaith(types::EntityID other_faith_id) const {
        return state_faith == other_faith_id;
    }

    /**
     * Get percentage of population following a specific faith
     */
    double GetFaithPercentage(types::EntityID faith_id) const {
        auto it = faith_demographics.find(faith_id);
        return (it != faith_demographics.end()) ? it->second : 0.0;
    }

    /**
     * Check if realm has significant religious diversity
     */
    bool HasReligiousDiversity() const {
        return faith_demographics.size() > 1;
    }

    /**
     * Get dominant faith (highest percentage)
     */
    types::EntityID GetDominantFaith() const {
        if (faith_demographics.empty()) return state_faith;

        types::EntityID dominant = state_faith;
        double max_percentage = 0.0;
        for (const auto& [faith_id, percentage] : faith_demographics) {
            if (percentage > max_percentage) {
                max_percentage = percentage;
                dominant = faith_id;
            }
        }
        return dominant;
    }

    /**
     * Normalize faith demographics to ensure they sum to 100%
     *
     * This method scales all faith percentages proportionally so that
     * they sum to exactly 100%. Useful after manual demographic changes.
     *
     * @note If demographics are empty, no action is taken.
     * @note If the sum is already 100% (within 0.01%), no scaling occurs.
     */
    void NormalizeDemographics() {
        if (faith_demographics.empty()) return;

        // Calculate current total
        double total = 0.0;
        for (const auto& [faith_id, percentage] : faith_demographics) {
            total += percentage;
        }

        // If already normalized (within tolerance), skip
        if (std::abs(total - 100.0) < 0.01) return;

        // Avoid division by zero
        if (total <= 0.0) {
            // Reset to 100% state faith if total is invalid
            faith_demographics.clear();
            faith_demographics[state_faith] = 100.0;
            return;
        }

        // Scale all percentages proportionally
        double scale_factor = 100.0 / total;
        for (auto& [faith_id, percentage] : faith_demographics) {
            percentage *= scale_factor;
        }
    }

    /**
     * Set faith percentage (automatically normalizes all demographics)
     *
     * @param faith_id The faith to set percentage for
     * @param percentage The desired percentage (will be normalized with others)
     *
     * @note After setting, all demographics will be normalized to sum to 100%
     */
    void SetFaithPercentage(types::EntityID faith_id, double percentage) {
        faith_demographics[faith_id] = std::max(0.0, percentage);
        NormalizeDemographics();
    }
};

// ============================================================================
// Religion System Data
// ============================================================================

/**
 * Global religion system state (singleton/system-level)
 * Stores all faith definitions
 */
class ReligionSystemData {
private:
    std::unordered_map<types::EntityID, FaithDefinition> m_faiths;
    types::EntityID m_next_faith_id = 1;

public:
    /**
     * Register a new faith
     *
     * @param name The name of the faith
     * @param group The religion group (e.g., CHRISTIAN, ISLAMIC)
     * @param denomination The denomination (defaults to name if empty)
     * @return The permanent faith ID
     *
     * @note Faith IDs are permanent and auto-incrementing. Once assigned, a
     *       faith ID will never be reused, even if the faith is removed.
     *       This ensures save game compatibility and prevents ID conflicts.
     */
    types::EntityID RegisterFaith(const std::string& name, ReligionGroup group,
                                   const std::string& denomination = "") {
        FaithDefinition faith(name);
        faith.faith_id = m_next_faith_id++;
        faith.religion_group = group;
        faith.denomination = denomination.empty() ? name : denomination;
        m_faiths[faith.faith_id] = faith;
        return faith.faith_id;
    }

    /**
     * Get faith definition
     */
    const FaithDefinition* GetFaith(types::EntityID faith_id) const {
        auto it = m_faiths.find(faith_id);
        return (it != m_faiths.end()) ? &it->second : nullptr;
    }

    /**
     * Get mutable faith definition
     */
    FaithDefinition* GetFaithMutable(types::EntityID faith_id) {
        auto it = m_faiths.find(faith_id);
        return (it != m_faiths.end()) ? &it->second : nullptr;
    }

    /**
     * Check if two faiths are the same
     */
    bool AreSameFaith(types::EntityID faith1, types::EntityID faith2) const {
        return faith1 == faith2;
    }

    /**
     * Check if two faiths are same denomination
     */
    bool AreSameDenomination(types::EntityID faith1, types::EntityID faith2) const {
        const auto* f1 = GetFaith(faith1);
        const auto* f2 = GetFaith(faith2);
        if (!f1 || !f2) return false;
        return f1->IsSameDenomination(*f2);
    }

    /**
     * Check if two faiths are same religion group
     */
    bool AreSameReligionGroup(types::EntityID faith1, types::EntityID faith2) const {
        const auto* f1 = GetFaith(faith1);
        const auto* f2 = GetFaith(faith2);
        if (!f1 || !f2) return false;
        return f1->IsSameReligionGroup(*f2);
    }

    /**
     * Initialize default faiths for testing
     */
    void InitializeDefaultFaiths() {
        // Christianity
        auto catholic_id = RegisterFaith("Catholic", ReligionGroup::CHRISTIAN, "Catholic");
        auto orthodox_id = RegisterFaith("Orthodox", ReligionGroup::CHRISTIAN, "Orthodox");
        auto protestant_id = RegisterFaith("Protestant", ReligionGroup::CHRISTIAN, "Protestant");

        // Islam
        auto sunni_id = RegisterFaith("Sunni", ReligionGroup::ISLAMIC, "Sunni");
        auto shia_id = RegisterFaith("Shia", ReligionGroup::ISLAMIC, "Shia");

        // Others
        auto hindu_id = RegisterFaith("Hinduism", ReligionGroup::DHARMIC, "Hindu");
        auto buddhist_id = RegisterFaith("Buddhism", ReligionGroup::DHARMIC, "Buddhist");

        // Set some basic properties
        if (auto* catholic = GetFaithMutable(catholic_id)) {
            catholic->has_religious_head = true;
            catholic->religious_authority = 80.0;
        }
    }
};

} // namespace religion
} // namespace game
