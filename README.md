# Mechanica Imperii - Historical Grand Strategy Game

**Project Status:** âœ… **OPERATIONAL** - Core systems complete, Windows build requires CMake reconfigure  
**Last Updated:** October 22, 2025  
**Recent Achievement:** CMake presets integration and build system cleanup

---

## ðŸ"‹ **Project Overview**

**Mechanica Imperii** is a historical grand strategy game combining elements of Europa Universalis and Crusader Kings, spanning from 1000 AD to 1900 AD with primary focus on the period 1066-1900.

**Core Features:**
- **Geographic Scope:** Europe, Middle East, North Africa, Eastern Europe  
- **Province Count:** ~5000 provinces (county-level granularity)  
- **AI Capacity:** 500+ nations, 3000+ characters supported  
- **Time Period:** 1000 AD to 1900 AD (focus on 1066-1900)

---

## ðŸ› ï¸ **Technology Stack**

- **Language:** Modern C++17 (header/implementation separation with RAII patterns)
- **Graphics:** SDL2 + OpenGL 3.2
- **UI Framework:** ImGui
- **Build System:** CMake 3.15+ with presets
- **Dependencies:** vcpkg (Windows) / pkg-config (Linux)
- **Platform:** Windows (primary), Linux (Codespaces support)

---

## ðŸ"š **Documentation**

For comprehensive project information, see:

- **[AI_CONTEXT.md](AI_CONTEXT.md)** - Complete project context for AI assistants
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Technical architecture deep-dive
- **[BUILD.md](BUILD.md)** - Build instructions and troubleshooting

---

## ðŸš€ **Quick Start**

### Prerequisites
- **CMake:** 3.15 or later
- **vcpkg:** Set `VCPKG_ROOT` environment variable
- **C++17 Compiler:** MSVC 2019+ (Windows) or GCC 9+ (Linux)

### CMake Presets

| Preset | Platform | Generator | Build Type | Use Case |
|--------|----------|-----------|------------|----------|
| `windows-debug` | Windows | Ninja | Debug | Fast debug builds |
| `windows-release` | Windows | Ninja | Release | Fast release builds |
| `windows-vs-debug` | Windows | Visual Studio | Debug | IDE integration (debugging) |
| `windows-vs-release` | Windows | Visual Studio | Release | IDE integration (release) |
| `dev` | Windows | Ninja | Debug | Quick iteration (tests ON) |
| `linux-debug` | Linux | Ninja | Debug | Linux debug builds |
| `linux-release` | Linux | Ninja | Release | Linux release builds |

### Windows
```powershell
# Set vcpkg path (one-time, or add to system environment)
$env:VCPKG_ROOT = "C:\vcpkg"

# Configure and build (Ninja - fastest)
cmake --preset windows-release
cmake --build --preset windows-release

# Or use Visual Studio for IDE integration
cmake --preset windows-vs-release
cmake --build --preset windows-vs-release

# Run
.\build\windows-release\bin\mechanica_imperii.exe
```

### Linux (Codespaces)
```bash
# Set vcpkg path (add to ~/.bashrc)
export VCPKG_ROOT="/path/to/vcpkg"

# Configure and build
cmake --preset linux-release
cmake --build --preset linux-release

# Run
./build/linux-release/bin/mechanica_imperii
```

---

## ðŸ"‹ **Current Status**

### âœ… **Application Status: Operational**
- **Main Executable**: Compiles successfully with all core systems
- **Build System**: CMake presets for clean cross-platform builds
- **Dependencies**: vcpkg.json manifest for reproducible builds
- **Test Results**: Integration tests passing for core systems

### ðŸ"§ **Operational Systems (18 Total)**

**Core Systems (4):**
- âœ… ECS Architecture (EntityManager, ComponentAccessManager, MessageBus)
- âœ… Threading System (Multi-threaded coordination)
- âœ… Save System (LZ4 compression, validation, recovery)
- âœ… Configuration System (JSON hot-reload, 119+ parameters)

**Game Systems (8):**
- âœ… Economic System (Production, trade, resources)
- âœ… Population System (Demographics, migration, PopulationAggregator)
- âœ… Military System (Recruitment, combat, units)
- âœ… Administrative System (Governance, officials)
- âœ… Diplomacy System (Treaties, embassies, relations)
- âœ… Technology System (Research trees)
- âœ… Time Management System (Game clock, events)
- âœ… Province Management System (Decision queues)

**AI Systems (5):**
- âœ… AI Director (Top-level coordination)
- âœ… Nation AI (Strategic decisions)
- âœ… Character AI (Individual behavior)
- âœ… AI Attention Manager (Budget allocation)
- âœ… Information Propagation System (Knowledge spreading)

**Rendering System (1):**
- âœ… Map Renderer (LOD 0-3, viewport culling)

### ðŸ"Š **Recent Achievements**
- **CMake Presets Integration** - Platform-agnostic build configuration (Oct 22, 2025)
- **vcpkg Manifest** - Declarative dependency management (Oct 22, 2025)
- **Build System Cleanup** - Fixed duplicates, output directories, variable mismatches (Oct 22, 2025)
- **DiplomacySystem Bundle A** - Core diplomacy features (Oct 20, 2025)
- **AI System Integration** - All 5 AI subsystems operational (Oct 20, 2025)

---

## ðŸ—ï¸ **Architecture Overview**

### **Core Architecture Patterns** âœ…
- **ECS (Entity Component System)** - Primary architectural pattern
- **Message Bus Pattern** - Inter-system communication
- **Type Registry System** - String/enum conversions
- **Threaded System Manager** - Performance optimization
- **Configuration System** - Hot-reloadable external configuration

### **ECS Architecture Status** âœ… **PRODUCTION READY**
- **EntityManager:** Header-only implementation with thread-safe operations
- **ComponentAccessManager:** Thread-safe component access with shared_mutex
- **Component System:** CRTP-based Component<T> inheritance pattern
- **Integration Status:** All core systems fully ECS-integrated

### **Recent Major Refactoring** âœ… **October 2025**
- **PopulationAggregator:** Centralized statistics calculator
- **Performance Optimization:** Single-pass algorithms with RAII
- **Code Quality:** Exception-safe design, modern C++ patterns

---

## ðŸ" **Project Structure**

```
mechanica_imperii/
â"œâ"€â"€ apps/                   # Application entry points
â"‚   â"œâ"€â"€ main.cpp           # Full game (requires all systems)
â"‚   â""â"€â"€ main_minimal.cpp   # Minimal build (default)
â"œâ"€â"€ include/               # Header files (public interfaces)
â"‚   â"œâ"€â"€ core/             # Core engine systems (ECS, threading, save)
â"‚   â"œâ"€â"€ game/             # Game systems (economy, military, diplomacy)
â"‚   â"œâ"€â"€ map/              # Map rendering and data
â"‚   â""â"€â"€ ui/               # ImGui UI components
â"œâ"€â"€ src/                  # Implementation files (mirrors include/)
â"œâ"€â"€ config/               # Configuration files
â"‚   â""â"€â"€ GameConfig.json   # Main game configuration
â"œâ"€â"€ build/                # CMake build outputs (per preset)
â"‚   â"œâ"€â"€ windows-release/  # Windows release build
â"‚   â"‚   â""â"€â"€ bin/          # Executable + runtime files
â"‚   â""â"€â"€ linux-release/    # Linux release build
â"‚       â""â"€â"€ bin/          # Executable + runtime files
â"œâ"€â"€ CMakeLists.txt        # Build configuration
â"œâ"€â"€ CMakePresets.json     # Platform-specific presets
â""â"€â"€ vcpkg.json           # Dependency manifest
```

---

## ðŸŽ¯ **Production-Ready Features**

### **Configuration System** âœ…
- Hot-reloadable JSON configuration
- 119+ parameters across all systems
- Type-safe access patterns
- Fallback defaults for missing values

### **Save System** âœ…
- Full and incremental saves
- LZ4 compression (60-80% size reduction)
- Validation and recovery mechanisms
- Schema version tracking

### **AI Performance** âœ…
- 98% CPU load reduction via attention management
- Supports 500+ nations with selective updates
- Information propagation with distance-based delays
- Personality-driven character decisions

### **Map Rendering** âœ…
- LOD 0-3 implemented (strategic to provincial view)
- Viewport culling (70-90% province elimination)
- Province boundaries, terrain, cities
- LOD 4 (terrain rendering) pending

---

## ðŸ"§ **Development Status**

### **Important: After Pulling Updates**

**⚠️ Windows Users - Action Required:**
- Recent CMakeLists.txt updates require reconfiguration
- **Action:** Delete `build\windows-release` and run `cmake --preset windows-release`
- **Details:** See [BUILD.md](BUILD.md#️-important-reconfiguring-after-updates)

**✅ Linux Build:**
- Fully operational with system packages (no vcpkg needed)
- Automatic FetchContent fallbacks for glad, ImGui, and lz4

### **Recent Updates (Oct 22, 2025)**
- âœ… Linux builds without vcpkg (system packages + FetchContent)
- âœ… Automatic glad generation via Python (OpenGL 3.3 core)
- âœ… C language support enabled (required for glad and lz4)
- âœ… ImGui linked with SDL2 and OpenGL for backend support
- âœ… Flexible GLAD_LIBRARIES variable for cross-platform builds
- âœ… BUILD.md updated with clear reconfiguration instructions

---

## ðŸ"Š **Build Configuration**

### **Using Presets**
```bash
# Configure
cmake --preset <preset-name>

# Build
cmake --build --preset <preset-name>

# Build options
cmake --preset windows-release -DBUILD_TESTS=ON
cmake --preset windows-release -DBUILD_DOCS=ON
```

### **Build Options**
| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | OFF | Build test executables |
| `BUILD_DOCS` | OFF | Generate Doxygen documentation |
| `USE_VENDOR_LZ4` | ON | Fetch LZ4 if not found system-wide |

### **Output Structure**
```
build/<preset-name>/
â"œâ"€â"€ bin/
â"‚   â"œâ"€â"€ mechanica_imperii(.exe)   # Main executable
â"‚   â"œâ"€â"€ data/                      # Runtime configuration
â"‚   â""â"€â"€ shaders/                   # GLSL shaders
â""â"€â"€ CMakeFiles/                    # Build internals
```

---

## ðŸ¤ **Contributing**

This project follows established ECS integration patterns. Use the Population, Economic, Military, and Administrative systems as templates.

### **ECS Integration Template**
1. Create `YourSystemComponents.h` with `Component<T>` inheritance
2. Update system class with ECS constructor and component methods
3. Add component creation/access via ComponentAccessManager
4. Follow namespace patterns (`game::your_system`)
5. Implement proper error handling and logging

---

## ðŸ"„ **License**

[License information to be added]

---

**Mechanica Imperii** - Bringing historical grand strategy to life with modern software engineering practices.
