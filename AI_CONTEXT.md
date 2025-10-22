# AI Agent Context - Mechanica Imperii

**Last Updated:** October 22, 2025  
**Purpose:** Comprehensive project context for AI assistants and automated tools

---

## Project Identity

**Name:** Mechanica Imperii  
**Type:** Historical Grand Strategy Game (C++17)  
**Genre:** EU4/CK3-inspired grand strategy spanning 1000-1900 AD  
**Status:** Operational - Core systems complete, Windows build requires CMake reconfigure

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

### ðŸ"§ Action Required
**Windows Build:**
- Status: Requires CMake reconfigure to apply recent changes
- Reason: CMakeLists.txt updated with fixes (duplicates removed, output dirs set)
- Action: Run `cmake --preset windows-release` on Windows machine

**Linux Build:**
- Status: Fully operational, no action required

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

## Recent Changes (October 22, 2025)

### Build System Fixes
1. **Removed duplicate `BUILD_TESTS` option** (single definition)
2. **Fixed `TIME_SOURCES` variable** (was `TIME_MANAGEMENT_SOURCES` in tests)
3. **Removed duplicate `REALM_SOURCES`** reference in ALL_SOURCES
4. **Set `RUNTIME_OUTPUT_DIRECTORY`** to `bin/` for all targets
5. **Added ImGui FetchContent fallback** for Linux distributions

### CMake Presets Expansion
1. **Added Visual Studio presets** (`windows-vs-debug`, `windows-vs-release`)
2. **Added `dev` preset** for quick iteration with tests enabled
3. **Improved preset inheritance** with hidden base presets
4. **Added generator options** (Ninja for speed, VS for IDE)

### Documentation Alignment
1. **Standardized system count** to 18 (4 core + 8 game + 5 AI + 1 render)
2. **Clarified Windows build status** (requires CMake reconfigure)
3. **Updated build commands** to reflect CMake presets
4. **Corrected output paths** (build/<preset>/bin/)

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
