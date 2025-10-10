# Debugging Methodology & Lessons Learned

## Project Status Summary

### Current State (Working)
- **Minimal Build**: `apps/main_minimal.cpp` successfully compiles and runs
- **Systems Added**: Config, Utils, Rendering, Map, UI placeholders
- **Next Target**: Realm system integration

### Original Codebase Analysis
- **Status**: Completely broken - couldn't even configure with CMake
- **Missing Files**: `src/core/Threading/ThreadedSystemManager.cpp`
- **Namespace Issues**: Pre-existing throughout ECS system
- **Include Path Problems**: Relative paths instead of absolute from project root

## Systematic Debugging Approach

### Phase 1: Establish Baseline
1. **Always check original state first**:
   ```bash
   git stash  # Save current changes
   cd build && make clean && make -j4  # Test original
   git stash pop  # Restore changes
   ```

2. **Identify what was actually working**:
   - Don't assume files were meant to compile
   - Check if CMakeLists.txt was including problematic files
   - Verify if missing dependencies were intentional

### Phase 2: Minimal Working Build Strategy
1. **Create minimal main**: Strip down to absolute essentials
2. **Add systems incrementally**: One at a time with build verification
3. **Isolate problems**: Don't fix everything at once

### Phase 3: Namespace Investigation Protocol
1. **Check namespace declarations**: `grep -r "namespace.*::" include/`
2. **Verify component inheritance**: Look for Component base classes
3. **Cross-reference headers vs implementations**: `.h` vs `.cpp` consistency
4. **Question assumptions**: "Wrong" namespaces might be architecturally correct

### Phase 4: Incremental Integration
1. **Add one system at a time**
2. **Build after each addition**
3. **Document what works vs what needs fixing**
4. **Don't assume file contents are wrong - investigate first**

## Common Anti-Patterns to Avoid

### ❌ Don't Do This
- Assume all compilation errors are "wrong code"
- Fix multiple namespace issues simultaneously 
- Change includes without understanding the architecture
- Skip verification of original codebase state

### ✅ Do This Instead
- Investigate whether code was meant to compile
- Fix one namespace issue at a time with build verification
- Understand the intended include path structure
- Always establish baseline before making changes

## Architecture Discoveries

### ECS Component System
```cpp
// CORRECT: Template-based components inherit from core::ecs::Component<T>
class MyComponent : public core::ecs::Component<MyComponent> {
    // This inherits from game::core::IComponent internally
};

// INCORRECT: Direct inheritance from game::core::IComponent
class MyComponent : public game::core::IComponent {
    // Missing template specialization
};
```

### Include Path Standards
```cpp
// CORRECT: Absolute paths from project root
#include "core/ECS/EntityManager.h"
#include "game/config/GameConfig.h"
#include "utils/ConfigManager.h"

// INCORRECT: Relative paths
#include "EntityManager.h"
#include "../config/GameConfig.h"
```

### Namespace Hierarchy
```
core::ecs          # ECS framework (EntityManager, Component<T>)
├─ game::core      # Game-specific interfaces (IComponent, ISystem)
├─ game::          # Game logic systems
└─ ui::            # User interface components
```

## Build System Knowledge

### CMakeLists.txt Structure
- **MAIN_SOURCES**: What actually gets compiled into executable
- **ECS_SOURCES**: Core system implementations (many broken)
- **UI_SOURCES**: User interface (we created placeholders)
- **CONFIG_SOURCES**: Configuration system (working)

### Incremental Addition Strategy
1. Start with `apps/main_minimal.cpp` only
2. Add source groups one at a time:
   - CONFIG_SOURCES ✅
   - UTILITY_SOURCES ✅  
   - RENDERING_SOURCES ✅
   - MAP_SOURCES ✅
   - UI_SOURCES (placeholders) ✅
   - REALM_SOURCES (in progress)
   - ECS_SOURCES (broken, avoid for now)

## Problem Classification

### Type A: Pre-existing Architectural Issues
- **Symptoms**: Namespace mismatches, missing ComponentTypeID, method signature mismatches
- **Root Cause**: Original codebase was broken
- **Solution**: Document and work around, or fix systematically

### Type B: Include Path Problems  
- **Symptoms**: "No such file or directory"
- **Root Cause**: Relative includes in a complex hierarchy
- **Solution**: Use absolute paths from project root

### Type C: Missing Dependencies
- **Symptoms**: CMake can't find source files
- **Root Cause**: Files referenced in CMakeLists.txt don't exist
- **Solution**: Remove from build or create minimal implementations

### Type D: Template Instantiation Issues
- **Symptoms**: Template errors, missing static members
- **Root Cause**: Header-only template code not properly instantiated
- **Solution**: Move templates to .inl files or explicit instantiation

## Success Metrics

### Build Health Indicators
- ✅ CMake configures without errors
- ✅ Compiles without warnings (target state)
- ✅ Links successfully
- ✅ Runs without crashes
- ✅ Core functionality accessible (entity creation, config loading)

### Integration Readiness Checklist
Before adding a system:
- [ ] Headers compile independently
- [ ] Implementation files exist and match headers
- [ ] Dependencies are satisfied
- [ ] Namespace consistency verified
- [ ] Include paths use absolute form

## Next Steps Protocol

1. **Document current working state**
2. **Identify next system to integrate** 
3. **Check dependencies of that system**
4. **Add to CMakeLists.txt**
5. **Build and fix issues one at a time**
6. **Update this document with new findings**

## Emergency Reset Procedure

If we get into a non-building state:
```bash
# Reset to last known working state
git checkout -- CMakeLists.txt
# Restore minimal main
git checkout -- apps/main_minimal.cpp
# Build to verify baseline
cd build && make clean && make -j4
```

Then re-add systems one by one using documented incremental approach.

---

**Key Insight**: The original codebase was fundamentally broken. Our job is to create a working architecture while preserving the intended design patterns. Don't assume errors indicate wrong code - investigate the architectural intent first.