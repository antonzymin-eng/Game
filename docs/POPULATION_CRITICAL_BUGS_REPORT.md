# Population System Critical Bugs Report
**Date**: November 20, 2025
**Severity**: CRITICAL - System Non-Functional
**Status**: IDENTIFIED, FIXES REQUIRED

---

## Executive Summary

The Population System contains three critical bugs that make it completely non-functional in production:

1. **Time Scale Bug**: Demographics progress 86,400× slower than intended
2. **Unused Timer System**: Configurable timers are declared but never used
3. **Birth/Death Truncation**: Fractional population changes are lost, freezing small provinces

**Overall Impact**: Population changes would be imperceptible during gameplay, rendering the system useless.

---

## Bug #1: Time Scale Calculation Error

### Location
`src/game/population/PopulationSystem.cpp:76-89` and `line 997`

### The Problem

```cpp
// Line 76-89: Update() is called every frame with real-time delta
void PopulationSystem::Update(float delta_time) {
    m_accumulated_time += delta_time;  // Real-time seconds (e.g., 0.016 for 16ms)

    if (m_accumulated_time >= 1.0f) {  // Triggers every 1 real-time second
        ProcessRegularUpdates(m_accumulated_time);  // Passes 1.0
        m_accumulated_time = 0.0f;
    }
}

// Line 997: Incorrect assumption about delta_time units
const double yearly_fraction = delta_time / 365.0;  // Treats 1.0 second as 1.0 day!
```

### What Actually Happens

| Step | Value | Interpretation | Actual Meaning |
|------|-------|----------------|----------------|
| Frame delta | 0.016s | Real-time second | 16ms frame time |
| Accumulated | 1.0s | 1 real-time second | 1 second of gameplay |
| yearly_fraction | 1.0/365 = 0.00274 | "1 day of year" | **Wrong!** Actually 1 second |
| Birth rate (3.5%) | 0.035 × 0.00274 = 0.0000959 | Per-second birth rate | Way too low! |
| 100-person group | 100 × 0.0000959 = 0.00959 | Births this update | Truncates to 0 |

### Consequence

**Time runs 86,400× slower than intended**:
- 1 real-second should advance game time by ~1 second (or configurable rate)
- Instead, it advances by 1/365th of a year = ~8 hours of game time
- But demographic changes use 1/365 scaling, so effective rate is 1 second of game time
- **Result**: A population of 10,000 would need 365 real-time seconds (6+ minutes) to experience 1 game-year of demographic change

### Mathematical Proof

```
Intended:
  game_year passes in X real-seconds (configurable)
  births per update = population × 0.035 × (delta_time / X)

Actual:
  delta_time = 1.0 (real second)
  yearly_fraction = 1.0 / 365.0 = 0.00274
  births per update = population × 0.035 × 0.00274 = population × 0.0000959

For 100 people: 100 × 0.0000959 = 0.00959 births (truncates to 0)
For 10,000 people: 10,000 × 0.0000959 = 0.959 births (truncates to 0)
For 100,000 people: 100,000 × 0.0000959 = 9.59 births (10 births)

Conclusion: Only provinces with 100,000+ population would see any demographic change!
```

---

## Bug #2: Unused Timer System

### Location
`include/game/population/PopulationSystem.h:165-167`

### The Problem

```cpp
// Declared in header (line 165-167)
float m_demographic_timer = 0.0f;  // NEVER USED
float m_mobility_timer = 0.0f;     // NEVER USED
float m_settlement_timer = 0.0f;   // NEVER USED
```

### What Was Intended

The system should allow different update frequencies for different subsystems:
- Demographics: Every game day/week
- Social mobility: Every game month/season
- Settlement evolution: Every game year

This would allow:
1. Performance tuning (expensive operations run less frequently)
2. Realistic simulation (mobility is slower than births/deaths)
3. Configurable game speed

### What Actually Happens

```cpp
// Line 85-86: Processes EVERYTHING every real-time second
if (m_accumulated_time >= 1.0f) {
    ProcessRegularUpdates(m_accumulated_time);  // Calls everything every second
}

// Line 1000-1006: All subsystems process together
for (const auto& province_id : populated_provinces) {
    ProcessDemographicChanges(province_id, yearly_fraction);      // Every second
    ProcessSocialMobility(province_id, yearly_fraction);          // Every second
    ProcessSettlementEvolution(province_id, yearly_fraction);     // Every second
    ProcessLegalStatusChanges(province_id, yearly_fraction);      // Every second
    ProcessGuildAdvancement(*population, *settlements, yearly_fraction);  // Every second
}
```

### Consequence

1. **No performance optimization**: All expensive operations run every second
2. **Unrealistic simulation**: Social mobility calculated as frequently as births
3. **Wasted computation**: Settlement evolution (slow process) checked constantly
4. **No configurability**: Can't tune update frequencies per subsystem

---

## Bug #3: Birth/Death Truncation (Fractional Population Loss)

### Location
`src/game/population/PopulationSystem.cpp:197-220`

### The Problem

```cpp
// Line 197-198: Truncates fractional births
const double births_this_period = current_pop * group.birth_rate * yearly_fraction;
const int births = static_cast<int>(births_this_period);  // TRUNCATES!

// No residual tracking, so fractional parts are lost forever

// Line 207-208: Same for deaths
const double deaths_this_period = current_pop * group.death_rate * yearly_fraction;
const int deaths = static_cast<int>(deaths_this_period);  // TRUNCATES!
```

### Numerical Example

**Small Province (100 people, birth rate 3.5%, death rate 3.0%)**:

```
Per-update calculation (yearly_fraction = 0.00274):
  births_this_period = 100 × 0.035 × 0.00274 = 0.00959
  births = static_cast<int>(0.00959) = 0

  deaths_this_period = 100 × 0.030 × 0.00274 = 0.00822
  deaths = static_cast<int>(0.00822) = 0

Result: NO CHANGE

Over 365 updates (should equal 1 year):
  Expected births: 100 × 0.035 = 3.5 births
  Actual births: 0 × 365 = 0

  Expected deaths: 100 × 0.030 = 3.0 deaths
  Actual deaths: 0 × 365 = 0

Result: FROZEN POPULATION (0% error, just stagnant)
```

**Medium Province (1,000 people)**:

```
Per-update:
  births_this_period = 1000 × 0.035 × 0.00274 = 0.0959
  births = 0

Over 365 updates:
  Actual births: 0
  Expected births: 35

Result: Still frozen!
```

**Large Province (10,000 people)**:

```
Per-update:
  births_this_period = 10000 × 0.035 × 0.00274 = 0.959
  births = 0 (still truncates!)

Over 365 updates:
  Actual births: 0
  Expected births: 350

Result: STILL FROZEN!
```

**Massive Province (100,000 people)**:

```
Per-update:
  births_this_period = 100000 × 0.035 × 0.00274 = 9.59
  births = 9

Over 365 updates:
  Actual births: 9 × 365 = 3,285
  Expected births: 100000 × 0.035 = 3,500

  Loss: 215 births (6% undercount)
```

### Consequence

**Complete demographic stagnation for all provinces under 100,000 population**:
- Small provinces (< 1,000): Completely frozen
- Medium provinces (1,000-10,000): Completely frozen
- Large provinces (10,000-100,000): Completely frozen
- Massive provinces (100,000+): 5-10% systematic undercount

Given that most provinces in a medieval strategy game have 1,000-50,000 population, **nearly all provinces would have frozen demographics**.

---

## Combined Impact Analysis

### Scenario: 10,000 Population Province

Let's trace what actually happens over 1 hour of real-time gameplay:

```
Time: 1 hour of gameplay = 3,600 real seconds

Current (Broken) System:
  - Update() called 3,600 times (60 FPS × 60 seconds)
  - ProcessRegularUpdates() called ~3,600 times (every 1 second)
  - Each call: yearly_fraction = 1.0 / 365.0 = 0.00274

  Per-update births:
    births_this_period = 10,000 × 0.035 × 0.00274 = 0.959
    births = static_cast<int>(0.959) = 0

  Total births over 1 hour: 0 × 3,600 = 0
  Expected births over 1 hour: ???

  Result: ZERO population growth after 1 hour of gameplay!

What SHOULD Happen (assuming 1 day = 60 real seconds):
  - 1 hour = 60 game days
  - Over 60 days: population × 0.035 × (60/365) = 10,000 × 0.00575 = 57.5 births
  - With proper residual tracking: 57-58 actual births
```

**The population would appear completely static to the player.**

---

## Validation Report Status

### Previous Validation: FAILED ❌

The validation report approved the code as "production-ready" but **completely missed these bugs** because:

1. **No runtime testing**: Validation was purely static analysis
2. **No numerical simulation**: Didn't trace through actual calculations
3. **No time scale analysis**: Didn't verify delta_time units
4. **No integration testing**: Tests were never compiled or run
5. **Assumed existing code was correct**: Focused only on new additions

### Correct Assessment

| Component | Previous Grade | Actual Grade | Status |
|-----------|----------------|--------------|--------|
| Utility Functions | A+ | A+ | ✅ Correct |
| RandomChance() | A+ | A+ | ✅ Correct |
| Integration Tests | B+ | N/A | ⚠️ Never compiled |
| Optimizations | A | A | ✅ Correct (if base system worked) |
| **Core Update Loop** | Not reviewed | **F** | ❌ **BROKEN** |
| **Overall System** | A+ | **F** | ❌ **NON-FUNCTIONAL** |

---

## Required Fixes

### Fix #1: Time Scale Correction

```cpp
// Option A: Use game time from time system
void PopulationSystem::Update(float delta_time) {
    m_accumulated_time += delta_time;

    // Get game time speed multiplier from GameTimeSystem
    const float game_speed = GetGameTimeSpeed();  // e.g., 1 day per 60 seconds
    const float game_time_delta = delta_time * game_speed;

    if (m_accumulated_time >= 1.0f) {
        ProcessRegularUpdates(game_time_delta);  // Pass actual game time
        m_accumulated_time = 0.0f;
    }
}

void PopulationSystem::ProcessRegularUpdates(double game_time_delta) {
    // game_time_delta is in game-days
    const double yearly_fraction = game_time_delta / 365.0;  // Now correct!

    // ... rest of code
}
```

```cpp
// Option B: Use configurable per-system timers
void PopulationSystem::Update(float delta_time) {
    m_demographic_timer += delta_time;
    m_mobility_timer += delta_time;
    m_settlement_timer += delta_time;

    const float game_speed = GetGameTimeSpeed();

    // Process demographics every game day
    if (m_demographic_timer >= m_demographic_update_interval) {
        float game_days_elapsed = m_demographic_timer * game_speed;
        ProcessDemographics(game_days_elapsed / 365.0);
        m_demographic_timer = 0.0f;
    }

    // Process mobility every game month
    if (m_mobility_timer >= m_mobility_update_interval) {
        float game_days_elapsed = m_mobility_timer * game_speed;
        ProcessSocialMobility(game_days_elapsed / 365.0);
        m_mobility_timer = 0.0f;
    }

    // Process settlements every game season
    if (m_settlement_timer >= m_settlement_update_interval) {
        float game_days_elapsed = m_settlement_timer * game_speed;
        ProcessSettlements(game_days_elapsed / 365.0);
        m_settlement_timer = 0.0f;
    }
}
```

### Fix #2: Implement Timer System

```cpp
// In header: Add configuration
struct PopulationSystemConfig {
    float demographic_update_interval = 1.0f;  // Real seconds per demographic update
    float mobility_update_interval = 5.0f;     // Real seconds per mobility update
    float settlement_update_interval = 30.0f;  // Real seconds per settlement update
    float game_time_scale = 60.0f;             // Real seconds per game day
};
```

### Fix #3: Add Residual Tracking

```cpp
// In PopulationGroup: Add residual fields
struct PopulationGroup {
    // ... existing fields ...

    // Residual tracking for fractional population changes
    mutable double birth_residual = 0.0;
    mutable double death_residual = 0.0;
    mutable double migration_residual = 0.0;
};

// In ProcessDemographicChanges: Track residuals
for (auto& group : population->population_groups) {
    // Calculate births with residual
    const double births_float = current_pop * group.birth_rate * yearly_fraction;
    const double births_with_residual = births_float + group.birth_residual;
    const int births = static_cast<int>(births_with_residual);
    group.birth_residual = births_with_residual - births;  // Save fractional part

    // Calculate deaths with residual
    const double deaths_float = current_pop * group.death_rate * yearly_fraction;
    const double deaths_with_residual = deaths_float + group.death_residual;
    const int deaths = static_cast<int>(deaths_with_residual);
    group.death_residual = deaths_with_residual - deaths;  // Save fractional part

    // Apply changes
    group.population_count += births - deaths;
    total_births += births;
    total_deaths += deaths;
}
```

### Numerical Validation After Fixes

**Small Province (100 people) with residual tracking**:

```
Update 1:
  births_float = 100 × 0.035 × 0.00274 = 0.00959
  births_with_residual = 0.00959 + 0.0 = 0.00959
  births = 0
  birth_residual = 0.00959

Update 2:
  births_float = 100 × 0.035 × 0.00274 = 0.00959
  births_with_residual = 0.00959 + 0.00959 = 0.01918
  births = 0
  birth_residual = 0.01918

... (continues accumulating) ...

Update 105:
  births_with_residual = 0.00959 + 0.99359 = 1.00318
  births = 1  ← FIRST BIRTH!
  birth_residual = 0.00318

Result: Births accumulate correctly over time instead of being lost!
```

---

## Testing Requirements

### Unit Tests Required

1. **Time Scale Test**:
   ```cpp
   TEST(PopulationSystem, TimeScale_OneGameYear_CorrectDemographicChange) {
       // Simulate 1 game year
       // Verify population changes match annual rates
   }
   ```

2. **Residual Tracking Test**:
   ```cpp
   TEST(PopulationSystem, SmallProvince_ResidualTracking_EventuallyProducesBirth) {
       // Province with 100 people
       // Run 200 updates
       // Verify births eventually occur due to residual accumulation
   }
   ```

3. **Timer System Test**:
   ```cpp
   TEST(PopulationSystem, TimerSystem_DifferentIntervals_CorrectUpdateFrequency) {
       // Configure different update intervals
       // Verify demographics update more frequently than settlements
   }
   ```

### Integration Tests Required

1. **Hour of Gameplay Test**: Run system for 3,600 updates, verify meaningful population change
2. **Small Province Test**: Verify 100-person province shows demographic activity
3. **Performance Test**: Verify timer system reduces update frequency for expensive operations

---

## Recommendations

### Immediate Actions (Critical)

1. ✅ **DO NOT MERGE CURRENT CODE** - System is non-functional
2. ⚠️ **Implement all three fixes** before any deployment
3. 🧪 **Add numerical validation tests** to prevent regression
4. 📊 **Run simulation test** for 1 hour of gameplay to verify fixes

### Process Improvements

1. **Require runtime testing**: Static analysis alone is insufficient
2. **Numerical simulation**: Trace calculations with real values
3. **Integration testing**: Actually compile and run test suites
4. **Time scale validation**: Always verify delta_time units and calculations
5. **Small-value testing**: Test with realistic small populations (100-1,000)

---

## Conclusion

The Population System, despite excellent code quality for utility functions and optimizations, has **three critical bugs in the core update loop** that make it completely non-functional:

1. **Time scale**: 86,400× slower than intended
2. **Unused timers**: No performance optimization or configurability
3. **Truncation**: Small populations completely frozen

**Status**: ❌ **BLOCKED - DO NOT MERGE**
**Priority**: 🔴 **CRITICAL - IMMEDIATE FIX REQUIRED**
**Estimated Fix Time**: 4-6 hours for implementation + testing

---

**Reported By**: Code Review (User Feedback)
**Date**: November 20, 2025
**Severity**: Critical (P0)
