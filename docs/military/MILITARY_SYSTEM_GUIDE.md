# Military System - Complete Guide

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Usage Examples](#usage-examples)
4. [Configuration](#configuration)
5. [Performance Considerations](#performance-considerations)
6. [Thread Safety](#thread-safety)
7. [Best Practices](#best-practices)

---

## Overview

The Military System is a comprehensive ECS-based system for managing all military operations in the game, including:
- Unit recruitment and management
- Army creation and movement
- Land and naval combat
- Sieges and fortifications
- Military economy integration

### Key Features
- ✅ Thread-safe operations
- ✅ Data-driven unit definitions (JSON)
- ✅ Configurable balancing parameters
- ✅ Automatic registry cleanup
- ✅ Full ECS integration
- ✅ Event-driven architecture

---

## Architecture

### Component Structure

```
MilitaryComponent        - Provincial military forces (garrisons, infrastructure)
ArmyComponent           - Mobile armies (units, location, status)
FortificationComponent  - Defensive structures
CombatComponent         - Active battle state
MilitaryEventsComponent - Battle history and events
```

### System Responsibilities

```
MilitarySystem              - Core military operations
MilitaryRecruitmentSystem   - Unit recruitment from population
BattleResolutionCalculator  - Combat resolution (pure functions)
NavalCombatCalculator       - Naval battle resolution
FleetManagementSystem       - Naval fleet operations
MilitaryCampaignManager     - FOW, orders, delays, news
```

---

## Usage Examples

### Basic Unit Recruitment

```cpp
#include "game/military/MilitarySystem.h"

// Get the military system
auto* military_system = GetSystem<MilitarySystem>();

// Recruit units for a province
types::EntityID province_id = 42;
UnitType unit_type = UnitType::SPEARMEN;
uint32_t quantity = 100;

if (military_system->RecruitUnit(province_id, unit_type, quantity)) {
    // Recruitment successful
    CORE_LOG_INFO("Game", "Recruited 100 spearmen");
} else {
    // Check why it failed (budget, capacity, etc.)
}
```

### Creating and Managing Armies

```cpp
// Create a new army
types::EntityID home_province = 42;
std::string army_name = "1st Legion";

types::EntityID army_id = military_system->CreateArmy(army_name, home_province);

if (army_id != 0) {
    CORE_LOG_INFO("Game", "Created army: " + army_name);

    // Get all armies
    auto all_armies = military_system->GetAllArmies();
    CORE_LOG_INFO("Game", "Total armies: " + std::to_string(all_armies.size()));
}
```

### Initiating Combat

```cpp
// Initiate battle between two armies
types::EntityID attacker_army = 1;
types::EntityID defender_army = 2;

military_system->InitiateBattle(attacker_army, defender_army);

// System will:
// 1. Create CombatComponent
// 2. Calculate strengths
// 3. Resolve battle
// 4. Apply casualties
// 5. Publish battle_resolved event
```

### Starting a Siege

```cpp
types::EntityID army_id = 1;
types::EntityID target_province = 42;

military_system->BeginSiege(army_id, target_province);

// Later, resolve the siege
bool attacker_wins = true; // Based on siege progress
military_system->ResolveSiege(target_province, attacker_wins);
```

### Upgrading Military Infrastructure

```cpp
// Upgrade training facilities
types::EntityID province_id = 42;
double investment = 1000.0; // Gold

military_system->UpgradeTrainingFacilities(province_id, investment);

// Improve equipment for a unit type
UnitType unit_type = UnitType::HEAVY_CAVALRY;
military_system->ImproveEquipment(province_id, unit_type, investment);

// Construct fortifications
// 0 = walls, 1 = towers, 2 = gates, 3 = citadel
int fortification_type = 0;
double cost = 500.0;
military_system->ConstructFortifications(province_id, fortification_type, cost);
```

---

## Configuration

### Loading Unit Definitions

Unit stats are loaded from `data/definitions/units.json`:

```json
{
  "version": "1.0",
  "units": [
    {
      "id": 0,
      "type": "LEVIES",
      "class": "INFANTRY",
      "role": "MELEE",
      "name": "Levies",
      "max_strength": 1000,
      "attack_strength": 5.0,
      "defense_strength": 4.0,
      "recruitment_cost": 50.0,
      "monthly_maintenance": 2.0,
      "equipment_requirements": {
        "FOOD": 10,
        "WOOD": 5,
        "IRON": 2
      }
    }
  ]
}
```

### System Configuration

Configuration constants are defined in `MilitarySystem.cpp`:

```cpp
namespace config {
    constexpr double DEFAULT_MILITARY_BUDGET = 1000.0;
    constexpr uint32_t DEFAULT_RECRUITMENT_CAPACITY = 100;
    constexpr double UNIT_BASE_COST = 50.0;
    constexpr double SIEGE_SUCCESS_DAMAGE = 0.3;
    // ... more constants
}
```

**Future**: Load from `data/config/military_config.json` using `MilitaryConfigManager`.

---

## Performance Considerations

### Thread Safety

All military operations are thread-safe:

```cpp
// Thread-safe component access
std::lock_guard<std::mutex> lock(military_comp->garrison_mutex);
// ... access garrison_units safely
```

**Mutexes Used:**
- `garrison_mutex` - Protects garrison unit list
- `battles_mutex` - Protects battle state
- `units_mutex` - Protects army unit list
- `m_active_battles_mutex` - Protects active battles list
- `m_armies_registry_mutex` - Protects army registry

### Registry Cleanup

Armies are automatically cleaned from the registry every 100 updates:

```cpp
// In Update():
m_update_counter++;
if (m_update_counter >= 100) {
    PerformArmyRegistryCleanup();
    m_update_counter = 0;
}
```

**Cleanup removes:**
- Disbanded armies (is_active = false)
- Deleted army entities

### Performance Characteristics

| Operation | Complexity | Notes |
|-----------|------------|-------|
| RecruitUnit | O(1) | Direct component access |
| CreateArmy | O(1) | Entity creation + registry insert |
| GetAllArmies | O(n) | Filters active armies, n = total armies |
| InitiateBattle | O(m) | m = units in both armies |
| ApplyCasualties | O(u) | u = units in army |
| PerformArmyRegistryCleanup | O(n) | n = armies in registry |

### Optimization Tips

1. **Batch Operations**: Group multiple recruitments together
2. **Registry Size**: Typical game has <100 armies, cleanup is fast
3. **Lock Contention**: Minimize time holding locks
4. **Event Filtering**: Subscribe only to needed military events

### Profiling Hooks

```cpp
// Add timing for performance-critical operations
auto start = std::chrono::high_resolution_clock::now();

// ... military operation ...

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

if (duration.count() > 1000) { // >1ms
    CORE_LOG_WARN("Performance",
        "Slow operation: " + std::to_string(duration.count()) + "μs");
}
```

---

## Thread Safety

### Safe Patterns

**✅ Correct: Lock then access**
```cpp
{
    std::lock_guard<std::mutex> lock(army.units_mutex);
    for (auto& unit : army.units) {
        // Safe access
    }
} // Lock released
```

**✅ Correct: Use provided accessors**
```cpp
// Thread-safe access by index
MilitaryUnit* unit = military_comp->GetUnitAt(index);
if (unit) {
    // Use unit safely
}
```

**❌ Wrong: Access without locking**
```cpp
// DANGEROUS - No lock!
for (auto& unit : army.units) {
    // Race condition!
}
```

**❌ Wrong: Holding lock too long**
```cpp
std::lock_guard<std::mutex> lock(army.units_mutex);
// ... lots of expensive operations ...
// Blocks other threads unnecessarily
```

### Copy Constructors

Components have thread-safe copy constructors:

```cpp
ArmyComponent copy(original); // Locks original, copies safely
```

### Morale Boundary Safety

Morale changes are protected against enum underflow:

```cpp
int current_morale_state = static_cast<int>(morale);
if (current_morale_state > static_cast<int>(MoraleState::ROUTING)) {
    morale = static_cast<MoraleState>(current_morale_state - 1);
} else {
    morale = MoraleState::ROUTING; // Clamp at minimum
}
```

---

## Best Practices

### 1. Always Check Return Values

```cpp
if (military_system->RecruitUnit(province_id, unit_type, quantity)) {
    // Success
} else {
    // Handle failure - check budget, capacity, etc.
    CORE_LOG_WARN("Game", "Recruitment failed");
}
```

### 2. Use Named Constants

```cpp
// ✅ Good
double cost = quantity * config::UNIT_BASE_COST;

// ❌ Bad
double cost = quantity * 50.0; // Magic number!
```

### 3. Subscribe to Military Events

```cpp
message_bus.Subscribe("military.battle_resolved",
    [](const Json::Value& event) {
        types::EntityID battle_id = event["battle_id"].asInt();
        std::string outcome = event["outcome"].asString();
        // Update UI, AI, etc.
    });
```

**Available Events:**
- `military.unit_recruited`
- `military.army_created`
- `military.battle_initiated`
- `military.battle_resolved`
- `military.siege_begun`
- `military.siege_resolved`

### 4. Clean Data Management

```cpp
// Disband armies when no longer needed
if (army_is_obsolete) {
    military_system->DisbandArmy(army_id);
    // Registry cleanup happens automatically
}
```

### 5. Configuration Management

```json
// Edit units.json for balance changes
{
  "type": "HEAVY_CAVALRY",
  "recruitment_cost": 500.0,  // Increase cost
  "attack_strength": 25.0,     // Buff attack
  "monthly_maintenance": 50.0  // Higher upkeep
}
// No code changes needed!
```

### 6. Error Handling

```cpp
auto* military_comp = GetMilitaryComponent(province_id);
if (!military_comp) {
    CORE_LOG_ERROR("Game", "Province has no military component");
    return;
}

if (military_comp->military_budget < cost) {
    CORE_LOG_WARN("Game", "Insufficient budget");
    // Show UI warning, etc.
    return;
}
```

---

## Testing

Comprehensive test suite in `tests/test_military_system_fixes.cpp`:

```cpp
// Thread safety test
TEST_F(MilitarySystemFixesTest, ConcurrentArmyModification) {
    // Multiple threads adding units simultaneously
    // Verifies no race conditions
}

// Boundary condition test
TEST_F(MilitarySystemFixesTest, MoraleEnumUnderflowProtection) {
    // Ensures morale can't go below ROUTING
}

// Casualty distribution test
TEST_F(MilitarySystemFixesTest, CasualtyDistributionProportional) {
    // Verifies casualties distributed correctly
}
```

---

## Troubleshooting

### Common Issues

**Issue**: Recruitment fails
- Check military budget: `military_comp->military_budget`
- Check recruitment capacity: `military_comp->recruitment_capacity`
- Check unit type availability: `unit_type_available[unit_type]`

**Issue**: GetAllArmies() returns empty
- Armies must be created with `CreateArmy()`
- Check armies are active: `army->is_active == true`
- Registry cleanup may have removed disbanded armies

**Issue**: Battle not resolving
- Check both armies have units: `army->units.size() > 0`
- Check armies have strength: `army->total_strength > 0`
- Verify combat component exists

**Issue**: Thread crashes
- Always use locks when accessing shared data
- Use provided accessor methods (GetUnitAt, etc.)
- Never return pointers to vector elements

---

## Future Enhancements

- [ ] Load config from `military_config.json`
- [ ] Technology tree from JSON
- [ ] Advanced AI command system
- [ ] Battle replay/visualization
- [ ] Multiplayer synchronization
- [ ] Save/load optimization

---

## References

- **Architecture**: See `docs/MILITARY_CAMPAIGN_INTEGRATION.md`
- **Unit Data**: See `data/definitions/units.json`
- **Tests**: See `tests/test_military_system_fixes.cpp`
- **Components**: See `include/game/military/MilitaryComponents.h`
- **System**: See `include/game/military/MilitarySystem.h`

---

*Last Updated: 2025-11-20*
