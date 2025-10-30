# Changelog - Mechanica Imperii

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### October 30, 2025 - TypeRegistry Enum Mismatch Fix

#### Fixed
- **TypeRegistry.cpp - SocialClass Enum Mappings** (Critical)
  - Added missing `OUTLAWS` enum mapping to TypeRegistry
  - Added missing `RELIGIOUS_ORDERS` enum mapping to TypeRegistry
  - All 17 SocialClass enum values now properly mapped (was 15/17)
  - TypeRegistry now fully functional with complete enum coverage
  - Resolves compilation issues with string-to-enum and enum-to-string conversions
  - Enables `ThreadingStrategyToString()` and `SocialClassToString()` functions

#### Changed
- **Documentation Updates**
  - BUILD.md: Updated status from "temporarily disabled" to "FULLY FUNCTIONAL"
  - AI_CONTEXT.md: Marked TypeRegistry as completed in action items
  - README.md: Moved TypeRegistry from "Temporarily Disabled" to "Recently Fixed"
  - System count updated: "16 of 18 Active" → "17 of 18 Active"

### October 29, 2025 - AI Systems Refactoring (Calculator Pattern)

#### Changed
- **Major Architectural Refactoring**: Extracted pure calculation functions from 4 AI systems (4,141 lines refactored)
- **CharacterAI → AICalculator** (1,267 lines)
  - Extracted plot, proposal, relationship, ambition, and mood calculations
  - All decision-making logic now in pure static functions
  - Created comprehensive test suite: `tests/test_ai_refactoring.cpp`
- **NationAI → NationAICalculator** (1,040 lines)
  - Extracted strategic goal, war decision, threat assessment calculations
  - Economic, military, and diplomatic decision logic isolated
  - Created test suite: `tests/test_nation_ai_refactoring.cpp`
- **AIDirector → AIDirectorCalculator** (960 lines)
  - Extracted message scheduling and priority calculations
  - Load balancing and actor classification logic centralized
  - Performance metrics calculations (EMA, frame timing) reusable
  - Created test suite: `tests/test_ai_director_refactoring.cpp`
- **AIAttentionManager → AIAttentionCalculator** (874 lines)
  - Extracted attention scoring with weighted components (type 40%, severity 30%, accuracy 20%, relevance 10%)
  - Distance/type filtering and special interest detection isolated
  - Personality/archetype mapping bidirectional
  - Created test suite: `tests/test_ai_attention_refactoring.cpp`

#### Added
- **Calculator Pattern**: Established architectural pattern for pure calculation extraction
  - All calculator classes use static methods with no side effects
  - 100% testable calculation logic in isolation
  - Calculation constants clearly defined for easy tuning
  - Pattern documented across all AI systems
- **Comprehensive Test Coverage**
  - 4 new test suites with extensive edge case coverage
  - All calculation functions validated with assertions
  - Test suites demonstrate expected behavior and usage patterns

#### Fixed
- **README.md Documentation Issues** (Critical)
  - System count corrected: "18 Total" → "16 of 18 Active"
  - Added section documenting 2 temporarily disabled systems (GameplayCoordinator, TypeRegistry)
  - Updated project status to reflect Oct 26 successful build
  - Removed outdated "Windows requires reconfigure" warning
  - Fixed broken emoji encoding throughout document
  - Last Updated: Oct 22 → Oct 29, 2025

#### Benefits
- All AI decision logic now testable without full ECS infrastructure
- Calculation logic reusable across different contexts
- Easier to tune AI behavior by adjusting calculation constants
- Clearer separation between business logic and state management
- Consistent architectural pattern across all AI systems

---

### October 26, 2025 - Major API Fixes

#### Fixed
- **main.cpp compilation errors** (30+ errors resolved)
  - All system constructor signatures corrected to use `ComponentAccessManager&`
  - Fixed `ThreadedSystemManager` to use pointer parameters
  - Fixed `TimeManagementSystem` to require 3 parameters
  - Fixed `EntityManager::AddComponent` usage (returns `shared_ptr`, not reference)
  - Fixed `EntityID` access (use `.id` member, not `Get()` method)
  - Fixed `Toast::Show()` calls to use `.c_str()` for string conversion

- **Missing virtual method implementations**
  - `EconomicSystem`: Added `GetSystemName()`, `Serialize()`, `Deserialize()`
  - `AdministrativeSystem`: Added `GetSystemName()`, `Serialize()`, `Deserialize()`
  - `MilitarySystem`: Added `GetThreadingStrategy()`
  - `MilitaryRecruitmentSystem`: Added `GetSystemName()`, `Serialize()`, `Deserialize()`, `GetThreadingStrategy()`
  - `PopulationSystem`: Added `GetSystemName()`, `Serialize()`, `Deserialize()`

- **Include issues**
  - Added `<json/json.h>` to: EconomicSystem.cpp, AdministrativeSystem.cpp, MilitarySystem.cpp, MilitaryRecruitmentSystem.cpp, PopulationSystem.cpp
  - Fixed include path in TypeRegistry.cpp: `"game_types.h"` → `"core/types/game_types.h"`
  - Fixed include path in CoreGameplaySystem.cpp: `"core/Types/TypeRegistry.h"` → `"core/types/game_types.h"`

#### Added
- **New infrastructure components**
  - `g_component_access_manager` global variable in main.cpp
  - `g_thread_safe_message_bus` global variable in main.cpp
  - Proper initialization chain for ECS components

- **CMakeLists.txt improvements**
  - Added C language support (required for LZ4 dependency)
  - Added `GAMEPLAY_SOURCES` section
  - Added TypeRegistry.cpp to build sources
  - Improved source file organization

- **Documentation**
  - Created `docs/API_REFERENCE.md` - comprehensive API documentation
  - Updated `AI_CONTEXT.md` with Oct 26 changes
  - Updated `BUILD.md` with current build status

#### Changed
- **System initialization pattern** (in main.cpp lines 230-290)
  ```cpp
  // Old pattern:
  g_system_manager = std::make_unique<ThreadedSystemManager>(*g_message_bus);
  g_population_system = std::make_unique<PopulationSystem>(*g_entity_manager, *g_message_bus);
  
  // New pattern:
  g_system_manager = std::make_unique<ThreadedSystemManager>(
      g_component_access_manager.get(), 
      g_thread_safe_message_bus.get()
  );
  g_population_system = std::make_unique<PopulationSystem>(
      *g_component_access_manager, 
      *g_message_bus
  );
  ```

- **Component access pattern**
  ```cpp
  // Old pattern:
  auto& component = manager->AddComponent<T>(entity);
  component.value = 42;  // ERROR: returns shared_ptr, not reference
  
  // New pattern:
  auto component = manager->AddComponent<T>(entity);
  component->value = 42;  // Use -> operator
  ```

#### Deprecated
- Using `EntityManager&` directly in system constructors (use `ComponentAccessManager&` instead)
- Accessing `EntityID.Get()` method (use `.id` member instead)

#### Removed
- None

#### Known Issues
- `GameplayCoordinator` temporarily disabled (method signature mismatches)
- `TypeRegistry.cpp` temporarily disabled (enum value mismatches)
- These 2 systems need header/implementation synchronization before re-enabling

---

## October 22, 2025 - CMake and Build System Updates

#### Fixed
- C language support added to CMakeLists.txt (required for LZ4)
- glad library auto-fetched via FetchContent when not found
- ImGui linking improved for Windows (vcpkg features) and Linux (pkg-config)
- Fixed duplicate `namespace game::economy` closure in EconomicSystem.cpp

#### Changed
- vcpkg baseline updated to: `3508985146f1b1d248c67ead13f8f54be5b4f5da` (2024-11-19)
- imgui dependency now includes features: `sdl2-binding`, `opengl3-binding`

---

## October 21, 2025 - Strategic System Rebuilds

#### Changed
- **EconomicSystem**: Rebuilt following PopulationSystem pattern
- **AdministrativeSystem**: Rebuilt following PopulationSystem pattern
- Both systems now use proper ECS integration with ComponentAccessManager

---

## October 16, 2025 - Logger System Refactor

#### Changed
- Converted Logger from class-based to free function approach
- All systems now use `::core::logging::LogInfo/Error/Warning("SystemName", "message")`
- Removed `std::shared_ptr<Logger> m_logger` from all systems

#### Fixed
- **CoreGameplaySystem**: Updated to use free function logging
- **GameSystemsIntegration**: Fixed namespace declarations

---

## October 12, 2025 - Military Systems Rebuild

#### Added
- **MilitarySystem**: Complete rebuild from PopulationSystem template
- **MilitaryRecruitmentSystem**: New system for recruitment management
- Proper ECS integration patterns established

---

## October 10, 2025 - Population System Overhaul

#### Changed
- **PopulationSystem**: Major refactor with full ECS integration
- Established template patterns for all future system development
- Component-based architecture fully implemented

---

## September 16, 2025 - Core Gameplay System

#### Added
- **CoreGameplaySystem.cpp**: Initial implementation of gameplay coordinator
- Decision-consequence system framework
- Delegation system for AI management
- Quiet period manager for pacing

---

## Version History Summary

### v0.9.0 - October 2025 (Current)
- All 18 core systems implemented
- ECS architecture complete
- Multi-threading system operational
- Save/load system with compression
- **Status:** 16 of 18 systems compiling, 2 temporarily disabled

### v0.8.0 - September 2025
- Core gameplay systems added
- AI systems framework established
- Map rendering with LOD system
- ImGui UI integration complete

### v0.7.0 - August 2025
- ECS foundation completed
- Threading system implemented
- Configuration system with hot reload
- Cross-platform build system established

### v0.6.0 - July 2025
- Initial CMake setup
- vcpkg integration
- Basic SDL2 + OpenGL rendering
- Project structure established

---

## Migration Guides

### Migrating from Pre-Oct 26 Code

If you have custom systems created before October 26, 2025, you need to:

1. **Update Constructor Signatures**
   ```cpp
   // OLD:
   MySystem(EntityManager& em, MessageBus& mb);
   
   // NEW:
   MySystem(ComponentAccessManager& cam, MessageBus& mb);
   ```

2. **Add Serialization Methods**
   ```cpp
   std::string GetSystemName() const override;
   Json::Value Serialize(int version) const override;
   bool Deserialize(const Json::Value& data, int version) override;
   ```

3. **Add Threading Method** (if implementing ISystem)
   ```cpp
   ::core::threading::ThreadingStrategy GetThreadingStrategy() const override;
   ```

4. **Update Component Access**
   ```cpp
   // OLD:
   auto& comp = manager->AddComponent<T>(entity);
   comp.value = 42;
   
   // NEW:
   auto comp = manager->AddComponent<T>(entity);
   comp->value = 42;
   ```

5. **Add Json Include**
   ```cpp
   #include <json/json.h>  // At top of .cpp file
   ```

### Migrating from Pre-Oct 22 Code

If you're on an older build, also apply these changes:

1. **Update CMakeLists.txt** (lines 12-13)
   ```cmake
   cmake_minimum_required(VERSION 3.15)
   project(mechanica_imperii VERSION 1.0.0 LANGUAGES C CXX)
   ```

2. **Update vcpkg.json** (lines 11-14)
   ```json
   {
     "name": "imgui",
     "features": ["sdl2-binding", "opengl3-binding"]
   }
   ```

3. **Reconfigure build**
   ```bash
   rm -rf build/windows-release  # or build\windows-release on Windows
   cmake --preset windows-release
   ```

---

## Breaking Changes

### October 26, 2025
- **EntityManager::AddComponent** now returns `std::shared_ptr<T>` (was: reference)
- **All system constructors** now require `ComponentAccessManager&` (was: `EntityManager&`)
- **TimeManagementSystem constructor** now requires 3 parameters (was: 0)
- **EntityID.Get()** method removed (use: `.id` member)

### October 16, 2025
- **Logger class** removed (use: `::core::logging::LogInfo/Error/Warning` free functions)
- **System constructors** no longer accept logger parameter

---

## Acknowledgments

- Template patterns established by PopulationSystem refactor (Oct 10, 2025)
- ECS architecture inspired by Unity DOTS and EnTT
- Build system patterns from vcpkg best practices
- Configuration hot-reload pattern from game engines

---

## Links

- **Repository:** [https://github.com/antonzymin-eng/Game](https://github.com/antonzymin-eng/Game)
- **Issues:** [https://github.com/antonzymin-eng/Game/issues](https://github.com/antonzymin-eng/Game/issues)
- **Documentation:** See `AI_CONTEXT.md`, `ARCHITECTURE.md`, `BUILD.md`, `docs/API_REFERENCE.md`
