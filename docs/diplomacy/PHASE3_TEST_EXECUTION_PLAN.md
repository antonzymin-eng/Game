# Phase 3 Influence System - Test Execution Plan

**Status**: Test Suite Complete (809 lines) - Ready for Execution
**Created**: 2025-11-12
**Test File**: `/tests/test_influence_system.cpp`
**Build Target**: `test_influence_system`

---

## Test Suite Overview

### Comprehensive Coverage (11 Test Cases)

1. **InfluenceComponent Serialization Round-Trip** (Lines 57-136)
   - Tests JSON serialization/deserialization
   - Validates all influence types, sphere metrics, conflicts
   - Ensures data integrity across save/load cycles

2. **BFS Propagation with Distance Decay** (Lines 138-212)
   - Tests breadth-first search algorithm
   - Validates multi-hop propagation
   - Confirms distance decay calculations

3. **Propagation Blocking (Closed Borders)** (Lines 214-282)
   - Tests border closure mechanics
   - Validates influence blocking
   - Ensures no leakage through blocked paths

4. **Sphere Conflict Detection** (Lines 284-358)
   - Tests conflict identification
   - Validates tension level calculations
   - Confirms flashpoint detection

5. **Conflict Escalation** (Lines 360-424)
   - Tests escalation mechanics over time
   - Validates crisis threshold triggers
   - Confirms escalation risk calculations

6. **DiplomacySystem Integration** (Lines 426-498)
   - Tests influence → opinion modifiers
   - Validates autonomy restrictions
   - Confirms bilateral influence scenarios

7. **Autonomy Calculation** (Lines 500-564)
   - Tests autonomy score computation
   - Validates threshold detection
   - Confirms diplomatic freedom restrictions

8. **Character Influence Targeting** (Lines 566-630)
   - Tests character-level influence
   - Validates compromise mechanics
   - Confirms trust erosion

9. **Vassal Defection Risk** (Lines 632-696)
   - Tests vassal loyalty calculations
   - Validates defection risk assessment
   - Confirms threshold warnings

10. **Performance: 500 Realms Monthly Update** (Lines 698-758)
    - **Target**: <5ms per realm update
    - Tests scalability at production scale
    - Validates computational efficiency

11. **Performance: Serialization** (Lines 760-809)
    - **Target**: <100ms for full state
    - Tests save/load performance
    - Validates compression efficiency

---

## Build Instructions

### Prerequisites

**Required**:
- CMake 3.15+
- C++17 compiler (GCC 13+ / Clang 15+)
- jsoncpp (auto-fetched if missing)

**Optional** (skipped for test-only builds):
- SDL2 (main game only)
- OpenGL (rendering only)
- OpenSSL (crypto only)

### Building Tests

```bash
cd /home/user/Game
mkdir -p build && cd build

# Configure with test support
cmake .. -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug

# Build test executable
make test_influence_system

# Run tests
./tests/test_influence_system
```

### CMake Improvements (2025-11-12)

- ✅ Made SDL2 optional for test-only builds
- ✅ Added FetchContent fallback for jsoncpp
- ✅ Made OpenGL/OpenSSL optional for tests
- ✅ Made GTest optional (only for AI Director tests)
- ✅ Integrated test_influence_system into test suite

---

## Expected Test Output

### Success Criteria

Each test should output:
```
============================================================
Testing: [Test Name]
============================================================
✓ TEST PASSED: [Specific validation]
```

### Performance Benchmarks

**Test 10**: 500 Realms Monthly Update
```
Average time per realm: X.XXms
✓ PASSED: < 5ms target
```

**Test 11**: Serialization Performance
```
Serialization time: XXms
Deserialization time: XXms
Total: XXms
✓ PASSED: < 100ms target
```

---

## Failure Analysis Guide

### Common Issues

1. **Serialization Round-Trip Failures**
   - Check JSON format compatibility
   - Validate all fields are serialized
   - Confirm type conversions

2. **Propagation Failures**
   - Verify BFS algorithm implementation
   - Check distance decay formula
   - Validate path tracking

3. **Performance Failures**
   - Profile with `perf` or `valgrind --tool=cachegrind`
   - Check for O(n²) algorithms
   - Optimize data structure access patterns

4. **Integration Failures**
   - Verify DiplomacySystem is initialized
   - Check component registration
   - Confirm message bus connectivity

---

## Test Data

### Realm Setup
- **Total Realms**: Configurable (default: 500 for performance tests)
- **Influence Sources**: 3-5 per realm
- **Network Density**: ~30% connectivity (realistic scenario)

### Parameters Used
- **Military Influence Weight**: 0.4
- **Economic Influence Weight**: 0.3
- **Prestige Influence Weight**: 0.2
- **Cultural Influence Weight**: 0.1
- **Distance Decay**: 0.85 per hop
- **Autonomy Threshold**: 50.0
- **Flashpoint Threshold**: 75.0

---

## Integration with CI/CD

### Automated Testing

```yaml
# Example .github/workflows/tests.yml
name: Phase 3 Tests
on: [push, pull_request]
jobs:
  test-influence-system:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt install -y cmake g++ libjsoncpp-dev
      - name: Build tests
        run: |
          mkdir build && cd build
          cmake .. -DBUILD_TESTS=ON
          make test_influence_system
      - name: Run tests
        run: ./build/tests/test_influence_system
```

---

## Next Steps After Successful Tests

1. **Balance Tuning**
   - Adjust influence weights based on test results
   - Tune decay rates for realistic propagation
   - Calibrate thresholds for flashpoints

2. **Performance Optimization**
   - If >5ms: Profile and optimize hot paths
   - If >100ms serialization: Enable compression
   - Consider caching frequently accessed data

3. **Integration Testing**
   - Connect to AI Director
   - Test with real game scenarios
   - Validate save game compatibility

4. **Documentation Updates**
   - Update PHASE3_STATUS.md with test results
   - Document any parameter changes
   - Add findings to PHASE3_TEST_REPORT.md

---

## Current Status

**Build Configuration**: ✅ Complete
**Test Suite**: ✅ Complete (809 lines)
**CMake Integration**: ✅ Complete
**Dependency Resolution**: ✅ Complete (FetchContent fallback)
**Execution**: ⏳ Pending environment with dependencies

**Next**: Execute tests in environment with build dependencies available
