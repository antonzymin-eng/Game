# Architecture Quick Reference Card
*For developers working on Mechanica Imperii*
*Keep this visible during development!*

## 🎯 Most Important Rules

### 1. Component Inheritance
```cpp
// ✅ DO THIS
class MyComponent : public ::game::core::Component<MyComponent> {
    // Your data here
};

// ❌ DON'T DO THIS
class MyComponent : public ::game::core::IComponent {  // Missing CRTP!
class MyComponent : public core::ecs::Component<MyComponent> {  // Less common
```

### 2. Namespace Structure
```
core::ecs          → ECS foundation (EntityManager, MessageBus)
game::core         → Interfaces (IComponent, ISystem, ISerializable)
game::realm        → Realm/political systems
game::population   → Population systems
game::economy      → Economic systems
ui                 → User interface
```

### 3. EntityID Conversions
```cpp
// Game ID → ECS ID
game::types::EntityID gameId = 12345;
core::ecs::EntityID ecsId = core::ecs::EntityID(gameId);

// ECS ID → Game ID
core::ecs::EntityID ecsId = entity;
game::types::EntityID gameId = ecsId.id;  // Extract id field
```

### 4. Include Paths (Always Absolute)
```cpp
// ✅ CORRECT
#include "core/ECS/EntityManager.h"
#include "game/realm/RealmComponents.h"
#include "core/types/game_types.h"

// ❌ WRONG
#include "EntityManager.h"           // Relative
#include "../realm/RealmComponents.h" // Relative
```

## 📋 Before Making Changes

### Pre-Work Checklist
- [ ] Read relevant sections of `ARCHITECTURE-DATABASE.md`
- [ ] Check which ECS layer your change affects
- [ ] Verify namespace consistency
- [ ] Consult system dependency matrix
- [ ] Review component inheritance pattern needed

### When Adding a Component
1. Inherit from `::game::core::Component<YourComponent>`
2. Include `"core/ECS/IComponent.h"`
3. Add to your system namespace (e.g., `game::population`)
4. Register in system's `Initialize()` method
5. Implement serialization if needed

### When Adding a System
1. Check dependency matrix in `ARCHITECTURE-DATABASE.md`
2. Implement `ISystem` interface
3. Add `GetThreadingStrategy()` method
4. Inherit from `ISerializable` if state needs saving
5. Register component types in `Initialize()`
6. Subscribe to events in `SubscribeToEvents()`

## 🔧 Common Patterns

### Component Access (Thread-Safe)
```cpp
// Read access
auto result = m_access_manager->GetComponent<MyComponent>(entityId);
if (result.IsValid()) {
    const auto* comp = result.Get();
    // Use comp (read-only)
}

// Write access
auto guard = m_access_manager->GetComponentForWrite<MyComponent>(entityId);
if (guard.IsValid()) {
    auto* comp = guard.Get();
    comp->value = 42;  // Modify
}
```

### Direct EntityManager Access
```cpp
auto em = m_access_manager->GetEntityManager();
auto comp = em->GetComponent<MyComponent>(core::ecs::EntityID(gameId));
if (comp) {
    // Use component
}
```

### Message Bus Events
```cpp
// Subscribe
m_message_bus->Subscribe<MyEventType>(
    [this](const auto& event) {
        HandleMyEvent(event);
    }
);

// Publish
MyEventType event;
event.entity_id = target;
m_message_bus->Publish(event);
```

## 🚨 Common Pitfalls

### ❌ Pitfall 1: Wrong Base Class
```cpp
// Wrong - direct IComponent inheritance
class MyComponent : public game::core::IComponent { };

// Right - CRTP template
class MyComponent : public ::game::core::Component<MyComponent> { };
```

### ❌ Pitfall 2: Relative Includes
```cpp
// Wrong
#include "EntityManager.h"
#include "../config/GameConfig.h"

// Right
#include "core/ECS/EntityManager.h"
#include "game/config/GameConfig.h"
```

### ❌ Pitfall 3: Mixing EntityID Types
```cpp
// Wrong - type mismatch
core::ecs::EntityManager* em = ...;
game::types::EntityID gameId = 123;
em->GetComponent<T>(gameId);  // Compile error!

// Right - convert first
em->GetComponent<T>(core::ecs::EntityID(gameId));
```

### ❌ Pitfall 4: Forgetting Component Registration
```cpp
// Wrong - component not registered
void MySystem::Initialize() {
    // Missing registration!
}

// Right - register all component types
void MySystem::Initialize() {
    m_access_manager->RegisterComponentType<MyComponent>();
    m_access_manager->RegisterComponentType<MyOtherComponent>();
}
```

## 🎯 Threading Strategy

```cpp
enum class ThreadingStrategy {
    MAIN_THREAD,        // UI, rendering, ImGui
    THREAD_POOL,        // Game logic (population, economy, military)
    DEDICATED_THREAD,   // AIDirector
    BACKGROUND_THREAD,  // Low-priority tasks
    HYBRID              // Mix of strategies
};
```

**How to Choose**:
- **MAIN_THREAD**: UI systems, rendering, anything using OpenGL/ImGui
- **THREAD_POOL**: Most game logic systems (safe default)
- **DEDICATED_THREAD**: Only for systems needing 100% dedicated thread (AIDirector)
- **BACKGROUND_THREAD**: Non-critical background tasks
- **HYBRID**: Complex systems with mixed requirements

## 📊 System Dependencies (Integration Order)

```
1. Core ECS Foundation
   ├─ EntityManager ✅
   ├─ ComponentAccessManager ✅
   └─ MessageBus ✅

2. Utilities
   └─ Configuration, Random, etc. ✅

3. Game Systems (by dependency)
   ├─ Population ← Demographics
   ├─ Economy ← Population
   ├─ Military ← Population + Economy
   ├─ Technology → Military + Economy
   ├─ Diplomacy ← All above
   └─ Realm ← Political structure

4. AI Systems (depend on all game systems)
   ├─ Information Propagation
   ├─ Attention Manager
   ├─ AI Director
   ├─ Nation AI
   └─ Character AI
```

**Rule**: Don't add system B if system A (its dependency) isn't working yet!

## 📁 File Organization

```
include/
  core/
    ECS/           → IComponent.h, EntityManager.h, etc.
    types/         → game_types.h (type definitions)
  game/
    realm/         → Realm system headers
    population/    → Population system headers
    economy/       → Economic system headers
    [etc.]

src/
  core/
    ECS/           → EntityManager.cpp, etc.
  game/
    realm/         → Realm system implementations
    population/    → Population system implementations
    [etc.]
```

## 🔍 Debugging Checklist

**Build Errors**:
- [ ] Check include paths are absolute
- [ ] Verify component inherits from correct base
- [ ] Check namespace is correct
- [ ] Verify EntityID type conversions

**Runtime Errors**:
- [ ] Component registered in Initialize()?
- [ ] Entity still valid (not destroyed)?
- [ ] Thread-safe access used?
- [ ] Event subscriptions set up?

**Link Errors**:
- [ ] Source file added to CMakeLists.txt?
- [ ] Template methods in header?
- [ ] Static members defined?

## 📖 Documentation References

| Document | Use When |
|----------|----------|
| `COMPONENT-INHERITANCE-GUIDE.md` | Creating new components |
| `ARCHITECTURE-DATABASE.md` | Looking up method signatures, patterns |
| `ARCHITECTURAL-CHECKLIST.md` | Starting any development work |
| `ARCHITECTURAL-INCONSISTENCIES.md` | Troubleshooting architecture issues |
| `DEBUGGING-METHODOLOGY.md` | Build failing, need systematic approach |

## 🆘 When Stuck

1. **Check the Checklist**: `ARCHITECTURAL-CHECKLIST.md` - Did you follow pre-work consultation?
2. **Search the Database**: `ARCHITECTURE-DATABASE.md` - Look for similar code patterns
3. **Review Examples**: Look at existing systems (Realm, Population) for patterns
4. **Verify Basics**: Include paths absolute? Correct base class? Namespace consistent?
5. **Ask for Help**: Reference specific section of documentation when asking

## 🎓 Learning Path

**New to Codebase**:
1. Read `ARCHITECTURAL-REVIEW-SUMMARY.md` (overview)
2. Study `COMPONENT-INHERITANCE-GUIDE.md` (how to create components)
3. Examine `RealmComponents.h` (real example)
4. Try creating simple test component

**Adding First System**:
1. Read `ARCHITECTURE-DATABASE.md` System Integration section
2. Check dependency matrix
3. Study similar existing system
4. Follow incremental integration approach

**Advanced Work**:
1. Deep dive into `ARCHITECTURE-DATABASE.md`
2. Study threading patterns
3. Review message bus integration
4. Understand ECS internals

## ⚡ Quick Commands

**Build**:
```bash
cd build && cmake .. && make -j4
```

**Check Includes**:
```bash
grep -r "^#include" include/game/mysystem/
```

**Find Namespace Usage**:
```bash
grep -r "namespace game::" include/game/
```

**Verify Component Pattern**:
```bash
grep -r "public.*Component<" include/game/mysystem/
```

---

## 💡 Remember

✅ **DO**: Follow documented patterns, use absolute includes, verify before coding  
❌ **DON'T**: Assume, use relative paths, skip documentation  
🔄 **ALWAYS**: Check docs first, verify against code, test incrementally  

*Keep this card visible and reference specific docs when needed!*
