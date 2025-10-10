# Component Inheritance Guide
*Created: October 10, 2025*
*Purpose: Clear decision matrix for component base class selection*

## Quick Decision Matrix

| Use Case | Recommended Base Class | Reason |
|----------|----------------------|---------|
| **New game component** | `game::core::Component<T>` | Standard pattern, type-safe, well-documented |
| **Existing components** | Keep current unless migrating | Both systems work, consistency matters more |
| **Low-level ECS work** | `core::ecs::Component<T>` | Closer to ECS foundation, atomic type IDs |
| **UI components** | `game::core::Component<T>` | Integrates with save system easily |
| **Performance-critical** | Either (equivalent performance) | Both use CRTP, choose based on features needed |

## Component Base Classes Available

### Option 1: `game::core::Component<T>` ✅ **RECOMMENDED**

**Location**: `include/core/ECS/IComponent.h`

**Advantages**:
- ✅ Hash-based type IDs (deterministic across runs)
- ✅ Direct save system integration (`JsonWriter`/`JsonReader`)
- ✅ EntityManager compatibility (string serialization)
- ✅ CRTP-based automatic implementations
- ✅ Most game systems use this pattern
- ✅ Well-documented in architecture database

**Disadvantages**:
- Type IDs based on `typeid().name()` hash (compiler-dependent)
- Slightly more boilerplate for serialization

**Usage Example**:
```cpp
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"

namespace game::mymodule {

class MyComponent : public ::game::core::Component<MyComponent> {
public:
    // Your data
    int value = 42;
    std::string name;
    
    // Constructor
    MyComponent() = default;
    explicit MyComponent(int v, const std::string& n) 
        : value(v), name(n) {}
    
    // Optional: Save system integration
    void Serialize(JsonWriter& writer) const override {
        // Write to JSON
    }
    
    bool Deserialize(const JsonReader& reader) override {
        // Read from JSON
        return true;
    }
    
    // Optional: EntityManager integration
    std::string Serialize() const override {
        return name + ":" + std::to_string(value);
    }
    
    bool Deserialize(const std::string& data) override {
        // Parse string format
        return true;
    }
    
    // Optional: Validation
    bool IsValid() const override {
        return value >= 0 && !name.empty();
    }
};

} // namespace game::mymodule
```

**When to Use**:
- Creating new game components (realm, population, economy, etc.)
- Components that need save/load functionality
- Standard game systems following documented patterns
- UI components that display game state

### Option 2: `core::ecs::Component<T>` ⚠️ **ALTERNATIVE**

**Location**: `include/core/types/game_types.h` (lines 32-73)

**Advantages**:
- ✅ Atomic counter for type IDs (guaranteed unique per execution)
- ✅ Simpler type ID generation (no hashing)
- ✅ Direct access to ECS internals
- ✅ Fully implemented and production-ready

**Disadvantages**:
- Type IDs not stable across program runs
- Less documentation and examples
- Fewer game systems use this pattern
- More direct but less abstraction

**Usage Example**:
```cpp
#include "core/types/game_types.h"

namespace game::mymodule {

class MyComponent : public ::core::ecs::Component<MyComponent> {
public:
    int value = 42;
    std::string name;
    
    MyComponent() = default;
    explicit MyComponent(int v, const std::string& n) 
        : value(v), name(n) {}
    
    // Note: Different serialization interface than game::core
    // Uses HasSerialize() / HasDeserialize() for optional serialization
};

} // namespace game::mymodule
```

**When to Use**:
- Low-level ECS system development
- Performance testing where stable type IDs aren't needed
- Internal components not exposed to save system
- Matching existing code that uses this pattern

### Option 3: Direct `game::core::IComponent` ❌ **DON'T USE**

**Why Not**:
```cpp
// ❌ WRONG - Missing CRTP template specialization
class MyComponent : public ::game::core::IComponent {
    // You'd have to manually implement:
    // - GetTypeID()
    // - GetComponentTypeName()
    // - Clone()
    // This defeats the purpose of the template system!
};
```

**Problem**: You lose all the automatic implementations that the CRTP templates provide.

## Migration Guide

### From Legacy to Modern

If you have old components using an incorrect pattern:

**Before** (incorrect):
```cpp
class OldComponent : public game::core::IComponent {
    // Missing template specialization
};
```

**After** (correct):
```cpp
class OldComponent : public ::game::core::Component<OldComponent> {
    // Now gets automatic implementations
};
```

**Steps**:
1. Change base class to `game::core::Component<YourClass>`
2. Add `::` prefix for clarity: `::game::core::Component<YourClass>`
3. Remove manual GetTypeID() implementation (now automatic)
4. Remove manual Clone() implementation (now automatic)
5. Keep custom serialization if needed
6. Build and test

### Between Component Systems

If you need to switch between `game::core` and `core::ecs`:

**From `game::core::Component<T>` to `core::ecs::Component<T>`**:
```cpp
// Before
class MyComp : public ::game::core::Component<MyComp> {
    void Serialize(JsonWriter& w) const override { }
};

// After
class MyComp : public ::core::ecs::Component<MyComp> {
    // Note: Different serialization interface
    // game::core uses JsonWriter, core::ecs uses bool flags
};
```

**Changes Needed**:
1. Update include: `"core/ECS/IComponent.h"` → `"core/types/game_types.h"`
2. Change base class namespace
3. Adjust serialization methods if used
4. Type IDs will be different (hash vs atomic counter)

**Caution**: Type IDs change, so save files won't be compatible!

### Consistency Across Systems

**Best Practice**: Pick one pattern per system/module

```cpp
// ✅ GOOD - All components in module use same base
namespace game::population {
    class PopulationComponent : public ::game::core::Component<PopulationComponent> { };
    class DemographicComponent : public ::game::core::Component<DemographicComponent> { };
    class SettlementComponent : public ::game::core::Component<SettlementComponent> { };
}

// ❌ BAD - Mixed bases in same module
namespace game::economy {
    class EconomyComponent : public ::game::core::Component<EconomyComponent> { };
    class TradeComponent : public ::core::ecs::Component<TradeComponent> { };  // Different!
}
```

## Common Patterns and Best Practices

### Pattern 1: Component with Validation

```cpp
class ValidatedComponent : public ::game::core::Component<ValidatedComponent> {
private:
    int m_value = 0;
    
public:
    void SetValue(int v) {
        m_value = std::clamp(v, 0, 100);
    }
    
    int GetValue() const { return m_value; }
    
    bool IsValid() const override {
        return m_value >= 0 && m_value <= 100;
    }
    
    std::vector<std::string> GetValidationErrors() const override {
        std::vector<std::string> errors;
        if (m_value < 0 || m_value > 100) {
            errors.push_back("Value out of range [0, 100]: " + std::to_string(m_value));
        }
        return errors;
    }
};
```

### Pattern 2: Component with Both Serialization Types

```cpp
class DualSerializeComponent : public ::game::core::Component<DualSerializeComponent> {
private:
    std::string m_data;
    
public:
    // For save system (detailed)
    void Serialize(JsonWriter& writer) const override {
        writer.StartObject();
        writer.Key("data");
        writer.String(m_data);
        writer.Key("timestamp");
        writer.Int64(std::time(nullptr));
        writer.EndObject();
    }
    
    bool Deserialize(const JsonReader& reader) override {
        // Parse JSON
        return true;
    }
    
    // For EntityManager (compact)
    std::string Serialize() const override {
        return m_data;  // Simple string format
    }
    
    bool Deserialize(const std::string& data) override {
        m_data = data;
        return true;
    }
};
```

### Pattern 3: Component with Entity References

```cpp
class RelationComponent : public ::game::core::Component<RelationComponent> {
private:
    game::types::EntityID m_parent{0};
    std::vector<game::types::EntityID> m_children;
    
public:
    void SetParent(game::types::EntityID parent) {
        m_parent = parent;
    }
    
    void AddChild(game::types::EntityID child) {
        m_children.push_back(child);
    }
    
    bool IsValid() const override {
        // EntityID 0 is invalid
        return m_parent != 0;
    }
    
    // EntityManager access pattern
    std::shared_ptr<ParentComponent> GetParentComponent(
        core::ecs::EntityManager* em) const 
    {
        return em->GetComponent<ParentComponent>(
            core::ecs::EntityID(m_parent)
        );
    }
};
```

### Pattern 4: Immutable Component

```cpp
class ImmutableComponent : public ::game::core::Component<ImmutableComponent> {
private:
    const std::string m_id;
    const int m_creation_time;
    
public:
    explicit ImmutableComponent(std::string id)
        : m_id(std::move(id))
        , m_creation_time(std::time(nullptr))
    {}
    
    // No setters - read-only
    const std::string& GetId() const { return m_id; }
    int GetCreationTime() const { return m_creation_time; }
    
    // Clone creates a new immutable copy
    // (automatically provided by CRTP base)
};
```

## Type ID Considerations

### `game::core::Component<T>` Type IDs

```cpp
// Type ID generated from type_index hash
ComponentTypeID GetTypeID() const override {
    return static_cast<ComponentTypeID>(
        std::hash<std::type_index>{}(
            std::type_index(typeid(MyComponent))
        )
    );
}
```

**Characteristics**:
- Based on `typeid().name()`
- Deterministic per compiler/platform
- Can collide (rare, but possible with hash collisions)
- Same across program runs on same platform

### `core::ecs::Component<T>` Type IDs

```cpp
// Type ID generated from atomic counter
static ComponentTypeID GetStaticTypeID() {
    if (s_type_id == 0) {
        s_type_id = GetNextComponentTypeID();  // Atomic increment
    }
    return s_type_id;
}
```

**Characteristics**:
- Guaranteed unique per execution
- Different each program run
- No collision possible
- Order-dependent (first initialized gets lower ID)

**Implication**: Use `game::core` if you need stable IDs for serialization!

## Testing Your Component

### Basic Compilation Test

```cpp
// test_component.cpp
#include "path/to/MyComponent.h"

int main() {
    // Test instantiation
    MyComponent comp;
    
    // Test type ID
    auto type_id = comp.GetTypeID();
    
    // Test type name
    auto name = comp.GetComponentTypeName();
    
    // Test clone
    auto clone = comp.Clone();
    
    // Test validation
    bool valid = comp.IsValid();
    
    return 0;
}
```

Compile with:
```bash
g++ -std=c++17 -I./include test_component.cpp -o test_component
```

### Integration Test with EntityManager

```cpp
#include "core/ECS/EntityManager.h"
#include "MyComponent.h"

int main() {
    core::ecs::EntityManager em;
    
    // Create entity
    auto entity = em.CreateEntity("test");
    
    // Add component
    auto comp = em.AddComponent<MyComponent>(entity, 42, "test");
    
    // Retrieve component
    auto retrieved = em.GetComponent<MyComponent>(entity);
    
    // Verify
    return (retrieved && retrieved->GetValue() == 42) ? 0 : 1;
}
```

## Troubleshooting

### "GetTypeID() not found"
**Problem**: Base class missing or incorrect

**Solution**: Ensure you inherit from `Component<YourClass>` not just `IComponent`

### "Static member s_type_id undefined"
**Problem**: Template static member not instantiated

**Solution**: For `core::ecs::Component<T>`, ensure game_types.h includes the definition (line 73)

### "Type ID collision detected"
**Problem**: Two components have same hash (rare with `game::core`)

**Solution**: Switch to `core::ecs::Component<T>` which uses atomic counter

### "Serialization methods not called"
**Problem**: EntityManager doesn't call your custom serialization

**Solution**: Check you're overriding the correct signature:
- `game::core`: `std::string Serialize() const`
- Not: `void Serialize(JsonWriter&) const` (that's for save system)

## Summary

**For Most Use Cases**: Use `game::core::Component<T>`
- It's the standard pattern
- Most examples use it
- Best documented
- Integrates with save system

**For ECS Core Work**: Use `core::ecs::Component<T>`
- Simpler type ID generation
- Direct ECS access
- Good for performance testing

**Never**: Inherit from `IComponent` directly
- You lose CRTP benefits
- Too much manual work
- Error-prone

---

*Next Steps*: See `ARCHITECTURE-DATABASE.md` for complete system integration patterns
