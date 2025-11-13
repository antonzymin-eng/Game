# DiplomacySystem & InfluenceSystem - Quick Reference

## File Locations

### DiplomacySystem
- **Header**: `/home/user/Game/include/game/diplomacy/DiplomacySystem.h`
- **Implementation**: `/home/user/Game/src/game/diplomacy/DiplomacySystem_minimal.cpp`
- **Components**: `/home/user/Game/include/game/diplomacy/DiplomacyComponents.h`
- **Component Implementation**: `/home/user/Game/src/game/diplomacy/DiplomacyComponents.cpp`

### InfluenceSystem
- **Header**: `/home/user/Game/include/game/diplomacy/InfluenceSystem.h`
- **Implementation**: `/home/user/Game/src/game/diplomacy/InfluenceSystem.cpp`
- **Components**: `/home/user/Game/include/game/diplomacy/InfluenceComponents.h`
- **Component Implementation**: `/home/user/Game/src/game/diplomacy/InfluenceComponents.cpp`
- **Calculator**: `/home/user/Game/include/game/diplomacy/InfluenceCalculator.h`
- **Calculator Implementation**: `/home/user/Game/src/game/diplomacy/InfluenceCalculator.cpp`

### Bridge System
- **Header**: `/home/user/Game/include/game/bridge/DiplomacyEconomicBridge.h`
- **Implementation**: `/home/user/Game/src/game/bridge/DiplomacyEconomicBridge.cpp`

---

## Key Integration Points

### 1. Opinion → Influence Pipeline

```
DiplomaticState::opinion ([-100, +100])
    ↓
InfluenceSystem::GetDiplomaticState()
    ↓
InfluenceSource::UpdateRelationshipModifier(opinion)
    ↓
relationship_modifier = 1.0 + (opinion / 200.0)  // [0.5, 1.5]
    ↓
effective_strength = base * distance_mod * relationship_mod
```

**Key Files**:
- `DiplomacyComponents.cpp`: Lines 226-246 (ModifyOpinion)
- `InfluenceSystem.cpp`: Lines 123-150 (CalculateInfluenceBetween)
- `InfluenceComponents.cpp`: Lines 49-56 (UpdateRelationshipModifier)

### 2. Autonomy Calculation

```
Total Influence Received (sum of all InfluenceSource::effective_strength)
    ↓
autonomy = 1.0 - (total / 200.0)  // clamped to [0.0, 1.0]
diplomatic_freedom = 1.0 - ((military + economic) / 150.0)
```

**Key Files**:
- `InfluenceComponents.cpp`: Lines 114-142 (CalculateAutonomy, CalculateDiplomaticFreedom)
- `InfluenceSystem.cpp`: Lines 1007-1009 (UpdateAutonomyAndFreedom)

### 3. System Connection

```
InfluenceSystem::SetDiplomacySystem(DiplomacySystem* diplo)
    ↓
stores reference to diplomacy system
    ↓
uses it in GetDiplomaticState() calls
```

**Key Files**:
- `InfluenceSystem.h`: Line 265 (SetDiplomacySystem declaration)
- `InfluenceSystem.cpp`: Lines 839-850 (Implementation and usage)

---

## Opinion Modifier System

### Structure
```cpp
struct OpinionModifier {
    std::string source;              // e.g., "joint_alliance", "war_atrocity"
    int value;                       // e.g., +10, -25
    double weight = 1.0;             // Decays from 1.0 to 0.0
    bool is_permanent = false;       // Some modifiers never decay
    std::chrono::system_clock::time_point created;
};
```

### Key Methods
- `AddOpinionModifier(source, value, permanent)` - Add/update modifier
- `RemoveOpinionModifier(source)` - Remove specific modifier
- `CalculateTotalOpinion()` - Sum all modifiers using GetCurrentValue()
- `ApplyModifierDecay(months)` - Exponential decay: weight *= 0.95^months
- `ApplyOpinionDecay(delta_time)` - Passive drift toward baseline

### Decay Mechanics
- **Modifier weight**: Exponential decay at 5% per month (can be permanent)
- **Opinion value**: Linear decay toward neutral baseline (configurable)
- **Trust value**: Linear decay toward 0.5 baseline

---

## Seven Types of Influence

| Type | Range | Decay/Hop | Opinion Sensitivity |
|------|-------|-----------|-------------------|
| Military | 2-4 | 40% | High (100%) |
| Economic | 5-8 | 15% | Low (50%) |
| Dynastic | Unlimited | 5% | N/A |
| Personal | 3-5 | 25% | High (based on diplo_state) |
| Religious | Unlimited* | 0% | N/A |
| Cultural | 4-6 | 20% | N/A |
| Prestige | Global | 10% | N/A |

*Religious: No decay within same faith

---

## Calculation Flow

### InfluenceSystem::MonthlyUpdate()

1. **BuildRealmNetwork()** - Rebuild adjacency lists (neighbors, vassals, allies)
2. **CalculateInfluenceProjection()** - Calculate base projection for each realm
   - Calls: Military, Economic, Prestige (others calculated per-target)
3. **PropagateInfluence()** - Spread influence through network
   - Uses BFS to find paths
   - Applies distance and relationship modifiers
4. **UpdateSphereMetrics()** - Categorize influenced realms
5. **UpdateVassalInfluences()** - Track vassal-specific influence
6. **UpdateCharacterInfluences()** - Track character-specific influence
7. **ProcessInfluenceDecay()** - Decay inactive influences
8. **UpdateAutonomyAndFreedom()** - Recalculate autonomy for all realms
9. **UpdateSphereConflicts()** - Detect and track conflicts
10. **CheckForFlashpoints()** - Identify crisis situations

---

## Configuration Keys

From `GameConfig`:

```cpp
// Opinion
diplomacy.opinion_decay_rate           // default: 0.1 per time unit
diplomacy.max_opinion                  // default: 100
diplomacy.min_opinion                  // default: -100
diplomacy.opinion_history_window       // default: 12 data points
diplomacy.max_recent_actions           // default: 10

// Trust
diplomacy.trust_decay_rate             // default: 0.01 per time unit

// Cooldowns
diplomacy.cooldown.declare_war         // default: 365 days
diplomacy.cooldown.propose_alliance    // default: 180 days
diplomacy.cooldown.propose_trade       // default: 90 days
diplomacy.cooldown.default             // default: 30 days
```

---

## Data Structures

### Opinion Side (DiplomacySystem)
```cpp
DiplomacyComponent
├── relationships[other_realm]
│   └── DiplomaticState
│       ├── opinion (int)
│       ├── trust (double)
│       ├── opinion_modifiers (vector<OpinionModifier>)
│       ├── historical_data
│       │   ├── monthly_opinions (deque)
│       │   └── yearly_opinions (deque)
│       └── action_cooldowns
├── allies
├── enemies
├── prestige
└── war_weariness
```

### Influence Side (InfluenceSystem)
```cpp
InfluenceComponent
├── influence_projection[type] (double)
├── influenced_realms[realm_id]
│   └── InfluenceState
│       ├── influences_by_type[type]
│       │   └── vector<InfluenceSource>
│       │       ├── effective_strength
│       │       ├── relationship_modifier (uses opinion)
│       │       └── distance_modifier
│       ├── autonomy (double)
│       └── diplomatic_freedom (double)
├── incoming_influence (InfluenceState)
├── sphere_conflicts
└── sphere_size
```

---

## Missing Implementations (TODOs)

From `InfluenceSystem::NotifyInfluenceChange()` (line 852-871):

1. **Event triggers** for autonomy thresholds
   - `autonomy < 0.5`: "Realm losing independence"
   - `diplomatic_freedom < 0.3`: "Realm has limited diplomatic freedom"

2. **Feedback to DiplomacySystem**
   - Affect proposal acceptance rates
   - Affect war declaration ability
   - Affect rebellion/defection rates

3. **Opinion-Autonomy feedback loops**
   - Low autonomy might affect opinion calculations
   - Conflict outcomes might affect autonomy

---

## How to Use This Info

### Finding Opinion Usage
```bash
grep -r "GetOpinion\|opinion_modifiers\|CalculateTotalOpinion" src/
grep -r "opinion\|Opinion" include/game/diplomacy/
```

### Finding Autonomy Usage
```bash
grep -r "GetRealmAutonomy\|CalculateAutonomy" src/
```

### Finding Integration Points
```bash
grep -r "GetDiplomaticState" src/game/diplomacy/InfluenceSystem.cpp
grep -r "SetDiplomacySystem" src/
```

---

## Example: Opinion Affects Influence

1. **Diplomacy System** creates ModifyOpinion event
   ```cpp
   diplomacy->ModifyOpinion(realm_a, realm_b, +20, "joint_victory");
   // Sets DiplomaticState::opinion = 20
   ```

2. **Influence System** recalculates monthly
   ```cpp
   const auto* diplo_state = GetDiplomaticState(realm_a, realm_b);
   influence.base_strength = InfluenceCalculator::CalculateMilitaryInfluence(*realm_a, diplo_state);
   ```

3. **InfluenceCalculator** applies opinion modifier
   ```cpp
   double total = military_strength + tech_bonus + prestige_bonus;
   if (diplo_state) {
       total = ApplyRelationshipModifier(total, diplo_state->opinion);
       // At opinion=20: total *= 1.10 (positive boost)
   }
   ```

4. **InfluenceSource** updates relationship modifier
   ```cpp
   relationship_modifier = 1.0 + (20 / 200.0);  // = 1.10
   effective_strength = base * distance_mod * 1.10;
   ```

5. **InfluenceState** recalculates autonomy
   ```cpp
   autonomy = 1.0 - (total_influence / 200.0);
   ```

---

## Key Differences Between Systems

| Aspect | DiplomacySystem | InfluenceSystem |
|--------|-----------------|-----------------|
| **Scope** | Bilateral relations | Multilateral spheres |
| **Time Unit** | Monthly updates | Monthly updates |
| **Main State** | Opinion, Trust, Treaties | Influence, Autonomy |
| **Data Storage** | DiplomacyComponent | InfluenceComponent |
| **Decay Model** | Linear (opinion) | Exponential (modifiers) |
| **Opinion Range** | [-100, +100] | N/A (uses others' opinion) |
| **Modifier Weight** | N/A | [0.0, 1.0] (decays) |
| **Autonomy** | N/A (calculated) | Stored and accessed |

