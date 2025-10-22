# AI Agent Context - Mechanica Imperii

**Last Updated:** October 22, 2025  
**Purpose:** Comprehensive project context for AI assistants and automated tools

---

## Project Identity

**Name:** Mechanica Imperii  
**Type:** Historical Grand Strategy Game (C++17)  
**Genre:** EU4/CK3-inspired grand strategy spanning 1000-1900 AD  
**Status:** Active Development - Core systems operational, Windows build in progress

---

## Quick Facts

- **Language:** Modern C++17 with header/implementation separation
- **Graphics:** SDL2 + OpenGL 3.2 + ImGui UI
- **Build:** CMake 3.28+ with vcpkg dependency management
- **Platforms:** Windows (primary), Linux (Codespaces/dev containers)
- **Scale:** ~5000 provinces, 500+ nations, 3000+ characters
- **Architecture:** Entity-Component-System (ECS) with multi-threading

---

## Project Structure

```
/workspaces/Game/
├── apps/               # Executables (main.cpp, main_minimal.cpp)
├── include/            # Public headers
│   ├── core/          # ECS, threading, types, save system
│   ├── game/          # Game systems (economy, diplomacy, military, etc.)
│   ├── map/           # Map rendering and data
│   ├── ui/            # ImGui UI windows
│   └── utils/         # PlatformCompat.h, ConfigManager, RandomGenerator
├── src/               # Implementation files (mirrors include/)
├── config/            # JSON configuration files
├── data/              # Game data (provinces, scenarios)
├── docs/              # Documentation (being consolidated)
├── build/             # CMake build output
└── CMakeLists.txt     # Build configuration
```

---

## Core Systems (All Operational)

### 1. ECS Architecture
- **EntityManager:** Entity lifecycle and component management
- **ComponentAccessManager:** Thread-safe component access
- **MessageBus:** Event-driven communication between systems
- **Location:** `include/core/ECS/`, `src/core/ECS/`

### 2. Game Systems
- **EconomicSystem:** Production, trade, resources
- **PopulationSystem:** Demographics, migration, employment
- **DiplomacySystem:** Treaties, embassies, relations (Bundle A complete)
- **MilitarySystem:** Units, recruitment, combat
- **AdministrativeSystem:** Governance and officials
- **TechnologySystem:** Research and tech trees
- **Location:** `include/game/`, `src/game/`

### 3. AI Systems
- **AIDirector:** Top-level AI coordination
- **NationAI:** National decision-making
- **CharacterAI:** Individual character behavior
- **AIAttentionManager:** Attention budget allocation
- **InformationPropagationSystem:** Knowledge spreading
- **Location:** `include/game/ai/`, `src/game/ai/`

### 4. Map & Rendering
- **MapRenderer:** Province rendering with LOD (0-3 complete, LOD 4 pending)
- **MapDataLoader:** GeoJSON/province data loading
- **ViewportCuller:** Frustum culling for performance
- **ProvinceGeometry:** Boundary and spatial data
- **Location:** `include/map/`, `src/map/`, `src/rendering/`

### 5. UI System
- **MainMenuUI:** Main game menu
- **PopulationInfoWindow:** Province population display
- **AdministrativeUI:** Administrative interface
- **TechnologyInfoWindow:** Tech tree display
- **PerformanceWindow:** Debug/performance metrics
- **Location:** `include/ui/`, `src/ui/`

### 6. Configuration & Data
- **ConfigManager:** Hot-reloadable JSON config (119+ parameters)
- **GameConfig:** Structured configuration objects
- **TypeRegistry:** Enum/string conversions
- **Location:** `include/game/config/`, `src/game/config/`

### 7. Save System
- **SaveManager:** Save/load with LZ4 compression
- **IncrementalSaveTracker:** Delta tracking
- **SaveCompression:** LZ4 integration
- **Location:** `include/core/save/`, `src/core/save/`

### 8. Threading
- **ThreadedSystemManager:** Multi-threaded system coordination
- **ThreadingTypes:** Strategy enums and configuration
- **Location:** `include/core/threading/`, `src/core/threading/`

---

## Type System

### Core Types (`include/core/types/game_types.h`)
- **SystemType:** ECS system identifiers (INVALID, ECS_FOUNDATION, MESSAGE_BUS, etc.)
- **DecisionType:** Decision categories for AI
- **FunctionType:** Function classification
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

## Platform Compatibility

### Current Approach (as of Oct 22, 2025)
- **WindowsCleanup.h:** Force-included header on Windows to clean up Windows.h macros
- **PlatformCompat.h:** Cross-platform library includes (SDL2, ImGui, JsonCpp, OpenGL)
- **CMake /FI option:** Automatically includes WindowsCleanup.h before every .cpp file on Windows

### Windows Macro Issues
Windows.h defines macros that conflict with C++ code:
- `INVALID` conflicts with enum values
- `ERROR`, `DELETE`, `IN`, `OUT` conflict with identifiers
- `min`, `max` conflict with std::min/max

**Solution:** WindowsCleanup.h includes Windows.h with `NOMINMAX` and `WIN32_LEAN_AND_MEAN`, then undefines all conflicting macros.

### Library Path Differences
- **JsonCpp:** `<json/json.h>` on Windows (vcpkg), `<jsoncpp/json/json.h>` on Linux (apt)
- **ImGui:** Same paths on both platforms
- **OpenGL:** `<glad/glad.h>` on Windows (vcpkg), `<GL/gl.h>` + `<GL/glext.h>` on Linux

---

## Build System

### Dependencies (vcpkg)
```
SDL2
glad
jsoncpp
openssl
lz4
imgui
```

### CMake Configuration
- **Minimum Version:** 3.15
- **C++ Standard:** C++17
- **Build Types:** Debug, Release
- **Force-Include:** `/FI"${CMAKE_SOURCE_DIR}/include/WindowsCleanup.h"` (Windows only)

### Build Commands
**Windows:**
```powershell
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

**Linux (Codespaces):**
```bash
cd build
cmake ..
make mechanica_imperii -j$(nproc)
./mechanica_imperii
```

---

## Current Development Status

### ✅ Completed
- ECS architecture with thread-safe component access
- All core game systems (economy, population, diplomacy, military, etc.)
- AI systems (director, nation AI, character AI, attention manager)
- Map rendering with LOD 0-3
- Configuration system with hot-reload
- Save/load system with compression
- Linux builds working perfectly
- Cross-platform compatibility layer (PlatformCompat.h)

### 🔧 In Progress
- **Windows build:** Fixing Windows.h macro pollution (build_errors_v7.txt)
- **CMake reconfiguration:** Need to apply force-include changes on Windows

### ⏳ Pending
- LOD 4 terrain rendering (heightmap-based)
- Performance optimization (texture atlasing, batching, mesh caching)
- Automated testing/CI setup

---

## Known Issues

### Windows Build (Oct 22, 2025)
**Problem:** Windows.h macros (`INVALID`, `ERROR`, etc.) pollute game_types.h enums  
**Root Cause:** Windows.h included by various headers without proper cleanup  
**Solution Implemented:** WindowsCleanup.h force-included via CMake `/FI` option  
**Status:** Waiting for CMake reconfiguration on Windows machine  

**Error Pattern:**
```
game_types.h(143,9): error C2143: syntax error: missing '}' before 'constant'
game_types.h(143,9): error C2059: syntax error: 'constant'
```
This occurs when `INVALID` macro (defined as 0) replaces `SystemType::INVALID = 0`.

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
- Manual memory management only when necessary for performance

### Threading
- Thread-safe component access via `ComponentAccessManager`
- Frame-based synchronization
- Background systems for non-critical operations

---

## AI Agent Guidelines

### When Modifying Code
1. **Check platform compatibility:** Use PlatformCompat.h for cross-platform includes
2. **Verify type usage:** Use strong types (ProvinceID, CharacterID) instead of raw integers
3. **Thread safety:** Use ComponentAccessManager for component access
4. **Error handling:** Check for nullptr before dereferencing component pointers
5. **Namespace structure:** Follow the directory-based namespace hierarchy

### When Debugging Build Errors
1. **Windows macro pollution:** Check if Windows.h is being included without cleanup
2. **Include order:** PlatformCompat.h should come before game headers
3. **Library paths:** Verify correct paths for JsonCpp, ImGui, OpenGL
4. **CMake changes:** Always reconfigure CMake after CMakeLists.txt modifications

### When Adding Features
1. **ECS pattern:** New features should use component-based design
2. **Configuration:** Add new parameters to GameConfig.json and ConfigHelpers
3. **Serialization:** Implement save/load methods for persistent state
4. **Threading:** Consider which systems need background processing

### Documentation Updates
- Update this file (AI_CONTEXT.md) when architecture changes
- Update ARCHITECTURE.md for design/pattern documentation
- Update BUILD.md for dependency or build process changes
- Keep main README.md high-level and user-focused

---

## Key Files for Reference

### Essential Headers
- `include/core/types/game_types.h` - Core type definitions
- `include/core/ECS/EntityManager.h` - ECS interface
- `include/game/config/GameConfig.h` - Configuration structure
- `include/utils/PlatformCompat.h` - Cross-platform compatibility
- `include/WindowsCleanup.h` - Windows macro cleanup

### Main Entry Points
- `apps/main.cpp` - Full game with all systems
- `apps/main_minimal.cpp` - Minimal test executable

### Build Configuration
- `CMakeLists.txt` - Build system configuration
- `config/GameConfig.json` - Runtime configuration

---

## Recent Changes (Oct 20-22, 2025)

1. **Cross-platform migration complete:** All source files use PlatformCompat.h
2. **Windows macro cleanup:** Created WindowsCleanup.h with force-include
3. **Include path fixes:** Standardized JsonCpp, ImGui, Windows.h includes
4. **DiplomacySystem Bundle A:** Embassy, treaty, and helper methods implemented
5. **Documentation consolidation:** Created this AI_CONTEXT.md file

---

## Next Steps

1. Reconfigure CMake on Windows to apply force-include changes
2. Verify Windows build completes successfully
3. Test executable on Windows
4. Implement LOD 4 terrain rendering
5. Performance profiling and optimization
6. Set up CI/CD pipeline for automated builds

---

**For More Details:**
- Architecture: See `ARCHITECTURE.md`
- Build Instructions: See `BUILD.md`
- User Documentation: See `README.md`
