# Architectural Consistency Checklist

> **Purpose**: Mandatory checklist to prevent architectural amnesia and ensure database knowledge is consulted before any development work.

## 🚨 **MANDATORY PRE-WORK CONSULTATION**

Before making ANY code changes, file modifications, or build system updates:

### **1. Architecture Database Consultation** ✅
- [ ] Read relevant sections of `ARCHITECTURE-DATABASE.md`
- [ ] Identify which of the 4 ECS systems the work involves:
  - [ ] `game::core` - High-level interfaces (IComponent, ISerializable, ISystem)
  - [ ] `core::ecs` - Production EntityManager with versioned EntityID struct
  - [ ] `core::ecs` Legacy - Old .cpp implementation (usually avoid)
  - [ ] `game::types` - Bridge layer with simple EntityID
- [ ] Check system dependencies matrix for integration order
- [ ] Review known architectural issues and their solutions

### **2. Dual ECS Architecture Awareness** ✅
- [ ] Confirmed which ECS layer the change affects
- [ ] Verified namespace consistency (`core::ecs` vs `game::core`)
- [ ] Checked EntityID type compatibility (uint32_t vs struct vs uint64_t)
- [ ] Identified serialization interface requirements (JsonWriter vs string)
- [ ] Reviewed component template inheritance pattern

### **3. Build Integration Strategy** ✅
- [ ] Consulted system dependencies matrix for proper order
- [ ] Identified which .cpp files are safe vs problematic
- [ ] Verified threading strategy compatibility
- [ ] Checked for namespace mismatches in implementation files

## 🔧 **DEVELOPMENT DECISION MATRIX**

### **When Adding Systems to Build:**
```
1. ECS Foundation FIRST:
   - MessageBus (safe) ✅
   - EntityManager (header-only) ✅  
   - ComponentAccessManager (check .cpp compatibility) ⚠️

2. Core Systems by Dependency Order:
   - Configuration & Utilities
   - Threading System
   - Population System (depends on demographics)
   - Economic System (depends on population)
   - Military System (depends on population + economy)
   - Technology System (provides to military + economy)
   - etc...

3. Integration Systems LAST:
   - AI Systems (depend on all game systems)
   - Save System (serializes all game state)
```

### **When Encountering Build Errors:**
```
❌ WRONG Approach: "Let me fix this compilation error"
✅ CORRECT Approach: 
   1. Consult architecture database for this system
   2. Identify which ECS layer has the conflict
   3. Check if .cpp file matches header architecture
   4. Apply documented architectural patterns
   5. Update database if new patterns discovered
```

## 📋 **ARCHITECTURAL MEMORY AIDS**

### **Quick Reference Card**
Keep this visible during development:

| Namespace | Purpose | EntityID Type | Primary Use |
|-----------|---------|---------------|-------------|
| `game::core` | Interfaces | N/A | Component templates, System interfaces |
| `core::ecs` | ECS Foundation | `struct{id,version}` | EntityManager, ComponentAccessManager |
| `game::types` | Bridge Layer | `uint32_t` | Game-level entity references |
| Legacy `core::ecs` | Old Implementation | `uint64_t` | **AVOID - causes conflicts** |

### **Common Architectural Patterns**
```cpp
// Component Creation
class MyComponent : public ::game::core::Component<MyComponent> {
    // Uses game::core layer for consistency
};

// EntityID Conversion  
game::types::EntityID gameId = 12345;
core::ecs::EntityID ecsId = ::core::ecs::EntityID(gameId);

// System Threading
void MySystem::Initialize() {
    m_access_manager.RegisterComponentType<MyComponent>();
    SubscribeToEvents();
}
```

## 🔄 **CONTINUOUS ARCHITECTURAL VALIDATION**

### **During Development:**
- [ ] Reference architecture database before each coding session
- [ ] Document new patterns discovered during implementation
- [ ] Update database when architectural decisions are made
- [ ] Cross-reference with dependency matrix when adding systems

### **Before Commits:**
- [ ] Verify namespace consistency across changed files
- [ ] Check that build order respects dependency matrix
- [ ] Confirm no architectural conflicts introduced
- [ ] Update architectural database if new information discovered

### **Code Review Questions:**
- Does this change respect the dual ECS architecture?
- Are we using the correct namespace for this functionality?
- Does the build order follow the dependency matrix?
- Are we mixing legacy and modern ECS patterns?

## 🎯 **SPECIFIC REMINDERS FOR CURRENT WORK**

### **Build System Integration:**
1. **ComponentAccessManager.cpp** - Check if implementation matches header namespace and interface
2. **EntityManager.cpp** - Avoid old implementation, use header-only version
3. **System Dependencies** - Population → Economy → Military → Technology → AI
4. **Threading Strategy** - Each system has documented threading requirements

### **ECS Layer Selection:**
- **Game Components**: Use `game::core::Component<T>`
- **Entity Management**: Use `core::ecs::EntityManager` 
- **Thread Safety**: Use `core::ecs::ComponentAccessManager`
- **Avoid**: Legacy .cpp implementations that conflict with headers

## ⚡ **EMERGENCY PROTOCOLS**

### **If Architectural Confusion Occurs:**
1. **STOP** current work immediately
2. **CONSULT** `ARCHITECTURE-DATABASE.md` relevant sections
3. **IDENTIFY** which ECS layer you're working with
4. **VERIFY** namespace and interface consistency
5. **PROCEED** only after confirming architectural alignment

### **If Database Information Seems Outdated:**
1. **INVESTIGATE** current codebase state
2. **DOCUMENT** findings in database
3. **UPDATE** architectural patterns if necessary
4. **VALIDATE** changes don't break existing systems

---

**Remember**: The architectural database exists to prevent exactly the kind of "forgetting" that leads to wasted effort and inconsistent systems. Always consult it FIRST, not as an afterthought! 

*"Architectural amnesia is the #1 cause of development rework"*