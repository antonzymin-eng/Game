# Entity ID Type System Documentation

## Overview

This document explains the Entity ID type system in the codebase and potential issues to be aware of.

## Two Different EntityID Types

### 1. `core::ecs::EntityID` (Versioned Handle)

**Location**: `include/core/ECS/EntityManager.h:29-56`

```cpp
struct EntityID {
    uint64_t id;        // Numeric identifier
    uint32_t version;   // Version number for safety

    EntityID(uint64_t entity_id, uint32_t entity_version);
    bool IsValid() const;
};
```

**Purpose**: Provides version-checked access to entities
- Version starts at 1 when entity is created
- Version increments when entity is destroyed
- Prevents use-after-free bugs (stale handles are rejected)
- Used internally by EntityManager

### 2. `game::types::EntityID` (Numeric ID)

**Location**: `include/core/types/game_types.h:95`

```cpp
using EntityID = uint32_t;  // Simple numeric ID
```

**Purpose**: Simple numeric identifier for game logic
- Used in UI layers (EconomyWindow, InGameHUD, etc.)
- Used in game systems (EconomicSystem, MilitarySystem, etc.)
- Easier to serialize, pass around, store
- **No version checking** - assumes entity is valid

## Critical Type Mismatch Issue

### The Problem

There's an **implicit narrowing conversion** happening throughout the codebase:

```cpp
// In main.cpp:
core::ecs::EntityID g_main_realm_entity = entity_manager->CreateEntity();

// Passed to UI:
economy_window->Render(*window_manager, g_main_realm_entity.id);
//                                       ^^^^^^^^^^^^^^^^^^^^^^^
//                                       uint64_t → uint32_t (NARROWING!)

// UI receives as:
void EconomyWindow::Render(WindowManager& wm, game::types::EntityID player_entity);
//                                            ^^^^^^^^^^^^^^^^^^^^^^
//                                            uint32_t
```

### Consequences

1. **Silent Data Loss**: Entity IDs > 4,294,967,295 will be truncated
2. **Hard to Debug**: Compiler may not warn about implicit conversion
3. **Architectural Confusion**: Two parallel type systems

## Why Version Mismatch Was an Issue

The original bug was caused by hardcoding `version=1`:

```cpp
// OLD CODE (BUGGY):
::core::ecs::EntityID entity_handle(static_cast<uint64_t>(entity_id), 1);
auto component = entity_manager->GetComponent<EconomicComponent>(entity_handle);
```

**Why this failed**:
- If entity was destroyed and recreated, version would be > 1
- GetComponent validates version - rejects if mismatch
- Component access returns nullptr silently
- Treasury operations fail without error

## Solution Implemented

### New `GetComponentById()` Method

**Location**: `include/core/ECS/EntityManager.h:478-512`

```cpp
template<typename ComponentType>
std::shared_ptr<ComponentType> GetComponentById(uint64_t entity_id) const;
```

**How it works**:
1. Takes numeric ID only (no version needed)
2. Looks up entity internally to validate it exists and is active
3. Accesses component directly by numeric ID
4. Thread-safe with proper locking

**Trade-offs**:
- ✅ **Safer**: Automatic version handling, no manual EntityID creation
- ✅ **Simpler**: UI/game code doesn't need to know about versions
- ⚠️ **Slower**: Double lookup (entity validation + component access)
- ⚠️ **Two APIs**: Both GetComponent and GetComponentById exist now

## Recommendations

### Short-term (Completed)

✅ Use `GetComponentById()` in systems that receive `game::types::EntityID`
✅ Add comprehensive logging to catch missing components
✅ Fix race conditions in the implementation

### Long-term (Future Work)

**Option A: Unify Types** (Breaking Change)
```cpp
namespace game::types {
    using EntityID = ::core::ecs::EntityID;  // Use versioned type everywhere
}
```
- Requires updating all game code to use versioned handles
- Provides best safety guarantees
- More complex for serialization

**Option B: Make ID Type Explicit** (Non-Breaking)
```cpp
namespace game::types {
    using EntityID = uint64_t;  // Match core::ecs::EntityID::id type
}
```
- Prevents uint64_t → uint32_t narrowing
- Still allows entity IDs > 4 billion
- Minimal code changes required

**Option C: Add Conversion Utilities**
```cpp
namespace game::ecs {
    // Safe conversion with validation
    game::types::EntityID ToGameEntityID(core::ecs::EntityID ecs_id);
    core::ecs::EntityID ToECSEntityID(game::types::EntityID game_id, EntityManager& em);
}
```
- Explicit conversion points
- Validation at boundaries
- Documents the impedance mismatch

## Performance Considerations

### GetComponent vs GetComponentById

| Method | Lookups | Locks | Use When |
|--------|---------|-------|----------|
| `GetComponent(EntityID)` | 1 | 1 | You have versioned handle from ECS |
| `GetComponentById(uint64_t)` | 2 | 2 | You have numeric ID from game logic |

**Recommendation**: If performance becomes an issue, cache the versioned EntityID in long-lived objects (e.g., store `core::ecs::EntityID` for player entity at startup, reuse throughout session).

## Testing Recommendations

1. **Stress Test Entity Lifecycle**
   - Create and destroy entities rapidly
   - Verify version numbers increment correctly
   - Ensure old handles are properly rejected

2. **Test ID Overflow**
   - Mock entity IDs near uint32_t limit (4 billion)
   - Verify no silent truncation occurs
   - Test with uint64_t IDs if possible

3. **Concurrency Testing**
   - Access components from multiple threads
   - Destroy entities while components are being accessed
   - Verify GetComponentById handles race conditions

## Related Files

- `include/core/ECS/EntityManager.h` - Core ECS entity management
- `include/core/types/game_types.h` - Game-level type definitions
- `src/game/economy/EconomicSystem.cpp` - Example of GetComponentById usage
- `apps/main.cpp` - Example of type conversion at boundaries

## See Also

- `ARCHITECTURE.md` - Overall system architecture
- `docs/ECS_DESIGN.md` - ECS design patterns (if exists)
