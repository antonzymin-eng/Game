# DiplomacySystem and InfluenceSystem Integration Analysis

## Overview
The game implements a sophisticated dual-system architecture for diplomatic relations and sphere of influence. These systems are designed to work together but have clear separation of concerns with defined integration points.

---

## 1. DiplomacySystem - Class Structure and Architecture

### Location
- **Header**: `/home/user/Game/include/game/diplomacy/DiplomacySystem.h`
- **Implementation**: `/home/user/Game/src/game/diplomacy/DiplomacySystem_minimal.cpp`

### Core Responsibilities
1. **Diplomatic relationships**: Track relationships between all realms
2. **Treaties and agreements**: Manage alliances, non-aggression pacts, trade agreements, marriages
3. **Diplomatic actions**: Propose alliances, declare war, sue for peace, arrange marriages
4. **War management**: Process war declarations, handle ally activation, negotiate peace
5. **AI diplomacy**: Generate diplomatic actions for AI-controlled realms
6. **Prestige system**: Calculate prestige gains/losses from diplomatic actions

### Key Methods

#### Query Methods (Public Interface)
```cpp
// Core relationships
DiplomaticRelation GetRelation(types::EntityID realm_a, types::EntityID realm_b) const;
int GetOpinion(types::EntityID realm_a, types::EntityID realm_b) const;
double GetPrestige(types::EntityID realm_id) const;
bool AreAtWar(types::EntityID realm_a, types::EntityID realm_b) const;

// Diplomatic state - CRITICAL FOR INFLUENCE SYSTEM INTEGRATION
const DiplomaticState* GetDiplomaticState(types::EntityID realm_a, 
                                          types::EntityID realm_b) const;

// Realm queries
std::vector<types::EntityID> GetAllRealms() const;
std::vector<types::EntityID> GetNeighboringRealms(types::EntityID realm_id) const;
std::vector<types::EntityID> GetPotentialAllies(types::EntityID realm_id) const;
std::vector<types::EntityID> GetPotentialEnemies(types::EntityID realm_id) const;
```

#### Action Methods (Private/Helper)
```cpp
bool ProposeAlliance(types::EntityID proposer, types::EntityID target, ...);
bool ProposeTradeAgreement(types::EntityID proposer, types::EntityID target, ...);
bool DeclareWar(types::EntityID aggressor, types::EntityID target, CasusBelli cb);
bool ArrangeMarriage(types::EntityID bride_realm, types::EntityID groom_realm, ...);
bool EstablishEmbassy(types::EntityID sender, types::EntityID host);
void SendDiplomaticGift(types::EntityID sender, types::EntityID recipient, double value);
```

#### Processing Methods
```cpp
void UpdateDiplomaticRelationships(types::EntityID realm_id);
void ProcessDiplomaticDecay(types::EntityID realm_id, float time_delta);
void CalculatePrestigeEffects(types::EntityID realm_id);
void ProcessAIDiplomacy(types::EntityID realm_id);
double EvaluateProposal(const DiplomaticProposal& proposal);
void GenerateAIDiplomaticActions(types::EntityID realm_id);
```

---

## 2. Opinion System and Modifiers

### DiplomaticState Structure
Located in `/home/user/Game/include/game/diplomacy/DiplomacyComponents.h`

```cpp
struct DiplomaticState {
    // Basic opinion tracking
    int opinion = 0;                              // Range: -100 to +100
    double trust = 0.5;                           // Range: 0.0 to 1.0
    double prestige_difference = 0.0;
    
    // Opinion modifiers - ENHANCED MEMORY SYSTEM
    struct OpinionModifier {
        std::string source;                       // What caused this modifier
        int value;                                // Opinion impact
        double weight = 1.0;                      // Current weight (decays)
        bool is_permanent = false;
        std::chrono::system_clock::time_point created;
        
        int GetCurrentValue() const {
            return is_permanent ? value : static_cast<int>(value * weight);
        }
    };
    
    std::vector<OpinionModifier> opinion_modifiers;
    
    // Historical tracking for long-term memory
    std::deque<int> opinion_history;              // Last 12 monthly values
    double historical_opinion_average = 0.0;      // Rolling average
    
    struct HistoricalOpinionData {
        std::deque<int> monthly_opinions;         // Last 120 months (10 years)
        std::deque<int> yearly_opinions;          // Last 100 years
        double short_term_average = 0.0;          // 1 year average
        double medium_term_average = 0.0;         // 10 year average
        double long_term_average = 0.0;           // 50+ year average
        int highest_ever = 0;
        int lowest_ever = 0;
    };
    
    HistoricalOpinionData historical_data;
    
    // Cooldowns to prevent spam
    std::unordered_map<DiplomaticAction, 
                      std::chrono::system_clock::time_point> action_cooldowns;
    std::chrono::system_clock::time_point last_major_action;
};
```

### Opinion Modifier Methods
Located in `/home/user/Game/src/game/diplomacy/DiplomacyComponents.cpp`

#### Adding/Removing Modifiers
```cpp
void DiplomaticState::AddOpinionModifier(const std::string& source, 
                                         int value, 
                                         bool permanent = false)
{
    // Check if modifier exists, update or create new
    auto it = std::find_if(opinion_modifiers.begin(), opinion_modifiers.end(),
        [&source](const OpinionModifier& mod) { return mod.source == source; });
    
    if (it != opinion_modifiers.end()) {
        it->value = value;
        it->weight = 1.0;
        it->is_permanent = permanent;
        it->created = std::chrono::system_clock::now();
    } else {
        // Create new modifier
        OpinionModifier modifier{source, value, 1.0, permanent, 
                                std::chrono::system_clock::now()};
        opinion_modifiers.push_back(modifier);
    }
}

void DiplomaticState::RemoveOpinionModifier(const std::string& source) {
    // Erase all modifiers with matching source
    opinion_modifiers.erase(
        std::remove_if(opinion_modifiers.begin(), opinion_modifiers.end(),
            [&source](const OpinionModifier& mod) { return mod.source == source; }),
        opinion_modifiers.end());
}
```

#### Calculating Total Opinion
```cpp
int DiplomaticState::CalculateTotalOpinion() const {
    int total = 0;
    for (const auto& modifier : opinion_modifiers) {
        total += modifier.GetCurrentValue();  // Uses weight for decay
    }
    return total;
}
```

#### Opinion Decay and Modifier Decay
```cpp
// Passive decay toward baseline
void DiplomaticState::ApplyOpinionDecay(float time_delta, int neutral_baseline = 0) {
    double decay_rate = config.GetDouble("diplomacy.opinion_decay_rate", 0.1) * time_delta;
    
    if (opinion > neutral_baseline) {
        opinion = std::max(neutral_baseline, opinion - static_cast<int>(decay_rate));
    } else if (opinion < neutral_baseline) {
        opinion = std::min(neutral_baseline, opinion + static_cast<int>(decay_rate));
    }
}

// Modifier weight decay - exponential
void DiplomaticState::ApplyModifierDecay(float months_elapsed = 1.0f) {
    for (auto& modifier : opinion_modifiers) {
        if (!modifier.is_permanent) {
            modifier.weight *= std::pow(0.95, months_elapsed);  // 5% decay per month
            
            if (modifier.weight < 0.05) {
                modifier.weight = 0.0;
            }
        }
    }
    
    // Remove fully decayed modifiers
    opinion_modifiers.erase(
        std::remove_if(opinion_modifiers.begin(), opinion_modifiers.end(),
            [](const OpinionModifier& mod) { return !mod.is_permanent && mod.weight <= 0.0; }),
        opinion_modifiers.end());
}

// Trust decay toward baseline
void DiplomaticState::ApplyTrustDecay(float time_delta, double neutral_baseline = 0.5) {
    double decay_rate = config.GetDouble("diplomacy.trust_decay_rate", 0.01) * time_delta;
    
    if (trust > neutral_baseline) {
        trust = std::max(neutral_baseline, trust - decay_rate);
    } else if (trust < neutral_baseline) {
        trust = std::min(neutral_baseline, trust + decay_rate);
    }
}
```

#### Historical Opinion Tracking
```cpp
void DiplomaticState::UpdateHistoricalData(int current_opinion, 
                                          bool is_monthly = false, 
                                          bool is_yearly = false)
{
    if (is_monthly) {
        historical_data.monthly_opinions.push_back(current_opinion);
        if (historical_data.monthly_opinions.size() > 120) {
            historical_data.monthly_opinions.pop_front();
        }
        
        // Calculate short-term average (1 year = last 12 months)
        size_t count = std::min(historical_data.monthly_opinions.size(), size_t(12));
        double sum = 0.0;
        for (size_t i = historical_data.monthly_opinions.size() - count;
             i < historical_data.monthly_opinions.size(); ++i) {
            sum += historical_data.monthly_opinions[i];
        }
        historical_data.short_term_average = sum / count;
    }
    
    if (is_yearly) {
        historical_data.yearly_opinions.push_back(current_opinion);
        // ... calculate long-term averages
    }
    
    // Track extremes
    if (current_opinion > historical_data.highest_ever) {
        historical_data.highest_ever = current_opinion;
        historical_data.best_relations_date = std::chrono::system_clock::now();
    }
    if (current_opinion < historical_data.lowest_ever) {
        historical_data.lowest_ever = current_opinion;
        historical_data.worst_relations_date = std::chrono::system_clock::now();
    }
}
```

### DiplomacyComponent (ECS Component)
```cpp
struct DiplomacyComponent : public game::core::Component<DiplomacyComponent> {
    // Core relationships indexed by other realm ID
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
    double war_weariness = 0.0;  // 0.0 to 1.0
};
```

---

## 3. InfluenceSystem - Class Structure and Architecture

### Location
- **Header**: `/home/user/Game/include/game/diplomacy/InfluenceSystem.h`
- **Implementation**: `/home/user/Game/src/game/diplomacy/InfluenceSystem.cpp`

### Core Responsibilities
1. **Influence calculation**: Calculate 7 types of influence (Military, Economic, Dynastic, Personal, Religious, Cultural, Prestige)
2. **Influence propagation**: Spread influence through realm networks with distance decay
3. **Autonomy management**: Track how much independence each realm has
4. **Sphere conflicts**: Detect and manage competition over contested realms
5. **Vassal influence**: Track foreign influence on vassals
6. **Character influence**: Track compromised characters
7. **Integration**: Query and notify diplomacy system of changes

### Key Methods

#### Initialization and Updates
```cpp
void Initialize();                              // Initialize after realm/diplomacy setup
void MonthlyUpdate();                          // Monthly recalculation of all influences
void UpdateRealmInfluence(types::EntityID realm_id);  // Immediate recalculation
```

#### Influence Calculation
```cpp
void CalculateInfluenceProjection(types::EntityID realm_id);
InfluenceSource CalculateInfluenceBetween(
    types::EntityID source_realm,
    types::EntityID target_realm,
    InfluenceType type);
void ApplyModifiersToInfluence(
    InfluenceSource& influence,
    int hops,
    const std::vector<types::EntityID>& path,
    int opinion);
```

#### Influence Propagation
```cpp
void PropagateInfluence(types::EntityID source_realm);
std::vector<types::EntityID> FindPathBetweenRealms(
    types::EntityID source,
    types::EntityID target,
    int max_hops = 10);
std::vector<types::EntityID> GetRealmsWithinRange(
    types::EntityID source,
    int max_hops);
bool AreRealmsConnected(types::EntityID realm1, types::EntityID realm2);
```

#### Autonomy and Diplomatic Freedom - CRITICAL FOR INTEGRATION
```cpp
double GetRealmAutonomy(types::EntityID realm_id);
double GetRealmDiplomaticFreedom(types::EntityID realm_id);
void UpdateAutonomyAndFreedom();  // Called monthly
```

#### Sphere Management
```cpp
void UpdateSphereMetrics(types::EntityID realm_id);
std::vector<InfluenceConflict> DetectSphereConflicts(types::EntityID realm_id);
std::vector<InfluenceConflict> CheckForFlashpoints();
void UpdateSphereConflicts();
void ProcessConflictEscalation();
void ResolveSphereConflict(InfluenceConflict& conflict);
```

#### Vassal and Character Influence
```cpp
void UpdateVassalInfluences(types::EntityID realm_id);
void UpdateCharacterInfluences(types::EntityID realm_id);
bool IsVassalAtRiskOfDefection(const VassalInfluence& vassal_influence);
bool IsCharacterCompromised(const CharacterInfluence& character_influence);
```

#### Integration Methods - CRITICAL INTEGRATION POINTS
```cpp
void SetDiplomacySystem(DiplomacySystem* diplomacy_system);
const DiplomaticState* GetDiplomaticState(
    types::EntityID realm1,
    types::EntityID realm2);
void NotifyInfluenceChange(types::EntityID realm_id);
```

### Seven Types of Influence

Calculated by `InfluenceCalculator` (see Section 4), each with different:
- **Range**: How far influence propagates
- **Decay Rate**: How quickly it diminishes with distance
- **Modifiers**: What affects its strength

1. **Military** (2-4 hop range, 40% decay/hop)
   - Based on army size, military tech, prestige, fortifications
   - Most affected by diplomatic opinion
   
2. **Economic** (5-8 hop range, 15% decay/hop)
   - Based on wealth, trade volume, economic dependency
   - Less affected by opinion (50% modifier)
   
3. **Dynastic** (Unlimited range, 5% decay/hop)
   - Based on marriage ties, family connections
   - Transcends distance
   
4. **Personal** (3-5 hop range, 25% decay/hop)
   - Based on ruler friendships, character bonds
   - Based on diplomatic opinion
   
5. **Religious** (Unlimited for same faith, no decay)
   - Based on religious authority and fervor
   - Same faith bonus applies
   
6. **Cultural** (4-6 hop range, 20% decay/hop)
   - Based on cultural similarity and attraction
   
7. **Prestige** (Global range, 10% decay/hop)
   - Based on diplomatic reputation, glory, victories

### Autonomy Calculation

Located in `/home/user/Game/src/game/diplomacy/InfluenceComponents.cpp`:

```cpp
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
    
    // Sum military influences
    auto mil_it = influences_by_type.find(InfluenceType::MILITARY);
    if (mil_it != influences_by_type.end()) {
        for (const auto& source : mil_it->second) {
            military_inf += source.effective_strength;
        }
    }
    
    // Sum economic influences
    auto econ_it = influences_by_type.find(InfluenceType::ECONOMIC);
    if (econ_it != influences_by_type.end()) {
        for (const auto& source : econ_it->second) {
            economic_inf += source.effective_strength;
        }
    }
    
    // Military and economic influence reduce diplomatic freedom
    diplomatic_freedom = 1.0 - ((military_inf + economic_inf) / 150.0);
    diplomatic_freedom = std::clamp(diplomatic_freedom, 0.0, 1.0);
}
```

### InfluenceComponent (ECS Component)

Located in `/home/user/Game/include/game/diplomacy/InfluenceComponents.h`:

```cpp
struct InfluenceComponent : public game::core::Component<InfluenceComponent> {
    types::EntityID realm_id;
    
    // Influence this realm projects outward
    std::unordered_map<InfluenceType, double> influence_projection;
    std::unordered_map<types::EntityID, InfluenceState> influenced_realms;
    
    // Influence this realm receives from others
    InfluenceState incoming_influence;
    
    // Vassal-specific influences
    std::vector<VassalInfluence> influenced_vassals;
    std::vector<VassalInfluence> foreign_vassals;
    
    // Character-specific influences
    std::vector<CharacterInfluence> influenced_characters;
    
    // Sphere of influence metrics
    double sphere_size = 0.0;
    double sphere_strength = 0.0;
    std::vector<types::EntityID> core_sphere;           // Fully dominated
    std::vector<types::EntityID> peripheral_sphere;     // Partial influence
    std::vector<types::EntityID> contested_sphere;      // Competed over
    
    // Conflicts
    std::vector<InfluenceConflict> sphere_conflicts;
};
```

### InfluenceSource Structure

Individual influence projections between realms:

```cpp
struct InfluenceSource {
    types::EntityID source_realm;
    InfluenceType type;
    
    double base_strength = 0.0;                 // Raw power (0-100+)
    double distance_modifier = 1.0;             // Geographic decay (0-1)
    double relationship_modifier = 1.0;         // Opinion affects effectiveness
    double effective_strength = 0.0;            // Final = base * distance * relationship
    
    // Propagation tracking
    int hops_from_source = 0;
    std::vector<types::EntityID> path;
    
    // Time tracking
    std::chrono::system_clock::time_point established_date;
    std::chrono::system_clock::time_point last_update;
    
    // Granular targeting
    std::vector<types::EntityID> targeted_vassals;
    std::vector<types::EntityID> targeted_characters;
    bool targets_whole_realm = true;
};
```

#### Distance Modifier Calculation

```cpp
void InfluenceSource::UpdateDistanceModifier(int hops, 
                                            const std::vector<types::EntityID>& influence_path) {
    hops_from_source = hops;
    path = influence_path;
    
    // Type-specific decay rates
    double decay_rate = 0.0;
    switch(type) {
        case InfluenceType::MILITARY:    decay_rate = 0.40; break;
        case InfluenceType::ECONOMIC:    decay_rate = 0.15; break;
        case InfluenceType::DYNASTIC:    decay_rate = 0.05; break;
        case InfluenceType::PERSONAL:    decay_rate = 0.25; break;
        case InfluenceType::RELIGIOUS:   decay_rate = 0.00; break;
        case InfluenceType::CULTURAL:    decay_rate = 0.20; break;
        case InfluenceType::PRESTIGE:    decay_rate = 0.10; break;
        default: decay_rate = 0.30; break;
    }
    
    // modifier = (1 - decay_rate)^hops
    distance_modifier = std::pow(1.0 - decay_rate, static_cast<double>(hops));
    distance_modifier = std::clamp(distance_modifier, 0.0, 1.0);
    
    CalculateEffectiveStrength();
}
```

#### Relationship Modifier Calculation

```cpp
void InfluenceSource::UpdateRelationshipModifier(int opinion) {
    // Opinion from -100 to +100 affects effectiveness
    // -100 opinion = 0.5x effectiveness, +100 = 1.5x effectiveness
    relationship_modifier = 1.0 + (opinion / 200.0);
    relationship_modifier = std::clamp(relationship_modifier, 0.5, 1.5);
    
    CalculateEffectiveStrength();
}
```

---

## 4. InfluenceCalculator - Pure Calculation Functions

### Location
- **Header**: `/home/user/Game/include/game/diplomacy/InfluenceCalculator.h`
- **Implementation**: `/home/user/Game/src/game/diplomacy/InfluenceCalculator.cpp`

### Purpose
All-static class providing pure calculation functions with no side effects. Calculates base influence projections before modifiers.

### Main Influence Calculation Functions
```cpp
class InfluenceCalculator {
    // All functions are static

    // Main calculators
    static double CalculateMilitaryInfluence(
        const realm::RealmComponent& realm,
        const DiplomaticState* diplo_state = nullptr);
    
    static double CalculateEconomicInfluence(
        const realm::RealmComponent& realm,
        const DiplomaticState* diplo_state = nullptr);
    
    static double CalculateDynasticInfluence(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm,
        const realm::DynastyComponent* source_dynasty = nullptr,
        const realm::DynastyComponent* target_dynasty = nullptr);
    
    static double CalculatePersonalInfluence(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm,
        const DiplomaticState* diplo_state = nullptr);
    
    static double CalculateReligiousInfluence(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm);
    
    static double CalculateCulturalInfluence(
        const realm::RealmComponent& source_realm,
        const realm::RealmComponent& target_realm);
    
    static double CalculatePrestigeInfluence(
        const realm::RealmComponent& realm,
        const realm::DynastyComponent* dynasty = nullptr);
};
```

### Component Calculations (Examples)

```cpp
// Military components
static double CalculateMilitaryStrength(const realm::RealmComponent& realm);
static double CalculateMilitaryTechBonus(const realm::RealmComponent& realm);
static double CalculateMilitaryPrestigeBonus(const realm::RealmComponent& realm);

// Economic components
static double CalculateWealthScore(const realm::RealmComponent& realm);
static double CalculateTradeDominance(
    const realm::RealmComponent& realm,
    const DiplomaticState* diplo_state);
static double CalculateTradeHubBonus(const realm::RealmComponent& realm);

// And many more...
```

### Key Utility Functions

```cpp
// Apply relationship/opinion modifier to any influence
static double ApplyRelationshipModifier(double base_influence, int opinion) {
    // Opinion from -100 to +100
    // Positive opinion boosts influence, negative reduces it
}

// Calculate geographic decay
static double CalculateGeographicDecay(int hops, InfluenceType type) {
    // Type-specific decay based on hops (handled by InfluenceSource)
}

// Normalize value to 0-100 range
static double NormalizeInfluence(double raw_value, double max_value = 100.0);

// Check neighborhoodrelation
static bool AreNeighbors(
    const realm::RealmComponent& realm1,
    const realm::RealmComponent& realm2);
```

---

## 5. Existing Integration Points Between Systems

### 1. **DiplomaticState Query** - CRITICAL INTEGRATION POINT

The `InfluenceSystem` uses `DiplomacySystem::GetDiplomaticState()` to access opinion data:

**From InfluenceSystem.cpp (Line 136):**
```cpp
const auto* diplo_state = GetDiplomaticState(source_realm, target_realm);
```

**Implementation in InfluenceSystem (Lines 843-850):**
```cpp
const DiplomaticState* InfluenceSystem::GetDiplomaticState(
    types::EntityID realm1,
    types::EntityID realm2)
{
    if (!m_diplomacy_system) return nullptr;
    
    return m_diplomacy_system->GetDiplomaticState(realm1, realm2);
}
```

**How it's used in calculations:**
```cpp
// In CalculateInfluenceBetween (InfluenceSystem.cpp, lines 123-150)
const auto* diplo_state = GetDiplomaticState(source_realm, target_realm);

// Base strength calculated from realm stats
influence.base_strength = InfluenceCalculator::CalculateMilitaryInfluence(*source, diplo_state);

// Then modifiers applied with opinion
ApplyModifiersToInfluence(influence, hops, path, diplo_state->opinion);
```

### 2. **System Reference** - INITIALIZATION INTEGRATION

**InfluenceSystem.h (Line 265):**
```cpp
void SetDiplomacySystem(DiplomacySystem* diplomacy_system);
```

**InfluenceSystem.cpp (Lines 839-841):**
```cpp
void InfluenceSystem::SetDiplomacySystem(DiplomacySystem* diplomacy_system) {
    m_diplomacy_system = diplomacy_system;
}
```

This allows InfluenceSystem to query the DiplomacySystem's state.

### 3. **Notification System** - EVENT INTEGRATION

**InfluenceSystem.h (Lines 279):**
```cpp
void NotifyInfluenceChange(types::EntityID realm_id);
```

**InfluenceSystem.cpp (Lines 852-871):**
```cpp
void InfluenceSystem::NotifyInfluenceChange(types::EntityID realm_id) {
    // Notify diplomacy system if influence has significantly changed
    // Could trigger events like "losing independence" or "falling under sphere"
    
    auto* component = GetInfluenceComponent(realm_id);
    if (!component) return;
    
    double autonomy = component->incoming_influence.autonomy;
    double freedom = component->incoming_influence.diplomatic_freedom;
    
    // TODO: Trigger events when thresholds are crossed
    if (autonomy < 0.5) {
        // Realm is losing independence
    }
    
    if (freedom < 0.3) {
        // Realm has very limited diplomatic freedom
    }
}
```

### 4. **Autonomy and Diplomatic Freedom** - STATE DEPENDENT INTEGRATION

**How Autonomy Affects Diplomacy:**
- Realms with low autonomy (< 0.5) are losing independence
- Realms with low diplomatic freedom (< 0.3) have very limited choices
- These should affect:
  - Ability to declare war
  - Acceptance of diplomatic proposals
  - Rebellion/defection rates
  - Treaty compliance

### 5. **Opinion Modifiers in Influence** - BIDIRECTIONAL

**From InfluenceCalculator.cpp (Line 49):**
```cpp
double InfluenceCalculator::CalculateEconomicInfluence(...) {
    // ... calculate base influence ...
    
    // Economic influence less affected by opinion
    if (diplo_state) {
        // Only apply 50% of relationship modifier for economic influence
        double modifier = ApplyRelationshipModifier(1.0, diplo_state->opinion);
        total *= (0.5 + 0.5 * modifier);
    }
    
    return NormalizeInfluence(total, 100.0);
}
```

**Opinion affects influence projection:**
- Negative opinion reduces influence effectiveness
- Positive opinion enhances influence effectiveness
- Different influence types have different opinion sensitivity

### 6. **Monthly Update Cycle** - SYNCHRONIZATION INTEGRATION

**InfluenceSystem::MonthlyUpdate() (Lines 50-88):**
```cpp
void InfluenceSystem::MonthlyUpdate() {
    m_current_month++;
    
    // 1. Rebuild realm network (realms may have changed)
    BuildRealmNetwork();
    
    // 2. Update all realm influence projections
    for (auto& [realm_id, component] : m_influence_components) {
        CalculateInfluenceProjection(realm_id);
    }
    
    // 3. Propagate influence through network
    for (auto& [realm_id, component] : m_influence_components) {
        PropagateInfluence(realm_id);
    }
    
    // 4. Update sphere metrics
    for (auto& [realm_id, component] : m_influence_components) {
        UpdateSphereMetrics(realm_id);
    }
    
    // 5. Update vassal and character influences
    for (auto& [realm_id, component] : m_influence_components) {
        UpdateVassalInfluences(realm_id);
        UpdateCharacterInfluences(realm_id);
    }
    
    // 6. Process influence decay
    ProcessInfluenceDecay();
    
    // 7. Update autonomy and diplomatic freedom
    UpdateAutonomyAndFreedom();
    
    // 8. Detect and update conflicts
    UpdateSphereConflicts();
    
    // 9. Check for flashpoints
    CheckForFlashpoints();
}
```

The `InfluenceSystem` should call this in sync with `DiplomacySystem` updates.

---

## 6. Bridge System - DiplomacyEconomicBridge

### Location
- **Header**: `/home/user/Game/include/game/bridge/DiplomacyEconomicBridge.h`
- **Implementation**: `/home/user/Game/src/game/bridge/DiplomacyEconomicBridge.cpp`

### Purpose
Manages the integration between diplomatic and economic systems. Shows the pattern for how systems should interact.

### Key Integration Points
1. **Trade impacts opinion**: Economic dependencies affect diplomatic relationships
2. **War disrupts trade**: Conflicts have economic consequences
3. **Sanctions affect opinion**: Economic punishment affects diplomacy
4. **Alliances boost trade**: Diplomatic ties improve economic relations

### Relevant Structures

```cpp
struct EconomicDependency {
    types::EntityID realm_id;
    types::EntityID trading_partner;
    double trade_dependency = 0.0;          // 0.0-1.0
    double overall_dependency = 0.0;        // Weighted average
    double vulnerability_to_disruption = 0.0;
};

struct Sanction {
    types::EntityID imposer;
    types::EntityID target;
    SanctionType type = SanctionType::TRADE_EMBARGO;
    int opinion_modifier = -50;              // DIPLOMACY IMPACT
    double prestige_cost = 10.0;
};
```

### Economic-to-Diplomacy Methods

```cpp
void AdjustOpinionBasedOnTrade(types::EntityID realm_a, types::EntityID realm_b);
void ProcessEconomicInfluenceOnRelations();
void OnTradeRouteDisrupted(types::EntityID source, types::EntityID destination, ...);
void OnEconomicCrisis(types::EntityID realm_id, const std::string& crisis_type, ...);
```

---

## 7. Autonomy System - Current Implementation

### Location
- **Calculation**: `/home/user/Game/src/game/diplomacy/InfluenceComponents.cpp` (Lines 114-142)
- **Access**: `/home/user/Game/src/game/diplomacy/InfluenceSystem.cpp` (Lines 807-819)

### Current Formula

```
Autonomy = 1.0 - (total_influence_received / 200.0)
           clamped to [0.0, 1.0]

Diplomatic Freedom = 1.0 - ((military_inf + economic_inf) / 150.0)
                    clamped to [0.0, 1.0]
```

### What Autonomy Currently Does

Based on the code, autonomy is:
1. **Calculated monthly** - recalculated every month based on influence
2. **Queried by InfluenceSystem** - used internally for sphere calculations
3. **Available to other systems** - accessible via `GetRealmAutonomy()`
4. **Affects vassal behavior** - influences vassal defection/revolt risk

### What Autonomy Should Do (Missing Implementations)

Based on the `NotifyInfluenceChange` TODO comments:
1. **Affect diplomatic proposal acceptance** - Low autonomy realms should accept more proposals
2. **Restrict military actions** - Very low autonomy realms can't declare war
3. **Trigger special events** - Autonomy thresholds could trigger events
4. **Affect treaty compliance** - Low autonomy realms more likely to violate treaties
5. **Impact on opinion changes** - Opinions might shift based on autonomy

---

## 8. Data Flow Diagram

### Monthly Update Cycle

```
DiplomacySystem::Update(delta_time)
    ↓
    ├─→ ProcessDiplomaticUpdates()
    │   └─→ UpdateDiplomaticRelationships()
    │       └─→ ApplyOpinionDecay()
    │       └─→ ApplyModifierDecay()
    │
    └─→ ProcessMonthlyDiplomacy()
        └─→ Various diplomatic actions

InfluenceSystem::MonthlyUpdate()
    ↓
    ├─→ BuildRealmNetwork()
    ├─→ CalculateInfluenceProjection() for each realm
    │   └─→ InfluenceCalculator::Calculate*Influence()
    │       └─→ Queries GetDiplomaticState() for opinion modifier
    │
    ├─→ PropagateInfluence() for each realm
    │   └─→ FindPathBetweenRealms()
    │   └─→ ApplyModifiersToInfluence()
    │       └─→ Uses opinion from DiplomaticState
    │
    ├─→ UpdateSphereMetrics()
    ├─→ UpdateVassalInfluences()
    ├─→ UpdateCharacterInfluences()
    ├─→ ProcessInfluenceDecay()
    ├─→ UpdateAutonomyAndFreedom()
    │   └─→ InfluenceState::CalculateAutonomy()
    ├─→ UpdateSphereConflicts()
    └─→ CheckForFlashpoints()
```

### Opinion Impact on Influence Calculation

```
DiplomaticState::opinion (range: -100 to +100)
    ↓
    ├─→ Used directly in InfluenceSource::UpdateRelationshipModifier()
    │   relationship_modifier = 1.0 + (opinion / 200.0)
    │   clamped to [0.5, 1.5]
    │
    └─→ Used in InfluenceCalculator::ApplyRelationshipModifier()
        Different application based on influence type:
        - Military: 100% modifier
        - Economic: 50% modifier
        - Others: varies
```

---

## 9. Key Data Structures Summary

### DiplomacySystem Side
- **DiplomacyComponent**: Per-realm diplomatic state
  - **DiplomaticState**: Per-relationship state with opinion
    - **OpinionModifier**: Individual contributors to opinion with decay
    - **HistoricalOpinionData**: Long-term memory of opinions
  - **Treaty**: Active treaties with compliance tracking
  - **DynasticMarriage**: Marriage alliances

### InfluenceSystem Side
- **InfluenceComponent**: Per-realm sphere of influence
  - **InfluenceState**: Incoming influences on a realm
    - **InfluenceSource**: Individual influence projections
      - Uses opinion modifier and distance modifier
  - **VassalInfluence**: Foreign influence on specific vassals
  - **CharacterInfluence**: Foreign influence on specific characters
  - **InfluenceConflict**: Contested sphere conflicts

### Bridge Connection
- DiplomacySystem holds opinion values
- InfluenceSystem queries those opinions
- InfluenceCalculator uses opinions as modifiers
- InfluenceSystem updates autonomy based on influence totals
- Autonomy should feed back into DiplomacySystem decision-making

---

## 10. Summary of Current Integration State

### What's Implemented
✓ Opinion storage with modifiers and decay
✓ Opinion affects influence projections (via relationship_modifier)
✓ Autonomy calculation from total influence
✓ Monthly update cycles that can be synchronized
✓ System reference pattern (SetDiplomacySystem)
✓ Query methods for cross-system access

### What's Partially Implemented
~ Autonomy affects diplomatic freedom (calculated, not fully used)
~ Notification system skeleton (NotifyInfluenceChange has TODOs)
~ Integration with bridge system (economic impacts diplomacy)

### What's Missing
✗ Autonomy affecting diplomatic decision-making
✗ Autonomy affecting treaty compliance
✗ Autonomy affecting proposal acceptance
✗ Autonomy-triggered events
✗ Bidirectional feedback loops
✗ Conflict escalation affecting opinion
✗ Opinion feedback affecting autonomy recalculation

---

## Conclusion

The DiplomacySystem and InfluenceSystem are well-structured with clear integration points. The systems are designed to work together through:

1. **Opinion as the bridge**: DiplomacySystem maintains opinion, InfluenceSystem uses it as a modifier
2. **Autonomy as feedback**: InfluenceSystem calculates autonomy, which should affect DiplomacySystem decisions
3. **Shared calculation patterns**: Both systems use similar delta_time and monthly update cycles
4. **Cross-system queries**: Defined methods for accessing each system's data

The main work needed is to:
1. Implement the missing autonomy feedback into DiplomacySystem
2. Add event triggers for autonomy thresholds
3. Create bidirectional feedback loops
4. Complete the NotifyInfluenceChange integration
5. Add tests to ensure opinion and autonomy calculations are synchronized

