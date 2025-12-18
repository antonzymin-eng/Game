# Performance Guidelines - Mechanica Imperii

**Version:** 1.0
**Last Updated:** December 2025
**Applies To:** All performance-critical code in Mechanica Imperii

---

## Table of Contents

1. [Performance Philosophy](#performance-philosophy)
2. [Performance Targets](#performance-targets)
3. [Profiling Guidelines](#profiling-guidelines)
4. [Optimization Priorities](#optimization-priorities)
5. [Memory Management](#memory-management)
6. [Cache-Friendly Patterns](#cache-friendly-patterns)
7. [Algorithm Selection](#algorithm-selection)
8. [Benchmarking Standards](#benchmarking-standards)
9. [Common Performance Pitfalls](#common-performance-pitfalls)
10. [Platform-Specific Optimizations](#platform-specific-optimizations)

---

## 1. Performance Philosophy

### Core Principles

**"Premature optimization is the root of all evil" - Donald Knuth**

**Our Approach:**
1. **Correctness First** - Working code before fast code
2. **Measure Before Optimizing** - Profile to find real bottlenecks
3. **Readability Matters** - Clear code is maintainable code
4. **Optimize Hot Paths** - Focus on code that runs frequently
5. **Benchmark Everything** - Verify optimizations actually help

**Decision Framework:**
```
Is it correct? ──NO──> Fix it first
     │
    YES
     │
Is it a bottleneck? ──NO──> Don't optimize
     │
    YES
     │
Can you measure it? ──NO──> Add benchmarks first
     │
    YES
     │
Optimize, then verify improvement
```

---

## 2. Performance Targets

### 2.1 Frame Budget

**Target: 60 FPS (16.67ms per frame)**

Frame time breakdown:
```
Total Frame:     16.67ms (100%)
├─ Game Logic:   10.00ms (60%)
│  ├─ ECS Update:     5.00ms
│  ├─ AI Processing:  3.00ms
│  └─ Game Systems:   2.00ms
├─ Rendering:     5.00ms (30%)
└─ Input/UI:      1.67ms (10%)
```

**Acceptable degradation:**
- 30 FPS (33.33ms) - Acceptable for complex scenarios
- Below 30 FPS - Unacceptable, optimization required

### 2.2 Operation Time Budgets

| Operation | Target | Maximum | Notes |
|-----------|--------|---------|-------|
| **ECS Update (all systems)** | <10ms | <16ms | Per frame |
| **AI Director Update** | <3ms | <5ms | Per frame |
| **Character AI Decision** | <100µs | <500µs | Per character |
| **Province Update** | <20µs | <50µs | Per province (5000 provinces) |
| **Save Game** | <500ms | <1000ms | User-initiated |
| **Load Game** | <1000ms | <2000ms | User-initiated |
| **Pathfinding** | <1ms | <5ms | Per query |
| **Influence Calculation** | <100ms | <200ms | Monthly update |

### 2.3 Memory Targets

**Target Memory Usage:**
- **Baseline:** 500 MB (game initialized, small scenario)
- **Large Scenario:** 2 GB (5000 provinces, 3000 characters, 500 nations)
- **Maximum:** 4 GB (should run on 8GB systems)

**Memory Budget:**
```
Total: 2 GB (large scenario)
├─ ECS Entities/Components:  800 MB (40%)
├─ Map Data:                 400 MB (20%)
├─ AI State:                 300 MB (15%)
├─ Rendering Resources:      300 MB (15%)
└─ Other:                    200 MB (10%)
```

---

## 3. Profiling Guidelines

### 3.1 When to Profile

**Profile before optimizing:**
- ✅ When implementing performance-critical features
- ✅ When frame rate drops below targets
- ✅ Before major releases
- ✅ After significant architectural changes

**Don't profile:**
- ❌ Code that runs once at startup
- ❌ Test/debug code
- ❌ Non-critical UI code

### 3.2 Profiling Tools

**Recommended Tools:**

**Linux:**
```bash
# perf - CPU profiling
perf record -g ./build/MechanicaImperii
perf report

# Valgrind/Callgrind - Detailed profiling
valgrind --tool=callgrind ./build/MechanicaImperii
kcachegrind callgrind.out.*

# heaptrack - Memory profiling
heaptrack ./build/MechanicaImperii
heaptrack_gui heaptrack.*.gz
```

**Windows:**
```powershell
# Visual Studio Profiler
# Performance Profiler > CPU Usage / Memory Usage

# Windows Performance Analyzer (WPA)
# Advanced profiling with ETW traces
```

**Cross-Platform:**
```bash
# Tracy Profiler (recommended for game profiling)
# https://github.com/wolfpld/tracy
# Integrate Tracy macros for frame profiling

# Chrome Tracing
# Generate trace.json for chrome://tracing
```

### 3.3 Profiling Workflow

**Step-by-step profiling:**

1. **Establish Baseline**
   ```bash
   # Run benchmark to get baseline
   ./build/tests/test_ai_director_performance
   # Record: AI Director update: 3.2ms
   ```

2. **Profile to Find Hotspots**
   ```bash
   perf record -g ./build/MechanicaImperii --scenario=large
   perf report
   # Identify: 60% time in CalculateLoyalty()
   ```

3. **Analyze Hot Function**
   ```bash
   # Annotate specific function
   perf annotate CalculateLoyalty
   # Find: 40% time in opinion lookup (unordered_map)
   ```

4. **Optimize**
   ```cpp
   // Replace unordered_map with flat_map for better cache locality
   // Or add caching layer
   ```

5. **Verify Improvement**
   ```bash
   # Re-run benchmark
   ./build/tests/test_ai_director_performance
   # New: AI Director update: 1.8ms (44% improvement)
   ```

6. **Check for Regressions**
   ```bash
   # Run full test suite
   ctest --output-on-failure
   # Verify no correctness regressions
   ```

### 3.4 Profiling Instrumentation

**Add instrumentation points:**

```cpp
#include <chrono>

class ScopedTimer {
public:
    ScopedTimer(const std::string& name) : name_(name), start_(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        CORE_LOG_DEBUG("Performance", name_ + ": " + std::to_string(duration.count()) + "µs");
    }

private:
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_;
};

void AIDirector::Update() {
    ScopedTimer timer("AIDirector::Update");

    {
        ScopedTimer task_timer("AssignTasks");
        AssignTasks();
    }

    {
        ScopedTimer process_timer("ProcessNations");
        ProcessNations();
    }
}
```

---

## 4. Optimization Priorities

### 4.1 Optimization Order

**Focus areas in priority order:**

1. **Algorithmic Complexity** - O(n²) → O(n log n)
2. **Data Structures** - Choose appropriate containers
3. **Memory Access Patterns** - Cache-friendly code
4. **Unnecessary Work** - Eliminate redundant calculations
5. **Compiler Optimizations** - Enable -O3, LTO
6. **Micro-optimizations** - Only after measuring

### 4.2 High-Impact Optimizations

**Common high-value optimizations:**

**Cache Results:**
```cpp
class LoyaltyCalculator {
public:
    double Calculate(EntityID character_id, EntityID liege_id) {
        // Check cache first
        auto cache_key = std::make_pair(character_id, liege_id);
        auto it = loyalty_cache_.find(cache_key);
        if (it != loyalty_cache_.end() && !cache_dirty_) {
            return it->second;  // Fast path: O(1)
        }

        // Slow path: Calculate
        double loyalty = ExpensiveCalculation(character_id, liege_id);
        loyalty_cache_[cache_key] = loyalty;
        return loyalty;
    }

    void InvalidateCache() {
        cache_dirty_ = true;
    }

private:
    std::unordered_map<std::pair<EntityID, EntityID>, double> loyalty_cache_;
    bool cache_dirty_ = false;
};
```

**Batch Processing:**
```cpp
// ❌ BAD: Process one at a time (poor cache utilization)
for (const auto& character_id : characters) {
    ProcessCharacter(character_id);  // Loads character data each time
}

// ✅ GOOD: Batch process (better cache utilization)
auto character_batch = LoadCharacterBatch(characters);
for (auto& character : character_batch) {
    ProcessCharacter(character);  // Data already in cache
}
```

**Avoid Allocations in Hot Paths:**
```cpp
// ❌ BAD: Allocates every call
std::vector<EntityID> GetFriends() const {
    std::vector<EntityID> result;  // Allocation!
    for (const auto& [id, rel] : relationships) {
        if (rel.type == RelationshipType::FRIEND) {
            result.push_back(id);
        }
    }
    return result;
}

// ✅ GOOD: Reuse buffer
void GetFriends(std::vector<EntityID>& out_friends) const {
    out_friends.clear();  // Reuse existing allocation
    for (const auto& [id, rel] : relationships) {
        if (rel.type == RelationshipType::FRIEND) {
            out_friends.push_back(id);
        }
    }
}
```

---

## 5. Memory Management

### 5.1 Object Pooling

**For frequently created/destroyed objects:**

```cpp
template <typename T>
class ObjectPool {
public:
    T* Acquire() {
        if (free_objects_.empty()) {
            return new T();  // Allocate new if pool empty
        }
        T* obj = free_objects_.back();
        free_objects_.pop_back();
        return obj;
    }

    void Release(T* obj) {
        // Reset object state
        obj->Reset();
        free_objects_.push_back(obj);
    }

private:
    std::vector<T*> free_objects_;
};

// Usage for AI tasks (created/destroyed frequently)
ObjectPool<AITask> task_pool;

void AIDirector::AssignTask(EntityID nation_id) {
    AITask* task = task_pool.Acquire();
    task->Initialize(nation_id);
    ProcessTask(task);
    task_pool.Release(task);  // Return to pool instead of delete
}
```

### 5.2 Memory Layout Optimization

**Structure of Arrays (SoA) for cache efficiency:**

```cpp
// ❌ BAD: Array of Structures (AoS) - Poor cache utilization
struct Character {
    EntityID id;
    std::string name;
    int age;
    double loyalty;
    // ... more fields
};
std::vector<Character> characters;

// When processing only loyalty, we load entire Character (cache waste)
for (const auto& character : characters) {
    ProcessLoyalty(character.loyalty);  // Loads entire struct
}

// ✅ GOOD: Structure of Arrays (SoA) - Better cache utilization
struct CharacterData {
    std::vector<EntityID> ids;
    std::vector<std::string> names;
    std::vector<int> ages;
    std::vector<double> loyalties;
};

// Only load loyalty array (cache-friendly)
for (double loyalty : character_data.loyalties) {
    ProcessLoyalty(loyalty);  // Compact, sequential access
}
```

### 5.3 Small String Optimization

**Avoid allocations for small strings:**

```cpp
// Use std::string_view for read-only strings
void ProcessName(std::string_view name) {  // No copy, no allocation
    // ...
}

// Use stack-allocated buffers for formatting
char buffer[64];
std::snprintf(buffer, sizeof(buffer), "Character %d", character_id);
std::string_view name(buffer);  // No heap allocation
```

---

## 6. Cache-Friendly Patterns

### 6.1 Data Locality

**Keep related data together:**

```cpp
// ✅ GOOD: Hot data in one struct, cold data separate
struct CharacterHotData {
    EntityID id;
    double loyalty;
    int opinion;
    uint8_t age;
    // Frequently accessed fields (32 bytes)
};

struct CharacterColdData {
    std::string full_name;
    std::string biography;
    std::vector<Achievement> achievements;
    // Rarely accessed fields
};

// Separate arrays
std::vector<CharacterHotData> hot_data;      // Cache-friendly
std::unordered_map<EntityID, CharacterColdData> cold_data;  // On-demand
```

### 6.2 Sequential Access

**Prefer sequential access over random:**

```cpp
// ❌ BAD: Random access (cache misses)
for (const auto& character_id : character_ids) {
    auto& character = characters[character_id];  // Random lookup
    ProcessCharacter(character);
}

// ✅ GOOD: Sequential access (cache-friendly)
for (auto& character : characters) {
    ProcessCharacter(character);  // Sequential iteration
}
```

### 6.3 Prefetching (Advanced)

**Manual prefetch for predictable access patterns:**

```cpp
// Advanced: Prefetch next iteration's data
for (size_t i = 0; i < characters.size(); ++i) {
    // Prefetch next character while processing current
    if (i + 1 < characters.size()) {
        __builtin_prefetch(&characters[i + 1], 0, 3);  // GCC/Clang
        // _mm_prefetch(&characters[i + 1], _MM_HINT_T0);  // MSVC
    }
    ProcessCharacter(characters[i]);
}
```

---

## 7. Algorithm Selection

### 7.1 Container Choice

**Choose containers based on access patterns:**

| Use Case | Container | Complexity | Notes |
|----------|-----------|------------|-------|
| Sequential iteration | `std::vector` | O(1) access | Best cache locality |
| Frequent insertions/deletions | `std::list` | O(1) insert | Poor cache locality |
| Fast lookup by key | `std::unordered_map` | O(1) avg | More memory |
| Ordered data + lookup | `std::map` | O(log n) | Balanced tree |
| Small sets (<100 items) | `std::vector` + linear search | O(n) | Cache-friendly |
| Fixed-size lookup | `std::array` | O(1) | Stack-allocated |

**Example: Small set optimization:**

```cpp
// For small relationship lists, vector is faster than unordered_map
class CharacterRelationships {
private:
    // Most characters have <10 relationships
    std::vector<std::pair<EntityID, CharacterRelationship>> relationships_;

public:
    const CharacterRelationship* Find(EntityID id) const {
        // Linear search is fast for small N due to cache locality
        for (const auto& [rel_id, rel] : relationships_) {
            if (rel_id == id) return &rel;
        }
        return nullptr;
    }
};
```

### 7.2 Algorithm Complexity

**Choose appropriate algorithms:**

```cpp
// ❌ BAD: O(n²) for large n
for (const auto& char1 : characters) {
    for (const auto& char2 : characters) {
        if (AreRelated(char1, char2)) {  // O(n²)
            // ...
        }
    }
}

// ✅ GOOD: O(n) with hash map
std::unordered_set<std::pair<EntityID, EntityID>> relationships;
for (const auto& character : characters) {
    for (const auto& related : character.GetRelationships()) {  // O(n * avg_relationships)
        ProcessRelationship(character, related);
    }
}
```

---

## 8. Benchmarking Standards

### 8.1 Benchmark Structure

**Performance test example:**

```cpp
#include <chrono>
#include <iostream>

TEST(AIDirectorPerformance, AssignTasks_3000Characters_CompletesUnder100ms) {
    // Arrange: Setup production-scale scenario
    AIDirector director;
    CreateTestCharacters(3000);
    CreateTestNations(100);

    // Warmup (avoid cold cache effects)
    for (int i = 0; i < 10; ++i) {
        director.AssignTasks();
    }

    // Measure: Run multiple iterations
    constexpr int ITERATIONS = 100;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; ++i) {
        director.AssignTasks();
    }

    auto end = std::chrono::high_resolution_clock::now();

    // Calculate average
    auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    auto avg_duration = total_duration.count() / ITERATIONS;

    // Assert: Within budget
    EXPECT_LT(avg_duration, 100000) << "Average time: " << avg_duration << "µs";

    // Report statistics
    std::cout << "AI Director Task Assignment Benchmark\n";
    std::cout << "  Iterations: " << ITERATIONS << "\n";
    std::cout << "  Average: " << avg_duration << "µs\n";
    std::cout << "  Total: " << total_duration.count() << "µs\n";
}
```

### 8.2 Benchmark Baselines

**Establish and track baselines:**

```bash
# Run benchmarks and save results
./build/tests/run_performance_tests > benchmarks/baseline_v0.2.0.txt

# Compare against baseline
./build/tests/run_performance_tests > benchmarks/current.txt
diff benchmarks/baseline_v0.2.0.txt benchmarks/current.txt
```

**Track in CI:**
```yaml
# .github/workflows/performance.yml
- name: Run performance tests
  run: |
    ./build/tests/run_performance_tests > perf_results.txt
    # Upload as artifact for comparison
```

---

## 9. Common Performance Pitfalls

### 9.1 String Operations

**❌ Avoid:**
```cpp
// String concatenation in loop (multiple allocations)
std::string result;
for (const auto& name : names) {
    result += name + ", ";  // Reallocates each time!
}

// Temporary string allocations
std::string message = "Character " + std::to_string(id) + " died";
```

**✅ Prefer:**
```cpp
// Reserve capacity upfront
std::string result;
result.reserve(estimated_size);
for (const auto& name : names) {
    result += name;
    result += ", ";
}

// Use string streams for complex formatting
std::ostringstream oss;
oss << "Character " << id << " died";
std::string message = oss.str();

// Or use format library (C++20)
std::string message = std::format("Character {} died", id);
```

### 9.2 Unnecessary Copies

**❌ Avoid:**
```cpp
// Pass by value (copies entire vector)
void ProcessCharacters(std::vector<Character> characters) {
    // ...
}

// Return large objects by value (may copy)
std::vector<EntityID> GetAllCharacters() {
    return all_characters_;  // May copy
}
```

**✅ Prefer:**
```cpp
// Pass by const reference (no copy)
void ProcessCharacters(const std::vector<Character>& characters) {
    // ...
}

// Return by const reference if object lifetime allows
const std::vector<EntityID>& GetAllCharacters() const {
    return all_characters_;  // No copy
}

// Or use move semantics
std::vector<EntityID> GetAllCharacters() {
    return std::move(all_characters_);  // Move, not copy
}
```

### 9.3 Virtual Function Calls

**❌ Avoid in hot paths:**
```cpp
// Virtual call overhead in tight loop
for (int i = 0; i < 1000000; ++i) {
    entity->Update();  // Virtual dispatch overhead
}
```

**✅ Optimize:**
```cpp
// Cache the concrete type
if (auto* character = dynamic_cast<Character*>(entity)) {
    for (int i = 0; i < 1000000; ++i) {
        character->UpdateCharacter();  // Direct call, no virtual dispatch
    }
}

// Or use CRTP for static polymorphism
template <typename Derived>
class Entity {
    void Update() {
        static_cast<Derived*>(this)->UpdateImpl();  // Static dispatch
    }
};
```

---

## 10. Platform-Specific Optimizations

### 10.1 Compiler Optimizations

**GCC/Clang:**
```cmake
# Release build flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")

# -O3: Aggressive optimizations
# -march=native: Use CPU-specific instructions
# -flto: Link-Time Optimization
```

**MSVC:**
```cmake
# Release build flags
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Oi /GL /DNDEBUG")

# /O2: Maximize speed
# /Oi: Enable intrinsic functions
# /GL: Whole program optimization
```

### 10.2 SIMD Optimizations (Advanced)

**Vectorize calculations:**

```cpp
#include <immintrin.h>  // AVX/SSE intrinsics

// Process 8 floats at once with AVX
void CalculateLoyaltyBatch(const float* opinions, float* loyalties, size_t count) {
    size_t i = 0;

    // Process 8 at a time with AVX
    for (; i + 8 <= count; i += 8) {
        __m256 opinion_vec = _mm256_loadu_ps(&opinions[i]);
        // ... SIMD calculations ...
        _mm256_storeu_ps(&loyalties[i], result);
    }

    // Handle remaining elements
    for (; i < count; ++i) {
        loyalties[i] = CalculateLoyalty(opinions[i]);
    }
}
```

---

## Summary

### Performance Optimization Checklist

Before optimizing:
- [ ] Profile to identify actual bottlenecks
- [ ] Establish baseline measurements
- [ ] Verify code correctness

When optimizing:
- [ ] Choose appropriate algorithms (O(n) vs O(n²))
- [ ] Select efficient data structures
- [ ] Minimize allocations in hot paths
- [ ] Improve cache locality (SoA, sequential access)
- [ ] Cache expensive calculations
- [ ] Batch operations where possible

After optimizing:
- [ ] Benchmark to verify improvement
- [ ] Run full test suite (no regressions)
- [ ] Update performance baselines
- [ ] Document optimization rationale

### Quick Reference

```bash
# Profile CPU usage
perf record -g ./build/MechanicaImperii
perf report

# Profile memory
valgrind --tool=massif ./build/MechanicaImperii
ms_print massif.out.*

# Run performance benchmarks
./build/tests/run_performance_tests

# Enable compiler optimizations
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

---

**For performance questions, consult this guide or profile the code.**

**Version History:**
- v1.0 (December 2025) - Initial performance guidelines
