# Architecture Documentation - Mechanica Imperii

**Last Updated:** October 22, 2025

---

## Table of Contents

1. [System Overview](#system-overview)
2. [ECS Architecture](#ecs-architecture)
3. [Game Systems](#game-systems)
4. [Threading Model](#threading-model)
5. [Data Flow](#data-flow)
6. [Save System](#save-system)
7. [AI Architecture](#ai-architecture)
8. [Map & Rendering](#map--rendering)
9. [Design Patterns](#design-patterns)
10. [Performance Considerations](#performance-considerations)

---

## System Overview

Mechanica Imperii is built on a modern Entity-Component-System (ECS) architecture with multi-threading support. The game is designed to handle:
- ~5000 provinces
- 500+ nations
- 3000+ characters
- Real-time updates at 60 FPS (rendering) with configurable game speed

### Core Principles

1. **Data-Oriented Design:** Components are plain data, systems contain logic
2. **Thread Safety:** ComponentAccessManager provides synchronized access
3. **Event-Driven:** MessageBus decouples systems
4. **Hot-Reloadable:** Configuration changes without restart
5. **Incremental Saves:** Delta tracking for efficient save/load

---

## ECS Architecture

### Entity Manager

**File:** `include/core/ECS/EntityManager.h`

The EntityManager is the central registry for all entities and their components.

**Key Methods:**
```cpp
Entity CreateEntity();
void DestroyEntity(Entity entity);
template<typename T> void AddComponent(Entity entity, T component);
template<typename T> void RemoveComponent(Entity entity);
template<typename T> T* GetComponent(Entity entity);
template<typename T> std::vector<Entity> GetEntitiesWithComponent();
```

**Storage:**
- Components stored in type-erased containers (`std::unordered_map<std::type_index, ComponentContainer>`)
- Entity-to-component mapping for fast lookups
- Component-to-entity reverse mapping for queries

### Component Access Manager

**File:** `include/core/ECS/ComponentAccessManager.h`

Thread-safe wrapper around EntityManager for concurrent access.

**Features:**
- Read/write locks per component type
- Batch operations for performance
- Deferred writes to avoid deadlocks
- Frame-based synchronization

**Usage Pattern:**
```cpp
// Single component access
auto* comp = component_access_.GetComponent<PopulationComponent>(entity);

// Bulk query
auto entities = component_access_.GetEntitiesWithComponent<ProvinceComponent>();
```

### Message Bus

**File:** `include/core/ECS/MessageBus.h`

Event system for decoupled communication between systems.

**Message Types:**
- `EntityCreated`, `EntityDestroyed`
- `ComponentAdded`, `ComponentRemoved`
- Custom game events (treaty signed, war declared, etc.)

**Usage:**
```cpp
// Subscribe
message_bus_.Subscribe<TreatySignedEvent>([this](const Event& e) {
    HandleTreatyEvent(static_cast<const TreatySignedEvent&>(e));
});

// Publish
message_bus_.Publish(TreatySignedEvent{realm1, realm2, treaty_type});
```

---

## Game Systems

All game systems follow a common interface pattern:

```cpp
class GameSystem {
public:
    virtual void Initialize() = 0;
    virtual void Update(float delta_time) = 0;
    virtual void Shutdown() = 0;
    virtual SystemType GetSystemType() const = 0;
};
```

### System Lifecycle

1. **Initialize:** Set up resources, register message handlers
2. **Update:** Process entities, update components (called every frame)
3. **Shutdown:** Clean up resources

### System Types (from `game_types.h`)

```cpp
enum class SystemType : uint8_t {
    INVALID = 0,
    ECS_FOUNDATION,
    MESSAGE_BUS,
    THREADING,
    ECONOMICS,
    MILITARY,
    DIPLOMACY,
    POPULATION,
    CONSTRUCTION,
    TECHNOLOGY,
    TRADE,
    PROVINCIAL_GOVERNANCE,
    REALM_MANAGEMENT,
    TIME_MANAGEMENT,
    AI_DIRECTOR,
    INFORMATION_PROPAGATION,
    ATTENTION_MANAGER,
    MAX_SYSTEM_TYPE
};
```

### System Dependencies

```
ECS_FOUNDATION (EntityManager, ComponentAccessManager, MessageBus)
    ↓
THREADING (ThreadedSystemManager)
    ↓
TIME_MANAGEMENT (GameClock, speed control)
    ↓
[POPULATION, ECONOMICS, MILITARY, DIPLOMACY, etc.] (Game systems in parallel)
    ↓
AI_DIRECTOR (coordinates AI systems)
    ↓
[INFORMATION_PROPAGATION, ATTENTION_MANAGER] (AI subsystems)
```

### Key Systems

#### Economic System
**File:** `include/game/economy/EconomicSystem.h`

- Production chains (raw materials → intermediate goods → finished products)
- Trade routes between provinces
- Market prices with supply/demand
- Resource stockpiles per realm

#### Population System
**File:** `include/game/population/PopulationSystem.h`

- Demographics (age, gender, class distribution)
- Employment and unemployment
- Migration (internal and external)
- Cultural assimilation and religious conversion

#### Diplomacy System
**File:** `include/game/diplomacy/DiplomacySystem.h`

- Treaties (alliance, non-aggression, trade, vassalage)
- Embassies and diplomatic missions
- Relation tracking (opinion, trust, rivalry)
- Casus belli and war justification

#### Military System
**File:** `include/game/military/MilitarySystem.h`

- Unit recruitment and maintenance
- Combat resolution
- Army composition and tactics
- Logistics and supply

---

## Threading Model

### ThreadedSystemManager

**File:** `include/core/threading/ThreadedSystemManager.h`

Coordinates multi-threaded system execution.

**Threading Strategies:**
- `MAIN_THREAD`: Single-threaded (default for most systems)
- `THREAD_POOL`: Parallel execution across multiple threads
- `DEDICATED_THREAD`: Own thread for long-running tasks
- `BACKGROUND_THREAD`: Low-priority background processing
- `HYBRID`: Mix of main thread and background

**Frame Synchronization:**
```
[Main Thread]
  1. Pre-update: Collect input, handle events
  2. Update Systems: Execute all registered systems
     - Main thread systems run sequentially
     - Thread pool systems run in parallel
     - Background systems continue independently
  3. Post-update: Apply deferred writes, synchronize state
  4. Render: Draw to screen
```

**Thread Safety:**
- Component access synchronized via ComponentAccessManager
- Message delivery queued and batched
- Frame-based barrier for inter-system dependencies

---

## Data Flow

### Typical Frame Flow

```
Input/Events → MessageBus → Game Systems → Component Updates → Rendering
                    ↓
            AI Systems (parallel processing)
                    ↓
            Deferred Writes → ComponentAccessManager
```

### Configuration Flow

```
GameConfig.json → ConfigManager → GameConfig struct → Game Systems
        ↓
   Hot-reload detection → Re-parse → Update live config
```

### Save/Load Flow

```
Game State → SaveManager → Component Serialization → LZ4 Compression → Disk
                                                              ↓
Disk → LZ4 Decompression → JSON Parse → Component Deserialization → Game State
```

---

## Save System

### Save Manager

**File:** `include/core/save/SaveManager.h`

Handles game state persistence with compression and validation.

**Features:**
- Full saves: Complete game state
- Incremental saves: Delta tracking (only changed components)
- Compression: LZ4 for 60-80% size reduction
- Versioning: Schema version tracking
- Validation: Checksum and integrity checks

**Save Format:**
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
                "ProvinceComponent": { ... },
                "PopulationComponent": { ... }
            }
        }
    ]
}
```

### Incremental Save Tracker

**File:** `include/core/save/IncrementalSaveTracker.h`

Tracks which components have changed since last save.

**Strategy:**
- Mark components dirty on write
- Save only dirty components
- Clear dirty flags after successful save
- Fallback to full save if delta grows too large

---

## AI Architecture

### AI Director

**File:** `include/game/ai/AIDirector.h`

Top-level coordinator for all AI systems.

**Responsibilities:**
- Schedule AI updates (not all AIs update every frame)
- Allocate attention budget across nations/characters
- Coordinate information sharing between AIs

### Nation AI

**File:** `include/game/ai/NationAI.h`

National-level decision making.

**Decisions:**
- Diplomacy (treaties, wars, alliances)
- Economic policy (taxation, trade, production)
- Military strategy (recruitment, deployment, campaigns)
- Technology research priorities

### Character AI

**File:** `include/game/ai/CharacterAI.h`

Individual character behavior (rulers, nobles, commanders).

**Decisions:**
- Personal ambitions (titles, wealth, power)
- Relationships (marriages, rivalries, friendships)
- Plot and intrigue
- Career advancement

### AI Attention Manager

**File:** `include/game/ai/AIAttentionManager.h`

Manages computational budget for AI processing.

**Strategy:**
- Prioritize important nations (player neighbors, great powers)
- Update distant/minor nations less frequently
- Scale AI complexity based on available budget
- 98% CPU load reduction achieved through attention management

### Information Propagation System

**File:** `include/game/ai/InformationPropagationSystem.h`

Simulates knowledge spreading across the map.

**Features:**
- Distance-based information delay
- Diplomatic channels (embassies speed up information)
- Rumor and misinformation
- Perfect information for player (configurable)

---

## Map & Rendering

### Map Data Structure

**Files:** `include/map/MapData.h`, `include/map/ProvinceGeometry.h`

Each province has:
- Unique ID (ProvinceID strong type)
- Boundary polygon (list of vertices)
- Center point (for name display, icons)
- Owner (RealmID)
- Terrain type, climate
- Connected provinces (for pathfinding)

### Level of Detail (LOD)

**File:** `src/rendering/MapRenderer.cpp`

Four LOD levels based on zoom:

**LOD 0 - Strategic View (furthest zoom):**
- Continental shapes only
- Major rivers, coastlines
- No province boundaries

**LOD 1 - Regional View:**
- Regional boundaries
- Major cities (capitals)
- Simplified province shapes

**LOD 2 - Provincial View (primary gameplay):**
- Full province boundaries
- Province names
- Cities sized by population
- Terrain features (mountains, forests)

**LOD 3 - Local View (closest zoom before terrain):**
- Detailed province features
- Roads and rivers
- Individual buildings (cities)
- Army units

**LOD 4 - Terrain View (planned, not yet implemented):**
- Heightmap-based 3D terrain
- Textured ground
- Trees, rocks, vegetation
- Buildings and structures

### Viewport Culling

**File:** `src/rendering/ViewportCuller.cpp`

Frustum culling to render only visible provinces.

**Algorithm:**
1. Calculate view frustum from camera position/zoom
2. Test each province's bounding box against frustum
3. Render only provinces that intersect frustum
4. Typical culling rate: 70-90% of provinces culled

---

## Design Patterns

### Strong Types

**File:** `include/core/types/game_types.h`

Type-safe wrappers for primitive IDs:

```cpp
template<typename T, typename Tag>
struct StrongType {
    T value;
    explicit constexpr StrongType(T val) : value(val) {}
    constexpr operator T() const { return value; }
};

using ProvinceID = StrongType<uint32_t, struct ProvinceTag>;
using CharacterID = StrongType<uint32_t, struct CharacterTag>;
using RealmID = StrongType<uint32_t, struct RealmTag>;
```

**Benefits:**
- Prevents mixing different ID types
- Self-documenting code
- Compiler catches errors

### RAII Pattern

All resource management uses RAII:
- Smart pointers (`std::unique_ptr`, `std::shared_ptr`)
- Scoped locks for thread safety
- Automatic cleanup in destructors

### Observer Pattern

MessageBus implements observer pattern:
- Systems subscribe to events
- MessageBus notifies all subscribers when event published
- Decouples event producers from consumers

### Strategy Pattern

Threading strategies use strategy pattern:
- `ThreadingStrategy` enum selects execution model
- `ThreadedSystemManager` applies appropriate strategy
- Easy to add new threading models

---

## Performance Considerations

### Memory Layout

- Components stored contiguously for cache efficiency
- Hot data (frequently accessed) separated from cold data
- Entity IDs are dense (reuse freed IDs)

### Update Frequency

Not all systems update every frame:
- Render: 60 FPS
- Gameplay: 30 FPS (configurable)
- AI: 1-10 FPS (varies by importance)
- Autosave: Every 5 minutes (configurable)

### Batching

- Component queries batched to reduce lock overhead
- Message delivery batched per frame
- Render calls batched by texture/shader

### Profiling Hooks

**File:** `include/ui/PerformanceWindow.h`

Performance metrics tracked:
- Frame time per system
- Component query counts
- Memory usage per component type
- AI attention budget utilization

**Target Performance:**
- Frame time: <16ms (60 FPS)
- Game update: <33ms (30 Hz gameplay)
- Memory: <2GB for typical game
- Load time: <10 seconds

---

## Component Hierarchy

Components are plain data structures (POD when possible).

### Base Component Pattern

```cpp
struct ComponentBase {
    // No virtual functions (keep POD)
    // Use composition over inheritance
};

struct ProvinceComponent {
    ProvinceID id{0};
    std::string name;
    RealmID owner{0};
    TerrainType terrain;
    float area_sq_km;
    glm::vec2 center_position;
};
```

### Composition Example

Instead of inheritance:
```cpp
// BAD (inheritance breaks POD)
struct MilitaryProvinceComponent : ProvinceComponent { ... };

// GOOD (composition)
struct ProvinceComponent { ... };
struct MilitaryComponent { ProvinceID province; ... };
```

---

## Error Handling

### Strategy

- Component access returns nullptr if not found (check before use)
- Configuration loading has fallback defaults
- Save/load operations return status codes
- Critical errors throw exceptions (caught at system level)

### Logging

Integrated logging throughout:
- Info: Normal operation
- Warning: Recoverable issues (missing config, invalid data)
- Error: Operation failures (save failed, resource missing)
- Debug: Verbose diagnostic info (disabled in release)

---

## Configuration System

### GameConfig Structure

**File:** `include/game/config/GameConfig.h`

119+ parameters organized by category:
- Performance (thread count, update rates)
- AI (attention budgets, decision weights)
- Economy (tax rates, production multipliers)
- Military (combat resolution, unit stats)
- Diplomacy (treaty durations, relation decay)

### Hot-Reload

ConfigManager watches GameConfig.json for changes:
1. File modification detected
2. Parse new JSON
3. Validate structure
4. Swap in new config atomically
5. Systems read new values on next update

---

## Testing Strategy

### Unit Tests
- Component serialization
- Configuration parsing
- Type conversions
- Math utilities

### Integration Tests
- System lifecycle (initialize → update → shutdown)
- Component access patterns
- Message bus delivery
- Save/load round-trip

### Performance Tests
- Component query speed
- Threading overhead
- Memory usage
- Frame time budgets

---

**For More Details:**
- AI Context: See `AI_CONTEXT.md`
- Build Instructions: See `BUILD.md`
- User Documentation: See `README.md`
