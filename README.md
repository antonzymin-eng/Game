# Mechanica Imperii - Historical Grand Strategy Game

**Project Status:** 18+ Production-Ready Systems - ECS Architecture Complete + Major Refactoring  
**Last Updated:** October 12, 2025  
**Recent Achievement:** Extensive codebase refactoring with performance optimizations ✅

---

## 📋 **Project Overview**

**Mechanica Imperii** is a historical grand strategy game combining elements of Europa Universalis and Crusader Kings, spanning from 1000 AD to 1900 AD with primary focus on the period 1066-1900.

**Core Features:**
- **Geographic Scope:** Europe, Middle East, North Africa, Eastern Europe  
- **Province Count:** ~5000 provinces (county-level granularity)  
- **AI Capacity:** 500+ nations, 3000+ characters supported  
- **Time Period:** 1000 AD to 1900 AD (focus on 1066-1900)

---

## 🛠️ **Technology Stack**

- **Language:** Modern C++17 (header/implementation separation with RAII patterns)
- **Graphics:** SDL2 + OpenGL 3.2
- **UI Framework:** ImGui
- **Build System:** CMake 3.28+
- **Platform:** Windows (primary development), Linux (dev container support)
- **JSON Library:** jsoncpp (fully integrated configuration system)

---

## 🏗️ **Architecture Overview**

### **Core Architecture Patterns** ✅
- **ECS (Entity Component System)** - Primary architectural pattern (FULLY RESOLVED ✅)
- **Message Bus Pattern** - Inter-system communication
- **Type Registry System** - String/enum conversions, type safety
- **Threaded System Manager** - Performance optimization
- **Configuration System** - Hot-reloadable external configuration
- **Single Responsibility Principle** - Specialized utility classes for performance

### **ECS Architecture Status** ✅ **PRODUCTION READY**
- **EntityManager:** Header-only implementation with thread-safe operations
- **ComponentAccessManager:** Thread-safe component access patterns with shared_mutex
- **Component System:** CRTP-based Component<T> inheritance pattern
- **Integration Status:** 4 core systems fully ECS-integrated with validation

### **Recent Major Refactoring** ✅ **October 12, 2025**
- **PopulationAggregator:** New centralized statistics calculator (eliminated 75+ lines of duplicated code)
- **Performance Optimization:** Single-pass aggregation algorithms with RAII patterns
- **Code Quality:** Exception-safe design with comprehensive data validation
- **Memory Management:** Modern C++ smart pointer usage throughout

---

## 📁 **Project Structure**

```
mechanica_imperii/
├── apps/                           # Application entry points
│   ├── main.cpp                   # Main game executable
│   └── main_minimal.cpp           # Minimal test build
├── include/                        # Header files (public interfaces)
│   ├── core/                      # Core engine systems
│   │   ├── ECS/                   # Entity Component System
│   │   ├── logging/               # Logging system
│   │   ├── save/                  # Save/load system
│   │   ├── threading/             # Multi-threading support
│   │   └── types/                 # Core type definitions
│   ├── game/                      # Game-specific systems
│   │   ├── administration/        # Administrative system (ECS ✅)
│   │   ├── config/                # Configuration management
│   │   ├── economy/               # Economic system (ECS ✅)
│   │   ├── military/              # Military system (ECS ✅)
│   │   ├── population/            # Population system (ECS ✅) + PopulationAggregator
│   │   ├── province/              # Province management
│   │   ├── technology/            # Technology research
│   │   └── time/                  # Time management
│   ├── map/                       # Geographic and mapping systems
│   └── ui/                        # User interface components
├── src/                           # Implementation files
│   ├── core/                      # Core system implementations
│   ├── game/                      # Game system implementations
│   ├── rendering/                 # Graphics and rendering
│   ├── ui/                        # UI system implementations
│   └── utils/                     # Utility functions
├── config/                        # Configuration files
│   └── GameConfig.json           # Main game configuration
├── tests/                         # Test files and validation
│   └── test_*_integration.cpp    # ECS integration tests
└── build/                         # CMake build directory
```

---

## 🎉 **Production-Ready Systems** (18+ Systems Complete)

### **🎯 ECS-Integrated Core Systems** (Template for remaining systems)

1. **Population System** ✅ **EXTENSIVELY REFACTORED**
   - **Status:** Full ECS integration + major performance refactoring
   - **Components:** PopulationComponent, SettlementComponent, PopulationEventsComponent
   - **New Feature:** PopulationAggregator - centralized statistics calculator
   - **Performance:** Eliminated duplicate code, single-pass algorithms, RAII patterns
   - **Location:** `src/game/population/` (5 files including new PopulationAggregator)

2. **Economic System** ✅
   - **Status:** Complete ECS integration with 5 specialized components
   - **Components:** EconomicComponent, TradeComponent, TreasuryComponent, EconomicEventsComponent, MarketComponent
   - **Location:** `src/game/economy/` (6 files)

3. **Military System** ✅
   - **Status:** Complete ECS integration with combat and recruitment
   - **Components:** MilitaryComponent, ArmyComponent, FortificationComponent, CombatComponent, MilitaryEventsComponent
   - **Location:** `src/game/military/` (8 files)

4. **Administrative System** ✅
   - **Status:** Complete ECS integration with governance mechanics
   - **Components:** GovernanceComponent, BureaucracyComponent, LawComponent, AdministrativeEventsComponent
   - **Location:** `src/game/administration/` (2 files)

### **🚀 Supporting Production Systems**

5. **Configuration System** ✅
   - **Status:** Working with jsoncpp integration
   - **Features:** Hot-reloadable configuration, type-safe access
   - **Location:** `src/game/config/`

6. **ECS Core** ✅
   - **Status:** Thread-safe, header-only EntityManager
   - **Features:** Component<T> CRTP, ComponentAccessManager, MessageBus
   - **Location:** `include/core/ECS/`

7. **Threading System** ✅
   - **Status:** ThreadedSystemManager operational
   - **Features:** Multi-threaded system updates, performance optimization
   - **Location:** `src/core/threading/`

8. **Save System** ✅
   - **Status:** Comprehensive serialization with validation
   - **Features:** SaveManager with recovery and validation
   - **Location:** `src/core/save/`

9. **Province Management System** ✅
   - **Status:** Priority-based decision queues
   - **Location:** `src/game/province/`

10. **Technology System** ✅
    - **Status:** Research trees with prerequisites
    - **Location:** `src/game/technology/`

11. **Time Management System** ✅
    - **Status:** Multi-tick clock system
    - **Location:** `src/game/time/`

12-18. **Additional Systems:** Diplomacy, Trade, AI, Realm Management, Map Systems, UI Systems, Rendering (all production-ready)

---

## 🔧 **Recent Major Achievements**

### **October 12, 2025 - Extensive Refactoring** ✅
- **Code Duplication Elimination:** Created PopulationAggregator class, removed 75+ lines of duplicate code
- **Performance Optimization:** Single-pass aggregation algorithms, optimized data structures
- **Modern C++ Patterns:** RAII implementation, exception safety, smart pointer usage
- **Build System:** Clean compilation with zero errors/warnings
- **Data Validation:** Comprehensive consistency checking with logging

### **October 11, 2025 - ECS Integration Completion** ✅
- **Administrative System:** Complete ECS integration (4th core system)
- **Architecture Resolution:** All ECS inconsistencies resolved
- **Template Established:** 4-system ECS pattern ready for remaining systems

### **Build Status** ✅
- **Current Status:** All systems compile and link successfully
- **Test Coverage:** ECS integration tests passing for all 4 core systems
- **Performance:** Optimized aggregation algorithms operational

---

## 🚀 **Getting Started**

### **Prerequisites**
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.28 or higher
- SDL2 development libraries
- jsoncpp library

### **Building the Project**

```bash
# Clone the repository
git clone <repository-url>
cd mechanica_imperii

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build (parallel build recommended)
make -j$(nproc)

# Run the game
./mechanica_imperii
```

### **Development Build**
```bash
# For development with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

---

## 📊 **Code Quality Metrics**

- **Architecture:** Modern ECS with CRTP patterns
- **Memory Management:** RAII throughout, smart pointers used
- **Thread Safety:** Thread-safe component access, shared_mutex patterns
- **Performance:** Optimized algorithms, single-pass aggregations
- **Maintainability:** Single responsibility principle, centralized utilities
- **Documentation:** Comprehensive inline documentation and external docs

---

## 📚 **Documentation**

All project documentation has been organized into the `docs/` directory:

- **📁 docs/architecture/** - System architecture, design patterns, and structural documentation
- **📁 docs/development/** - Development workflows, project tracking, and procedures  
- **📁 docs/integration/** - System integration documentation and summaries
- **📁 docs/reference/** - API references, quick guides, and lookup documentation

**Quick Start:** See `docs/README.md` for complete documentation index and navigation guide.

---

## 🤝 **Contributing**

This project follows the established ECS integration pattern. Use the Population, Economic, Military, and Administrative systems as templates for implementing new systems.

### **ECS Integration Template**
1. Create `YourSystemComponents.h` with `game::core::Component<T>` inheritance
2. Update system class with ECS constructor and component methods
3. Add component creation/access methods using ComponentAccessManager
4. Follow the established namespace patterns (`game::your_system`)
5. Implement proper error handling and logging

---

## 📄 **License**

[License information to be added]

---

**Mechanica Imperii** - Bringing historical grand strategy to life with modern software engineering practices.