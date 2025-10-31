# AI Agent Context - Mechanica Imperii

**Last Updated:** October 29, 2025
**Purpose:** Comprehensive project context for AI assistants and automated tools

---

## Project Identity

**Name:** Mechanica Imperii
**Type:** Historical Grand Strategy Game (C++17)
**Genre:** EU4/CK3-inspired grand strategy spanning 1000-1900 AD
**Status:** Operational - Windows build compiling successfully, AI systems refactored with calculator pattern (4,141 lines)

---

## Quick Facts

- **Language:** Modern C++17 with header/implementation separation
- **Graphics:** SDL2 + OpenGL 3.2 + ImGui UI
- **Build:** CMake 3.15+ with presets and vcpkg manifest
- **Platforms:** Windows (primary), Linux (Codespaces/dev containers)
- **Scale:** ~5000 provinces, 500+ nations, 3000+ characters
- **Architecture:** Entity-Component-System (ECS) with multi-threading

---

## Project Structure

```
/workspaces/Game/
â"œâ"€â"€ apps/                      # Executables
â"‚   â"œâ"€â"€ main.cpp              # Full game (requires all systems)
â"‚   â""â"€â"€ main_minimal.cpp      # Minimal build (default)
â"œâ"€â"€ include/                   # Public headers
â"‚   â"œâ"€â"€ core/                 # ECS, threading, types, save system
â"‚   â"œâ"€â"€ game/                 # Game systems (economy, diplomacy, etc.)
â"‚   â"œâ"€â"€ map/                  # Map rendering and data
â"‚   â""â"€â"€ ui/                   # ImGui UI windows
â"œâ"€â"€ src/                      # Implementation files (mirrors include/)
â"œâ"€â"€ config/                   # JSON configuration files
â"œâ"€â"€ data/                     # Game data (provinces, scenarios)
â"œâ"€â"€ build/                    # CMake build outputs (per preset)
â"‚   â"œâ"€â"€ windows-release/      # Windows release build
â"‚   â"‚   â""â"€â"€ bin/              # Executable + runtime files
â"‚   â"œâ"€â"€ windows-vs-release/   # Visual Studio release build
â"‚   â"‚   â""â"€â"€ bin/              # Executable + runtime files
â"‚   â""â"€â"€ linux-release/        # Linux release build
â"‚       â""â"€â"€ bin/              # Executable + runtime files
â"œâ"€â"€ CMakeLists.txt            # Build configuration
â"œâ"€â"€ CMakePresets.json         # Platform-specific presets
â""â"€â"€ vcpkg.json               # Dependency manifest
```

---

## Core Systems (18 Total)

### 1. Core Infrastructure (4 systems)
- **ECS Architecture:** EntityManager, ComponentAccessManager, MessageBus
- **Threading System:** ThreadedSystemManager with multiple strategies
- **Save System:** SaveManager with LZ4 compression, validation, recovery
- **Configuration System:** Hot-reloadable JSON (119+ parameters)
- **Location:** `include/core/`, `src/core/`

### 2. Game Systems (8 systems)
- **EconomicSystem:** Production, trade, resources
- **PopulationSystem:** Demographics, migration, PopulationAggregator
- **MilitarySystem:** Units, recruitment, combat
- **AdministrativeSystem:** Governance, officials
- **DiplomacySystem:** Treaties, embassies, relations
- **TechnologySystem:** Research trees
- **TimeManagementSystem:** Game clock, events
- **ProvinceManagementSystem:** Decision queues
- **Location:** `include/game/`, `src/game/`

### 3. AI Systems (5 systems)
- **AIDirector:** Top-level AI coordination
- **NationAI:** National decision-making
- **CharacterAI:** Individual character behavior
- **AIAttentionManager:** Attention budget allocation
- **InformationPropagationSystem:** Knowledge spreading
- **Location:** `include/game/ai/`, `src/game/ai/`

### 4. Rendering System (1 system)
- **MapRenderer:** Province rendering with LOD 0-3
- **TerrainRenderer:** Terrain features
- **ViewportCuller:** Frustum culling for performance
- **Location:** `include/map/`, `src/rendering/`

---

## Type System

### Core Types (`include/core/types/game_types.h`)
- **SystemType:** ECS system identifiers (18 enum values)
- **DecisionType:** Decision categories for AI
- **RegionType:** Geographic region types
- **EventType:** Game event categories
- **TechnologyCategory:** Tech tree categories
- **ResourceType:** Economic resources
- **StrongType<T>:** Type-safe wrapper for primitives

### Strong Types
- **ProvinceID:** Province identifier
- **CharacterID:** Character identifier
- **RealmID:** Realm identifier
- **NationID:** Nation identifier

---

## Build System

### CMake Presets (`CMakePresets.json`)

**Available Presets:**

| Preset | Platform | Generator | Build Type | Use Case |
|--------|----------|-----------|------------|----------|
| `windows-debug` | Windows | Ninja | Debug | Fast debug builds |
| `windows-release` | Windows | Ninja | Release | Fast release builds |
| `windows-vs-debug` | Windows | Visual Studio | Debug | IDE integration (debugging) |
| `windows-vs-release` | Windows | Visual Studio | Release | IDE integration (release) |
| `dev` | Windows | Ninja | Debug | Quick iteration (tests ON) |
| `linux-debug` | Linux | Ninja | Debug | Linux debug builds |
| `linux-release` | Linux | Ninja | Release | Linux release builds |

**Usage:**
```bash
# Configure
cmake --preset windows-release

# Build
cmake --build --preset windows-release

# Output location
build/windows-release/bin/mechanica_imperii.exe
```

**Preset Selection:**
- **Daily development:** `windows-debug` or `linux-debug` (Ninja - fastest)
- **IDE users:** `windows-vs-debug` (Visual Studio integration)
- **Testing:** `dev` preset (enables BUILD_TESTS=ON)
- **Production:** `windows-release` or `linux-release`

### Dependencies (`vcpkg.json`)

**Core Libraries:**
- SDL2 2.0+
- glad (OpenGL loader)
- jsoncpp 1.9+
- openssl 3.0+ (save checksums)
- lz4 1.9+ (save compression)
- imgui 1.89+ (UI framework)

**Dependency Management:**
- vcpkg on Windows (via VCPKG_ROOT environment variable)
- pkg-config on Linux (with FetchContent fallback for ImGui)
- Automatic LZ4 vendoring if not found system-wide

### Build Options
```cmake
option(BUILD_TESTS "Build test executables" OFF)
option(BUILD_DOCS "Build documentation" OFF)
option(USE_VENDOR_LZ4 "Fetch LZ4 if not found" ON)
```

---

## Platform Compatibility

### Windows Configuration
- **Compiler:** MSVC 2019+ (Visual Studio 2022 recommended)
- **WindowsCleanup.h:** Force-included via `/FI` to clean Windows.h macros
- **Definitions:** `PLATFORM_WINDOWS`, `NOMINMAX`, `_CRT_SECURE_NO_WARNINGS`
- **System Libraries:** ws2_32 (sockets), winmm (multimedia)

### Linux Configuration
- **Compiler:** GCC 9+ or Clang 10+
- **Definitions:** `PLATFORM_LINUX`
- **System Libraries:** pthread, dl
- **ImGui Fallback:** FetchContent if pkg-config fails

### Windows Macro Issues
Windows.h defines macros that conflict with C++ code:
- `INVALID` conflicts with `SystemType::INVALID`
- `ERROR`, `DELETE`, `IN`, `OUT` conflict with identifiers
- `min`, `max` conflict with std::min/max

**Solution:** WindowsCleanup.h includes Windows.h with `NOMINMAX` and `WIN32_LEAN_AND_MEAN`, then undefines all conflicting macros. Applied automatically via CMake `/FI` option.

---

## Current Development Status

### âœ… Completed (October 2025)
- ECS architecture with thread-safe component access
- All 18 core systems operational
- CMake presets for cross-platform builds
- vcpkg manifest for reproducible dependencies
- Build system cleanup (removed duplicates, fixed output directories)
- Windows macro cleanup (WindowsCleanup.h force-include)
- ImGui fallback for Linux distributions

### ðŸ"§ Action Required (October 22, 2025)
**Windows Build:**
- Status: Requires CMake reconfigure after recent CMakeLists.txt updates
- Changes: C language support, glad FetchContent, ImGui linking improvements
- Action: Delete `build\windows-release` and run `cmake --preset windows-release`
- Details: See BUILD.md for full reconfiguration instructions

**Linux Build:**
- Status: âœ… Fully operational with system packages
- vcpkg: No longer required (uses system packages + FetchContent)
- Auto-fetch: glad, ImGui, and lz4 fetched automatically if needed

### â³ Pending
- LOD 4 terrain rendering (heightmap-based 3D)
- Performance optimization (texture atlasing, batching)
- Automated testing/CI pipeline

---

## Code Conventions

### File Organization
- Header/implementation separation (`.h` in `include/`, `.cpp` in `src/`)
- Matching directory structure between `include/` and `src/`
- Namespace structure mirrors directory structure

### Naming Conventions
- **Classes:** PascalCase (`EntityManager`, `GameConfig`)
- **Functions/Methods:** PascalCase (`GetComponent`, `ProcessFrame`)
- **Variables:** snake_case (`entity_manager_`, `province_id`)
- **Constants:** UPPER_SNAKE_CASE (`MAX_PROVINCES`, `DEFAULT_VALUE`)
- **Private Members:** Trailing underscore (`entity_manager_`, `config_`)

### Memory Management
- RAII patterns throughout
- Smart pointers preferred (`std::unique_ptr`, `std::shared_ptr`)
- Manual memory management only for performance-critical paths

### Threading
- Thread-safe component access via `ComponentAccessManager`
- Frame-based synchronization
- Background systems for non-critical operations

---

## AI Agent Guidelines

### When Modifying Code
1. **Check platform compatibility:** Use correct library includes
2. **Verify type usage:** Use strong types (ProvinceID, CharacterID)
3. **Thread safety:** Use ComponentAccessManager for component access
4. **Error handling:** Check for nullptr before dereferencing
5. **Namespace structure:** Follow directory-based namespace hierarchy

### When Debugging Build Errors
1. **Windows macro pollution:** Verify WindowsCleanup.h is force-included
2. **Library paths:** Check vcpkg.json for correct package names
3. **CMake changes:** Always reconfigure after CMakeLists.txt modifications
4. **Output directories:** Executable is in `build/<preset>/bin/`, not `build/Release/`
5. **Preset usage:** Use `cmake --preset <name>` instead of manual configuration

### When Adding Features
1. **ECS pattern:** New features use component-based design
2. **Configuration:** Add parameters to GameConfig.json and ConfigHelpers
3. **Serialization:** Implement save/load for persistent state
4. **Threading:** Consider background processing for non-critical systems

### Documentation Updates
- Update AI_CONTEXT.md for architecture changes
- Update ARCHITECTURE.md for design/pattern documentation
- Update BUILD.md for dependency or build process changes
- Keep README.md high-level and user-focused

---

## Key Files for Reference

### Essential Headers
- `include/core/types/game_types.h` - Core type definitions
- `include/core/ECS/EntityManager.h` - ECS interface
- `include/game/config/GameConfig.h` - Configuration structure
- `include/WindowsCleanup.h` - Windows macro cleanup

### Build Configuration
- `CMakeLists.txt` - Build system (corrected Oct 22, 2025)
- `CMakePresets.json` - Platform-specific presets
- `vcpkg.json` - Dependency manifest
- `config/GameConfig.json` - Runtime configuration

### Main Entry Points
- `apps/main.cpp` - Full game with all systems
- `apps/main_minimal.cpp` - Minimal test executable (default)

---

## Recent Changes

### October 29, 2025 - AI Systems Refactoring (Calculator Pattern)

**Achievement:** Completed systematic refactoring of 4 major AI systems, extracting pure calculation functions for testability and reusability.

**Systems Refactored (4,141 lines total):**

1. **CharacterAI → AICalculator** (1,267 lines)
   - **Files Created**: `include/game/ai/calculators/AICalculator.h`, `src/game/ai/calculators/AICalculator.cpp`
   - **Test Suite**: `tests/test_ai_refactoring.cpp`
   - **Extracted Calculations**:
     - Plot success chance and desirability scoring
     - Proposal acceptance calculations
     - Opinion decay and relationship type determination
     - Ambition scoring and mood determination
     - Decision scoring with mood modifiers

2. **NationAI → NationAICalculator** (1,040 lines)
   - **Files Created**: `include/game/ai/calculators/NationAICalculator.h`, `src/game/ai/calculators/NationAICalculator.cpp`
   - **Test Suite**: `tests/test_nation_ai_refactoring.cpp`
   - **Extracted Calculations**:
     - Strategic goal desirability and progress scoring
     - War success chance and relative strength calculations
     - Threat assessment and threat level classification
     - Economic health scoring and action determination
     - Military readiness and required forces calculations
     - Diplomatic relationship scoring and action determination
     - Personality trait adjustments (aggressiveness, risk tolerance)

3. **AIDirector → AIDirectorCalculator** (960 lines)
   - **Files Created**: `include/game/ai/calculators/AIDirectorCalculator.h`, `src/game/ai/calculators/AIDirectorCalculator.cpp`
   - **Test Suite**: `tests/test_ai_director_refactoring.cpp`
   - **Extracted Calculations**:
     - Message scheduling delays (CRITICAL=0ms, HIGH=24h, MEDIUM=7d, LOW=14d)
     - Load balancing decisions and optimal actors per frame
     - Actor type classification (Nation/Character/Council by ID ranges)
     - Processing priority scoring and actor comparison
     - Performance metrics (EMA, average decision time, frame sleep time)

4. **AIAttentionManager → AIAttentionCalculator** (874 lines)
   - **Files Created**: `include/game/ai/calculators/AIAttentionCalculator.h`, `src/game/ai/calculators/AIAttentionCalculator.cpp`
   - **Test Suite**: `tests/test_ai_attention_refactoring.cpp`
   - **Extracted Calculations**:
     - Attention scoring with weighted components (type 40%, severity 30%, accuracy 20%, relevance 10%)
     - Distance filtering (200 units per hop estimation)
     - Type filtering (minimum weight threshold)
     - Special interest detection (rivals, allies, watched provinces)
     - Relevance adjustment based on attention thresholds
     - Processing delay calculation (0/1/3/7 days for critical/high/medium/low)
     - Bidirectional personality/archetype mapping

**Benefits Achieved:**
- ✅ All AI decision logic now testable without full ECS infrastructure
- ✅ Calculation logic reusable across different contexts
- ✅ Easier to tune AI behavior by adjusting calculation constants
- ✅ Clearer separation between business logic and state management
- ✅ Consistent architectural pattern across all AI systems

**Calculator Pattern Established:**
- All calculator classes use static methods with no side effects
- 100% testable calculation logic in isolation
- Calculation constants clearly defined for easy tuning
- Comprehensive test coverage with edge case validation

---

### October 26, 2025 - Major API Fixes for Windows Build

**Problem:** main.cpp had 30+ compilation errors due to API mismatches with actual system implementations.

**Root Cause:** 
- Systems were refactored to use `ComponentAccessManager` instead of `EntityManager`
- Missing serialization methods (GetSystemName, Serialize, Deserialize)
- Missing threading methods (GetThreadingStrategy)
- Incorrect constructor signatures throughout

**Solutions Implemented:**

1. **Added Missing Infrastructure Components**
   - Added `ComponentAccessManager` global variable and initialization
   - Added `ThreadSafeMessageBus` global variable and initialization
   - Updated all system constructors to use correct parameter types

2. **Fixed System Constructors (10 systems updated)**
   - `ThreadedSystemManager`: Now takes `ComponentAccessManager*` and `ThreadSafeMessageBus*` (pointers)
   - `PopulationSystem`: Changed from `EntityManager&` to `ComponentAccessManager&`
   - `TechnologySystem`: Changed from `EntityManager&` to `ComponentAccessManager&`
   - `EconomicSystem`: Changed from `EntityManager&` to `ComponentAccessManager&`
   - `AdministrativeSystem`: Changed from `EntityManager&` to `ComponentAccessManager&`
   - `MilitarySystem`: Changed from `EntityManager&` to `ComponentAccessManager&`
   - `MilitaryRecruitmentSystem`: Changed from `EntityManager&` to `ComponentAccessManager&`
   - `DiplomacySystem`: Changed from `EntityManager&` to `ComponentAccessManager&`
   - `TimeManagementSystem`: Now requires 3 parameters: `ComponentAccessManager&`, `ThreadSafeMessageBus&`, `GameDate`

3. **Implemented Missing Virtual Methods**
   - **EconomicSystem**: Added `GetSystemName()`, `Serialize()`, `Deserialize()`
   - **AdministrativeSystem**: Added `GetSystemName()`, `Serialize()`, `Deserialize()`
   - **MilitarySystem**: Added `GetThreadingStrategy()`
   - **MilitaryRecruitmentSystem**: Added `GetSystemName()`, `Serialize()`, `Deserialize()`, `GetThreadingStrategy()`
   - **PopulationSystem**: Added `GetSystemName()`, `Serialize()`, `Deserialize()`

4. **Fixed Include Issues**
   - Added `<json/json.h>` to all system implementations using serialization
   - Fixed include paths in `TypeRegistry.cpp` and `CoreGameplaySystem.cpp`
   - Added proper namespace prefixes for core types

5. **Updated CMakeLists.txt**
   - Added C language support (required for LZ4 dependency)
   - Added `GAMEPLAY_SOURCES` section (commented out temporarily due to mismatches)
   - Added `TypeRegistry.cpp` to build (ENABLED and FUNCTIONAL - all enum mismatches resolved)
   - Properly ordered source file groups

6. **Fixed Toast::Show() String Conversions (2 locations)**
   - Line 478: Added `.c_str()` to message variable
   - Line 486: Store concatenated string then call `.c_str()`

**Current Status:**
- ✅ All constructor signatures fixed
- ✅ All serialization methods implemented
- ✅ All threading methods implemented
- ✅ Toast string conversions fixed
- ✅ Include paths corrected
- ✅ TypeRegistry.cpp FULLY FUNCTIONAL (enum mismatches fixed - added OUTLAWS and RELIGIOUS_ORDERS)
- ⚠️ CoreGameplaySystem.cpp temporarily disabled (method mismatches)

**Known Limitations:**
- GameplayCoordinator features temporarily unavailable (complexity system, decision tracking)

### 🔧 Action Required

**For Full Functionality:**
1. ✅ ~~Fix enum mismatches in `TypeRegistry.cpp`~~ (COMPLETED - all SocialClass enums now mapped)
2. Fix method mismatches in `CoreGameplaySystem.cpp` (Decision class missing methods)
3. Re-enable CoreGameplaySystem.cpp in CMakeLists.txt

**Immediate Next Steps:**
1. Test Windows build: `cmake --build --preset windows-vs-release`
2. Run executable: `build\windows-vs-release\bin\mechanica_imperii.exe`
3. Verify all 16 systems initialize correctly (18 total minus 2 disabled)

---

### October 22, 2025 - Build System Enhancements
1. **C language support enabled** - Required for glad and lz4 libraries
2. **Linux builds without vcpkg** - System packages + FetchContent fallbacks
3. **Automatic glad generation** - Python-based glad loader for OpenGL 3.3 core
4. **ImGui backend linking** - Properly linked with SDL2 and OpenGL
5. **Flexible GLAD_LIBRARIES** - Cross-platform variable for glad linking

### Platform-Specific Improvements
**Linux:**
- Uses system packages (SDL2, jsoncpp, OpenSSL, lz4) via pkg-config
- Auto-fetches glad via Python generator (no pre-built binaries needed)
- Auto-fetches ImGui if not available via pkg-config
- No vcpkg dependency required

**Windows:**
- Continues to use vcpkg for all dependencies
- glad provided by vcpkg (fallback not needed)
- Requires reconfiguration after CMakeLists.txt updates

### Documentation Updates
1. **BUILD.md enhanced** - Added reconfiguration section with platform-specific instructions
2. **README.md updated** - Clarified Windows action required, Linux build status
3. **AI_CONTEXT.md refreshed** - Latest build system changes documented

---

## Next Steps

1. âœ… **Reconfigure CMake on Windows** to apply recent fixes
2. **Verify Windows build** completes successfully
3. **Test executable** on both Windows and Linux
4. **Implement LOD 4 terrain rendering** (future enhancement)
5. **Set up CI/CD pipeline** for automated builds

---

## System Count Breakdown (18 Total)

**Core Infrastructure (4):**
1. ECS Architecture
2. Threading System
3. Save System
4. Configuration System

**Game Systems (8):**
5. Economic System
6. Population System
7. Military System
8. Administrative System
9. Diplomacy System
10. Technology System
11. Time Management System
12. Province Management System

**AI Systems (5):**
13. AI Director
14. Nation AI
15. Character AI
16. AI Attention Manager
17. Information Propagation System

**Rendering (1):**
18. Map Renderer (with LOD, culling, terrain)

---

**For More Details:**
- Architecture: See `ARCHITECTURE.md`
- Build Instructions: See `BUILD.md`
- User Documentation: See `README.md`
