# Character System Architecture

## Overview

The Character System manages all character entities in the game, including their lifecycle, relationships, education, and integration with other game systems (Realm, AI, Diplomacy).

## System Components

### Core Components

1. **CharacterSystem** - Main character management system
   - Creates and destroys character entities
   - Loads historical characters from JSON
   - Manages character lifecycle updates
   - Tracks all active characters
   - Publishes character-related events

2. **CharacterComponent** - Character data storage
   - Name, age, stats (diplomacy, martial, etc.)
   - Primary title (realm ownership)
   - Health, prestige, gold

3. **CharacterRelationshipsComponent** - Social connections
   - Friends, rivals, spouses
   - Opinion modifiers
   - Family relationships

4. **TraitsComponent** - Character traits
   - Personality traits (brave, cruel, etc.)
   - Genetic traits
   - Learned skills

5. **CharacterEducation** - Learning and skill development
   - Education focus areas
   - Skill progression
   - Mentorship relationships

6. **CharacterLifeEvents** - Historical record
   - Major life events
   - Achievements and failures
   - Decision history

## System Interaction Diagram

```
┌─────────────────────┐
│   RealmManager      │
│  CreateRealm()      │
└──────────┬──────────┘
           │ publishes RealmCreated event
           │
           ▼
┌─────────────────────────────────┐
│   ThreadSafeMessageBus          │
│   (event delivery infrastructure)│
└──────────┬──────────────────────┘
           │ delivers to subscribers
           │
           ▼
┌─────────────────────┐      ┌──────────────────┐
│  CharacterSystem    │      │   AIDirector     │
│  OnRealmCreated()   │      │  (also subscribed)│
└──────────┬──────────┘      └──────────────────┘
           │
           │ CreateCharacter(ruler_name, stats)
           ▼
┌─────────────────────┐
│  EntityManager      │
│  CreateEntity()     │
│  AddComponent<...>  │
└──────────┬──────────┘
           │ returns versioned EntityID
           ▼
┌─────────────────────┐
│  CharacterSystem    │
│  publishes          │
│  CharacterNeedsAI   │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   AIDirector        │
│  CreateCharacterAI()│
│  (handles event)    │
└─────────────────────┘
           │
           ▼
┌─────────────────────┐
│   CharacterAI       │
│  (decision making)  │
└─────────────────────┘
```

## Initialization Sequence

**CRITICAL:** Systems must initialize in this exact order to ensure dependencies are satisfied:

1. **EntityManager** - Must exist before any entities
2. **ComponentAccessManager** - Provides component access interface
3. **ThreadSafeMessageBus** - Event infrastructure must exist before subscribers
4. **RealmManager** - Creates realms and publishes RealmCreated events
5. **CharacterSystem** - Subscribes to RealmCreated in constructor
6. **AIDirector** - Subscribes to CharacterNeedsAI in constructor
7. **InfluenceSystem** (optional) - Call SetCharacterSystem() before Initialize()
8. Load historical characters - After all systems initialized

### Initialization Code Pattern (main.cpp)

```cpp
// 1. Core infrastructure
g_entity_manager = std::make_unique<EntityManager>();
g_component_access_manager = std::make_unique<ComponentAccessManager>(*g_entity_manager);
g_thread_safe_message_bus = std::make_unique<ThreadSafeMessageBus>();

// 2. Game systems (order matters!)
g_realm_manager = std::make_unique<RealmManager>(*g_component_access_manager, *g_thread_safe_message_bus);

g_character_system = std::make_unique<CharacterSystem>(*g_component_access_manager, *g_thread_safe_message_bus);
// CharacterSystem constructor subscribes to RealmCreated

g_ai_director = std::make_unique<AIDirector>(*g_thread_safe_message_bus);
// AIDirector constructor subscribes to CharacterNeedsAI

// 3. Optional integrations
if (g_influence_system) {
    g_influence_system->SetCharacterSystem(g_character_system.get());
}

// 4. Initialize all systems
g_realm_manager->Initialize();
g_character_system->Initialize();
g_ai_director->Initialize();

// 5. Load initial data
g_character_system->LoadHistoricalCharacters("data/characters/characters_11th_century.json");
```

## Event Flow

### Character Creation Flow

1. **Trigger**: RealmManager creates new realm
2. **Event**: `RealmCreated { realmId, rulerId }`
3. **Subscriber**: CharacterSystem::OnRealmCreated()
4. **Action**:
   - Convert legacy EntityID to versioned EntityID
   - Create ruler character if rulerId valid
   - Publish `CharacterNeedsAI` event
5. **Subscriber**: AIDirector receives CharacterNeedsAI
6. **Action**: Create CharacterAI actor for decision-making

### Character Lifecycle

- **Birth**: CreateCharacter() → Add components → Publish events
- **Aging**: Update() processes monthly aging
- **Education**: CharacterEducation updates skills
- **Relationships**: CharacterRelationships tracks social bonds
- **Death**: DestroyCharacter() → Remove components → Clean up references

## Threading Model

### Thread Safety Policy

**ALL GAME LOGIC RUNS ON MAIN THREAD**

- **CharacterSystem**: NOT thread-safe, main thread only
  - Constructor/Destructor: Main thread
  - Update(): Main thread (game loop)
  - Event handlers: Main thread (events published from main thread)
  - CreateCharacter/DestroyCharacter: Main thread only
  - Query methods: Main thread only

- **ThreadSafeMessageBus**: Thread-safe for publish/subscribe
  - Can publish from any thread
  - Event handlers execute on publisher's thread
  - Since game logic is single-threaded, handlers run on main thread

- **AIDirector**: Main thread only
  - Event handlers check m_shouldStop flag for shutdown safety

### Concurrency Notes

- No mutexes needed in CharacterSystem (single-threaded access)
- Event handlers execute synchronously on caller's thread
- ThreadSafeMessageBus doesn't imply thread-safety of subscribers
- Future: If multi-threaded updates needed, add std::shared_mutex

## Error Handling Strategy

### Character Creation Failures

- **EntityManager null**: Log ERROR, return invalid EntityID
- **CreateEntity fails**: Log ERROR, return invalid EntityID
- **Component creation fails**: Rollback entity, return invalid EntityID
- **Validation fails**: Log ERROR with details, return invalid EntityID

### JSON Loading Failures

- **File not found**: Log ERROR, return false
- **Parse error**: Log ERROR with JSON error details, return false
- **Invalid character data**: Log WARN, skip character, continue loading
- **Missing 'characters' array**: Log ERROR, return false

### Strategy

- **Fail gracefully**: Character creation errors don't crash game
- **Partial success**: Load as many characters as possible
- **Detailed logging**: All errors logged with context
- **Return status**: Methods return invalid EntityID or false on failure
- **No exceptions**: Errors handled locally, no propagation

## EntityID Type System

### Two Types in Codebase

1. **game::types::EntityID** (legacy)
   - Type: `uint32_t`
   - Used by: RealmManager, legacy systems
   - No version tracking
   - Risk: ID reuse after entity deletion

2. **core::ecs::EntityID** (versioned)
   - Type: `struct { uint64_t id; uint32_t version; }`
   - Used by: CharacterSystem, EntityManager
   - Version tracking prevents use-after-free
   - Safer for long-lived references

### Conversion Strategy

- **CharacterSystem maintains bidirectional mapping**:
  - `m_legacyToVersioned`: Maps legacy ID → versioned EntityID
  - Updated in CreateCharacter() and DestroyCharacter()
  - Helper: `LegacyToVersionedEntityID()`

- **OnRealmCreated() conversion**:
  ```cpp
  void OnRealmCreated(types::EntityID legacyRulerId) {
      core::ecs::EntityID versionedId = LegacyToVersionedEntityID(legacyRulerId);
      if (versionedId.IsValid()) {
          // Safe to use versioned ID
      }
  }
  ```

## Integration Points

### Realm System Integration

- **Phase 2**: RealmManager → CharacterSystem
  - RealmCreated event creates ruler character
  - Characters linked to realms via primary_title field

### AI System Integration

- **Phase 3**: CharacterSystem → AIDirector
  - CharacterNeedsAI event triggers AI actor creation
  - CharacterAI makes decisions for character

### Diplomacy System Integration

- **Phase 4**: CharacterSystem → InfluenceSystem
  - SetCharacterSystem() links systems
  - Character relationships affect diplomatic influence
  - Marriage ties provide dynasty bonuses

### Religion System Integration

- **Future**: Character faith and piety
- **Future**: Religious events affect character traits

## Data Files

### Historical Characters JSON

**Location**: `data/characters/characters_11th_century.json`

**Format**:
```json
{
  "characters": [
    {
      "name": "William the Conqueror",
      "age": 43,
      "stats": {
        "diplomacy": 12,
        "martial": 16,
        "stewardship": 14,
        "intrigue": 10,
        "learning": 8,
        "health": 95.0,
        "prestige": 500.0,
        "gold": 10000.0
      }
    }
  ]
}
```

**Validation**:
- Name: 1-64 characters
- Age: 0-120
- Stats: 0-20 for attributes
- Health: 0-100

## Future Improvements

### Performance Optimizations

1. **Spatial indexing by realm**
   - Current: `GetCharactersByRealm()` is O(N) scan
   - Improvement: Index characters by primary_title
   - Benefit: O(1) realm lookups

2. **Event batching**
   - Batch multiple character creations
   - Single event with vector of IDs

### Architecture Improvements

1. **Unified EntityID type**
   - Migrate all systems to core::ecs::EntityID
   - Eliminate legacy/versioned conversion

2. **Configuration system**
   - Move hardcoded paths to game_config.json
   - Configurable archetype selection logic

3. **Thread-safe queries**
   - Add std::shared_mutex for concurrent reads
   - Enable background query threads

## Testing Recommendations

### Unit Tests Needed

- Character creation with valid/invalid data
- EntityID conversion edge cases
- Event handler ordering
- Error handling paths

### Integration Tests Needed

- Full initialization sequence
- Realm creation → Character creation flow
- Character → AI actor creation
- Historical character loading

### Performance Tests Needed

- 10,000 character creation
- GetCharactersByRealm() with 1000+ characters
- Update() with large character counts
