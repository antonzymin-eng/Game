# Threading Fixes - Implementation Checklist

**Branch:** claude/fix-critical-threading-issues-011CUzi7nhUEHd8XzPPQJB21
**Developer(s):** _________________
**Start Date:** _________________
**Target Date:** _________________

---

## Week 1: Critical Fixes

### Day 1-2: DiplomacySystem Strategy Fix 🔴

#### Implementation
- [ ] Open `src/game/diplomacy/DiplomacySystem.cpp`
- [ ] Navigate to line 79 (GetThreadingStrategy method)
- [ ] Change `BACKGROUND_THREAD` to `MAIN_THREAD`
- [ ] Save file

**Code Change:**
```cpp
// Line 79
::core::threading::ThreadingStrategy DiplomacySystem::GetThreadingStrategy() const {
    return ::core::threading::ThreadingStrategy::MAIN_THREAD;  // Changed from BACKGROUND_THREAD
}
```

#### Testing
- [ ] Compile project (`make -j8`)
- [ ] Run basic game test (create world, run 100 game months)
- [ ] Verify diplomacy events still work
- [ ] Check performance: diplomacy update should be < 5ms
- [ ] Run existing diplomacy tests: `ctest -R diplomacy`

#### Success Criteria
- [ ] Game runs without crashes
- [ ] Diplomacy actions complete successfully
- [ ] No frame rate drops
- [ ] All diplomacy tests pass

---

### Day 3-5: MessageBus Migration 🔴

#### Step 1: Update System Headers

**File:** `include/game/diplomacy/DiplomacySystem.h`
- [ ] Line 10: Change `#include "core/ECS/MessageBus.h"` to `#include "core/threading/ThreadSafeMessageBus.h"`
- [ ] Line 43: Change constructor parameter `::core::ecs::MessageBus&` to `::core::threading::ThreadSafeMessageBus&`
- [ ] Line 126: Change member variable `::core::ecs::MessageBus&` to `::core::threading::ThreadSafeMessageBus&`

**File:** `include/game/economy/EconomicSystem.h`
- [ ] Line 10: Change include to ThreadSafeMessageBus.h
- [ ] Line 56: Change constructor parameter to ThreadSafeMessageBus&
- [ ] Update member variable declaration

**File:** `include/game/military/MilitarySystem.h`
- [ ] Line 12: Change include to ThreadSafeMessageBus.h
- [ ] Line 38: Change member variable to ThreadSafeMessageBus&
- [ ] Line 42: Change constructor parameter to ThreadSafeMessageBus&

**File:** `include/game/trade/TradeSystem.h`
- [ ] Update include statement
- [ ] Update constructor parameter
- [ ] Update member variable

**File:** `include/game/administration/AdministrativeSystem.h`
- [ ] Update include statement
- [ ] Update constructor parameter
- [ ] Update member variable

#### Step 2: Update System Implementations

**File:** `src/game/diplomacy/DiplomacySystem.cpp`
- [ ] Update constructor parameter type
- [ ] Verify no other changes needed (interface is compatible)

**File:** `src/game/economy/EconomicSystem.cpp`
- [ ] Update constructor parameter type

**File:** `src/game/military/MilitarySystem.cpp`
- [ ] Update constructor parameter type

**File:** `src/game/trade/TradeSystem.cpp`
- [ ] Update constructor parameter type

**File:** `src/game/administration/AdministrativeSystem.cpp`
- [ ] Update constructor parameter type

#### Step 3: Update Bridge Systems

- [ ] `include/game/bridge/DiplomacyEconomicBridge.h`
- [ ] `src/game/bridge/DiplomacyEconomicBridge.cpp`
- [ ] `src/game/economy/TechnologyEconomicBridge.cpp`
- [ ] `src/game/economy/TradeEconomicBridge.cpp`
- [ ] `src/game/economy/EconomicPopulationBridge.cpp`
- [ ] `src/game/military/MilitaryEconomicBridge.cpp`

**For each file:**
- [ ] Change MessageBus includes to ThreadSafeMessageBus
- [ ] Update constructor/member variable types
- [ ] Compile and test

#### Step 4: Update Application Initialization

**File:** `apps/main.cpp`
- [ ] Find system creation (search for "DiplomacySystem")
- [ ] Change message_bus parameter to thread_safe_message_bus
- [ ] Repeat for all affected systems:
  - [ ] DiplomacySystem
  - [ ] EconomicSystem
  - [ ] MilitarySystem
  - [ ] TradeSystem
  - [ ] AdministrativeSystem

**Pattern:**
```cpp
// OLD:
auto diplomacy = std::make_unique<game::diplomacy::DiplomacySystem>(
    component_manager, message_bus);

// NEW:
auto diplomacy = std::make_unique<game::diplomacy::DiplomacySystem>(
    component_manager, thread_safe_message_bus);
```

#### Compilation & Quick Test
- [ ] Compile project: `make -j8`
- [ ] Fix any compilation errors
- [ ] Run game: verify all systems initialize
- [ ] Quick smoke test: play for 5 minutes
- [ ] Check logs for errors

---

## Week 2: Verification

### Day 6-7: AI System Review 🟡

- [ ] Review `include/game/ai/AIDirector.h`
- [ ] Verify all shared data protected by mutexes:
  - [ ] `m_actorMutex` protects `m_nationActors`, `m_characterActors`, `m_councilActors`
  - [ ] `m_stateMutex` protects `m_state`
  - [ ] `m_backgroundMutex` protects `m_backgroundTasks`
- [ ] Review `src/game/ai/AIDirector.cpp`
- [ ] Check all mutex usage follows consistent pattern
- [ ] Document any issues found
- [ ] Create tickets for any fixes needed

### Day 8-9: Create Tests 🟠

**Test 1:** `tests/test_diplomacy_threading.cpp`
```cpp
TEST(DiplomacySystem, MainThreadStrategy) {
    // Verify returns MAIN_THREAD
}

TEST(DiplomacySystem, PerformanceUnder16ms) {
    // Verify monthly update < 16ms
}
```
- [ ] Create file
- [ ] Implement tests
- [ ] Run: `ctest -R test_diplomacy_threading`

**Test 2:** `tests/test_message_bus_threading.cpp`
```cpp
TEST(ThreadSafeMessageBus, ConcurrentPublish) {
    // Verify multiple threads can publish safely
}

TEST(ThreadSafeMessageBus, ConcurrentSubscribe) {
    // Verify subscription is thread-safe
}
```
- [ ] Create file
- [ ] Implement tests
- [ ] Run: `ctest -R test_message_bus_threading`

**Test 3:** `tests/test_threaded_game_session.cpp`
```cpp
TEST(ThreadedGame, LongRunningSession) {
    // Run game for 1000 frames
    // Verify no crashes
}
```
- [ ] Create file
- [ ] Implement test
- [ ] Run: `ctest -R test_threaded_game_session`

### Day 10: ThreadSanitizer 🟠

- [ ] Build with ThreadSanitizer:
  ```bash
  mkdir build-tsan
  cd build-tsan
  cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" ..
  make -j8
  ```
- [ ] Run all tests: `ctest --output-on-failure`
- [ ] Review TSan output for warnings
- [ ] Fix any detected issues
- [ ] Re-run until clean

**TSan Output to Check:**
- [ ] No data races detected
- [ ] No lock order violations
- [ ] No deadlocks

---

## Week 3: Polish & Ship

### Day 11-12: Performance Benchmarking 🎯

**Benchmark 1:** DiplomacySystem performance
- [ ] Create `tests/benchmark_diplomacy.cpp`
- [ ] Measure monthly update time
- [ ] Target: < 2ms per update
- [ ] Document results

**Benchmark 2:** Overall frame time
- [ ] Run game with all systems active
- [ ] Measure frame time over 1000 frames
- [ ] Compare before/after fix
- [ ] Acceptable regression: < 5%
- [ ] Document results

### Day 13-14: Final Testing & Review 🎯

**Integration Testing:**
- [ ] Full game session (100+ game years)
- [ ] All systems active
- [ ] Verify no crashes
- [ ] Verify no data corruption
- [ ] Check logs for errors

**Code Review:**
- [ ] Self-review all changes
- [ ] Create pull request
- [ ] Assign reviewers
- [ ] Address review comments
- [ ] Get approval

**Documentation:**
- [ ] Update test reports with new grades
- [ ] Update CHANGELOG.md
- [ ] Add migration notes if needed
- [ ] Update architecture docs

### Day 15: Merge 🚀

**Pre-merge Checklist:**
- [ ] All tests passing
- [ ] ThreadSanitizer clean
- [ ] Performance benchmarks acceptable
- [ ] Code review approved
- [ ] Documentation updated
- [ ] Changelog updated

**Merge Process:**
- [ ] Rebase on main branch
- [ ] Resolve conflicts if any
- [ ] Final test run
- [ ] Merge to main
- [ ] Tag release (if applicable)
- [ ] Close related issues/tickets

**Post-merge:**
- [ ] Monitor CI/CD pipeline
- [ ] Verify deployment (if applicable)
- [ ] Notify team
- [ ] Archive notes and learnings

---

## Troubleshooting

### Compilation Errors

**Error:** `MessageBus is not a member of core::ecs`
- **Fix:** Check you changed the include to ThreadSafeMessageBus.h

**Error:** `cannot convert MessageBus to ThreadSafeMessageBus`
- **Fix:** Update constructor parameter types in header AND implementation

### Runtime Errors

**Error:** Crash on system initialization
- **Fix:** Check main.cpp passes thread_safe_message_bus, not message_bus

**Error:** Messages not being received
- **Fix:** ThreadSafeMessageBus interface is compatible, check subscription logic

### Test Failures

**Error:** Diplomacy tests fail after strategy change
- **Fix:** Tests may expect BACKGROUND_THREAD, update test expectations

**Error:** ThreadSanitizer warnings
- **Fix:** Add proper locking around shared data access

---

## Rollback Procedure

If critical issues found:

### Rollback DiplomacySystem Change
```bash
git checkout HEAD -- src/game/diplomacy/DiplomacySystem.cpp
git commit -m "Rollback DiplomacySystem to BACKGROUND_THREAD (temporary)"
```

### Rollback MessageBus Changes
```bash
git checkout HEAD -- include/game/economy/EconomicSystem.h
git checkout HEAD -- src/game/economy/EconomicSystem.cpp
# Repeat for each system
git commit -m "Rollback MessageBus migration for [SystemName]"
```

### Full Rollback
```bash
git revert <commit-hash>
git push origin claude/fix-critical-threading-issues-011CUzi7nhUEHd8XzPPQJB21
```

---

## Success Metrics

Track these metrics throughout implementation:

| Metric | Before | Target | Actual |
|--------|--------|--------|--------|
| Critical threading issues | 2 | 0 | ___ |
| Systems using unsafe MessageBus | 5+ | 0 | ___ |
| ThreadSanitizer warnings | Unknown | 0 | ___ |
| Average system grade | C+ (2.5) | B (3.0) | ___ |
| Test coverage | ~60% | 70%+ | ___ |
| Crash rate (per 1000 frames) | Unknown | 0 | ___ |

---

## Notes & Learnings

**Developer Notes:**
(Use this space to document issues encountered, solutions found, learnings, etc.)

---

**Completion Date:** _________________
**Sign-off:** _________________

**Full Plan:** See [`CRITICAL_THREADING_FIX_PLAN.md`](./CRITICAL_THREADING_FIX_PLAN.md)
