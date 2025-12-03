# Character System Implementation Plan (CORRECTED)
**Date:** December 3, 2025
**Status:** REVISED AFTER CRITICAL ANALYSIS
**Branch:** claude/review-character-system-01QSndaAXeZYUejtvrzhtcgz

---

## Executive Summary

After comprehensive review, the character system has **excellent foundational architecture** but is **NOT integrated with the game world**. All components and AI exist, but **zero character entities are created**.

**Critical Finding:**
- ✅ All character components implemented (6 components, ~4,000 LOC)
- ✅ CharacterAI system complete with 10 archetypes
- ✅ 150+ historical characters in JSON data
- ❌ **NO CODE creates character entities**
- ❌ **NO CharacterManager/System exists**
- ❌ **Components not registered for serialization**

**Bottom Line:** The system is like a car with all parts manufactured but never assembled. We need integration, not implementation.

---

## What Already Exists (Verified)

### ✅ Complete Character Components

1. **CharacterComponent** (`include/game/components/CharacterComponent.h`)
   - Basic attributes: diplomacy, martial, stewardship, intrigue, learning
   - Health, prestige, gold tracking
   - Relationships: primary title, liege, dynasty
   - Full Clone() implementation for ECS

2. **CharacterRelationshipsComponent** (`include/game/character/CharacterRelationships.h`)
   - 6 relationship types (friend, rival, lover, mentor, student, blood brother)
   - 5 marriage types (normal, matrilineal, political, secret, morganatic)
   - Family tree tracking (parents, siblings, children)
   - Opinion system (-100 to +100)
   - Bond strength with decay tracking

3. **CharacterEducationComponent** (`include/game/character/CharacterEducation.h`)
   - 6 education focuses (diplomacy, martial, stewardship, intrigue, learning, balanced)
   - 5 quality levels (poor, average, good, excellent, outstanding)
   - Skill XP system with level-up mechanics
   - Tutor tracking and quality modifiers
   - Education completion with trait rewards

4. **CharacterLifeEventsComponent** (`include/game/character/CharacterLifeEvents.h`)
   - 100+ life event types categorized:
     - Birth/childhood (3 types)
     - Education (4 types)
     - Relationships (10 types)
     - Achievements (8 types)
     - Religious (4 types)
     - Health (5 types)
     - Political (13 types)
     - Social (4 types)
     - Death (6 types)
   - Event generator with historical context
   - Biography generation from major events
   - Age calculation and tracking

5. **TraitsComponent** (`include/game/components/TraitsComponent.h`, 580 LOC implementation)
   - 9 trait categories (personality, education, lifestyle, physical, mental, health, fame, religious, reputation)
   - Comprehensive modifier system (diplomacy, martial, stewardship, intrigue, learning)
   - AI personality modifiers (ambition, loyalty, honor, greed, boldness, compassion)
   - Trait incompatibility checking (brave vs craven, etc.)
   - Temporary traits with expiry
   - Cached modifier recalculation
   - TraitDatabase singleton with 30+ predefined traits

6. **NobleArtsComponent** (`include/game/components/NobleArtsComponent.h`)
   - 8 cultural skills (poetry, music, painting, philosophy, theology, architecture, strategy, etiquette)
   - Created works tracking
   - Cultural influence calculation

### ✅ Character AI System (Fully Implemented)

**CharacterAI** (`include/game/ai/CharacterAI.h`, `src/game/ai/CharacterAI.cpp` - 1,292 LOC)
- 10 character archetypes with distinct personalities
- 6 personality traits affecting decision-making
- 7 mood states (content, happy, stressed, angry, afraid, ambitious, desperate)
- 4 decision queues (plots, proposals, relationships, personal)
- Memory system (30 max memories with time-based decay)
- 11 ambition types with pursuit logic
- Thread-safe with mutex protection
- Integrated with AIDirector (updates in main loop line 1613)

### ✅ Data Assets

**Historical Characters** (`data/characters/characters_11th_century.json`)
- 150+ historical figures
- 9 regions (Western Europe, Eastern Europe, British Isles, Scandinavia, etc.)
- Complete stats and traits per character
- Historical relationships and events

### ✅ Test Coverage

**Test Files Exist:**
- `tests/test_character_improvements.cpp` - Traits, education, relationships
- `tests/performance/test_character_system_performance.cpp` - Performance benchmarks
- Character AI tests in AI system test suites

---

## What's Actually Missing (The Real Gap)

### ❌ **No Character Entity Creation**

**Problem:** Components exist, but NO CODE instantiates character entities.

**Missing:**
```cpp
// This does NOT exist anywhere:
EntityID characterEntity = g_entity_manager->CreateEntity();
g_component_access_manager->AddComponent<CharacterComponent>(characterEntity);
g_component_access_manager->AddComponent<TraitsComponent>(characterEntity);
// etc...
```

### ❌ **No CharacterManager/System**

Following the existing pattern (PopulationSystem, EconomicSystem, MilitarySystem), we need:

```cpp
class CharacterSystem {
public:
    CharacterSystem(ComponentAccessManager& cam, ThreadSafeMessageBus& bus);

    // Core entity management
    EntityID CreateCharacter(const CharacterData& data);
    void DestroyCharacter(EntityID id);

    // Data loading
    void LoadHistoricalCharacters(const std::string& json_path);

    // Lifecycle
    void Update(float deltaTime);  // Aging, education, relationships

private:
    ComponentAccessManager& m_componentAccess;
    ThreadSafeMessageBus& m_messageBus;
    std::unordered_map<EntityID, bool> m_activeCharacters;
};
```

**Current Status:** Does not exist.

### ❌ **No Component Registration**

Components must be registered for:
- Serialization/save-load
- Dynamic component queries
- Message bus integration

**Required in main.cpp:**
```cpp
// This is NOT present in main.cpp initialization:
g_component_access_manager->RegisterComponentType<CharacterComponent>();
g_component_access_manager->RegisterComponentType<CharacterRelationshipsComponent>();
g_component_access_manager->RegisterComponentType<CharacterEducationComponent>();
g_component_access_manager->RegisterComponentType<CharacterLifeEventsComponent>();
g_component_access_manager->RegisterComponentType<TraitsComponent>();
g_component_access_manager->RegisterComponentType<NobleArtsComponent>();
```

### ❌ **No System Integration**

**Blocked systems waiting for characters:**

1. **InfluenceSystem** (`src/game/diplomacy/InfluenceSystem.cpp:761`)
   ```cpp
   // TODO: Detect new character influences when character system is implemented
   ```

2. **Character marriage ties** (`docs/diplomacy/PHASE3_STATUS.md:72`)
   ```cpp
   // TODO: Implement actual marriage tie checking when character system is ready
   ```

3. **RealmManager** - No character-realm linking
4. **AIDirector** - Can create AI actors but has no character entities to attach to

---

## Corrected Implementation Plan

### **Phase 1: Character Entity Infrastructure** (CRITICAL - 8-12 hours)

**Priority: HIGHEST** - Nothing else works without this.

#### 1.1 CharacterSystem Class (4-5 hours)

**Location:** `src/game/systems/CharacterSystem.{h,cpp}`

**Architecture:** Follow existing system pattern

```cpp
class CharacterSystem {
public:
    CharacterSystem(
        core::ecs::ComponentAccessManager& componentAccess,
        core::threading::ThreadSafeMessageBus& messageBus
    );

    ~CharacterSystem();

    // Entity creation
    types::EntityID CreateCharacter(
        const std::string& name,
        uint32_t age,
        const CharacterStats& stats
    );

    // Bulk loading
    bool LoadHistoricalCharacters(const std::string& json_path);

    // Entity queries
    types::EntityID GetCharacterByName(const std::string& name) const;
    std::vector<types::EntityID> GetAllCharacters() const;
    std::vector<types::EntityID> GetCharactersByRealm(types::EntityID realmId) const;

    // Lifecycle management
    void Update(float deltaTime);
    void DestroyCharacter(types::EntityID characterId);

    // Integration hooks
    void OnRealmCreated(types::EntityID realmId, types::EntityID rulerId);
    void OnCharacterDeath(types::EntityID characterId);

private:
    void UpdateAging(float deltaTime);
    void UpdateEducation(float deltaTime);
    void UpdateRelationships(float deltaTime);
    void UpdateLifeEvents(float deltaTime);
    void UpdateTraits(float deltaTime);

    core::ecs::ComponentAccessManager& m_componentAccess;
    core::threading::ThreadSafeMessageBus& m_messageBus;

    // Character tracking
    std::unordered_map<types::EntityID, std::string> m_characterNames;
    std::unordered_map<std::string, types::EntityID> m_nameToEntity;
    std::vector<types::EntityID> m_allCharacters;

    // Timing
    float m_ageTimer = 0.0f;
    float m_relationshipTimer = 0.0f;
};
```

**Threading Strategy:** `BACKGROUND` (independent calculations, no UI dependencies)

**Key Methods:**

1. **CreateCharacter()** - Core entity spawning
   ```cpp
   EntityID CharacterSystem::CreateCharacter(const string& name, uint32_t age, const CharacterStats& stats) {
       // 1. Create entity via EntityManager
       EntityID id = m_componentAccess.GetEntityManager()->CreateEntity();

       // 2. Add CharacterComponent
       auto charComp = std::make_unique<CharacterComponent>();
       charComp->SetName(name);
       charComp->SetAge(age);
       charComp->SetDiplomacy(stats.diplomacy);
       charComp->SetMartial(stats.martial);
       // ... set all stats
       m_componentAccess.AddComponent(id, std::move(charComp));

       // 3. Add supporting components
       m_componentAccess.AddComponent(id, std::make_unique<TraitsComponent>());
       m_componentAccess.AddComponent(id, std::make_unique<CharacterRelationshipsComponent>(id));
       m_componentAccess.AddComponent(id, std::make_unique<CharacterEducationComponent>(id));
       m_componentAccess.AddComponent(id, std::make_unique<CharacterLifeEventsComponent>(id));
       m_componentAccess.AddComponent(id, std::make_unique<NobleArtsComponent>());

       // 4. Track character
       m_characterNames[id] = name;
       m_nameToEntity[name] = id;
       m_allCharacters.push_back(id);

       // 5. Post creation event
       CharacterCreatedEvent event{id, name, age};
       m_messageBus.Publish(event);

       return id;
   }
   ```

2. **LoadHistoricalCharacters()** - JSON data import
   ```cpp
   bool CharacterSystem::LoadHistoricalCharacters(const string& json_path) {
       // Parse characters_11th_century.json
       // For each character:
       //   - CreateCharacter() with historical stats
       //   - Add historical traits
       //   - Set up family relationships
       //   - Link to realms if ruler
       //   - Create life events for historical achievements

       // Return count of loaded characters
   }
   ```

3. **Update()** - Lifecycle management
   ```cpp
   void CharacterSystem::Update(float deltaTime) {
       // Age characters (once per in-game year)
       m_ageTimer += deltaTime;
       if (m_ageTimer >= YEAR_IN_SECONDS) {
           UpdateAging(m_ageTimer);
           m_ageTimer = 0.0f;
       }

       // Decay relationships (periodic)
       m_relationshipTimer += deltaTime;
       if (m_relationshipTimer >= 30.0f) { // Every 30 seconds real-time
           UpdateRelationships(m_relationshipTimer);
           m_relationshipTimer = 0.0f;
       }

       // Process active education
       UpdateEducation(deltaTime);

       // Remove expired temporary traits
       UpdateTraits(deltaTime);

       // Trigger pending life events
       UpdateLifeEvents(deltaTime);
   }
   ```

**Estimated Time:** 4-5 hours
- Setup: 30 min
- CreateCharacter: 1 hour
- LoadHistoricalCharacters: 2 hours
- Update methods: 1.5 hours

#### 1.2 Component Registration (30 minutes)

**Location:** `apps/main.cpp` in `InitializeEnhancedSystems()`

**After line 647 (after ThreadedSystemManager creation):**

```cpp
// Register character components for serialization
CORE_STREAM_INFO("Main") << "Registering character components...";
g_component_access_manager->RegisterComponentType<game::character::CharacterComponent>();
g_component_access_manager->RegisterComponentType<game::character::CharacterRelationshipsComponent>();
g_component_access_manager->RegisterComponentType<game::character::CharacterEducationComponent>();
g_component_access_manager->RegisterComponentType<game::character::CharacterLifeEventsComponent>();
g_component_access_manager->RegisterComponentType<game::character::TraitsComponent>();
g_component_access_manager->RegisterComponentType<game::character::NobleArtsComponent>();
CORE_STREAM_INFO("Main") << "Character components registered";
```

**Estimated Time:** 30 minutes (including testing)

#### 1.3 System Initialization (1 hour)

**Location:** `apps/main.cpp` in `InitializeEnhancedSystems()`

**After Realm System initialization (line 719):**

```cpp
// Character System - Character entities and lifecycle management
g_character_system = std::make_unique<game::character::CharacterSystem>(
    *g_component_access_manager, *g_thread_safe_message_bus);
auto char_strategy = game::config::helpers::GetThreadingStrategyForSystem("CharacterSystem");
std::cout << "Character System: " << game::types::TypeRegistry::ThreadingStrategyToString(char_strategy) << std::endl;

// Load historical characters
std::cout << "Loading historical characters..." << std::endl;
if (g_character_system->LoadHistoricalCharacters("data/characters/characters_11th_century.json")) {
    std::cout << "Historical characters loaded: " << g_character_system->GetAllCharacters().size() << std::endl;
} else {
    std::cerr << "WARNING: Failed to load historical characters" << std::endl;
}
```

**Global variable declaration (with other systems ~line 397):**

```cpp
static std::unique_ptr<game::character::CharacterSystem> g_character_system;
```

**Main loop update (after line 1617):**

```cpp
// Character System - Aging, education, relationships
if (g_character_system) {
    g_character_system->Update(delta_time);
}
```

**Shutdown (after line 1703):**

```cpp
if (g_character_system) {
    CORE_STREAM_INFO("Main") << "Shutting down character system...";
    g_character_system.reset();
}
```

**Estimated Time:** 1 hour

#### 1.4 CMakeLists.txt Update (15 minutes)

**Location:** `CMakeLists.txt`

Add to sources:
```cmake
# Character System
src/game/systems/CharacterSystem.cpp
```

**Estimated Time:** 15 minutes

---

### **Phase 2: RealmManager Integration** (3-4 hours)

**Priority: HIGH** - Link characters to realms

#### 2.1 Character-Realm Linking

When a realm is created with a ruler, create the character entity:

**Location:** `src/game/realm/RealmManager.cpp`

**In `CreateRealm()` method:**

```cpp
// After realm creation, create ruler character if specified
if (rulerName != "") {
    EntityID rulerCharacterId = g_character_system->CreateCharacter(
        rulerName,
        30, // Default age
        CharacterStats::DefaultRuler() // Decent stats for ruler
    );

    // Add RulerComponent to realm
    auto rulerComp = std::make_unique<RulerComponent>();
    rulerComp->character_id = rulerCharacterId;
    rulerComp->realm_id = realmId;
    m_componentAccess.AddComponent(realmId, std::move(rulerComp));

    // Link character's primary title to this realm
    auto charComp = m_componentAccess.GetComponent<CharacterComponent>(rulerCharacterId);
    if (charComp) {
        charComp->SetPrimaryTitle(realmId);
    }
}
```

**Challenges:**
- RealmManager doesn't have direct access to CharacterSystem
- Need to pass CharacterSystem reference or use message bus
- Could use message bus: RealmCreatedEvent → CharacterSystem subscribes

**Estimated Time:** 3-4 hours

---

### **Phase 3: AIDirector Integration** (2-3 hours)

**Priority: HIGH** - Link AI actors to character entities

#### 3.1 AI Actor-Character Binding

**Current:** AIDirector creates CharacterAI actors but they're not linked to entities

**Solution:** When creating AI actors, pass character EntityID

**Location:** `src/game/ai/AIDirector.cpp`

**Modify `CreateCharacterActor()`:**

```cpp
void AIDirector::CreateCharacterActorForRuler(EntityID characterId, const string& name) {
    // Create AI actor
    auto actor = CharacterAIFactory::CreateAmbitiousNoble(
        m_nextActorId++,
        characterId,  // This EntityID links AI to character entity
        name
    );

    actor->SetComponentAccess(m_componentAccess);

    m_characterActors[characterId] = std::move(actor);

    CORE_STREAM_INFO("AIDirector") << "Created AI actor for character " << name
                                     << " (entity " << characterId << ")";
}
```

**Update loop modification:**

```cpp
void AIDirector::Update(float deltaTime) {
    // For each character with AI
    for (auto& [charId, actor] : m_characterActors) {
        // Actor can now read CharacterComponent via charId
        auto charComp = m_componentAccess->GetComponent<CharacterComponent>(charId);
        if (charComp && !charComp->IsDead()) {
            actor->UpdateAmbitions();
            actor->UpdateRelationships();
            actor->ExecuteDecisions();
        }
    }
}
```

**Integration point:** When CharacterSystem loads historical characters, notify AIDirector to create AI actors for rulers:

```cpp
// In CharacterSystem::LoadHistoricalCharacters()
for (auto& [charId, isRuler] : loadedCharacters) {
    if (isRuler) {
        CharacterNeedsAIEvent event{charId, characterName};
        m_messageBus.Publish(event);
    }
}
```

**Estimated Time:** 2-3 hours

---

### **Phase 4: Diplomacy System Integration** (4-5 hours)

**Priority: MEDIUM** - Unblock TODOs in InfluenceSystem

#### 4.1 Character Influence Detection

**Location:** `src/game/diplomacy/InfluenceSystem.cpp:752`

**Replace TODO with:**

```cpp
void InfluenceSystem::UpdateCharacterInfluences(types::EntityID realm_id) {
    auto influenceComp = m_componentAccess.GetComponent<InfluenceComponent>(realm_id);
    if (!influenceComp) return;

    // Get ruler character
    auto rulerComp = m_componentAccess.GetComponent<RulerComponent>(realm_id);
    if (!rulerComp) return;

    EntityID rulerCharId = rulerComp->character_id;

    // Get character relationships
    auto relComp = m_componentAccess.GetComponent<CharacterRelationshipsComponent>(rulerCharId);
    if (!relComp) return;

    // Add personal influence for each friend
    for (auto friendId : relComp->GetFriends()) {
        auto friendCharComp = m_componentAccess.GetComponent<CharacterComponent>(friendId);
        if (friendCharComp) {
            EntityID friendRealm = friendCharComp->GetPrimaryTitle();
            if (friendRealm != 0) {
                // Bond strength affects influence amount
                double bondStrength = relComp->GetFriendshipBondStrength(friendId);
                float influenceAmount = static_cast<float>(bondStrength / 100.0 * 20.0); // 0-20 influence

                influenceComp->AddPersonalInfluence(friendRealm, influenceAmount);
            }
        }
    }
}
```

#### 4.2 Marriage Tie Checking

**Location:** `docs/diplomacy/PHASE3_STATUS.md:72`

**Implement actual marriage checking:**

```cpp
bool InfluenceSystem::HasMarriageTies(EntityID realm1, EntityID realm2) const {
    // Get rulers of both realms
    auto ruler1Comp = m_componentAccess.GetComponent<RulerComponent>(realm1);
    auto ruler2Comp = m_componentAccess.GetComponent<RulerComponent>(realm2);
    if (!ruler1Comp || !ruler2Comp) return false;

    // Check if rulers are married to each other
    auto rel1 = m_componentAccess.GetComponent<CharacterRelationshipsComponent>(ruler1Comp->character_id);
    auto rel2 = m_componentAccess.GetComponent<CharacterRelationshipsComponent>(ruler2Comp->character_id);

    if (rel1 && rel2) {
        return rel1->IsMarriedTo(ruler2Comp->character_id) ||
               rel2->IsMarriedTo(ruler1Comp->character_id);
    }

    return false;
}
```

**Estimated Time:** 4-5 hours

---

### **Phase 5: Character UI** (6-8 hours)

**Priority: MEDIUM-LOW** - Visibility and debugging

#### 5.1 CharacterListWindow

**Location:** `src/ui/CharacterListWindow.{h,cpp}`

Following existing pattern (EconomyWindow, MilitaryWindow, etc.)

**Features:**
- Scrollable list of all characters
- Filter by realm, age, traits
- Sort by attributes (diplomacy, martial, etc.)
- Click to open CharacterDetailWindow
- Portrait thumbnails using PortraitGenerator

**Estimated Time:** 3-4 hours

#### 5.2 CharacterDetailWindow

**Location:** `src/ui/CharacterDetailWindow.{h,cpp}`

**Features:**
- Full character portrait (large)
- All attributes with trait modifiers shown
- Relationships web (friends, rivals, family)
- Life events timeline
- Education progress bar
- Current ambitions (if has AI)
- Trait list with descriptions

**Estimated Time:** 3-4 hours

#### 5.3 UI Integration

**Location:** `src/ui/LeftSidebar.cpp`

Add character button to sidebar:

```cpp
if (ImGui::Button("Characters", ImVec2(sidebar_width - 20, 40))) {
    WindowManager::Get().ToggleWindow("CharacterList");
}
```

**Estimated Time:** 30 minutes

---

### **Phase 6: Save/Load Support** (8-10 hours)

**Priority: LOW** - Can defer until later

#### 6.1 Component Serialization

Each character component needs:
- Serialize() method
- Deserialize() method
- Handle EntityID references correctly

**Challenge:** Graph serialization (relationships form cycles)

**Strategy:**
1. Serialize all character entities with local IDs
2. Build EntityID mapping table
3. Serialize relationships with local ID references
4. On load, restore EntityID mapping
5. Resolve relationship references

**Estimated Time:** 8-10 hours

---

## Revised Time Estimates

### By Phase

| Phase | Task | Hours | Priority |
|-------|------|-------|----------|
| 1 | Character entity infrastructure | 8-12 | CRITICAL |
| 2 | RealmManager integration | 3-4 | HIGH |
| 3 | AIDirector integration | 2-3 | HIGH |
| 4 | Diplomacy system integration | 4-5 | MEDIUM |
| 5 | Character UI | 6-8 | MEDIUM-LOW |
| 6 | Save/load support | 8-10 | LOW |
| **TOTAL** | | **31-42 hours** | |

### Critical Path (Minimum Viable)

For characters to exist and function:
1. Phase 1: CharacterSystem (8-12 hours) ← **START HERE**
2. Phase 2: RealmManager integration (3-4 hours)
3. Phase 3: AIDirector integration (2-3 hours)

**Minimum viable:** 13-19 hours

Everything else is enhancement/polish.

---

## Success Criteria

### Phase 1 Complete When:
- [ ] CharacterSystem class exists and compiles
- [ ] Can call `CreateCharacter()` and entity is created
- [ ] All 6 components attached to character entity
- [ ] `LoadHistoricalCharacters()` loads from JSON
- [ ] At least 10 characters spawn on game start
- [ ] Components registered in main.cpp
- [ ] CharacterSystem::Update() called in main loop

### Phase 2 Complete When:
- [ ] Realm creation triggers character creation
- [ ] RulerComponent links to character entity
- [ ] Character's primary title links to realm
- [ ] Can query "who rules this realm?" via components

### Phase 3 Complete When:
- [ ] AIDirector creates AI actors for ruler characters
- [ ] AI actors can read/modify character components
- [ ] AI decisions affect character stats/relationships
- [ ] CharacterAI mood/ambitions drive actual behavior

### Phase 4 Complete When:
- [ ] InfluenceSystem TODOs removed
- [ ] Character friendships grant diplomatic influence
- [ ] Marriage ties detected correctly
- [ ] Personal relationships affect realm diplomacy

---

## Risk Assessment

### High Risk
- **Graph serialization complexity** - Circular references in relationships
- **EntityID versioning** - Git log mentions this as "CRITICAL FIX"
- **Performance with 1000+ characters** - Need profiling

### Medium Risk
- **AI-Character synchronization** - AI state vs component state consistency
- **Relationship graph integrity** - Ensuring bidirectional links stay consistent

### Low Risk
- **Component registration** - Straightforward, well-documented pattern
- **UI integration** - Follows existing WindowManager pattern

---

## Dependencies

### External Dependencies
- EntityManager (exists, stable)
- ComponentAccessManager (exists, stable)
- ThreadSafeMessageBus (exists, stable)
- RealmManager (exists, needs integration point)
- AIDirector (exists, needs binding to entities)
- InfluenceSystem (waiting for character integration)

### Data Dependencies
- `data/characters/characters_11th_century.json` (exists)
- Trait definitions (implemented in TraitDatabase)
- Configuration for threading strategy (GameConfig)

---

## Testing Strategy

### Unit Tests (Write First - TDD)

**Location:** `tests/test_character_system.cpp`

```cpp
TEST(CharacterSystem, CreateCharacter) {
    // Verify entity creation
    // Verify all components attached
    // Verify tracking maps updated
}

TEST(CharacterSystem, LoadHistoricalCharacters) {
    // Load test JSON
    // Verify count
    // Verify stats match JSON
}

TEST(CharacterSystem, UpdateAging) {
    // Create character age 20
    // Advance time by 1 year
    // Verify age is 21
}
```

### Integration Tests

**Location:** `tests/integration/test_character_integration.cpp`

```cpp
TEST(CharacterIntegration, RealmRulerLink) {
    // Create realm with ruler
    // Verify character entity created
    // Verify RulerComponent links correctly
}

TEST(CharacterIntegration, AIActorBinding) {
    // Create character
    // Spawn AI actor
    // Verify AI can read character components
}
```

### Performance Tests

**Location:** `tests/performance/test_character_system_performance.cpp` (already exists)

- 1000 character creation benchmark
- 1000 character update benchmark
- Relationship query performance
- Memory usage profiling

---

## Architectural Notes

### Threading Strategy

**CharacterSystem should use `BACKGROUND` threading:**

**Reasoning:**
- Independent calculations (aging, education XP, relationship decay)
- No direct UI dependencies
- High parallelization potential (each character updates independently)
- Similar to PopulationSystem pattern

**Configuration:** Add to `data/config/threading_config.json`:

```json
{
  "CharacterSystem": {
    "strategy": "BACKGROUND",
    "rationale": "Character lifecycle updates are independent calculations with high parallelization potential"
  }
}
```

### Message Bus Events

**Events to publish:**

```cpp
struct CharacterCreatedEvent {
    EntityID characterId;
    std::string name;
    uint32_t age;
};

struct CharacterDiedEvent {
    EntityID characterId;
    LifeEventType deathType;
};

struct CharacterNeedsAIEvent {
    EntityID characterId;
    std::string name;
};

struct RelationshipChangedEvent {
    EntityID character1;
    EntityID character2;
    RelationshipType type;
    float opinionDelta;
};
```

**Subscribers:**
- AIDirector subscribes to CharacterNeedsAIEvent
- InfluenceSystem subscribes to RelationshipChangedEvent
- UI subscribes to CharacterCreatedEvent/DiedEvent for updates

---

## Migration Path

### Immediate (Week 1)
1. Implement Phase 1 (CharacterSystem)
2. Test with 10 characters manually created
3. Verify components work correctly

### Short-term (Week 2)
1. Implement Phase 2 (RealmManager integration)
2. Implement Phase 3 (AIDirector integration)
3. Load historical characters
4. Profile performance

### Medium-term (Week 3-4)
1. Implement Phase 4 (Diplomacy integration)
2. Implement Phase 5 (UI)
3. Extensive testing with full character set

### Long-term (Month 2+)
1. Implement Phase 6 (Save/load)
2. Optimization based on profiling
3. Additional features (events, traits triggering)

---

## Conclusion

The character system has **excellent architecture** but needs **integration work**, not reimplementation. The critical path is:

1. **CharacterSystem class** - Creates entities
2. **JSON loader** - Populates world with characters
3. **System integration** - Links to realms and AI

Everything else is polish and can be done incrementally.

**Recommendation:** Start with Phase 1, get characters spawning, then iterate on integration.

---

**Document Version:** 2.0 (Corrected)
**Last Updated:** December 3, 2025
**Next Review:** After Phase 1 completion
