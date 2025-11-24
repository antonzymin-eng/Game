# Phase 0: Test Infrastructure Setup Report

**Date:** 2025-11-10
**Status:** In Progress
**Goal:** Establish testing infrastructure and baseline metrics before systematic testing

---

## 1. TEST INFRASTRUCTURE ANALYSIS

### Existing Test Framework
**Framework:** ❌ None (Standalone test executables)
- No Google Test, Catch2, or similar framework detected
- Tests are simple standalone C++ executables
- 23 test files in `/home/user/Game/tests/` directory

### Test Files Found
```
tests/
├── test_ecs_integration.cpp
├── test_administrative_components_simple.cpp
├── test_economic_ecs_integration.cpp
├── test_military_ecs_integration.cpp
├── test_military_components_simple.cpp
├── test_diplomacy_refactoring.cpp
├── test_diplomacy_memory.cpp
├── test_diplomacy_cooldown.cpp
├── test_diplomacy_decay.cpp
├── test_decay_standalone.cpp
├── test_ai_refactoring.cpp
├── test_ai_attention_refactoring.cpp
├── test_ai_director_refactoring.cpp
├── test_nation_ai_refactoring.cpp
├── test_population_refactoring.cpp
├── test_population_ui.cpp
├── test_realm_refactoring.cpp
├── test_technology_ecs_integration.cpp
├── test_trade_refactoring.cpp
├── test_gameplay_refactoring.cpp
├── test_enhanced_config.cpp
├── test_scenario_demo.cpp
└── test_map_loading.cpp
```

**Total:** 23 test files covering various systems

### Test Build Configuration
- **BUILD_TESTS option:** OFF by default in CMakeLists.txt
- **Configured tests:** Only 2 tests configured in CMake:
  1. `test_enhanced_config` - Config system testing
  2. `test_scenario_demo` - Scenario system testing
- **Other tests:** Located in `tests/` but not integrated into CMake build

### Test Structure
- Simple standalone executables (main() function)
- Manual verification (std::cout for output)
- No automated assertions
- Integration-style tests (test multiple systems together)

### Example Test Pattern
```cpp
int main() {
    std::cout << "=== ECS Integration Test ===" << std::endl;

    // Create components
    auto entity_manager = std::make_unique<EntityManager>();
    auto message_bus = std::make_unique<MessageBus>();

    // Test functionality
    auto population_system = std::make_unique<PopulationSystem>();
    population_system->Initialize();

    std::cout << "✅ Test passed" << std::endl;
    return 0;
}
```

---

## 2. BUILD SYSTEM ANALYSIS

### CMake Configuration
- **Version:** 3.15+
- **C++ Standard:** C++17
- **Project:** Mechanica Imperii v1.0.0
- **Main executable:** mechanica_imperii

### Dependencies
```
Core:
- SDL2 (graphics/windowing)
- OpenGL (rendering)
- GLAD (OpenGL loader)
- ImGui (UI)
- JsonCpp (configuration)
- OpenSSL (cryptography)
- LZ4 (compression - optional with auto-fetch)

Platform-Specific:
- Linux: pthread, dl
- Windows: ws2_32, winmm
```

### Build Options Available
- `BUILD_TESTS` - Build test executables (currently OFF)
- `BUILD_DOCS` - Build documentation (OFF)
- `USE_VENDOR_LZ4` - Auto-fetch LZ4 if not found (ON)

### Sanitizer Support
**Status:** ⚠️ Not configured in CMakeLists.txt
- No predefined ENABLE_SANITIZERS option
- Can be enabled via CMAKE_CXX_FLAGS
- ThreadSanitizer, AddressSanitizer, UBSan available

---

## 3. SOURCE CODE ORGANIZATION

### Directory Structure
```
/home/user/Game/
├── apps/                    # Main applications
│   ├── main.cpp            # Main game executable
│   ├── test_enhanced_config.cpp
│   └── test_scenario_demo.cpp
├── src/                     # Implementation files
│   ├── core/               # Core systems (ECS, threading, save)
│   ├── game/               # Game systems (economy, military, etc.)
│   ├── map/                # Map systems
│   ├── rendering/          # Rendering systems
│   └── ui/                 # UI systems
├── include/                 # Header files (mirrors src/)
├── tests/                   # Test files (not fully integrated)
├── data/                    # Game data (JSON definitions)
├── config/                  # Configuration files
├── shaders/                 # GLSL shaders
├── archive/                 # Archived test files
└── CMakeLists.txt          # Build configuration
```

### Source File Count
- **Core Systems:** ~10 files
- **Game Systems:** ~40 files
- **AI Systems:** 5 files
- **Rendering:** ~10 files
- **UI:** 13 files
- **Total:** ~80+ source files

---

## 4. TESTING GAPS IDENTIFIED

### Critical Gaps
1. **No Automated Test Framework**
   - No Google Test, Catch2, or similar
   - All tests manual verification
   - No assertions or automated pass/fail

2. **Most Tests Not in Build System**
   - Only 2/23 tests configured in CMake
   - Remaining 21 tests orphaned
   - Cannot run full test suite

3. **No Sanitizer Integration**
   - ThreadSanitizer not configured
   - AddressSanitizer not configured
   - UBSan not configured

4. **No Code Coverage**
   - No coverage flags
   - No coverage reports
   - Unknown test coverage percentage

5. **No CI/CD Integration**
   - No GitHub Actions workflows
   - No automated testing on commits
   - Manual testing only

6. **No Performance Benchmarking**
   - No Google Benchmark or similar
   - No performance regression detection
   - Manual performance validation only

### Testing Strategy Gaps
- No unit tests (only integration tests)
- No thread safety tests
- No memory leak tests with Valgrind
- No regression test suite
- No load/stress tests

---

## 5. RECOMMENDATIONS

### Immediate Actions (Phase 0)

#### 1. Enable Existing Tests
```bash
# Build with tests enabled
cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON ..
make -j8
```

#### 2. Add Sanitizer Builds
Create separate build directories for sanitizers:
```bash
# ThreadSanitizer build
mkdir build_tsan && cd build_tsan
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" ..
make -j8

# AddressSanitizer build
mkdir build_asan && cd build_asan
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g" ..
make -j8
```

#### 3. Baseline Profiling
```bash
# Build clean debug version
mkdir build_profile && cd build_profile
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j8

# Run game briefly and profile
./bin/mechanica_imperii &
PID=$!
sleep 30  # Let game run for 30 seconds
perf record -p $PID -g -- sleep 10
perf report

# Memory profiling
valgrind --tool=massif ./bin/mechanica_imperii
ms_print massif.out.*
```

#### 4. Document Baseline Metrics
Create `BASELINE_METRICS.md` with:
- Current FPS (if measurable)
- Memory usage (RSS, heap)
- CPU usage per system
- Load times
- Known issues/crashes

### Short-Term Actions (Week 1-2)

#### 1. Consider Test Framework Integration
**Options:**
- **Catch2** (header-only, easy integration)
- **Google Test** (full-featured, industry standard)
- **Doctest** (lightweight, fast compilation)

**Recommendation:** Start with Catch2 (easiest to integrate)

#### 2. Convert Standalone Tests to Framework Tests
Example conversion:
```cpp
// Before (standalone)
int main() {
    std::cout << "Testing..." << std::endl;
    // manual checks
    return 0;
}

// After (Catch2)
#include <catch2/catch.hpp>

TEST_CASE("ECS Integration", "[ecs]") {
    REQUIRE(entity_manager != nullptr);
    REQUIRE(entity_manager->CreateEntity().id > 0);
}
```

#### 3. Create Test Organization
```
tests/
├── unit/              # Unit tests (new)
│   ├── core/
│   ├── game/
│   └── ai/
├── integration/       # Integration tests (existing)
│   └── test_*.cpp
├── regression/        # Regression tests (new)
└── performance/       # Performance benchmarks (new)
```

---

## 6. PHASE 0 EXECUTION PLAN

### Step 1: Build Verification ✅
- [x] Verify CMakeLists.txt structure
- [x] Identify test files
- [x] Document existing test infrastructure
- [ ] Attempt clean build
- [ ] Verify game runs

### Step 2: Sanitizer Builds
- [ ] Create ThreadSanitizer build
- [ ] Create AddressSanitizer build
- [ ] Run game with each sanitizer
- [ ] Document any immediate issues found

### Step 3: Baseline Profiling
- [ ] Profile CPU usage (perf)
- [ ] Profile memory usage (Valgrind Massif)
- [ ] Measure frame rate
- [ ] Measure load times
- [ ] Document baseline metrics

### Step 4: Test Execution
- [ ] Enable BUILD_TESTS
- [ ] Build test executables
- [ ] Run test_enhanced_config
- [ ] Run test_scenario_demo
- [ ] Document test results

### Step 5: Documentation
- [ ] Create BASELINE_METRICS.md
- [ ] Create TESTING_INFRASTRUCTURE.md
- [ ] Update TESTING_PLAN.md with findings
- [ ] Document known issues

---

## 7. NEXT STEPS

After Phase 0 completion:

1. **Phase 1 Start:** Configuration System Testing
   - Simple system to validate testing approach
   - Quick win to build confidence
   - Establish testing patterns

2. **Progressive Enhancement:**
   - Add Catch2 framework (optional but recommended)
   - Integrate remaining tests into CMake
   - Add regression tests for each bug fixed
   - Set up automated testing

3. **Continue Through Testing Plan:**
   - Follow updated TESTING_PLAN.md
   - 13-week systematic testing
   - Fix issues as discovered
   - Document all findings

---

## 8. CURRENT STATUS

### Completed
- ✅ Identified all 50+ game systems
- ✅ Created comprehensive testing plan
- ✅ Reviewed and updated plan
- ✅ Analyzed existing test infrastructure
- ✅ Documented test infrastructure gaps

### In Progress
- ⏳ Phase 0 setup execution
- ⏳ Build verification
- ⏳ Baseline profiling

### Pending
- ⬜ Sanitizer builds
- ⬜ Test execution
- ⬜ Metrics documentation
- ⬜ Phase 1 start (Configuration System testing)

---

## 9. RISK ASSESSMENT

### High Risks
1. **No automated testing** - Can't catch regressions
2. **Most tests not integrated** - Can't run full suite
3. **No sanitizers** - May miss threading/memory issues
4. **Manual verification only** - Time-consuming, error-prone

### Mitigation
1. Integrate tests incrementally
2. Add sanitizer builds immediately
3. Start with critical systems (ECS, Threading, Save)
4. Consider test framework for new tests

### Opportunities
1. Existing tests provide good starting point
2. Clear system boundaries aid testing
3. Well-organized codebase
4. Comprehensive testing plan in place

---

**End of Phase 0 Setup Report**
**Next Action:** Attempt clean build and baseline profiling
