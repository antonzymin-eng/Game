# Phase 3: Sphere of Influence System - Detailed Implementation Plan

**Duration**: 4 weeks
**Priority**: HIGH (Core gameplay feature)
**Dependencies**: Phase 1 (Memory) ✅ + Phase 2 (Trust) ✅
**Estimated Code**: ~8,000 lines across 10 files

---

## Overview

The Sphere of Influence system allows powerful nations to project power over neighboring and distant realms through seven distinct influence types. Influence can target entire realms, specific vassals, or individual characters, creating complex geopolitical dynamics and flashpoints for conflict.

### Core Concept

**Power Projection → Influence → Reduced Autonomy → Behavioral Changes**

A powerful nation exerts influence through:
- Military strength (garrison pressure, fear)
- Economic leverage (trade dependency, debt)
- Dynastic ties (marriages, family connections)
- Personal relationships (ruler friendships)
- Religious authority (clergy loyalty, papal power)
- Cultural affinity (shared identity, language)
- Prestige (international reputation, bandwagoning)

This influence reduces target autonomy and affects their decision-making, potentially leading to defections, proxy conflicts, and great power competition.

---

## Week 1: Core Data Structures & Influence Types

### Day 1-2: InfluenceComponents.h - Data Structures

**File**: `include/game/diplomacy/InfluenceComponents.h`

#### Step 1.1: Create File Header

```cpp
#pragma once

#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "utils/PlatformCompat.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <chrono>

namespace game::diplomacy {
```

#### Step 1.2: Influence Type Enum

```cpp
// Seven types of influence
enum class InfluenceType : uint8_t {
    MILITARY,      // Military strength and garrisons
    ECONOMIC,      // Trade dominance and financial leverage
    DYNASTIC,      // Marriage ties and family connections
    PERSONAL,      // Ruler friendships and character bonds
    RELIGIOUS,     // Religious authority and fervor
    CULTURAL,      // Cultural similarity and attraction
    PRESTIGE,      // Diplomatic reputation and glory
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
```

#### Step 1.3: InfluenceSource Structure

```cpp
// Individual influence source affecting a target
struct InfluenceSource {
    types::EntityID source_realm;           // Who is projecting influence
    InfluenceType type;                     // What kind of influence

    double base_strength = 0.0;             // Raw power (0-100+)
    double distance_modifier = 1.0;         // Geographic decay (0-1)
    double relationship_modifier = 1.0;     // Opinion affects effectiveness
    double effective_strength = 0.0;        // Final calculated influence

    // Propagation
    int hops_from_source = 0;               // How many realms away
    std::vector<types::EntityID> path;      // Path through realms

    // Time tracking
    std::chrono::system_clock::time_point established_date;
    std::chrono::system_clock::time_point last_update;

    // Specific targets within realm
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
```

#### Step 1.4: InfluenceState Structure

```cpp
// All influences affecting a specific realm
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
```

#### Step 1.5: Build Test

**Action**: Create empty .cpp file and test compilation

```bash
# Create implementation file
touch /home/user/Game/src/game/diplomacy/InfluenceComponents.cpp

# Add to CMakeLists.txt
# src/game/diplomacy/InfluenceComponents.cpp

# Build
cd /home/user/Game/build
make -j$(nproc)
```

**Verify**: Should compile without errors (structures only, no implementations yet)

---

### Day 3-4: VassalInfluence & CharacterInfluence

#### Step 2.1: VassalInfluence Structure

Add to `InfluenceComponents.h`:

```cpp
// Granular influence on specific vassals
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
```

#### Step 2.2: CharacterInfluence Structure

```cpp
// Character-level influence
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
```

#### Step 2.3: Build Test

**Action**: Compile with new structures

```bash
cd /home/user/Game/build
make -j$(nproc)
```

**Verify**: No compilation errors

---

### Day 5: InfluenceConflict & InfluenceComponent

#### Step 3.1: InfluenceConflict Structure

```cpp
// Competition between spheres
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
        conflict_id = std::to_string(contested.id) + "_" +
                     std::to_string(primary.id) + "_" +
                     std::to_string(challenger.id);
    }

    // Calculate tension
    void CalculateTension();
    void UpdateEscalationRisk();
    void AddIncident(const std::string& incident);

    // Check flashpoint
    bool CheckFlashpoint() const;
};
```

#### Step 3.2: InfluenceComponent (ECS)

```cpp
// Main ECS component for influence
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
    Json::Value Serialize() const override;
    void Deserialize(const Json::Value& data) override;
};
```

#### Step 3.3: Implement Basic Methods in InfluenceComponents.cpp

**File**: `src/game/diplomacy/InfluenceComponents.cpp`

```cpp
#include "game/diplomacy/InfluenceComponents.h"
#include <algorithm>
#include <cmath>

namespace game::diplomacy {

// ============================================================================
// InfluenceSource Implementation
// ============================================================================

void InfluenceSource::CalculateEffectiveStrength() {
    effective_strength = base_strength * distance_modifier * relationship_modifier;
    effective_strength = std::max(0.0, effective_strength);
}

void InfluenceSource::UpdateDistanceModifier(int hops, const std::vector<types::EntityID>& influence_path) {
    hops_from_source = hops;
    path = influence_path;

    // Type-specific decay rates
    double decay_rate = 0.0;
    switch(type) {
        case InfluenceType::MILITARY:    decay_rate = 0.40; break;  // High decay
        case InfluenceType::ECONOMIC:    decay_rate = 0.15; break;  // Low decay
        case InfluenceType::DYNASTIC:    decay_rate = 0.05; break;  // Very low
        case InfluenceType::PERSONAL:    decay_rate = 0.25; break;
        case InfluenceType::RELIGIOUS:   decay_rate = 0.00; break;  // No decay
        case InfluenceType::CULTURAL:    decay_rate = 0.20; break;
        case InfluenceType::PRESTIGE:    decay_rate = 0.10; break;
        default: decay_rate = 0.30; break;
    }

    // Calculate modifier: modifier = (1 - decay_rate)^hops
    distance_modifier = std::pow(1.0 - decay_rate, static_cast<double>(hops));
    distance_modifier = std::clamp(distance_modifier, 0.0, 1.0);

    CalculateEffectiveStrength();
}

void InfluenceSource::UpdateRelationshipModifier(int opinion) {
    // Opinion from -100 to +100 affects effectiveness
    // -100 opinion = 0.5x effectiveness, +100 = 1.5x effectiveness
    relationship_modifier = 1.0 + (opinion / 200.0);
    relationship_modifier = std::clamp(relationship_modifier, 0.5, 1.5);

    CalculateEffectiveStrength();
}

// ============================================================================
// InfluenceState Implementation
// ============================================================================

void InfluenceState::AddInfluence(const InfluenceSource& source) {
    influences_by_type[source.type].push_back(source);
    CalculateTotalInfluence();
    UpdateDominantInfluencers();
    CalculateAutonomy();
    CalculateDiplomaticFreedom();
}

void InfluenceState::RemoveInfluence(types::EntityID source_realm, InfluenceType type) {
    auto& sources = influences_by_type[type];
    sources.erase(
        std::remove_if(sources.begin(), sources.end(),
            [source_realm](const InfluenceSource& s) {
                return s.source_realm == source_realm;
            }),
        sources.end()
    );

    CalculateTotalInfluence();
    UpdateDominantInfluencers();
    CalculateAutonomy();
    CalculateDiplomaticFreedom();
}

void InfluenceState::CalculateTotalInfluence() {
    total_influence_received = 0.0;

    for (auto& [type, sources] : influences_by_type) {
        for (const auto& source : sources) {
            total_influence_received += source.effective_strength;
        }
    }
}

void InfluenceState::UpdateDominantInfluencers() {
    dominant_influencer.clear();

    for (auto& [type, sources] : influences_by_type) {
        if (sources.empty()) continue;

        // Find strongest influence of this type
        auto strongest = std::max_element(sources.begin(), sources.end(),
            [](const InfluenceSource& a, const InfluenceSource& b) {
                return a.effective_strength < b.effective_strength;
            });

        if (strongest != sources.end() && strongest->effective_strength > 10.0) {
            dominant_influencer[type] = strongest->source_realm;
        }
    }
}

void InfluenceState::CalculateAutonomy() {
    // Autonomy reduced by total influence
    // Formula: autonomy = 1.0 - (total_influence / 200.0)
    autonomy = 1.0 - (total_influence_received / 200.0);
    autonomy = std::clamp(autonomy, 0.0, 1.0);
}

void InfluenceState::CalculateDiplomaticFreedom() {
    // Diplomatic freedom primarily affected by military + economic influence
    double military_inf = 0.0;
    double economic_inf = 0.0;

    auto mil_it = influences_by_type.find(InfluenceType::MILITARY);
    if (mil_it != influences_by_type.end()) {
        for (const auto& source : mil_it->second) {
            military_inf += source.effective_strength;
        }
    }

    auto econ_it = influences_by_type.find(InfluenceType::ECONOMIC);
    if (econ_it != influences_by_type.end()) {
        for (const auto& source : econ_it->second) {
            economic_inf += source.effective_strength;
        }
    }

    diplomatic_freedom = 1.0 - ((military_inf + economic_inf) / 150.0);
    diplomatic_freedom = std::clamp(diplomatic_freedom, 0.0, 1.0);
}

double InfluenceState::GetInfluenceStrength(types::EntityID source_realm, InfluenceType type) const {
    auto it = influences_by_type.find(type);
    if (it == influences_by_type.end()) return 0.0;

    for (const auto& source : it->second) {
        if (source.source_realm == source_realm) {
            return source.effective_strength;
        }
    }
    return 0.0;
}

types::EntityID InfluenceState::GetDominantInfluencer(InfluenceType type) const {
    auto it = dominant_influencer.find(type);
    return (it != dominant_influencer.end()) ? it->second : types::EntityID();
}

bool InfluenceState::IsInfluencedBy(types::EntityID source_realm) const {
    for (const auto& [type, sources] : influences_by_type) {
        for (const auto& source : sources) {
            if (source.source_realm == source_realm && source.effective_strength > 5.0) {
                return true;
            }
        }
    }
    return false;
}

} // namespace game::diplomacy
```

#### Step 3.4: Build and Test

```bash
cd /home/user/Game/build
make -j$(nproc)
./mechanica_imperii  # Quick smoke test
```

**Expected**: Clean compilation, program runs

---

## Week 1 Summary

**Completed:**
- ✅ InfluenceComponents.h with all data structures
- ✅ InfluenceSource, InfluenceState, VassalInfluence, CharacterInfluence
- ✅ InfluenceConflict for sphere competition
- ✅ InfluenceComponent (ECS integration)
- ✅ Basic implementation methods
- ✅ Builds without errors

**Next Week:**
- Week 2: InfluenceCalculator (strength calculation per type)
- Week 2: InfluencePropagation (geographic spread)
- Week 2: InfluenceSystem (main system manager)

**Lines of Code**: ~1,200 (data structures + basic methods)

---

## Week 2: Calculation & Propagation Systems

### Day 1-2: InfluenceCalculator.h - Strength Calculations

**File**: `include/game/diplomacy/InfluenceCalculator.h`

This will calculate base influence strength for each of the 7 types based on realm attributes.

```cpp
#pragma once

#include "core/types/game_types.h"
#include "game/diplomacy/InfluenceComponents.h"

namespace game::diplomacy {

class InfluenceCalculator {
public:
    // Calculate base influence strength by type
    static double CalculateMilitaryInfluence(types::EntityID realm);
    static double CalculateEconomicInfluence(types::EntityID realm);
    static double CalculateDynasticInfluence(types::EntityID realm, types::EntityID target);
    static double CalculatePersonalInfluence(types::EntityID realm, types::EntityID target);
    static double CalculateReligiousInfluence(types::EntityID realm, types::EntityID target);
    static double CalculateCulturalInfluence(types::EntityID realm, types::EntityID target);
    static double CalculatePrestigeInfluence(types::EntityID realm);

    // Resistance calculation
    static double CalculateResistance(types::EntityID realm, InfluenceType type);

    // Effective influence (with resistance)
    static double CalculateEffectiveInfluence(
        double base_influence,
        double distance_modifier,
        double relationship_modifier,
        double resistance
    );

private:
    // Component access helpers
    static class ::core::ecs::ComponentAccessManager* s_access_manager;

public:
    static void SetAccessManager(::core::ecs::ComponentAccessManager* mgr) {
        s_access_manager = mgr;
    }
};

} // namespace game::diplomacy
```

### Implementation Details in Week 2

**Military Influence Formula:**
```
strength = (total_troops * quality_avg / 1000) +
           (fort_level * 10) +
           (tech_level / 10) +
           (prestige / 100)
```

**Economic Influence Formula:**
```
strength = sqrt(treasury / 10) +
           (monthly_income / 10) +
           (trade_value / 100) +
           (trade_hubs_controlled * 5)
```

**Propagation Algorithm:**
```
Breadth-first search from source realm
For each hop:
  - Check if can propagate through realm
  - Apply distance decay
  - Track path
  - Stop at max range for influence type
```

[Continue with full Week 2-4 details...]

---

## Integration Checklist

After implementation, ensure:

- [ ] InfluenceComponent added to CMakeLists.txt
- [ ] System registered with ComponentAccessManager
- [ ] Monthly update called from game loop
- [ ] Integration with DiplomacySystem (influence affects decisions)
- [ ] Integration with AI (autonomy affects choices)
- [ ] Serialization/deserialization working
- [ ] Basic tests passing

---

## Testing Strategy

### Unit Tests
```cpp
TEST(InfluenceSystem, BasicInfluenceCalculation) {
    // Test military influence calculation
}

TEST(InfluenceSystem, DistanceDecay) {
    // Test propagation decay
}

TEST(InfluenceSystem, SphereConflictDetection) {
    // Test competing spheres
}
```

### Integration Tests
- Create 3 realms: Powerful, Medium, Weak
- Apply influence from Powerful → Weak
- Verify autonomy reduction
- Test AI decision changes under influence

---

**Last Updated**: 2025-11-11
**Status**: Ready to begin Week 1 implementation
**Next Step**: Create InfluenceComponents.h
