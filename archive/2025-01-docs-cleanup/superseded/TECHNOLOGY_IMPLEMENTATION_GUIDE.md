# Mechanica Imperii - Technology System Implementation Guide

## 1. PROJECT OVERVIEW

**Project Type:** C++ Strategy Game (Medieval/Grand Strategy)
**Architecture:** Entity-Component-System (ECS)
**Current Status:** 18+ core systems implemented with bridge integrations
**Scale:** Supports ~5000 provinces, 500+ nations, 3000+ characters

### Core Technology Stack
- **Build System:** CMake
- **Language:** C++17
- **Threading:** Multi-threaded with configurable strategies
- **Serialization:** JSON + LZ4 compression
- **ECS Framework:** Custom in-house implementation

---

## 2. OVERALL ARCHITECTURE

### 2.1 ECS Foundation

The project uses a **three-tier ECS architecture**:

1. **EntityManager** (`include/core/ECS/EntityManager.h`)
   - Central registry for all entities and components
   - Type-erased component storage
   - Fast entity-to-component lookup

2. **ComponentAccessManager** (`include/core/ECS/ComponentAccessManager.h`)
   - Thread-safe wrapper around EntityManager
   - Read/write locks per component type
   - Batch operations and deferred writes
   - Frame-based synchronization

3. **MessageBus** (`include/core/ECS/MessageBus.h`)
   - Event-driven communication between systems
   - Event subscription/publication pattern
   - Decouples systems for independent updates

### 2.2 System Architecture

All game systems inherit from `ISystem` interface:

```cpp
class ISystem {
    virtual void Initialize() = 0;
    virtual void Update(float delta_time) = 0;
    virtual void Shutdown() = 0;
    virtual ThreadingStrategy GetThreadingStrategy() = 0;
};
```

**System Types (from game_types.h):**
- ECONOMICS, MILITARY, DIPLOMACY, POPULATION
- CONSTRUCTION, TECHNOLOGY, TRADE
- PROVINCIAL_GOVERNANCE, REALM_MANAGEMENT
- TIME_MANAGEMENT, AI_DIRECTOR, etc.

### 2.3 Threading Model

Three main threading strategies:
- **MAIN_THREAD:** Synchronous on main thread
- **THREAD_POOL:** Parallel execution
- **DEDICATED_THREAD:** Own independent thread
- **BACKGROUND_THREAD:** Low-priority processing
- **HYBRID:** Mix of main + background

---

## 3. EXISTING TECHNOLOGY SYSTEM

### 3.1 Components (TechnologyComponents.h)

Four main components for technology:

#### 1. **ResearchComponent** - Research and Development
```
- Technology states & progress tracking
- Current research focus with bonus multiplier
- Research infrastructure (universities, libraries, workshops)
- Research efficiency modifiers
- Monthly research budget & category investment
- Research specialization
```

#### 2. **InnovationComponent** - Innovation and Invention
```
- Innovation capacity & breakthrough chance
- Inventor sources (scholars, craftsmen)
- Innovation environment (cultural openness, experimentation freedom)
- Guild resistance & religious restrictions
- Royal patronage & merchant funding
```

#### 3. **KnowledgeComponent** - Knowledge Networks
```
- Knowledge infrastructure (manuscripts, scribes, translators)
- Knowledge preservation & transmission rates
- Knowledge networks (scholarly exchanges, trade routes)
- Language support & literacy rates
- Translation accuracy
```

#### 4. **TechnologyEventsComponent** - Event Tracking
```
- Discovery, breakthrough, innovation events
- Adoption & implementation events
- Knowledge acquisition history
- Research failures & setbacks
- Technology reputation & prestige
```

### 3.2 Technology Definitions

**TechnologyType Enum** includes 50+ technologies across 6 categories:
- **Agricultural** (1000-1099): Three-Field System, Heavy Plow, Horse Collar, etc.
- **Military** (1100-1199): Chainmail, Plate Armor, Crossbow, Cannons, etc.
- **Craft** (1200-1299): Blast Furnace, Printing Press, Metallurgy, etc.
- **Administrative** (1300-1399): Written Law, Census, Tax Systems, etc.
- **Academic** (1400-1499): University System, Scientific Method, etc.
- **Naval** (1500-1599): Ship Design, Navigation, Naval Tactics, etc.

Each technology has:
- Base research cost (in gold)
- Literacy requirement
- Prerequisites (tech dependencies)
- Historical emergence year
- Historical spread duration
- Technology effects (map of effect names to values)

### 3.3 TechnologySystem Class

**File:** `include/game/technology/TechnologySystem.h`

**Core Methods:**
```cpp
// Lifecycle
void Initialize();
void Update(float delta_time);
void Shutdown();

// Component Management
bool CreateResearchComponent(EntityID);
bool RemoveResearchComponent(EntityID);
ResearchComponent* GetResearchComponent(EntityID);
// Same for Innovation, Knowledge, TechnologyEvents

// ECS Integration
bool InitializeTechnologyComponents(EntityID, year, budget);
bool CleanupTechnologyComponents(EntityID);
bool ValidateTechnologyComponents(EntityID);
size_t GetTechnologyComponentCount();
```

---

## 4. HOW OTHER SYSTEMS ARE STRUCTURED

### 4.1 Economic System Pattern

**File:** `include/game/economy/EconomicSystem.h`

Implements `ISystem` interface with:

1. **Components** (EconomicComponents.h):
   - EconomicComponent: Treasury, taxes, trade, production
   - TradeComponent: Trade routes, merchants, goods
   - EconomicEventsComponent: Events & effects
   - MarketComponent: Prices, supply/demand
   - TreasuryComponent: Financial reserves

2. **System Methods:**
   ```cpp
   void CreateEconomicComponents(EntityID);
   void ProcessMonthlyUpdate(EntityID);
   bool SpendMoney(EntityID, amount);
   void AddTradeRoute(from, to, efficiency, value);
   void ProcessRandomEvents(EntityID);
   ```

3. **Configuration:**
   - EconomicSystemConfig struct with:
     - Update intervals (30 days in-game)
     - Economic parameters (tax rates, inflation)
     - Event probabilities

### 4.2 Diplomacy System Pattern

**File:** `include/game/diplomacy/DiplomacySystem.h`

Implements `ISystem` with:

1. **Components** (DiplomacyComponents.h):
   - DiplomacyComponent: Relations, treaties, marriages
   - TreatyComponent: Treaty metadata
   - DiplomaticActionComponent: Actions & impacts

2. **Data Structures:**
   - DiplomaticState: Per-realm relationship data
   - Treaty: Formal agreements with terms
   - DynasticMarriage: Marriage alliances
   - DiplomaticProposal: Pending proposals

3. **Enums:**
   - DiplomaticRelation: ALLIED, FRIENDLY, NEUTRAL, HOSTILE, AT_WAR
   - TreatyType: ALLIANCE, TRADE, NON_AGGRESSION, MARRIAGE_PACT, etc.
   - CasusBelli: War justification reasons

### 4.3 Trade System Pattern

Uses **Repository Pattern** (TradeRepository) to encapsulate component access:

1. **Components:**
   - TradeRouteComponent: Route information
   - TradeHubComponent: Trade node properties
   - TradeInventoryComponent: Goods & inventory

2. **Methods in Repository:**
   - GetRouteComponent, GetOrCreateRouteComponent
   - HasRouteComponent, etc.

This pattern **reduces boilerplate** and provides **type-safe access**.

---

## 5. GAME'S DATA MODEL

### 5.1 Core Entity Types

All identified by `types::EntityID` (uint32_t):

1. **Realms** (Nations/States)
   - RealmComponent with government, succession laws
   - Treasury, military, stability
   - Ruled by a character with heirs

2. **Provinces** (Territorial units)
   - ProvinceComponent: ID, name, owner, terrain, area
   - Population, economy, culture, religion
   - Connected to neighboring provinces

3. **Characters** (Rulers, nobles, commanders)
   - Personal stats, ambitions, relationships
   - Career advancement, traits

### 5.2 Component Composition Pattern

System uses **composition over inheritance**:

```cpp
// GOOD: Composition
struct ProvinceComponent { ... };
struct MilitaryComponent { ProvinceID province; ... };

// BAD: Inheritance (breaks POD)
struct MilitaryProvinceComponent : ProvinceComponent { ... };
```

All components are plain-data structures (POD when possible) with no virtual functions.

### 5.3 Strong Type System

Uses strong types to prevent ID mixing:

```cpp
using ProvinceID = StrongType<uint32_t, struct ProvinceTag>;
using CharacterID = StrongType<uint32_t, struct CharacterTag>;
using RealmID = StrongType<uint32_t, struct RealmTag>;
```

---

## 6. SYSTEM INTERACTIONS & BRIDGES

### 6.1 Bridge Pattern

Bridge systems integrate two game systems through:
- **Data structures** capturing state of both systems
- **Effect calculations** (System A → System B)
- **Contribution calculations** (System B → System A)
- **Event messages** for major changes

### 6.2 Technology-Economic Bridge

**File:** `include/game/economy/TechnologyEconomicBridge.h`

This is the **PRIMARY INTEGRATION POINT** for the technology system:

**Data Structures:**
```cpp
struct TechnologyEconomicEffects {
    double production_efficiency;      // From Agricultural + Craft tech
    double trade_efficiency;           // From Naval + Craft tech
    double tax_efficiency;             // From Administrative tech
    double market_sophistication;      // From Academic + Craft tech
    double innovation_rate_modifier;   // From Academic tech
};

struct EconomicTechnologyContribution {
    double research_budget;            // Treasury allocation
    double research_infrastructure_count;
    double trade_network_bonus;        // From trade routes
    double wealth_innovation_bonus;
};
```

**Bridge Methods:**
- `CalculateTechnologyEffects(entity)` - How tech helps economy
- `CalculateEconomicContributions(entity)` - How economy supports research
- `ApplyTechnologyEffectsToEconomy()` - Apply bonuses to economic component
- `ApplyEconomicContributionsToTechnology()` - Apply budget to research
- `ProcessCrisisDetection()` - Detect funding/implementation crises

**Event Messages (Crisis Detection):**
- `TechnologyBreakthroughEconomicImpact` - Tech discovered
- `ResearchFundingCrisis` - Not enough budget
- `BrainDrainEvent` - Scholars leaving
- `TechnologyImplementationComplete` - Tech applied

### 6.3 Diplomacy-Economic Bridge

**File:** `include/game/bridge/DiplomacyEconomicBridge.h`

Shows how major system integration works:

**Features:**
- Trade embargoes & sanctions
- Trade agreements
- Economic dependencies
- War economic impacts
- Diplomatic event → Economic consequences

**Key Pattern:**
```cpp
class Bridge : public ISystem {
    void Initialize();
    void Update(float delta_time);
    void Shutdown();
    
    // Cross-system methods
    void OnWarDeclared(aggressor, defender);
    void OnAllianceFormed(realm_a, realm_b);
    void ProcessEconomicEvents();
};
```

### 6.4 Economic-Population Bridge

Shows how complex interdependencies work:
- Tax happiness effects
- Wage-happiness relationships
- Employment impact
- Wealth inequality effects
- Crisis detection

---

## 7. CONFIGURATION SYSTEM

### 7.1 Configuration File Structure

**File:** `config/GameConfig.json`

Example configuration sections:

```json
{
  "diplomacy": {
    "non_aggression_duration_years": 5,
    "alliance_duration_years": 25,
    "friendly_threshold": 80,
    "hostile_threshold": -50
  },
  
  "economy": {
    "starting_treasury": 1000,
    "random_event_chance_percent": 5,
    "trade_base_efficiency": 0.5
  },
  
  "economic_bridge": {
    "update_interval_days": 1.0,
    "tax_happiness_base_effect": -0.5,
    "crisis_severity_increase": 0.1
  }
}
```

### 7.2 Configuration Access Pattern

Systems access config through:
- `GameConfig::Get()` static accessor
- Specific config structs (EconomicSystemConfig, etc.)
- Hot-reload detection (file watching)

---

## 8. TESTING & VALIDATION

### 8.1 Testing Patterns

Tests exist in `/home/user/Game/tests/`:
- Unit tests for components
- Integration tests for systems
- Calculator pattern tests

### 8.2 Calculator Pattern

Used for testable calculations:

```cpp
class TechSystemCalculator {
public:
    // Pure static functions (no state)
    static double CalculateResearchSpeed(budget, efficiency, focus);
    static bool ShouldLaunchProject(metrics);
    
    // Constants for tuning
    static constexpr double RESEARCH_COST_PER_LEVEL = 100.0;
};
```

Benefits:
- Testable without ECS infrastructure
- Reusable across contexts
- Easy parameter tuning

---

## 9. SERIALIZATION SYSTEM

### 9.1 Save Format

```json
{
    "metadata": {
        "version": "1.0.0",
        "timestamp": "2025-10-22T10:30:00Z",
        "game_date": { "year": 1066, "month": 10, "day": 14 }
    },
    "entities": [
        {
            "id": 1,
            "components": {
                "RealmComponent": { ... },
                "ResearchComponent": { ... },
                "EconomicComponent": { ... }
            }
        }
    ]
}
```

### 9.2 Save System

**File:** `include/core/save/SaveManager.h`

Features:
- Full saves (complete state)
- Incremental saves (delta tracking)
- LZ4 compression (60-80% reduction)
- Version tracking
- Validation checksums

---

## 10. IMPLEMENTATION RECOMMENDATIONS FOR TECHNOLOGY SYSTEM

### 10.1 Current State

The technology system skeleton exists with:
- ✓ Components defined (Research, Innovation, Knowledge, Events)
- ✓ System class scaffold
- ✓ Configuration structure
- ✓ Technology definitions (50+ techs across 6 categories)
- ✓ Bridge integration partially defined

### 10.2 Missing Pieces to Implement

1. **System Core Logic** - Update methods for each component type
2. **Research Mechanics** - Progress calculation, cost scaling, prerequisites
3. **Knowledge Transfer** - Trade/diplomacy/migration-based learning
4. **Technology Effects** - How techs modify other systems
5. **Event Processing** - Discovery events, breakthroughs, cascades
6. **Complete Bridge Integration** - Full economic feedback loops

### 10.3 Design Patterns to Follow

1. **Use Repository Pattern** for component access (like Trade system)
2. **Use Calculator Pattern** for complex calculations
3. **Use Message Bus** for inter-system events
4. **Use Strong Types** for all IDs
5. **Publish Events** when major changes occur
6. **Add Configuration** options for tuning
7. **Implement Serialization** for save/load support

### 10.4 Key Integration Points

1. **Economic Bridge** (PRIMARY)
   - Research budget from treasury
   - Technology effects boost production/trade

2. **Diplomacy Bridge** (SECONDARY)
   - Knowledge transfer through embassies
   - Tech espionage or sharing agreements

3. **Trade Bridge** (SECONDARY)
   - Knowledge spreads via merchants
   - Naval techs boost trade efficiency

4. **Population Bridge** (TERTIARY)
   - Scholar availability
   - Literacy affects research speed

---

## 11. FILE LOCATIONS SUMMARY

### Headers
- Technology Components: `/home/user/Game/include/game/technology/TechnologyComponents.h`
- Technology System: `/home/user/Game/include/game/technology/TechnologySystem.h`
- Technology Bridge: `/home/user/Game/include/game/economy/TechnologyEconomicBridge.h`

### Implementation
- Technology System: `/home/user/Game/src/game/technology/TechnologySystem.cpp`
- Technology Utils: `/home/user/Game/src/game/technology/TechnologyUtils.cpp`

### Configuration
- Game Config: `/home/user/Game/config/GameConfig.json`

### Reference Systems
- Economic System: `include/game/economy/EconomicSystem.h`
- Diplomacy System: `include/game/diplomacy/DiplomacySystem.h`
- Trade System: `include/game/trade/` (uses Repository pattern)

---

## 12. KEY ARCHITECTURAL PRINCIPLES

1. **Data-Oriented Design** - Components are pure data, systems contain logic
2. **Thread Safety** - ComponentAccessManager ensures synchronized access
3. **Event-Driven** - Systems communicate via MessageBus
4. **Composability** - Systems can be combined via bridges
5. **Hot-Reloadable** - Config changes apply without restart
6. **Incremental Saves** - Only changed components saved for efficiency
7. **Strong Typing** - Type-safe ID system prevents bugs
8. **Testability** - Calculator pattern separates logic from state
