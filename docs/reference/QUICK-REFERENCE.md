# Quick Reference: Common Issues & Solutions

## 🔍 Diagnostic Commands

### Check Original Codebase State
```bash
git stash                    # Save current work
cd build && make clean       # Clean build
make -j4                     # Test original build
git stash pop               # Restore work
```

### Investigate Namespaces
```bash
grep -r "namespace.*::" include/                    # Find all namespace declarations
grep -r "class.*Component" include/                 # Find Component classes  
grep -r "core::ecs::Component" include/             # Find ECS component usage
```

### Check File Dependencies
```bash
find . -name "*.h" | grep ComponentAccessManager   # Find header files
grep -r "#include.*ComponentAccessManager" .       # Find include usage
```

## 🚨 Error Pattern Recognition

### Pattern 1: Include Path Error
```
fatal error: game/GameWorld.h: No such file or directory
```
**Solution**: Fix include path or remove from build
**Root Cause**: Relative include paths, missing files

### Pattern 2: Namespace Mismatch
```
error: 'ComponentTypeID' does not name a type
error: 'IComponent' was not declared in this scope
```
**Solution**: Check namespace declarations and inheritance hierarchy
**Root Cause**: .cpp file namespace doesn't match .h file declarations

### Pattern 3: Template Instantiation Error
```
error: expected initializer before '<' token
```
**Solution**: Verify template syntax: `Component<T>` not `Component`
**Root Cause**: Missing template parameters

### Pattern 4: Missing Source File
```
CMake Error: Cannot find source file: src/core/Threading/ThreadedSystemManager.cpp
```
**Solution**: Remove from CMakeLists.txt or create minimal stub
**Root Cause**: CMakeLists.txt references non-existent files

### Pattern 5: Incomplete Template Error
```
error: invalid use of incomplete type 'class MyComponent'
note: forward declaration of 'class MyComponent'
```
**Solution**: Check template class definition completeness
**Root Cause**: Template class contains only `// ...` comment instead of implementation

## 🛠️ Standard Fix Procedures

### Fix Include Paths
```cpp
// BEFORE (relative)
#include "EntityManager.h"
#include "../config/GameConfig.h"

// AFTER (absolute from project root)  
#include "core/ECS/EntityManager.h"
#include "game/config/GameConfig.h"
```

### Fix Component Inheritance
```cpp
// BEFORE (incorrect)
class MyComponent : public game::core::IComponent {
class MyComponent : public game::core::Component {

// AFTER (correct)
class MyComponent : public core::ecs::Component<MyComponent> {
```

### Incremental CMakeLists.txt Testing
```cmake
# Test one source group at a time
set(MAIN_SOURCES
    apps/main_minimal.cpp
    # Add ONE group, build, verify, then add next
)
```

## 📊 System Integration Checklist

Before adding a system to build:
- [ ] Headers exist and compile independently
- [ ] Implementation files exist  
- [ ] Include paths are absolute from project root
- [ ] Namespace consistency verified
- [ ] Dependencies satisfied
- [ ] No CMake file references

Quick test:
```bash
# Test header compilation
g++ -I./include -fsyntax-only include/game/realm/RealmComponents.h
```

## 🔄 Reset to Working State

If build breaks:
```bash
# Reset CMakeLists.txt to minimal working state
git checkout HEAD -- CMakeLists.txt

# Ensure minimal main is active
ls apps/main_minimal.cpp

# Verify build works
cd build && make clean && make -j4

# Re-add systems incrementally
```

## 📚 Architecture Quick Reference

### Working Namespace Hierarchy
```
core::ecs                   # ECS framework
├─ EntityManager           # Entity lifecycle
├─ Component<T>            # Template base class  
└─ MessageBus              # Event system

game::core                 # Game interfaces
├─ IComponent              # Component interface
└─ ISystem                 # System interface

game::                     # Game systems
├─ config::                # Configuration
├─ realm::                 # Realm management  
└─ population::            # Population simulation

ui::                       # User interface
└─ Toast                   # Notification system
```

### Include Path Patterns
```cpp
#include "core/ECS/EntityManager.h"        # Core ECS
#include "game/config/GameConfig.h"        # Game config
#include "game/realm/RealmComponents.h"    # Game components  
#include "utils/ConfigManager.h"           # Utilities
#include "ui/Toast.h"                      # UI components
```

---

*Use this as a quick reference during debugging. Refer to DEBUGGING-METHODOLOGY.md for detailed procedures.*