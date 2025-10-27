# API Reference - Mechanica Imperii

**Last Updated:** October 26, 2025  
**Purpose:** Complete reference of all functions, classes, namespaces, and variables

---

## Table of Contents
1. [Core Namespace](#core-namespace)
2. [Game Namespace](#game-namespace)
3. [Global Variables](#global-variables)
4. [System Constructors](#system-constructors)
5. [Common Patterns](#common-patterns)

---

## Core Namespace

### core::ecs

#### EntityManager
**Location:** `include/core/ECS/EntityManager.h`, `src/core/ECS/EntityManager.cpp`

**Purpose:** Manages entity lifecycle and component attachment

**Key Methods:**
```cpp
// Entity creation/destruction
EntityID CreateEntity();
void DestroyEntity(EntityID entity);
bool IsEntityValid(EntityID entity) const;

// Component management (returns shared_ptr, NOT reference!)
template<typename ComponentType, typename... Args>
std::shared_ptr<ComponentType> AddComponent(uint64_t entity_id, Args&&... args);

template<typename ComponentType>
std::shared_ptr<ComponentType> GetComponent(EntityID entity);

template<typename ComponentType>
bool HasComponent(EntityID entity) const;

template<typename ComponentType>
void RemoveComponent(EntityID entity);
```

**Critical Notes:**
- `AddComponent` returns `std::shared_ptr<T>`, use `->` operator, NOT `.`
- `EntityID` has `.id` member (uint64_t), no `Get()` method
- Always check return value for nullptr before accessing

---

#### ComponentAccessManager
**Location:** `include/core/ECS/ComponentAccessManager.h`, `src/core/ECS/ComponentAccessManager.cpp`

**Purpose:** Thread-safe wrapper around EntityManager for system access

**Constructor:**
```cpp
explicit ComponentAccessManager(EntityManager* entity_manager, MessageBus* message_bus);
```

**Key Methods:**
```cpp
// Component access (thread-safe)
template<typename ComponentType>
std::shared_ptr<ComponentType> GetComponent(EntityID entity);

template<typename ComponentType>
std::vector<std::shared_ptr<ComponentType>> GetAllComponents();

// Batch operations
template<typename ComponentType>
void ForEachComponent(std::function<void(ComponentType&)> func);

// Access underlying managers
EntityManager* GetEntityManager();
MessageBus* GetMessageBus();
```

**Usage Pattern:**
```cpp
// Get component via ComponentAccessManager
auto component = access_manager.GetComponent<PopulationComponent>(entity);
if (component) {
    component->total_population = 10000;  // Use -> operator
}
```

---

#### MessageBus
**Location:** `include/core/ECS/MessageBus.h`, `src/core/ECS/MessageBus.cpp`

**Purpose:** Event system for inter-system communication

**Key Methods:**
```cpp
// Subscribe to messages
template<typename MessageType>
void Subscribe(std::function<void(const MessageType&)> handler);

// Publish messages
template<typename MessageType>
void Publish(const MessageType& message);

// Clear all handlers
void Clear();
```

**Message Structure:**
```cpp
template<typename T>
struct Message {
    T data;
    EntityID source;
    double timestamp;
};
```

**Usage:**
```cpp
// Define message type
struct PopulationChangeMessage {
    EntityID entity;
    int delta;
};

// Subscribe
message_bus.Subscribe<PopulationChangeMessage>([](const auto& msg) {
    // Handle message
});

// Publish
message_bus.Publish(PopulationChangeMessage{entity, 100});
```

---

### core::threading

#### ThreadedSystemManager
**Location:** `include/core/threading/ThreadedSystemManager.h`, `src/core/threading/ThreadedSystemManager.cpp`

**Purpose:** Manages system execution across threads

**Constructor:**
```cpp
ThreadedSystemManager(core::ecs::ComponentAccessManager* access_manager,
                     ThreadSafeMessageBus* message_bus);
```

**Key Methods:**
```cpp
// System management
template<typename SystemType, typename... Args>
SystemType* AddSystem(ThreadingStrategy strategy, Args&&... args);

void AddSystem(std::unique_ptr<game::core::ISystem> system, 
               ThreadingStrategy strategy);

template<typename SystemType>
SystemType* GetSystem();

// Lifecycle
void Initialize();
void StartSystems();
void Update(float delta_time);
void StopSystems();
void Shutdown();
```

**Threading Strategies:**
```cpp
enum class ThreadingStrategy {
    MAIN_THREAD,        // Execute on main thread only
    THREAD_POOL,        // Execute on worker thread pool
    DEDICATED_THREAD,   // Execute on dedicated thread
    BACKGROUND          // Execute on low-priority background thread
};
```

---

#### ThreadSafeMessageBus
**Location:** `include/core/threading/ThreadSafeMessageBus.h`

**Purpose:** Thread-safe message bus for cross-thread communication

**Constructor:**
```cpp
ThreadSafeMessageBus() = default;
```

**Key Methods:**
```cpp
// Thread-safe publish/subscribe
template<typename MessageType>
void Publish(const MessageType& message);

template<typename MessageType>
void Subscribe(std::function<void(const MessageType&)> handler);
```

---

### core::types

#### EntityID
**Location:** `include/core/ECS/EntityManager.h`

**Structure:**
```cpp
struct EntityID {
    uint64_t id;        // Unique identifier
    uint32_t version;   // Version for recycling detection
    
    // Operators
    bool operator==(const EntityID& other) const;
    bool operator!=(const EntityID& other) const;
};
```

**Usage:**
```cpp
EntityID entity = entity_manager->CreateEntity();
uint64_t raw_id = entity.id;  // Use .id member, NOT Get()
```

---

#### TypeRegistry
**Location:** `include/core/types/game_types.h`, `src/core/types/TypeRegistry.cpp`

**Purpose:** String-to-enum conversions (currently disabled due to mismatches)

**Key Methods (when enabled):**
```cpp
static std::string ThreadingStrategyToString(::core::threading::ThreadingStrategy type);
static std::string SystemTypeToString(SystemType type);
static std::string DecisionTypeToString(DecisionType type);
```

---

## Game Namespace

### game::core

#### ISystem Interface
**Location:** `include/core/ECS/ISystem.h`

**Purpose:** Base interface for all game systems

**Required Virtual Methods:**
```cpp
// Lifecycle
virtual void Initialize() = 0;
virtual void Update(float delta_time) = 0;
virtual void Shutdown() = 0;

// Threading
virtual ::core::threading::ThreadingStrategy GetThreadingStrategy() const = 0;

// Identification
virtual std::string GetSystemName() const = 0;

// Serialization
virtual Json::Value Serialize(int version) const = 0;
virtual bool Deserialize(const Json::Value& data, int version) = 0;
```

**Implementation Pattern:**
```cpp
class MySystem : public game::core::ISystem {
public:
    void Initialize() override { /* ... */ }
    void Update(float delta_time) override { /* ... */ }
    void Shutdown() override { /* ... */ }
    
    ::core::threading::ThreadingStrategy GetThreadingStrategy() const override {
        return ::core::threading::ThreadingStrategy::THREAD_POOL;
    }
    
    std::string GetSystemName() const override {
        return "MySystem";
    }
    
    Json::Value Serialize(int version) const override {
        Json::Value data;
        data["system_name"] = "MySystem";
        data["version"] = version;
        return data;
    }
    
    bool Deserialize(const Json::Value& data, int version) override {
        if (data["system_name"].asString() != "MySystem") {
            return false;
        }
        return true;
    }
};
```

---

### game::population

#### PopulationSystem
**Location:** `include/game/population/PopulationSystem.h`, `src/game/population/PopulationSystem.cpp`

**Constructor:**
```cpp
explicit PopulationSystem(::core::ecs::ComponentAccessManager& access_manager,
                         ::core::ecs::MessageBus& message_bus);
```

**Key Methods:**
```cpp
void Initialize() override;
void Update(float delta_time) override;
void Shutdown() override;

// Population management
void CreatePopulationComponents(game::types::EntityID entity_id);
void ProcessMonthlyUpdate(game::types::EntityID entity_id);

// Demographics
int GetTotalPopulation(game::types::EntityID entity_id) const;
double GetGrowthRate(game::types::EntityID entity_id) const;
std::vector<PopulationSegment> GetPopulationBreakdown(game::types::EntityID entity_id) const;
```

---

### game::technology

#### TechnologySystem
**Location:** `include/game/technology/TechnologySystem.h`, `src/game/technology/TechnologySystem.cpp`

**Constructor:**
```cpp
explicit TechnologySystem(::core::ecs::ComponentAccessManager& access_manager,
                         ::core::ecs::MessageBus& message_bus);
```

**Key Methods:**
```cpp
void Initialize() override;
void Update(float delta_time) override;
void Shutdown() override;

// Research management
bool StartResearch(game::types::EntityID entity_id, TechnologyID tech_id);
void CancelResearch(game::types::EntityID entity_id);
bool IsTechnologyResearched(game::types::EntityID entity_id, TechnologyID tech_id) const;
```

---

### game::economy

#### EconomicSystem
**Location:** `include/game/economy/EconomicSystem.h`, `src/game/economy/EconomicSystem.cpp`

**Constructor:**
```cpp
explicit EconomicSystem(::core::ecs::ComponentAccessManager& access_manager,
                       ::core::ecs::MessageBus& message_bus);
```

**Key Methods:**
```cpp
void Initialize() override;
void Update(float delta_time) override;
void Shutdown() override;

// Serialization (NEW - Added Oct 26, 2025)
std::string GetSystemName() const override;
Json::Value Serialize(int version) const override;
bool Deserialize(const Json::Value& data, int version) override;

// Treasury management
bool SpendMoney(game::types::EntityID entity_id, int amount);
void AddMoney(game::types::EntityID entity_id, int amount);
int GetTreasury(game::types::EntityID entity_id) const;
int GetMonthlyIncome(game::types::EntityID entity_id) const;
int GetMonthlyExpenses(game::types::EntityID entity_id) const;
```

---

### game::administration

#### AdministrativeSystem
**Location:** `include/game/administration/AdministrativeSystem.h`, `src/game/administration/AdministrativeSystem.cpp`

**Constructor:**
```cpp
explicit AdministrativeSystem(::core::ecs::ComponentAccessManager& access_manager,
                             ::core::ecs::MessageBus& message_bus);
```

**Key Methods:**
```cpp
void Initialize() override;
void Update(float delta_time) override;
void Shutdown() override;

// Serialization (NEW - Added Oct 26, 2025)
std::string GetSystemName() const override;
Json::Value Serialize(int version) const override;
bool Deserialize(const Json::Value& data, int version) override;

// Official management
bool AppointOfficial(game::types::EntityID entity_id, OfficialType type, const std::string& name);
bool DismissOfficial(game::types::EntityID entity_id, uint32_t official_id);
double GetAdministrativeEfficiency(game::types::EntityID entity_id) const;
```

---

### game::military

#### MilitarySystem
**Location:** `include/game/military/MilitarySystem.h`, `src/game/military/MilitarySystem.cpp`

**Constructor:**
```cpp
explicit MilitarySystem(::core::ecs::ComponentAccessManager& access_manager,
                       ::core::ecs::MessageBus& message_bus);
```

**Key Methods:**
```cpp
void Initialize() override;
void Update(float delta_time) override;
void Shutdown() override;

// Threading (NEW - Added Oct 26, 2025)
::core::threading::ThreadingStrategy GetThreadingStrategy() const override;

// Unit management
bool CreateUnit(game::types::EntityID entity_id, UnitType type);
void MoveUnit(UnitID unit_id, ProvinceID destination);
bool EngageCombat(UnitID attacker, UnitID defender);
```

---

#### MilitaryRecruitmentSystem
**Location:** `include/game/military/MilitaryRecruitmentSystem.h`, `src/game/military/MilitaryRecruitmentSystem.cpp`

**Constructor:**
```cpp
explicit MilitaryRecruitmentSystem(::core::ecs::ComponentAccessManager& access_manager,
                                  ::core::ecs::MessageBus& message_bus);
```

**Key Methods:**
```cpp
void Initialize() override;
void Update(float delta_time) override;
void Shutdown() override;

// Threading (NEW - Added Oct 26, 2025)
::core::threading::ThreadingStrategy GetThreadingStrategy() const override;

// Serialization (NEW - Added Oct 26, 2025)
std::string GetSystemName() const override;
Json::Value Serialize(int version) const override;
bool Deserialize(const Json::Value& data, int version) override;

// Recruitment
bool StartRecruitment(game::types::EntityID entity_id, UnitType type, int quantity);
void CancelRecruitment(game::types::EntityID entity_id);
int GetRecruitmentProgress(game::types::EntityID entity_id) const;
```

---

### game::diplomacy

#### DiplomacySystem
**Location:** `include/game/diplomacy/DiplomacySystem.h`, `src/game/diplomacy/DiplomacySystem.cpp`

**Constructor:**
```cpp
explicit DiplomacySystem(::core::ecs::ComponentAccessManager& access_manager,
                        ::core::ecs::MessageBus& message_bus);
```

**Key Methods:**
```cpp
void Initialize() override;
void Update(float delta_time) override;
void Shutdown() override;

// Diplomatic actions
bool ProposeAlliance(game::types::EntityID proposer, game::types::EntityID target);
bool DeclareWar(game::types::EntityID aggressor, game::types::EntityID target);
void SendDiplomaticMessage(game::types::EntityID sender, game::types::EntityID receiver, const std::string& message);
int GetRelationValue(game::types::EntityID entity1, game::types::EntityID entity2) const;
```

---

### game::time

#### TimeManagementSystem
**Location:** `include/game/time/TimeManagementSystem.h`, `src/game/time/TimeManagementSystem.cpp`

**Constructor (UPDATED Oct 26, 2025):**
```cpp
explicit TimeManagementSystem(::core::ecs::ComponentAccessManager& access_manager,
                             ::core::threading::ThreadSafeMessageBus& message_bus,
                             const GameDate& start_date = GameDate(1066, 10, 14));
```

**Key Methods:**
```cpp
void Initialize() override;
void Update(float delta_time) override;
void Shutdown() override;

// Time control
void Pause();
void Resume();
void SetTimeScale(TimeScale scale);
TimeScale GetTimeScale() const;
bool IsPaused() const;

// Date management
GameDate GetCurrentDate() const;
void SetCurrentDate(const GameDate& date);
```

---

### game::config

#### GameConfig
**Location:** `include/game/config/GameConfig.h`, `src/game/config/GameConfig.cpp`

**Purpose:** Singleton configuration manager

**Key Methods:**
```cpp
// Singleton access
static GameConfig& Instance();
static void Initialize(const std::string& config_dir);

// Configuration loading
bool LoadFromFile(const std::string& filepath);
bool SaveToFile(const std::string& filepath) const;
bool ValidateConfiguration() const;
std::vector<std::string> GetValidationErrors() const;

// Hot reload
void EnableHotReload(bool enable);
void ForceReloadConfiguration();
```

---

## Global Variables

### In main.cpp
**Location:** `apps/main.cpp`

```cpp
// Core ECS Foundation (UPDATED Oct 26, 2025)
static std::unique_ptr<core::ecs::EntityManager> g_entity_manager;
static std::unique_ptr<core::ecs::ComponentAccessManager> g_component_access_manager;
static std::unique_ptr<core::ecs::MessageBus> g_message_bus;
static std::unique_ptr<core::threading::ThreadSafeMessageBus> g_thread_safe_message_bus;
static std::unique_ptr<core::threading::ThreadedSystemManager> g_system_manager;

// Game Systems
static std::unique_ptr<game::population::PopulationSystem> g_population_system;
static std::unique_ptr<game::technology::TechnologySystem> g_technology_system;
static std::unique_ptr<game::economy::EconomicSystem> g_economic_system;
static std::unique_ptr<game::administration::AdministrativeSystem> g_administrative_system;
static std::unique_ptr<game::military::MilitarySystem> g_military_system;
static std::unique_ptr<game::military::MilitaryRecruitmentSystem> g_military_recruitment_system;
static std::unique_ptr<game::diplomacy::DiplomacySystem> g_diplomacy_system;
static std::unique_ptr<game::gameplay::GameplayCoordinator> g_gameplay_system;  // Currently disabled
static std::unique_ptr<game::time::TimeManagementSystem> g_time_system;

// Main realm entity
static core::ecs::EntityID g_main_realm_entity{};

// UI Systems
static ui::AdministrativeUI* g_administrative_ui = nullptr;
static ui::SimpleProvincePanel* g_province_panel = nullptr;
static ui::MainMenuUI* g_main_menu_ui = nullptr;
static ui::PopulationInfoWindow* g_population_window = nullptr;
static ui::TechnologyInfoWindow* g_technology_window = nullptr;
static ui::PerformanceWindow* g_performance_window = nullptr;

// Map Rendering
static std::unique_ptr<game::map::MapRenderer> g_map_renderer;

// Application state
static bool g_running = true;
static bool g_show_demo_window = false;
static bool g_show_performance_metrics = false;
```

---

## System Constructors

### Correct Constructor Signatures (Updated Oct 26, 2025)

```cpp
// Core ECS initialization order (CRITICAL!)
g_entity_manager = std::make_unique<core::ecs::EntityManager>();
g_message_bus = std::make_unique<core::ecs::MessageBus>();
g_thread_safe_message_bus = std::make_unique<core::threading::ThreadSafeMessageBus>();

g_component_access_manager = std::make_unique<core::ecs::ComponentAccessManager>(
    g_entity_manager.get(), 
    g_message_bus.get()
);

g_system_manager = std::make_unique<core::threading::ThreadedSystemManager>(
    g_component_access_manager.get(),
    g_thread_safe_message_bus.get()
);

// Game systems (all use ComponentAccessManager reference)
g_population_system = std::make_unique<game::population::PopulationSystem>(
    *g_component_access_manager, 
    *g_message_bus
);

g_technology_system = std::make_unique<game::technology::TechnologySystem>(
    *g_component_access_manager, 
    *g_message_bus
);

g_economic_system = std::make_unique<game::economy::EconomicSystem>(
    *g_component_access_manager, 
    *g_message_bus
);

g_administrative_system = std::make_unique<game::administration::AdministrativeSystem>(
    *g_component_access_manager, 
    *g_message_bus
);

g_military_system = std::make_unique<game::military::MilitarySystem>(
    *g_component_access_manager, 
    *g_message_bus
);

g_military_recruitment_system = std::make_unique<game::military::MilitaryRecruitmentSystem>(
    *g_component_access_manager, 
    *g_message_bus
);

g_diplomacy_system = std::make_unique<game::diplomacy::DiplomacySystem>(
    *g_component_access_manager, 
    *g_message_bus
);

// Time system requires 3 parameters
g_time_system = std::make_unique<game::time::TimeManagementSystem>(
    *g_component_access_manager,
    *g_thread_safe_message_bus,
    game::time::GameDate(1066, 10, 14)  // Starting date
);
```

---

## Common Patterns

### Pattern 1: Adding a Component

**CORRECT (Oct 26, 2025):**
```cpp
auto component = g_entity_manager->AddComponent<PopulationComponent>(entity.id);
if (component) {
    component->total_population = 10000;  // Use -> operator
    component->growth_rate = 0.01;
}
```

**INCORRECT (Old pattern):**
```cpp
auto& component = g_entity_manager->AddComponent<PopulationComponent>(entity.id);
component.total_population = 10000;  // ERROR: AddComponent returns shared_ptr, not reference!
```

---

### Pattern 2: Getting Entity ID

**CORRECT:**
```cpp
EntityID entity = g_entity_manager->CreateEntity();
uint64_t raw_id = entity.id;  // Use .id member
```

**INCORRECT:**
```cpp
uint64_t raw_id = entity.Get();  // ERROR: Get() doesn't exist!
```

---

### Pattern 3: Toast Notifications

**CORRECT:**
```cpp
std::string message = "Configuration valid";
ui::Toast::Show(message.c_str(), 3.0f);  // Convert to const char*

// Or for concatenated strings:
std::string error_msg = "Error: " + std::string(e.what());
ui::Toast::Show(error_msg.c_str(), 5.0f);
```

**INCORRECT:**
```cpp
std::string message = "Configuration valid";
ui::Toast::Show(message, 3.0f);  // ERROR: Toast::Show takes const char*, not std::string
```

---

### Pattern 4: Message Publishing

**CORRECT:**
```cpp
// Template-based message type
struct PopulationChangeMessage {
    EntityID entity;
    int delta;
};

g_message_bus->Publish(PopulationChangeMessage{entity, 100});
```

**INCORRECT:**
```cpp
// Old struct with type field
struct Message {
    MessageType type;  // ERROR: Message is a template, not a struct with type field
    EntityID entity;
};
```

---

### Pattern 5: System Serialization

**Implementation Template:**
```cpp
// In header file
class MySystem : public game::core::ISystem {
public:
    std::string GetSystemName() const override;
    Json::Value Serialize(int version) const override;
    bool Deserialize(const Json::Value& data, int version) override;
};

// In cpp file (must include <json/json.h>)
#include <json/json.h>

std::string MySystem::GetSystemName() const {
    return "MySystem";
}

Json::Value MySystem::Serialize(int version) const {
    Json::Value data;
    data["system_name"] = "MySystem";
    data["version"] = version;
    data["initialized"] = m_initialized;
    // Add system-specific state here
    return data;
}

bool MySystem::Deserialize(const Json::Value& data, int version) {
    if (data["system_name"].asString() != "MySystem") {
        return false;
    }
    m_initialized = data["initialized"].asBool();
    // Load system-specific state here
    return true;
}
```

---

## Namespace Hierarchy

```
::core
  ::ecs
    - EntityManager
    - ComponentAccessManager
    - MessageBus
  ::threading
    - ThreadedSystemManager
    - ThreadSafeMessageBus
    - ThreadingStrategy (enum)
  ::types
    - TypeRegistry
    - EntityID
  ::logging
    - LogInfo()
    - LogError()
    - LogWarning()
  ::save
    - SaveManager

::game
  ::core
    - ISystem (interface)
    - ISerializable (interface)
  ::population
    - PopulationSystem
    - PopulationComponent
  ::technology
    - TechnologySystem
    - TechnologyComponent
  ::economy
    - EconomicSystem
    - EconomicComponent
  ::administration
    - AdministrativeSystem
    - GovernanceComponent
  ::military
    - MilitarySystem
    - MilitaryRecruitmentSystem
    - MilitaryComponent
  ::diplomacy
    - DiplomacySystem
    - DiplomaticRelations
  ::time
    - TimeManagementSystem
    - GameDate
  ::config
    - GameConfig (singleton)
    - ConfigHelpers
  ::gameplay
    - GameplayCoordinator (currently disabled)
    - DecisionConsequenceSystem

::ui
  - Toast
  - PopulationInfoWindow
  - TechnologyInfoWindow
  - AdministrativeUI
  - etc.
```

---

## Function Signatures Quick Reference

### Logging (Free Functions)
```cpp
void ::core::logging::LogInfo(const std::string& system, const std::string& message);
void ::core::logging::LogError(const std::string& system, const std::string& message);
void ::core::logging::LogWarning(const std::string& system, const std::string& message);
```

### Component Access
```cpp
// EntityManager (returns shared_ptr)
template<typename T, typename... Args>
std::shared_ptr<T> AddComponent(uint64_t entity_id, Args&&... args);

template<typename T>
std::shared_ptr<T> GetComponent(EntityID entity);

// ComponentAccessManager (thread-safe wrapper)
template<typename T>
std::shared_ptr<T> GetComponent(EntityID entity);

template<typename T>
std::vector<std::shared_ptr<T>> GetAllComponents();
```

### Toast Notifications
```cpp
static void ui::Toast::Show(const char* message, float duration);
```

---

## Migration Guide (Oct 22 → Oct 26, 2025)

### If You Have Old Code Using EntityManager

**Old Pattern:**
```cpp
g_system = std::make_unique<MySystem>(*g_entity_manager, *g_message_bus);
```

**New Pattern:**
```cpp
g_system = std::make_unique<MySystem>(*g_component_access_manager, *g_message_bus);
```

### If You Were Using AddComponent with References

**Old Pattern:**
```cpp
auto& comp = manager->AddComponent<T>(entity);
comp.value = 42;
```

**New Pattern:**
```cpp
auto comp = manager->AddComponent<T>(entity);
comp->value = 42;  // Use -> operator
```

### If You Had Custom Systems Without Serialization

**Old Pattern:**
```cpp
class MySystem : public game::core::ISystem {
    // Only lifecycle methods
};
```

**New Pattern:**
```cpp
class MySystem : public game::core::ISystem {
public:
    // Add these three methods:
    std::string GetSystemName() const override { return "MySystem"; }
    
    Json::Value Serialize(int version) const override {
        Json::Value data;
        data["system_name"] = "MySystem";
        return data;
    }
    
    bool Deserialize(const Json::Value& data, int version) override {
        return data["system_name"].asString() == "MySystem";
    }
};

// And in the .cpp file, add:
#include <json/json.h>
```

---

**For Implementation Examples:**
- See `src/game/economy/EconomicSystem.cpp` for serialization pattern
- See `src/game/military/MilitarySystem.cpp` for threading pattern
- See `apps/main.cpp` lines 230-290 for initialization pattern

**For Testing:**
- Windows: `cmake --build --preset windows-vs-release`
- Run: `build\windows-vs-release\bin\mechanica_imperii.exe`
