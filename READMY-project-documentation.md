# Mechanica Imperii - Complete Project Documentation Standard

**Updated:** October 7, 2025

## 📋 **Master Project Overview**

**Project Name:** Mechanica Imperii  
**Genre:** Historical Grand Strategy Game  
**Time Period:** 1000 AD to 1900 AD (focus on 1066-1900)  
**Geographic Scope:** Europe, Middle East, North Africa, Eastern Europe  
**Province Count:** ~5000 provinces (county-level granularity)  
**AI Capacity:** 500+ nations, 3000+ characters supported  

**Core Technology Stack:**
- **Language:** C++17 (always create both .cpp and .h files)
- **Graphics:** SDL2 + OpenGL 3.2
- **UI Framework:** ImGui
- **Build System:** CMake
- **Platform:** Windows (primary development)

---

## 🏗️ **Architecture & System Organization**

### **Core Architecture Patterns**
- **ECS (Entity Component System)** - Primary architectural pattern
- **Message Bus Pattern** - Inter-system communication
- **Type Registry System** - String/enum conversions, type safety
- **Threaded System Manager** - Performance optimization
- **Configuration System** - Hot-reloadable external configuration
- **Information-Driven Load Balancing** - AI decision distribution

### **Directory Structure Standard**
```
include/           <- All .h header files
  core/
    config/        <- GameConfig system
    ECS/           <- Entity Component System
    Threading/     <- Threading management
    save/          <- Save/load systems
  game/
    diplomacy/     <- Diplomatic relations
    economic/      <- Trade and economy
    population/    <- Population management
    technology/    <- Technology trees
    military/      <- Military units
    administration/<- Administrative systems
    ai/            <- AI systems
src/               <- All .cpp implementation files
  (mirrors include/ structure)
data/              <- Configuration files
  GameConfig.json  <- Centralized configuration
```

### **File Creation Standards**
- **Every file must include:** Date and time of creation at top
- **Every file must include:** Intended folder location at top
- **Maximum file length:** 500 lines of code (split at line 501)
- **.txt files:** Manually converted to proper format
- **No inline implementation:** All files must be standalone
- **Headers:** Declarations only in .h files
- **Implementation:** All code in .cpp files

---

## 🎯 **Game System Specifications**

### **Population System** ✅ PRODUCTION READY
- 17 social classes with employment patterns
- Medieval demographics with birth/death rates
- Crisis management (plague, famine, disasters)
- Settlement evolution and specialization
- **Threading:** THREAD_POOL (10/1/0.5 FPS)

### **Technology System** ✅ PRODUCTION READY
- Research trees: Military, Economic, Administrative, Cultural
- Historical progression (1000-1900 AD)
- Education-dependent research speed
- **Threading:** THREAD_POOL (monthly updates)

### **Military System** ✅ PRODUCTION READY
- 21 historical unit types
- Quality progression: Green → Legendary
- Population-integrated recruitment
- **Threading:** THREAD_POOL (1 FPS)

### **Economic/Trade System** ✅ PRODUCTION READY
**Status:** Fully compliant with hot-reload configuration
- Dynamic trade routes
- Multiple regional currencies
- Production, consumption, trade balance
- **Threading:** THREAD_POOL (10 FPS)
- **Config:** 31 externalized parameters

### **Diplomacy System** ✅ PRODUCTION READY
**Status:** Fully compliant with hot-reload configuration
- Treaty system (alliances, trade, marriage)
- Opinion and trust mechanics
- AI diplomatic decisions
- **Threading:** THREAD_POOL
- **Config:** 27 externalized parameters

### **EconomicPopulationBridge** ✅ PRODUCTION READY
**Status:** Fully compliant with hot-reload configuration
- Links economic and population systems
- Tax collection and happiness effects
- Crisis detection and response
- **Threading:** THREAD_POOL
- **Config:** 40+ externalized parameters

### **Administrative System** ✅ PRODUCTION READY
- Efficiency based on technology and literacy
- Corruption management
- Provincial autonomy
- **Threading:** THREAD_POOL (1 FPS)

### **ProvinceManagementSystem** ✅ PRODUCTION READY
- Priority-based decision queue
- Construction, policy, research orders
- Automation levels (Manual → Automated)
- **Threading:** MAIN_THREAD (UI compatibility)

### **Time Management System** ✅ PRODUCTION READY
- Multi-tick clock (hourly/daily/monthly)
- Historical calendar from 1066 AD
- Message delays based on distance
- Event scheduling with callbacks
- **Threading:** THREAD_POOL (variable frequency)

---

## 🤖 **AI System Specifications**

### **InformationPropagationSystem** ✅ PRODUCTION READY
- Distance-based message delays (50 km/day)
- Accuracy degradation per hop
- Priority-based propagation
- **Performance:** 98% load reduction
- **Threading:** THREAD_POOL

### **AIAttentionManager** ✅ PRODUCTION READY
- 10 character archetypes
- 4-tier relevance filtering
- Special interest tracking
- **Threading:** THREAD_POOL

### **Realm Entity System** ✅ PRODUCTION READY
- Nation/state political structures
- Dynasty and succession
- Diplomatic relations
- **Threading:** MAIN_THREAD

### **AIDirector** ✅ PRODUCTION READY
- Master AI coordinator
- 60 FPS processing target
- Dynamic load balancing (5-20 actors/frame)
- **Threading:** DEDICATED_THREAD
- **Performance:** <1% thread utilization for 500+ nations

### **NationAI** ✅ PRODUCTION READY
- Strategic decision-making
- Personality-driven behavior
- Threat assessment (5 levels)
- **Threading:** Via AIDirector

### **CharacterAI** ✅ PRODUCTION READY
- Personal ambitions and goals
- Mood system
- Relationship tracking
- **Threading:** Via AIDirector

---

## ⚙️ **Configuration Management System**

### **GameConfig Architecture** ✅ ENHANCED
**Updated:** September 27, 2025

**Features:**
- **Hot Reload:** Automatic file change detection
- **Change Callbacks:** Section-specific notifications
- **Thread Safety:** Mutex-protected operations
- **Validation:** Range checking and section validation
- **Single File:** All configuration in one JSON file

**Location:**
- Header: `include/core/config/GameConfig.h`
- Implementation: `src/core/config/GameConfig.cpp`
- Data: `data/GameConfig.json`

**Usage Pattern:**
```cpp
auto& config = game::config::GameConfig::Instance();

// Load configuration
config.LoadFromFile("data/GameConfig.json");

// Enable hot reload (1 second check interval)
config.EnableHotReload(1.0);

// Register section callbacks
config.RegisterChangeCallback("diplomacy", [](const std::string& section) {
    // Reload diplomacy parameters
});

// Access values with defaults
int value = config.GetInt("section.parameter", default_value);
```

### **Configuration Categories**

**diplomacy (27 parameters):**
- Treaty durations, opinion limits, trust mechanics
- Marriage bonuses, prestige effects
- Compliance thresholds, decay rates

**economy (31 parameters):**
- Starting treasury, monthly expenses
- Stability modifiers, event chances
- Trade efficiency, harvest effects
- War exhaustion, plague thresholds

**economic_bridge (40+ parameters):**
- Tax rates and happiness effects
- Employment effects, inequality thresholds
- Productivity bonuses, crisis detection
- Infrastructure effects, performance settings

**military (4 parameters):**
- Unit costs, recruitment rates
- Maintenance costs, quality progression

**population (4 parameters):**
- Growth rates, migration thresholds
- Education effects, class mobility

**technology (3 parameters):**
- Research speeds, prerequisites
- Education requirements

**administration (3 parameters):**
- Efficiency calculations, corruption rates
- Autonomy effects

**threading (3 parameters):**
- Thread pool sizes, update frequencies
- Load balancing parameters

**debug (4 parameters):**
- Logging levels, profiling options
- Performance monitoring

**hot_reload (2 parameters):**
- Enable/disable, check interval

---

## 🔀 **Threading Architecture**

### **Threading Strategy Assignment**
- **PopulationSystem:** THREAD_POOL (10/1/0.5 FPS)
- **TechnologySystem:** THREAD_POOL (monthly)
- **MilitarySystem:** THREAD_POOL (1 FPS)
- **EconomicSystem:** THREAD_POOL (10 FPS)
- **DiplomacySystem:** THREAD_POOL (event-driven)
- **EconomicPopulationBridge:** THREAD_POOL (continuous)
- **AdministrativeSystem:** THREAD_POOL (1 FPS)
- **ProvinceManagementSystem:** MAIN_THREAD (0.5 Hz)
- **TimeManagementSystem:** THREAD_POOL (variable)
- **InformationPropagationSystem:** THREAD_POOL (continuous)
- **AIAttentionManager:** THREAD_POOL (continuous)
- **RealmManager:** MAIN_THREAD (infrequent)
- **AIDirector:** DEDICATED_THREAD (60 FPS)
- **NationAI/CharacterAI:** Via AIDirector
- **RenderSystem:** MAIN_THREAD_ONLY
- **UISystem:** MAIN_THREAD_ONLY

### **Thread Safety Requirements**
- All shared data must be thread-safe
- Message passing between threads only
- No raw pointer sharing between threads
- Proper mutex protection for caches
- AI systems isolated on DEDICATED_THREAD

---

## 📈 **Performance Standards**

### **Update Frequency Tiers**
- **Real-time:** 60 FPS (Rendering, Input, UI, AIDirector)
- **High-frequency:** 10 FPS (Economics, Population, Trade)
- **Medium-frequency:** 1 FPS (Diplomacy, Administration, Military)
- **Low-frequency:** 0.5 FPS (Province Management UI)
- **Ultra-low:** Monthly/Yearly (Technology)
- **Variable:** Time-dependent (Time Management)
- **Event-driven:** AI decisions (information packets)

### **AI Performance Metrics**
- **Peak Load Reduction:** 98% (200 → 4/sec)
- **Sustained Load:** 73/sec → 1.4/sec
- **Thread Utilization:** <1% for 500+ nations
- **Scalability:** 500+ nations, 3000+ characters
- **Response Time:** Critical <1 sec, routine 5-30 sec

---

## 💾 **Save System Standards**

### **Save Format**
- **JSON-based:** Human-readable
- **Versioned:** Forward/backward compatibility
- **Incremental:** Save only changed data
- **Compressed:** For large game states

### **SaveManager Features** ✅
- Atomic file operations with SHA-256 checksums
- Crash recovery with automatic backup
- Thread-safe operations
- C++17 compliant (custom Expected<T,E>)
- Migration system for version upgrades

---

## 📊 **System Completion Status**

### **Production Ready Systems** ✅
**Total: 18 Complete Systems**

**Core Systems (12):**
1. SaveManager System
2. Threading System
3. Administrative System
4. Military System
5. Population System
6. ProvinceManagementSystem
7. Time Management System
8. Technology System
9. Trade/Economic System
10. Diplomacy System ✅ *Recently Fixed*
11. EconomicPopulationBridge ✅ *Recently Fixed*
12. GameConfig System ✅ *Enhanced with Hot Reload*

**AI Systems (6):**
13. InformationPropagationSystem
14. AIAttentionManager
15. Realm Entity System
16. AIDirector
17. NationAI
18. CharacterAI

### **Recent System Updates (Sept 27, 2025)**

**DiplomacySystem:**
- Fixed duplicate Initialize() method
- Added GetThreadingStrategy()
- Implemented ISerializable
- Externalized 27 parameters to GameConfig

**EconomicSystem:**
- Added GetThreadingStrategy()
- Implemented ISerializable
- Externalized 31 parameters to GameConfig

**EconomicPopulationBridge:**
- Added GetThreadingStrategy()
- Implemented ISerializable
- Externalized 40+ parameters to GameConfig

**GameConfig System:**
- Added hot reload capability
- Section-based change callbacks
- Configuration validation
- 1-second file monitoring interval

---

## 🔧 **Build System Integration**

### **CMake Configuration**
```cmake
# Config System
set(CONFIG_SOURCES
    src/core/config/GameConfig.cpp
)

# Diplomacy System
set(DIPLOMACY_SOURCES
    src/game/diplomacy/DiplomacySystem.cpp
    src/game/diplomacy/DiplomacySystemSerialization.cpp
)

# Economic Bridge
set(ECONOMIC_BRIDGE_SOURCES
    src/game/economic/EconomicPopulationBridge.cpp
    src/game/economic/EconomicPopulationBridgeSerialization.cpp
)

# Economic System (updated)
set(ECONOMIC_SOURCES
    src/game/economic/EconomicSystem.cpp
    src/game/economic/EconomicSystemSerialization.cpp
)

# AI Systems
set(AI_SOURCES
    src/game/ai/information/InformationPropagationSystem.cpp
    src/game/ai/attention/AIAttentionManager.cpp
    src/game/ai/realm/RealmManager.cpp
    src/game/ai/director/AIDirector.cpp
    src/game/ai/nation/NationAI.cpp
    src/game/ai/character/CharacterAI.cpp
)

# Complete build
add_executable(mechanica_imperii
    ${CONFIG_SOURCES}
    ${DIPLOMACY_SOURCES}
    ${ECONOMIC_BRIDGE_SOURCES}
    ${ECONOMIC_SOURCES}
    ${AI_SOURCES}
    # ... other sources
)

# Copy config file
configure_file(
    ${CMAKE_SOURCE_DIR}/data/GameConfig.json
    ${CMAKE_BINARY_DIR}/data/GameConfig.json
    COPYONLY
)
```

---

## 🚀 **Development Standards**

### **Code Quality Checklist**
For each system:
- [ ] GetThreadingStrategy() method added
- [ ] ISerializable interface implemented
- [ ] All hardcoded values moved to GameConfig
- [ ] Component registration in Initialize()
- [ ] No duplicate methods
- [ ] Proper header/cpp separation
- [ ] Thread-safe operations
- [ ] Error handling in serialization
- [ ] Config defaults for all parameters

### **Hot Reload Integration**
```cpp
// In main game loop
auto& config = game::config::GameConfig::Instance();

while (running) {
    // Check for config changes
    if (config.CheckForChanges()) {
        std::cout << "Config reloaded!" << std::endl;
    }
    
    Update(deltaTime);
    Render();
}
```

---

## 📚 **Documentation Requirements**

### **File Documentation**
- **Header comments:** File purpose, creation date, location
- **Function documentation:** Parameters, return values, side effects
- **Complex algorithms:** Step-by-step explanation
- **Threading concerns:** Concurrency notes
- **Config dependencies:** List required GameConfig parameters

### **System Documentation**
- **Architecture decisions:** Why specific patterns chosen
- **Performance characteristics:** Expected behavior under load
- **Dependencies:** What systems this relies on
- **Configuration options:** All externalized parameters
- **Integration points:** How systems connect

---

This documentation serves as the **single source of truth** for all development decisions, ensuring consistency across the project lifecycle.