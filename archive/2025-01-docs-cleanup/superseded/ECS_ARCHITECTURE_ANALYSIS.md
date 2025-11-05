# ECS Architecture Analysis - Mechanica Imperii

## 1. ECS Architecture Overview

### Architecture Type
The project uses a **modern Entity-Component-System (ECS) architecture** with:
- **Namespace**: `game::core` and `core::ecs` for core ECS infrastructure
- **Language**: C++17 with thread-safe implementations
- **Design Pattern**: CRTP (Curiously Recurring Template Pattern) for type-safe component registration

### Core ECS Foundation

#### 1.1 Entity Management (`EntityManager.h`)
- **Entity Representation**: `EntityID` struct with version tracking
  - Contains: `uint64_t id` + `uint32_t version`
  - Provides use-after-destroy bug prevention through version validation
- **Entity Lifecycle**:
  - `CreateEntity(name)` → Creates new entity with auto-generated ID
  - `DestroyEntity(handle)` → Marks inactive, increments version
  - `IsEntityValid(handle)` → Validates entity+version against current state
- **Thread Safety**: 
  - Uses `std::shared_mutex` for concurrent access
  - Lock-free entity ID generation with `std::atomic<uint64_t>`
- **Statistics Tracking**:
  - Entity count, component distribution, memory usage
  - Lazy-computed with dirty flag optimization

#### 1.2 Component System
- **Base Class**: `IComponent` interface with CRTP template: `Component<Derived>`
- **Key Methods**:
  - `GetTypeID()` → Unique type hash for component type
  - `GetComponentTypeName()` → Human-readable type name
  - `Clone()` → Deep copy support for serialization
  - `Serialize()`/`Deserialize()` → Optional JSON serialization
  - `IsValid()` → Component state validation

#### 1.3 Component Access (`ComponentAccessManager.h`)
- **Thread-safe Access Patterns**:
  - `GetComponent<T>(entity_id)` → Returns `ComponentAccessResult<T>` (shared lock)
  - `GetComponentMutable<T>(entity_id)` → Returns `ComponentWriteGuard<T>` (exclusive lock)
  - `HasComponent<T>(entity_id)` → Check without loading
  - `AddComponent<T>(entity_id, ...)` → Create component with perfect forwarding
  - `RemoveComponent<T>(entity_id)` → Remove component
- **RAII-based Lock Guards**: 
  - Move-only semantics prevent accidental data races
  - Automatic lock release on guard destruction
- **Statistics**:
  - Tracks read/write counts and contention events

#### 1.4 Message Bus (`MessageBus.h`)
- **Type-safe Event System**:
  - `IMessage` base class for all event types
  - `MessageHandler<T>` for message subscriptions
  - `Subscribe<T>(handler)` → Register message listener
  - `Publish<T>(...args)` → Send event with type checking
  - `ProcessQueuedMessages()` → Batch process events
- **Uses `std::type_index`** for runtime type identification
- **Decoupled Communication**: Systems communicate via events

#### 1.5 System Interface (`ISystem.h`)
- **Required Methods**:
  - `Initialize()` → Setup resources
  - `Update(deltaTime)` → Process logic
  - `Shutdown()` → Cleanup
  - `GetThreadingStrategy()` → Specify threading model
  - `GetSystemName()` → Debug identifier
  - `Serialize(version)` / `Deserialize(data)` → Save/load support

---

## 2. Game Components (46 Total)

### Category 1: Population System
**Files**: `include/game/population/PopulationComponents.h`
- **PopulationComponent**
  - `std::vector<PopulationGroup>` - Individual population groups
  - Demographics: `total_population`, `total_children`, `total_adults`, `total_elderly`
  - Statistics: `average_happiness`, `average_literacy`, `average_wealth`, `average_health`
  - Employment: `productive_workers`, `unemployed_seeking`, `unemployable`
  - Social: `culture_distribution`, `religion_distribution`, `class_distribution`, `legal_status_distribution`
  - Historical: `historical_events`, `last_update`
  
- **SettlementComponent**
  - `std::vector<Settlement>` - Urban centers
  - Infrastructure: `average_infrastructure`, `average_fortification`, `average_sanitation`
  - Trade: `trade_income_total`, `total_market_importance`
  - Culture: `cultural_diversity_index`, `religious_diversity_index`
  - Settlement Types: HAMLET, VILLAGE, TOWN, CITY

- **PopulationEventsComponent**
  - `std::vector<MigrationEvent>` - Migration tracking
  - `std::vector<SocialMobilityEvent>` - Class movement events
  - `std::vector<LegalStatusChangeEvent>` - Legal status changes
  - `std::vector<EmploymentShiftEvent>` - Employment transitions

### Category 2: Technology System
**Files**: `include/game/technology/TechnologyComponents.h`
- **TechnologyComponent**
  - 100+ technology types enumerated (1000-1500 range)
  - Categories: Agricultural, Military, Craft, Administrative, Academic
  - Examples: PRINTING_PRESS, HEAVY_PLOW, CANNONS, UNIVERSITY_SYSTEM

### Category 3: Economic System
**Files**: `include/game/economy/EconomicComponents.h`
- **EconomicComponent**
  - Treasury: `treasury`, `monthly_income`, `monthly_expenses`, `net_income`
  - Taxation: `tax_rate`, `tax_income`, `tax_collection_efficiency`
  - Trade: `trade_income`, `trade_efficiency`, `std::vector<TradeRoute>`
  - Events: `std::vector<EconomicEvent>` - Good/bad harvests, market booms, etc.

### Category 4: Military System
**Files**: `include/game/military/MilitaryComponents.h`
- **MilitaryComponent**
  - Unit Types: LEVIES, SPEARMEN, CROSSBOWMEN, LONGBOWMEN, CAVALRY, etc.
  - Unit Stats: `max_strength`, `current_strength`, `experience`, `training`
  - Equipment: `equipment_quality`, `supply_level`, `ammunition`
  - Morale: `morale` (ROUTING, BROKEN, STEADY, FANATICAL), `cohesion`, `loyalty`
  - Combat: `attack_strength`, `defense_strength`, `movement_speed`

### Category 5: Diplomacy System
**Files**: `include/game/diplomacy/DiplomacyComponents.h`
- **DiplomaticComponent**
  - Relations: ALLIED, FRIENDLY, NEUTRAL, UNFRIENDLY, HOSTILE, AT_WAR
  - Treaties: ALLIANCE, TRADE_AGREEMENT, NON_AGGRESSION, MARRIAGE_PACT, etc.
  - Actions: PROPOSE_ALLIANCE, DECLARE_WAR, SUE_FOR_PEACE, SEND_GIFT, etc.
  - Personality Types: AGGRESSIVE, DIPLOMATIC, ISOLATIONIST, OPPORTUNISTIC, etc.
  - Casus Belli: BORDER_DISPUTE, DYNASTIC_CLAIM, RELIGIOUS_CONFLICT, etc.

### Category 6: Administration System
**Files**: `include/game/administration/AdministrativeComponents.h`
- **AdministrativeComponent**
  - Officials: TAX_COLLECTOR, TRADE_MINISTER, MILITARY_GOVERNOR, etc.
  - Official Traits: CORRUPT, EFFICIENT, LOYAL, AMBITIOUS, EXPERIENCED, etc.
  - Governance Types: FEUDAL, CENTRALIZED, BUREAUCRATIC, MERCHANT_REPUBLIC, etc.
  - Law Types: COMMON_LAW, CIVIL_LAW, RELIGIOUS_LAW, TRIBAL_LAW, etc.

### Category 7: Time Management System
**Files**: `include/game/time/TimeComponents.h`
- **EntityTimeComponent** - Per-entity timing
- **ScheduledEventComponent** - Events at specific times
- **MessageTransitComponent** - Messages traveling between locations
  - Travel speed: 2.0 km/hour (historical)
  - Message types: DIPLOMATIC, TRADE, MILITARY, INTELLIGENCE, etc.
- **TimeClockComponent** - Global game time
  - Time Scales: PAUSED, SLOW, NORMAL, FAST, VERY_FAST, ULTRA_FAST
  - Tick Types: HOURLY, DAILY, MONTHLY, YEARLY
  - GameDate: year, month (1-12), day (1-30), hour (0-23)
- **RouteNetworkComponent** - Travel routes with quality modifiers
- **TimePerformanceComponent** - Performance monitoring

### Category 8: Realm System
**Files**: `include/game/realm/RealmComponents.h`
- **RealmComponent** (Inherits from `core::ecs::Component<RealmComponent>`)
  - Identity: `realmName`, `adjective`, `RealmRank` (BARONY, COUNTY, DUCHY, KINGDOM, EMPIRE)
  - Government: `GovernmentType`, `SuccessionLaw`
  - Territory: `capitalProvince`, `ownedProvinces`, `claimedProvinces`
  - Ruler: `currentRuler`, `heir`, `claimants`
  - Hierarchy: `liegeRealm`, `vassalRealms`
  - Stats: `legitimacy`, `centralAuthority`, `stability` (0-1 range)
  - Economics: `treasury`, `monthlyIncome`, `monthlyExpenses`
  - Military: `levySize`, `standingArmy`, `militaryMaintenance`
  - Dates: `foundedDate`, `lastSuccession`

### Category 9: Province Component
**Files**: `include/game/components/ProvinceComponent.h`
- **ProvinceComponent** (AI namespace)
  - Position: `m_x`, `m_y`
  - Ownership: `m_ownerNationId`
  - Designed for AI province representation

---

## 3. Game Systems (14+ Systems)

All inherit from `game::core::ISystem`

| System | Location | Purpose | Threading |
|--------|----------|---------|-----------|
| **PopulationSystem** | `game/population/` | Demographics, growth, migrations | THREAD_POOL |
| **EconomicSystem** | `game/economy/` | Treasury, trade, taxation | THREAD_POOL |
| **MilitarySystem** | `game/military/` | Unit management, recruitment | THREAD_POOL |
| **TechnologySystem** | `game/technology/` | Research, tech effects | THREAD_POOL |
| **DiplomacySystem** | `game/diplomacy/` | Relations, treaties, war | THREAD_POOL |
| **AdministrativeSystem** | `game/administration/` | Officials, governance | THREAD_POOL |
| **TimeManagementSystem** | `game/time/` | Game time, events, messages | MAIN_THREAD |
| **TradeSystem** | `game/trade/` | Trade routes, commerce | THREAD_POOL |
| **RealmManager** | `game/realm/` | Nation/realm management | MAIN_THREAD |
| **ProvinceManagementSystem** | `game/province/` | Province administration | MAIN_THREAD |
| **MilitaryRecruitmentSystem** | `game/military/` | Troop recruitment | THREAD_POOL |
| **CoreGameplaySystem** | `game/gameplay/` | Game coordination | MAIN_THREAD |
| **ScenarioSystem** | `game/scenario/` | Event-driven scenarios | MAIN_THREAD |
| **InformationPropagationSystem** | `game/ai/` | AI information flow | THREAD_POOL |

### System Configuration Example (PopulationSystem)
```cpp
struct PopulationSystemConfig {
    double demographic_update_interval = 0.1;      // 10 Hz
    double mobility_update_interval = 1.0;         // 1 Hz
    double settlement_update_interval = 2.0;       // 0.5 Hz
    double base_birth_rate = 0.035;
    double base_death_rate = 0.030;
    double plague_death_multiplier = 3.0;
    double cultural_assimilation_rate = 0.02;
    // ... 10+ more configuration parameters
};
```

---

## 4. Entity Creation & Management

### 4.1 Entity Creation Flow

**Realm Creation** (via `RealmManager`)
```cpp
RealmManager::CreateRealm(
    const std::string& name,
    GovernmentType government,
    types::EntityID capitalProvince,
    types::EntityID ruler = types::EntityID{0}
) -> types::EntityID
```
1. EntityManager creates new entity
2. RealmComponent added to entity
3. Message "RealmCreated" published to MessageBus
4. Realm registered in RealmManager's registry

**Province Creation** (via `ProvinceSystem`)
```cpp
ProvinceSystem::CreateProvince(
    const std::string& name,
    double x,
    double y
) -> types::EntityID
```

**Population Creation** (via `EnhancedPopulationFactory`)
- Creates PopulationComponent with demographic groups
- Creates SettlementComponent for urban centers
- Creates PopulationEventsComponent for event tracking

### 4.2 Entity Lifecycle Safety
- **Version Checking**: Entity handles include version number
- **Use-After-Destroy Prevention**: Old handles automatically invalidated
- **Statistics**: Auto-tracked entity counts and memory usage
- **Validation**: `ValidateIntegrity()` checks component-entity consistency

---

## 5. Data Loading & Initialization

### 5.1 Configuration System

**GameConfig.json** - Central configuration
- Location: `/home/user/Game/config/GameConfig.json`
- Sections:
  - `diplomacy` - 12 parameters (treaty durations, opinion thresholds)
  - `economy` - 20+ parameters (taxation, trade, events, harvests)
  - `military` - 4 parameters (recruitment costs, morale, experience)
  - `population` - 4 parameters (growth rate, famine threshold)
  - `technology` - 3 parameters (research costs, scaling)
  - `administration` - 3 parameters (efficiency, corruption, salaries)
  - `threading` - 3 parameters (worker threads, frame budget)
  - `battle_resolution` - 20+ parameters (casualties, morale, terrain)

### 5.2 Scenario System

**ScenarioSystem** (`include/game/scenario/ScenarioSystem.h`)
- Loads JSON scenario files from `config/scenarios/`
- Available scenarios:
  - `economic_crisis.json`
  - `tech_breakthrough.json`
- Scenario Structure:
  ```cpp
  struct ScenarioData {
      std::string id, name, description;
      int duration_days;
      std::vector<ScenarioTrigger> triggers;  // Condition-based events
      std::vector<std::string> completion_messages;
      int current_day, is_active, is_completed;
  };
  ```
- Event Types: ECONOMIC_SHOCK, POPULATION_UNREST, MILITARY_BUDGET_CUT, etc.
- Effects: Multiply/add/set operations on game parameters

### 5.3 Test Data

**Test Provinces JSON** (`data/test_provinces.json`)
- Contains 4+ historical provinces
- Example: Wessex province with:
  - Terrain type, boundary coordinates, center position
  - Features: Cities, forests, hills with population data
  - Owner realm ID, level-of-detail settings

**GameWorld Initialization**
```cpp
GameWorld::initializeTestProvinces()
```
- Populates game world with test provinces
- Creates Province objects with:
  - ID, name, terrain type, position
  - Population, economic data
  - Military units, culture, religion

### 5.4 Data Format Support
- **JSON Parsing**: Uses JsonCpp (json-cpp)
- **Serialization**: ISerializable interface for save/load
- **Component Serialization**: Optional Serialize()/Deserialize() methods

---

## 6. Bridge/Integration Systems

The codebase uses **Bridge Pattern** to connect systems:

- **EconomicPopulationBridge** - Links economy and population effects
  - Tax impact on happiness
  - Unemployment effects on population
  - Wage and wealth impacts
  - Crisis detection and propagation
  
- **TradeEconomicBridge** - Links trade to economic output
  - Trade route efficiency affects income
  - Economic state affects trade ability

- **TechnologyEconomicBridge** - Links research to economy
  - Technology effects multiply economic output
  - Cost of research impacts treasury

- **MilitaryEconomicBridge** - Links army to expenses
  - Unit maintenance costs
  - Recruitment and training expenses

- **DiplomacyEconomicBridge** - Diplomatic status affects economics
  - Trade embargo effects
  - Alliance trade bonuses

---

## 7. Threading Model

### Threading Strategies (from `core/threading/ThreadingTypes.h`)
```cpp
enum ThreadingStrategy {
    MAIN_THREAD,        // UI, rendering (ImGui, OpenGL)
    THREAD_POOL,        // CPU-intensive (population, economy, military)
    DEDICATED_THREAD,   // Continuous processing (AIDirector)
    MAIN_THREAD_ONLY    // Strict main thread requirement (render systems)
};
```

### System Threading Assignments
- **MAIN_THREAD**: TimeManagement, Realm, Province, Gameplay, Scenario
- **THREAD_POOL**: Population, Economy, Military, Technology, Diplomacy, Trade, Information
- **MAIN_THREAD_ONLY**: Rendering systems

### Thread Safety Infrastructure
- `ThreadedSystemManager` - Coordinates system execution
- `ThreadSafeMessageBus` - Thread-safe event communication
- `std::shared_mutex` - Read-write locks for components
- Lock-free atomic operations for counters

---

## 8. Current Data Initialization Flow

```
main.cpp
    ↓
[Create EntityManager, ComponentAccessManager, MessageBus]
    ↓
[Create all Game Systems with ECS references]
    ↓
GameSystemsManager::Initialize()
    ├→ PopulationSystem::Initialize()
    ├→ EconomicSystem::Initialize()
    ├→ MilitarySystem::Initialize()
    ├→ TechnologySystem::Initialize()
    ├→ DiplomacySystem::Initialize()
    ├→ AdministrativeSystem::Initialize()
    ├→ TimeManagementSystem::Initialize()
    ├→ TradeSystem::Initialize()
    └→ RealmManager::Initialize()
    ↓
[Load GameConfig.json]
    ↓
[Create test realms/provinces via CreateRealm/CreateProvince]
    ↓
[Load scenario files via ScenarioSystem]
    ↓
Game Loop
    ├→ Update(deltaTime)
    ├→ Process MessageBus events
    └→ Render UI
```

---

## 9. Key Design Patterns Used

### 1. CRTP (Curiously Recurring Template Pattern)
```cpp
template<typename Derived>
class Component : public IComponent { ... };

class PopulationComponent : public Component<PopulationComponent> { ... };
```

### 2. Factory Pattern
- `EnhancedPopulationFactory` - Creates population entities
- `RealmFactory` - Creates realm entities
- `ProvinceBuilder` - Constructs province data

### 3. Bridge Pattern
- Multiple bridge systems connect game domains

### 4. Observer Pattern
- MessageBus implements publish-subscribe

### 5. Component Pattern (ECS)
- Entities as ID + components
- Systems operate on entities with specific components

### 6. RAII (Resource Acquisition Is Initialization)
- `ComponentAccessResult`, `ComponentWriteGuard` for lock management

---

## 10. Summary Statistics

| Metric | Value |
|--------|-------|
| Total Components | 46 |
| Total Systems | 14+ |
| Core ECS Files | 6 (EntityManager, IComponent, ISystem, etc.) |
| Configuration Parameters | 100+ |
| Technology Types | 100+ |
| Military Unit Types | 20+ |
| Settlement Types | 4 |
| Diplomatic Personalities | 8 |
| Government Types | 10 |
| Threading Strategies | 4 |
| Message Types | 7 |

---

## 11. Current Limitations & TODOs

1. **Serialization**: Basic JSON support, needs full implementation
2. **Entity Querying**: No advanced query system (e.g., "all entities with PopulationComponent AND EconomicComponent")
3. **Component Pooling**: No object pooling for performance optimization
4. **Batch Operations**: Limited batch update support
5. **UI System**: Many UI systems commented out or incomplete
6. **Save/Load**: Basic framework, needs full persistence layer
7. **Network**: No multiplayer/networking support

---

