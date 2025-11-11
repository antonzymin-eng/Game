// ============================================================================
// InfluenceComponents.h - Sphere of Influence Data Structures
// Created: November 11, 2025 - Phase 3: Sphere of Influence System
// Location: include/game/diplomacy/InfluenceComponents.h
// ============================================================================

#pragma once

#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "utils/PlatformCompat.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

namespace game {
namespace diplomacy {

// ============================================================================
// Influence Type Enum
// ============================================================================

// Seven types of power projection
enum class InfluenceType : uint8_t {
    MILITARY,      // Military strength and garrisons (2-4 hop range)
    ECONOMIC,      // Trade dominance and financial leverage (5-8 hop range)
    DYNASTIC,      // Marriage ties and family connections (unlimited range)
    PERSONAL,      // Ruler friendships and character bonds (3-5 hop range)
    RELIGIOUS,     // Religious authority and fervor (unlimited for same faith)
    CULTURAL,      // Cultural similarity and attraction (4-6 hop range)
    PRESTIGE,      // Diplomatic reputation and glory (global range)
    COUNT
};

// Utility function
inline const char* InfluenceTypeToString(InfluenceType type) {
    switch(type) {
        case InfluenceType::MILITARY: return "Military";
        case InfluenceType::ECONOMIC: return "Economic";
        case InfluenceType::DYNASTIC: return "Dynastic";
        case InfluenceType::PERSONAL: return "Personal";
        case InfluenceType::RELIGIOUS: return "Religious";
        case InfluenceType::CULTURAL: return "Cultural";
        case InfluenceType::PRESTIGE: return "Prestige";
        default: return "Unknown";
    }
}

// ============================================================================
// InfluenceSource - Individual influence projection
// ============================================================================

struct InfluenceSource {
    types::EntityID source_realm;           // Who is projecting influence
    InfluenceType type;                     // What kind of influence

    double base_strength = 0.0;             // Raw power (0-100+)
    double distance_modifier = 1.0;         // Geographic decay (0-1)
    double relationship_modifier = 1.0;     // Opinion affects effectiveness
    double effective_strength = 0.0;        // Final calculated influence

    // Propagation tracking
    int hops_from_source = 0;               // How many realms away
    std::vector<types::EntityID> path;      // Path through realms

    // Time tracking
    std::chrono::system_clock::time_point established_date;
    std::chrono::system_clock::time_point last_update;

    // Granular targeting (optional)
    std::vector<types::EntityID> targeted_vassals;     // Specific vassals influenced
    std::vector<types::EntityID> targeted_characters;  // Specific characters influenced
    bool targets_whole_realm = true;        // Or just specific entities

    InfluenceSource() = default;
    InfluenceSource(types::EntityID source, InfluenceType influence_type)
        : source_realm(source)
        , type(influence_type)
        , established_date(std::chrono::system_clock::now())
        , last_update(std::chrono::system_clock::now())
    {}

    // Calculate effective strength
    void CalculateEffectiveStrength();

    // Update modifiers
    void UpdateDistanceModifier(int hops, const std::vector<types::EntityID>& influence_path);
    void UpdateRelationshipModifier(int opinion);
};

// ============================================================================
// InfluenceState - All influences affecting a specific realm
// ============================================================================

struct InfluenceState {
    types::EntityID affected_realm;

    // Influences by type
    std::unordered_map<InfluenceType, std::vector<InfluenceSource>> influences_by_type;

    // Dominant influencer per type
    std::unordered_map<InfluenceType, types::EntityID> dominant_influencer;

    // Total influence received
    double total_influence_received = 0.0;

    // Effects on realm
    double autonomy = 1.0;                  // 1.0 = fully independent, 0.0 = puppet
    double diplomatic_freedom = 1.0;        // Ability to make own choices

    // Resistance
    double resistance_strength = 0.0;       // Ability to resist influence
    bool actively_resisting = false;

    InfluenceState() = default;
    InfluenceState(types::EntityID realm) : affected_realm(realm) {}

    // Add influence source
    void AddInfluence(const InfluenceSource& source);

    // Remove influence
    void RemoveInfluence(types::EntityID source_realm, InfluenceType type);

    // Calculate totals
    void CalculateTotalInfluence();
    void UpdateDominantInfluencers();
    void CalculateAutonomy();
    void CalculateDiplomaticFreedom();

    // Query
    double GetInfluenceStrength(types::EntityID source_realm, InfluenceType type) const;
    types::EntityID GetDominantInfluencer(InfluenceType type) const;
    bool IsInfluencedBy(types::EntityID source_realm) const;
};

// ============================================================================
// VassalInfluence - Granular influence on specific vassals
// ============================================================================

struct VassalInfluence {
    types::EntityID vassal_id;
    types::EntityID liege_realm;
    types::EntityID influencing_realm;

    InfluenceType primary_type;
    double influence_strength = 0.0;

    // Effects
    double loyalty_shift = 0.0;             // Shift away from liege
    double independence_desire = 0.0;       // Want to break free
    double allegiance_shift = 0.0;          // Considering switching sides

    // Potential outcomes
    bool may_defect = false;
    bool may_revolt = false;
    bool may_request_protection = false;

    // Tracking
    std::chrono::system_clock::time_point influence_start;
    int months_under_influence = 0;

    VassalInfluence() = default;
    VassalInfluence(types::EntityID vassal, types::EntityID liege, types::EntityID influencer)
        : vassal_id(vassal)
        , liege_realm(liege)
        , influencing_realm(influencer)
        , influence_start(std::chrono::system_clock::now())
    {}

    // Calculate effects
    void CalculateEffects(double base_influence);
    void CheckDefectionRisk(double threshold = 0.7);

    // Update
    void UpdateMonthly();
};

// ============================================================================
// CharacterInfluence - Character-level influence
// ============================================================================

struct CharacterInfluence {
    types::EntityID character_id;
    types::EntityID character_realm;
    types::EntityID influencing_realm;

    InfluenceType primary_type;             // Usually PERSONAL or DYNASTIC
    double influence_strength = 0.0;

    // Personal relationship
    types::EntityID foreign_friend;         // Specific ruler they're close to
    double personal_loyalty = 0.0;          // Loyalty to foreign power

    // Effects on decision-making
    double opinion_bias = 0.0;              // Bias toward influencer
    bool compromised = false;               // Actively working for foreign power

    // Tracking
    std::chrono::system_clock::time_point influence_start;
    std::string recruitment_method;         // How they were influenced

    CharacterInfluence() = default;
    CharacterInfluence(types::EntityID character, types::EntityID realm, types::EntityID influencer)
        : character_id(character)
        , character_realm(realm)
        , influencing_realm(influencer)
        , influence_start(std::chrono::system_clock::now())
    {}

    // Calculate effects
    void CalculateOpinionBias(double base_influence);
    void CheckCompromised(double threshold = 0.8);

    // Actions
    bool WouldSabotage() const;
    bool WouldLeak() const;
    double GetDecisionBias() const;
};

// ============================================================================
// InfluenceConflict - Competition between spheres
// ============================================================================

struct InfluenceConflict {
    std::string conflict_id;

    types::EntityID contested_realm;        // Who is being fought over
    types::EntityID primary_influencer;     // Current dominant
    types::EntityID challenging_influencer; // Challenger

    InfluenceType conflict_type;

    double primary_strength = 0.0;
    double challenger_strength = 0.0;
    double tension_level = 0.0;             // 0-100

    // Flashpoint data
    bool is_flashpoint = false;
    double escalation_risk = 0.0;           // Chance of war/crisis

    std::chrono::system_clock::time_point conflict_start;
    std::vector<std::string> incidents;     // Diplomatic incidents

    InfluenceConflict() = default;
    InfluenceConflict(types::EntityID contested, types::EntityID primary, types::EntityID challenger)
        : contested_realm(contested)
        , primary_influencer(primary)
        , challenging_influencer(challenger)
        , conflict_start(std::chrono::system_clock::now())
    {
        conflict_id = std::to_string(contested) + "_" +
                     std::to_string(primary) + "_" +
                     std::to_string(challenger);
    }

    // Calculate tension
    void CalculateTension();
    void UpdateEscalationRisk();
    void AddIncident(const std::string& incident);

    // Check flashpoint
    bool CheckFlashpoint() const;
};

// ============================================================================
// InfluenceComponent - Main ECS component for influence
// ============================================================================

struct InfluenceComponent : public game::core::Component<InfluenceComponent> {
    types::EntityID realm_id;

    // Influence this realm projects outward
    std::unordered_map<InfluenceType, double> influence_projection;
    std::unordered_map<types::EntityID, InfluenceState> influenced_realms;

    // Influence this realm receives from others
    InfluenceState incoming_influence;

    // Vassal-specific influences
    std::vector<VassalInfluence> influenced_vassals;  // Our vassals under foreign influence
    std::vector<VassalInfluence> foreign_vassals;     // Other realm's vassals we influence

    // Character-specific influences
    std::vector<CharacterInfluence> influenced_characters;

    // Sphere of influence metrics
    double sphere_size = 0.0;               // Total influenced realms
    double sphere_strength = 0.0;           // Average influence strength
    std::vector<types::EntityID> core_sphere;       // Fully dominated
    std::vector<types::EntityID> peripheral_sphere;  // Partial influence
    std::vector<types::EntityID> contested_sphere;   // Competed over

    // Conflicts
    std::vector<InfluenceConflict> sphere_conflicts;

    InfluenceComponent() = default;
    explicit InfluenceComponent(types::EntityID realm)
        : realm_id(realm)
        , incoming_influence(realm)
    {}

    std::string GetComponentTypeName() const override {
        return "InfluenceComponent";
    }

    // Helper methods
    void AddInfluenceSource(const InfluenceSource& source);
    void RemoveInfluenceSource(types::EntityID source_realm, InfluenceType type);

    void UpdateSphereMetrics();
    void UpdateInfluencedRealms();

    // Query
    double GetProjectionStrength(InfluenceType type) const;
    const InfluenceState* GetInfluenceOn(types::EntityID target) const;

    // Serialization
    std::string Serialize() const override;
    bool Deserialize(const std::string& data) override;
};

} // namespace diplomacy
} // namespace game
