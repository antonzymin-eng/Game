# Testing Strategy - Mechanica Imperii

**Version:** 1.0
**Last Updated:** December 2025
**Applies To:** All code in Mechanica Imperii project

---

## Table of Contents

1. [Testing Philosophy](#testing-philosophy)
2. [Test Types and Coverage](#test-types-and-coverage)
3. [Unit Testing](#unit-testing)
4. [Integration Testing](#integration-testing)
5. [Performance Testing](#performance-testing)
6. [Threading and Concurrency Testing](#threading-and-concurrency-testing)
7. [System Testing](#system-testing)
8. [Test Organization](#test-organization)
9. [Testing Tools](#testing-tools)
10. [Writing Effective Tests](#writing-effective-tests)
11. [CI/CD Integration](#cicd-integration)
12. [Code Coverage Requirements](#code-coverage-requirements)

---

## 1. Testing Philosophy

### Core Principles

**Test-Driven Development (TDD) Encouraged**
- Write tests before implementation when feasible
- Tests document expected behavior
- Red-Green-Refactor cycle

**Testing Pyramid**
```
         /\
        /  \      E2E Tests (Few)
       /____\     - System tests
      /      \    - Full game scenarios
     /________\   Integration Tests (Some)
    /          \  - System interactions
   /____________\ Unit Tests (Many)
                  - Component-level
                  - Fast, isolated
```

**Quality Goals**
- **Reliability:** Tests must be deterministic
- **Speed:** Unit tests complete in milliseconds
- **Maintainability:** Tests are code - follow same quality standards
- **Coverage:** Aim for 80%+ on critical systems

---

## 2. Test Types and Coverage

### 2.1 Test Type Matrix

| Test Type | Purpose | Scope | Speed | Quantity |
|-----------|---------|-------|-------|----------|
| **Unit** | Component logic | Single class/function | <1ms | Many (1000s) |
| **Integration** | System interactions | Multiple systems | <100ms | Some (100s) |
| **Performance** | Benchmarking | System/subsystem | Varies | Few (10s) |
| **Threading** | Race conditions | Concurrent code | <1s | Some (100s) |
| **System** | End-to-end scenarios | Full game | Minutes | Few (12) |

### 2.2 Coverage Targets by System

| System | Unit Tests | Integration Tests | Performance Tests | Coverage Target |
|--------|-----------|-------------------|-------------------|-----------------|
| **ECS Core** | ✅ Required | ✅ Required | ⚠️ Optional | 90%+ |
| **Save System** | ✅ Required | ✅ Required | ✅ Required | 90%+ |
| **AI Systems** | ✅ Required | ✅ Required | ✅ Required | 85%+ |
| **Economy** | ✅ Required | ✅ Required | ✅ Required | 85%+ |
| **Diplomacy** | ✅ Required | ✅ Required | ⚠️ Optional | 80%+ |
| **Rendering** | ⚠️ Optional | ✅ Required | ⚠️ Optional | 60%+ |
| **UI** | ⚠️ Optional | ⚠️ Optional | ❌ Not needed | 40%+ |

---

## 3. Unit Testing

### 3.1 What to Unit Test

**Always unit test:**
- Business logic and calculations
- Data transformations
- Validation functions
- Pure functions (no side effects)
- Algorithm implementations

**Example - Character Loyalty Calculation:**
```cpp
TEST(LoyaltyCalculatorTest, NegativeOpinionReducesLoyalty) {
    LoyaltyCalculator calculator;

    // Arrange
    EntityID character = CreateTestCharacter();
    EntityID liege = CreateTestLiege();
    SetOpinion(character, liege, -50);  // Negative opinion

    // Act
    double loyalty = calculator.Calculate(character, liege);

    // Assert
    EXPECT_LT(loyalty, 50.0);  // Loyalty should be below neutral
    EXPECT_GE(loyalty, 0.0);   // But within valid range
}
```

### 3.2 Unit Test Structure

**Follow AAA Pattern:**
```cpp
TEST(ComponentTest, MethodUnderTest_Scenario_ExpectedBehavior) {
    // Arrange - Set up test data
    CharacterRelationshipsComponent comp(EntityID{1});
    EntityID friend_id{2};

    // Act - Execute the method under test
    comp.SetRelationship(friend_id, RelationshipType::FRIEND, 50, 75.0);

    // Assert - Verify expectations
    auto rel = comp.GetRelationship(friend_id);
    ASSERT_TRUE(rel.has_value());
    EXPECT_EQ(rel->type, RelationshipType::FRIEND);
    EXPECT_EQ(rel->opinion, 50);
    EXPECT_DOUBLE_EQ(rel->bond_strength, 75.0);
}
```

### 3.3 Test Naming Conventions

**Format:** `MethodUnderTest_Scenario_ExpectedBehavior`

```cpp
TEST(CharacterRelationships, AddMarriage_ValidSpouse_AddsToMarriagesList)
TEST(CharacterRelationships, GetFriends_NoFriends_ReturnsEmptyVector)
TEST(CharacterRelationships, ModifyBondStrength_ExceedsMax_ClampsToMaximum)
TEST(CharacterRelationships, IsMarriedTo_NotMarried_ReturnsFalse)
```

### 3.4 Test Fixtures for Shared Setup

```cpp
class CharacterRelationshipsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for all tests
        character_id = EntityID{1};
        component = std::make_unique<CharacterRelationshipsComponent>(character_id);
    }

    void TearDown() override {
        // Cleanup after each test
        component.reset();
    }

    // Helper methods for tests
    void AddTestFriend(EntityID friend_id, double bond_strength = 50.0) {
        component->SetRelationship(friend_id, RelationshipType::FRIEND,
                                   /*opinion=*/50, bond_strength);
    }

    EntityID character_id;
    std::unique_ptr<CharacterRelationshipsComponent> component;
};

TEST_F(CharacterRelationshipsTest, GetFriends_WithSignificantBond_ReturnsFriend) {
    AddTestFriend(EntityID{2}, 30.0);  // Above threshold
    auto friends = component->GetFriends();
    EXPECT_EQ(friends.size(), 1);
}
```

### 3.5 Edge Cases to Test

**Boundary Values:**
```cpp
TEST(CharacterRelationships, ModifyBondStrength_AtMinimum_StaysAtMinimum) {
    component.SetRelationship(friend_id, RelationshipType::FRIEND, 0, 0.0);
    component.ModifyBondStrength(friend_id, -10.0);  // Try to go below min
    EXPECT_DOUBLE_EQ(component.GetRelationship(friend_id)->bond_strength, 0.0);
}

TEST(CharacterRelationships, ModifyBondStrength_AtMaximum_StaysAtMaximum) {
    component.SetRelationship(friend_id, RelationshipType::FRIEND, 0, 100.0);
    component.ModifyBondStrength(friend_id, 10.0);  // Try to go above max
    EXPECT_DOUBLE_EQ(component.GetRelationship(friend_id)->bond_strength, 100.0);
}
```

**Null/Empty Cases:**
```cpp
TEST(CharacterRelationships, GetRelationship_NonexistentCharacter_ReturnsNullopt) {
    auto rel = component.GetRelationship(EntityID{999});
    EXPECT_FALSE(rel.has_value());
}

TEST(CharacterRelationships, GetFriends_NoFriends_ReturnsEmpty) {
    auto friends = component.GetFriends();
    EXPECT_TRUE(friends.empty());
}
```

---

## 4. Integration Testing

### 4.1 What to Integration Test

**System Interactions:**
- Message bus communication between systems
- ECS component interactions
- Data flow through multiple systems
- System initialization and shutdown

**Example - AI Director Integration:**
```cpp
TEST(AIDirectorIntegration, NationAI_ReceivesTasksFromDirector) {
    // Arrange: Set up AI Director and Nation AI
    AIDirector director;
    NationAISystem nation_ai;
    MessageBus message_bus;

    director.RegisterWithMessageBus(message_bus);
    nation_ai.RegisterWithMessageBus(message_bus);

    // Act: Director processes and assigns tasks
    director.AssignTasks();
    message_bus.DispatchMessages();

    // Assert: Nation AI received tasks
    EXPECT_TRUE(nation_ai.HasPendingTasks());
    EXPECT_GT(nation_ai.GetTaskCount(), 0);
}
```

### 4.2 Integration Test Scenarios

**Cross-System Workflows:**
```cpp
TEST(EconomyIntegration, TradeRoute_AffectsProvinceWealth) {
    // Test that creating a trade route:
    // 1. Updates TradeSystem
    // 2. Sends message to EconomySystem
    // 3. EconomySystem updates province wealth
    // 4. Province component reflects new wealth value
}

TEST(DiplomacyIntegration, Marriage_CreatesAlliance) {
    // Test that marriage creation:
    // 1. Updates CharacterRelationships
    // 2. Triggers DiplomacySystem alliance creation
    // 3. Alliance stored in AllianceComponent
    // 4. Both realms have alliance reference
}
```

### 4.3 Integration Test Data

**Use realistic data volumes:**
```cpp
class AIDirectorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create realistic game scenario
        CreateTestWorld(
            /*provinces=*/100,
            /*characters=*/500,
            /*nations=*/20
        );
    }

    // Test with production-like scale
    void CreateTestWorld(int provinces, int characters, int nations);
};
```

---

## 5. Performance Testing

### 5.1 What to Benchmark

**Critical Performance Paths:**
- AI decision-making (must complete within time budget)
- ECS system updates (run every tick)
- Save/load operations
- Large-scale calculations (influence, economy)

### 5.2 Performance Test Structure

```cpp
TEST(AIDirectorPerformance, AssignTasks_3000Characters_CompletesUnder100ms) {
    // Arrange: Production-scale scenario
    AIDirector director;
    CreateCharacters(3000);
    CreateNations(100);

    // Act: Measure performance
    auto start = std::chrono::high_resolution_clock::now();
    director.AssignTasks();
    auto end = std::chrono::high_resolution_clock::now();

    // Assert: Within time budget
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 100) << "Task assignment took " << duration.count() << "ms";
}
```

### 5.3 Performance Baselines

**Establish benchmarks:**

| Operation | Target | Maximum | Current |
|-----------|--------|---------|---------|
| AI Task Assignment | <50ms | <100ms | TBD |
| ECS Update (all systems) | <16ms | <33ms | TBD |
| Save Game | <500ms | <1000ms | TBD |
| Load Game | <1000ms | <2000ms | TBD |
| Influence Calculation | <100ms | <200ms | TBD |

### 5.4 Profiling Integration

```cpp
TEST(PerformanceProfile, DISABLED_ProfileAIDirector) {
    // This test is disabled by default
    // Run manually with profiler to identify bottlenecks
    AIDirector director;
    SetupProductionScenario();

    // Run 1000 iterations for profiling
    for (int i = 0; i < 1000; ++i) {
        director.Update();
    }
}
```

---

## 6. Threading and Concurrency Testing

### 6.1 ThreadSanitizer (TSan) Tests

**Detect race conditions:**

```cpp
// Dedicated threading test files in tests/threading/
// Built with -fsanitize=thread

TEST(AIDirectorThreading, ConcurrentTaskAssignment_NoDataRaces) {
    AIDirector director;
    SetupTestScenario();

    // Launch multiple threads accessing shared state
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&director]() {
            director.AssignTasks();
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // TSan will detect any data races
}
```

### 6.2 Concurrency Correctness

```cpp
TEST(ThreadingSafety, MessageBus_ConcurrentPublish_AllMessagesReceived) {
    MessageBus bus;
    std::atomic<int> messages_received{0};

    bus.Subscribe("test_event", [&](const Message& msg) {
        messages_received++;
    });

    // Multiple threads publishing simultaneously
    std::vector<std::thread> publishers;
    constexpr int MESSAGES_PER_THREAD = 100;
    constexpr int THREAD_COUNT = 4;

    for (int i = 0; i < THREAD_COUNT; ++i) {
        publishers.emplace_back([&bus]() {
            for (int j = 0; j < MESSAGES_PER_THREAD; ++j) {
                bus.Publish("test_event", Message{});
            }
        });
    }

    for (auto& t : publishers) {
        t.join();
    }

    bus.DispatchMessages();

    EXPECT_EQ(messages_received.load(), MESSAGES_PER_THREAD * THREAD_COUNT);
}
```

---

## 7. System Testing

### 7.1 System Test Procedures

**Manual system tests documented in `/tests/SYSTEM_TEST_*.md`:**

- **SYSTEM_TEST_001** - ECS Entity Lifecycle
- **SYSTEM_TEST_002** - Save/Load Cycle
- **SYSTEM_TEST_003** - AI Director Integration
- **SYSTEM_TEST_004** - Economy System Workflow
- **SYSTEM_TEST_005** - Diplomacy System
- **SYSTEM_TEST_006** - Military System
- **SYSTEM_TEST_007** - Character System
- **SYSTEM_TEST_008** - Trade System
- **SYSTEM_TEST_009** - Population System
- **SYSTEM_TEST_010** - Technology System
- **SYSTEM_TEST_011** - Map Rendering
- **SYSTEM_TEST_012** - Full Game Scenario

### 7.2 Automated System Test Example

```cpp
TEST(SystemTest, FullGameScenario_1066Start_Runs100Years) {
    // This test runs a full game scenario
    // May take several minutes to complete

    GameEngine engine;
    engine.LoadScenario("1066_norman_conquest");

    // Simulate 100 years of game time
    for (int year = 0; year < 100; ++year) {
        for (int tick = 0; tick < TICKS_PER_YEAR; ++tick) {
            engine.Update();
        }
    }

    // Verify game state is consistent
    EXPECT_TRUE(engine.IsStateValid());
    EXPECT_GT(engine.GetActiveCharacters(), 0);
    EXPECT_GT(engine.GetActiveNations(), 0);
}
```

---

## 8. Test Organization

### 8.1 Directory Structure

```
/tests/
├── unit/                           # Unit tests (fast, isolated)
│   ├── test_character_relationships.cpp
│   ├── test_loyalty_calculator.cpp
│   ├── test_economy_calculator.cpp
│   └── ...
├── integration/                    # Integration tests (system interactions)
│   ├── test_ai_director_integration.cpp
│   ├── test_diplomacy_integration.cpp
│   └── ...
├── performance/                    # Performance benchmarks
│   ├── test_ai_director_performance.cpp
│   ├── test_ecs_performance.cpp
│   └── ...
├── threading/                      # Threading safety (TSan)
│   ├── test_ai_director_threading_tsan.cpp
│   ├── test_message_bus_threading_tsan.cpp
│   └── ...
├── system/                         # End-to-end system tests
│   ├── test_full_game_scenario.cpp
│   └── ...
├── SYSTEM_TEST_001.md             # Manual test procedures
├── SYSTEM_TEST_002.md
└── ...
```

### 8.2 Test File Naming

**Convention:** `test_<component>_<type>.cpp`

```
test_character_relationships.cpp          # Unit test
test_ai_director_integration.cpp          # Integration test
test_ai_director_performance.cpp          # Performance test
test_ai_director_threading_tsan.cpp       # Threading test
```

---

## 9. Testing Tools

### 9.1 Test Frameworks

**Google Test (Optional)**
```cmake
find_package(GTest CONFIG)
if(GTest_FOUND)
    target_link_libraries(tests PRIVATE GTest::gtest GTest::gtest_main)
endif()
```

**Custom Test Harness (Fallback)**
```cpp
// Simple assertion macros for lightweight testing
#define EXPECT_TRUE(condition) if (!(condition)) { /* fail */ }
#define EXPECT_EQ(a, b) if ((a) != (b)) { /* fail */ }
```

### 9.2 Sanitizers

**AddressSanitizer (ASan)** - Memory errors
```bash
cmake -B build -DENABLE_ASAN=ON
cmake --build build
./build/tests/run_all_tests
```

**ThreadSanitizer (TSan)** - Race conditions
```bash
cmake -B build -DENABLE_TSAN=ON
cmake --build build
./build/tests/run_tsan_tests
```

**UndefinedBehaviorSanitizer (UBSan)** - Undefined behavior
```bash
cmake -B build -DENABLE_UBSAN=ON
cmake --build build
./build/tests/run_all_tests
```

### 9.3 Code Coverage

**Generate coverage report:**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON
cmake --build build
./build/tests/run_all_tests
lcov --capture --directory build --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

---

## 10. Writing Effective Tests

### 10.1 Test Quality Guidelines

**✅ Good Test Characteristics:**
- **Fast** - Unit tests complete in <1ms
- **Isolated** - No dependencies on external state
- **Deterministic** - Same input always gives same output
- **Readable** - Clear intent and assertions
- **Maintainable** - Easy to update when code changes

**❌ Test Anti-Patterns to Avoid:**
- **Flaky tests** - Sometimes pass, sometimes fail
- **Slow tests** - Take seconds to run
- **Coupled tests** - Depend on execution order
- **Obscure tests** - Unclear what is being tested
- **Fragile tests** - Break with minor code changes

### 10.2 Mocking and Test Doubles

**Use test doubles for external dependencies:**

```cpp
// Mock EntityManager for testing
class MockEntityManager : public IEntityManager {
public:
    MOCK_METHOD(Entity, CreateEntity, (), (override));
    MOCK_METHOD(void, DestroyEntity, (EntityID), (override));
    MOCK_METHOD(bool, HasComponent, (EntityID, ComponentType), (const, override));
};

TEST(CharacterSystem, Update_WithMockEntityManager_CallsCorrectMethods) {
    MockEntityManager entity_manager;
    CharacterSystem system(entity_manager);

    EXPECT_CALL(entity_manager, HasComponent(_, ComponentType::Character))
        .WillOnce(Return(true));

    system.Update();
}
```

### 10.3 Data-Driven Tests

**Test multiple scenarios with parameterized tests:**

```cpp
class BondStrengthTest : public ::testing::TestWithParam<std::tuple<double, double, double>> {
    // param: (initial_bond, delta, expected_result)
};

TEST_P(BondStrengthTest, ModifyBondStrength_ClampsCorrectly) {
    auto [initial, delta, expected] = GetParam();

    CharacterRelationshipsComponent comp(EntityID{1});
    comp.SetRelationship(EntityID{2}, RelationshipType::FRIEND, 0, initial);
    comp.ModifyBondStrength(EntityID{2}, delta);

    auto rel = comp.GetRelationship(EntityID{2});
    EXPECT_DOUBLE_EQ(rel->bond_strength, expected);
}

INSTANTIATE_TEST_SUITE_P(
    BondStrengthBoundaries,
    BondStrengthTest,
    ::testing::Values(
        std::make_tuple(0.0, -10.0, 0.0),     // Below min clamped to 0
        std::make_tuple(100.0, 10.0, 100.0),  // Above max clamped to 100
        std::make_tuple(50.0, 25.0, 75.0),    // Normal case
        std::make_tuple(50.0, -25.0, 25.0)    // Normal case
    )
);
```

---

## 11. CI/CD Integration

### 11.1 Automated Test Execution

**GitHub Actions Workflow:**
```yaml
# .github/workflows/tests.yml
name: Tests

on: [push, pull_request]

jobs:
  unit-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build and run unit tests
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Debug
          cmake --build build
          cd build && ctest --output-on-failure

  sanitizer-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Run with AddressSanitizer
        run: |
          cmake -B build -DENABLE_ASAN=ON
          cmake --build build
          ./build/tests/run_all_tests

      - name: Run with ThreadSanitizer
        run: |
          cmake -B build-tsan -DENABLE_TSAN=ON
          cmake --build build-tsan
          ./build-tsan/tests/run_tsan_tests
```

### 11.2 Pull Request Requirements

**All PRs must:**
- ✅ Pass all unit tests
- ✅ Pass all integration tests
- ✅ Pass ASan (AddressSanitizer)
- ✅ Pass TSan (ThreadSanitizer) if touching concurrent code
- ✅ Maintain or improve code coverage
- ✅ Include tests for new features

---

## 12. Code Coverage Requirements

### 12.1 Coverage Targets

**Minimum Coverage by System Priority:**

| Priority | System Type | Minimum Coverage |
|----------|-------------|-----------------|
| **Critical** | ECS, Save, Core | 90%+ |
| **High** | AI, Economy, Diplomacy | 85%+ |
| **Medium** | Military, Population | 80%+ |
| **Low** | Rendering, UI | 60%+ |

### 12.2 Coverage Reporting

**Generate and review coverage:**
```bash
# Generate coverage report
./scripts/generate_coverage.sh

# View HTML report
open coverage_report/index.html
```

### 12.3 Coverage Exceptions

**Acceptable reasons for low coverage:**
- Rendering code (hard to test without GPU)
- Platform-specific code (requires multiple test environments)
- Debug/logging code (non-critical paths)
- Generated code (e.g., serialization boilerplate)

**Document exceptions:**
```cpp
// LCOV_EXCL_START - Renderer OpenGL code, tested manually
void RenderMap() {
    // OpenGL rendering calls
}
// LCOV_EXCL_STOP
```

---

## Summary

### Testing Checklist for New Features

When adding a new feature:

- [ ] Write unit tests for business logic
- [ ] Write integration tests for system interactions
- [ ] Add performance tests if performance-critical
- [ ] Run tests with ASan and TSan
- [ ] Verify code coverage meets target
- [ ] Document any complex test scenarios
- [ ] Update system test procedures if needed

### Quick Reference

```bash
# Run all tests
ctest --output-on-failure

# Run specific test
./build/tests/test_character_relationships

# Run with sanitizers
./build-asan/tests/run_all_tests
./build-tsan/tests/run_tsan_tests

# Generate coverage
./scripts/generate_coverage.sh
```

---

**For questions about testing strategy, consult this document or ask the development team.**

**Version History:**
- v1.0 (December 2025) - Initial testing strategy document
