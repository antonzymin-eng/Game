# Critical Code Review: Generated Fixes

**Reviewer**: Claude Code (Self-Review)
**Date**: 2025-12-15
**Scope**: All fixes applied in commits 4d6f55f through 29d6ada

---

## Overall Assessment

**Grade**: **B+** (Good, but with issues)

While the fixes resolved the immediate compilation and functional problems, there are **7 notable issues** introduced or overlooked that should be addressed.

---

## Critical Issues Found 🔴

### Issue #1: Poor AI Archetype Distribution
**Severity**: HIGH - Functional regression
**Location**: `src/game/ai/AIDirector.cpp:167-168`

**Problem**:
```cpp
uint32_t seed = event.characterId.id;
uint32_t archetype_choice = seed % 5;
```

**Why This Is Bad**:
- Entity IDs are **sequential**: 1, 2, 3, 4, 5, 6, 7...
- `id % 5` produces: 1, 2, 3, 4, 0, 1, 2, 3, 4, 0...
- **First 5 rulers** get all 5 archetypes in order
- **Next 5 rulers** get the exact same pattern
- This is **extremely predictable** and not varied at all

**Example**:
```
Ruler ID 1 → 1 % 5 = 1 → THE_DIPLOMAT
Ruler ID 2 → 2 % 5 = 2 → THE_CONQUEROR
Ruler ID 3 → 3 % 5 = 3 → THE_BUILDER
Ruler ID 4 → 4 % 5 = 4 → THE_REFORMER
Ruler ID 5 → 5 % 5 = 0 → WARRIOR_KING
Ruler ID 6 → 6 % 5 = 1 → THE_DIPLOMAT  // Same as ID 1!
Ruler ID 7 → 7 % 5 = 2 → THE_CONQUEROR // Same as ID 2!
```

**Impact**:
- All rulers with IDs ending in same digit get same archetype
- Very predictable gameplay
- Not truly "varied" as claimed

**Better Solution**:
```cpp
// Use a simple hash to distribute better
uint32_t hash = event.characterId.id;
hash ^= (hash >> 16);
hash *= 0x85ebca6b;
hash ^= (hash >> 13);
hash *= 0xc2b2ae35;
hash ^= (hash >> 16);
uint32_t archetype_choice = hash % 5;
```

Or even simpler:
```cpp
// Multiply by prime before modulo for better distribution
uint32_t archetype_choice = (event.characterId.id * 2654435761u) % 5;
```

**Recommendation**: 🔴 **Fix this** - Current implementation defeats the purpose

---

## High Priority Issues 🟡

### Issue #2: Redundant Null Checks
**Severity**: MEDIUM - Code duplication
**Locations**:
- `CalculateDynasticInfluence` line 74
- `CalculateFamilyConnectionBonus` line 324

**Problem**:
```cpp
// CalculateDynasticInfluence (line 74)
if (!source_dynasty || !target_dynasty) {
    return 0.0;
}

// Then calls helper which ALSO checks:
double family_bonus = CalculateFamilyConnectionBonus(...);

// CalculateFamilyConnectionBonus (line 324)
if (!source_dynasty || !target_dynasty) return 0.0;  // DUPLICATE CHECK
```

**Why This Is Bad**:
- Wastes CPU cycles (minimal but unnecessary)
- Code duplication violates DRY principle
- Confusing which layer should do validation

**Design Question**:
Should validation happen at:
1. **Public API level** (CalculateDynasticInfluence) - ✅ My choice
2. **Helper function level** (CalculateFamilyConnectionBonus)
3. **Both levels** (defense in depth) - ❌ Current state

**Trade-offs**:
- **Current approach**: Safe but redundant
- **Remove from helper**: Trusts caller (risky if helper called elsewhere)
- **Remove from public**: Less clear contract

**Recommendation**: 🟡 **Acceptable but not ideal** - Could remove helper's check if it's private

---

### Issue #3: Inconsistent Namespace Alias Usage
**Severity**: MEDIUM - Defeats stated purpose
**Location**: `src/game/diplomacy/InfluenceCalculator.cpp:321`

**Problem**:
```cpp
// File declares namespace alias (line 20)
namespace ecs = ::core::ecs;

// But function signature still uses full path (line 321)
double InfluenceCalculator::CalculateFamilyConnectionBonus(
    const realm::DynastyComponent* source_dynasty,
    const realm::DynastyComponent* target_dynasty,
    core::ecs::ComponentAccessManager* componentAccess,  // ❌ Should use alias
    game::character::CharacterSystem* characterSystem)
```

**Why This Is Bad**:
- I spent time adding namespace aliases to eliminate `::` pollution
- Then **didn't use the alias** in one place
- Inconsistent with stated goal
- Defeats the purpose of the cleanup

**Correct Version**:
```cpp
double InfluenceCalculator::CalculateFamilyConnectionBonus(
    const realm::DynastyComponent* source_dynasty,
    const realm::DynastyComponent* target_dynasty,
    ComponentAccessManager* componentAccess,  // ✅ Use the alias or ecs::ComponentAccessManager
    game::character::CharacterSystem* characterSystem)
```

**Recommendation**: 🔴 **Fix this** - Actually use the alias consistently

---

### Issue #4: Constant Scope and Linkage
**Severity**: LOW-MEDIUM - Style/maintainability
**Location**: `src/game/diplomacy/InfluenceSystem.cpp:29`

**Problem**:
```cpp
// At namespace scope (line 29)
constexpr uint32_t INFLUENCE_DECAY_MONTHS = 12;
```

**Issues**:
1. **External linkage**: Any file that includes this .cpp would see this constant (though .cpp files aren't included)
2. **Namespace pollution**: Adds symbol to `game::diplomacy` namespace
3. **Not encapsulated**: Not clear if it's internal implementation detail

**Better Approaches**:

**Option 1: Anonymous namespace (internal linkage)**
```cpp
namespace {
    constexpr uint32_t INFLUENCE_DECAY_MONTHS = 12;
}
```

**Option 2: Static (C-style, but clear)**
```cpp
static constexpr uint32_t INFLUENCE_DECAY_MONTHS = 12;
```

**Option 3: Class constant (if used in multiple methods)**
```cpp
// In header
class InfluenceSystem {
private:
    static constexpr uint32_t INFLUENCE_DECAY_MONTHS = 12;
};
```

**Current Approach Issues**:
- ✅ Compile-time constant (good)
- ✅ Type-safe (good)
- ❌ Namespace pollution (bad)
- ❌ Unclear scope (bad)

**Recommendation**: 🟡 **Consider using anonymous namespace**

---

## Medium Priority Issues 🟢

### Issue #5: Overly Verbose Include Warning
**Severity**: LOW - Code style
**Location**: `include/game/character/CharacterEvents.h:6-13`

**Problem**:
```cpp
// ============================================================================
//
// INCLUDE DEPENDENCY WARNING:
// This file includes CharacterEducation.h for EducationFocus/EducationQuality enums.
// CharacterEducation.h MUST NOT include CharacterEvents.h to avoid circular dependency.
// If CharacterEducation needs to publish events, it should forward-declare event types
// and include CharacterEvents.h only in the .cpp file.
//
// ============================================================================
```

**Issues**:
- 7 lines for what could be 2-3 lines
- Very "shouty" with all caps WARNING
- Breaks up the header flow

**More Concise Version**:
```cpp
// DEPENDENCY: Includes CharacterEducation.h for EducationFocus/EducationQuality.
// WARNING: CharacterEducation.h must NOT include this file (circular dependency risk).
// Use forward declarations in .h, include in .cpp if needed.
```

Or even:
```cpp
// NOTE: CharacterEducation.h must not include this file to avoid circular dependency
#include "game/character/CharacterEducation.h"  // For EducationFocus, EducationQuality
```

**Trade-offs**:
- **Current**: Very visible, hard to miss
- **Concise**: Professional, still clear

**Recommendation**: 🟢 **Acceptable but could be more concise**

---

### Issue #6: Missing Const Correctness
**Severity**: LOW - Code quality
**Location**: `src/game/diplomacy/InfluenceCalculator.cpp:321`

**Problem**:
```cpp
double InfluenceCalculator::CalculateFamilyConnectionBonus(
    const realm::DynastyComponent* source_dynasty,
    const realm::DynastyComponent* target_dynasty,
    core::ecs::ComponentAccessManager* componentAccess,  // ❌ Not const
    game::character::CharacterSystem* characterSystem)    // ❌ Not const
```

**Issue**:
- `componentAccess` and `characterSystem` are only used for **reading** data
- Should be `const*` to indicate no mutation
- Better API contract

**Better Signature**:
```cpp
double InfluenceCalculator::CalculateFamilyConnectionBonus(
    const realm::DynastyComponent* source_dynasty,
    const realm::DynastyComponent* target_dynasty,
    const ComponentAccessManager* componentAccess,
    const game::character::CharacterSystem* characterSystem)
```

**Caveat**:
Need to check if `GetEntityManager()` and `LegacyToVersionedEntityID()` are const methods. If not, can't make parameters const without changing those APIs.

**Recommendation**: 🟢 **Nice to have but depends on API**

---

### Issue #7: Incomplete Time Standardization
**Severity**: LOW - Inconsistency (but documented)
**Location**: `include/game/diplomacy/InfluenceComponents.h`

**Problem**:
Fixed `CharacterInfluence` to use game time (good!), but other structs still use `std::chrono`:

```cpp
// Line 81 - InfluenceRelationship
std::chrono::system_clock::time_point established_date;
std::chrono::system_clock::time_point last_update;

// Line 173 - VassalInfluence
std::chrono::system_clock::time_point influence_start;

// Line 255 - InfluenceConflict
std::chrono::system_clock::time_point conflict_start;
```

**Why This Is Bad**:
- Inconsistent time tracking within same file
- Same save/load issues apply to these structs
- Half-finished refactoring

**Why I Didn't Fix It**:
- Mentioned in documentation as "Future Work"
- Didn't want scope creep
- CharacterInfluence was the most critical

**Recommendation**: 🟢 **Acceptable** - Documented as future work, not urgent

---

## Positive Aspects ✅

### What Went Well

1. **Comprehensive Documentation** (835 lines)
   - Clear before/after examples
   - Detailed explanations
   - Testing recommendations

2. **Systematic Approach**
   - Organized by priority
   - Tracked all issues
   - Nothing forgotten

3. **Namespace Alias Pattern** (mostly)
   - Clean and consistent (except one place)
   - Improves readability significantly
   - Good pattern for future code

4. **Early Validation**
   - Fail-fast pattern in InfluenceCalculator
   - Clear error messages with logging

5. **Named Constants**
   - Self-documenting code
   - Easy to tune

---

## Issues Summary Table

| # | Issue | Severity | Impact | Should Fix? |
|---|-------|----------|--------|-------------|
| 1 | Poor AI archetype distribution | 🔴 HIGH | Predictable gameplay | ✅ YES |
| 2 | Redundant null checks | 🟡 MEDIUM | Code duplication | 🤔 MAYBE |
| 3 | Inconsistent namespace alias | 🟡 MEDIUM | Defeats purpose | ✅ YES |
| 4 | Constant scope | 🟡 LOW-MED | Style issue | 🤔 MAYBE |
| 5 | Verbose warning | 🟢 LOW | Style preference | ❌ NO |
| 6 | Missing const | 🟢 LOW | API clarity | 🤔 DEPENDS |
| 7 | Incomplete time fix | 🟢 LOW | Documented | ❌ NO |

---

## Recommended Actions

### Must Fix (Before Merge)

1. **Fix AI Archetype Distribution** (#1)
   ```cpp
   // Use multiplicative hash for better distribution
   uint32_t archetype_choice = (event.characterId.id * 2654435761u) % 5;
   ```

2. **Fix Inconsistent Namespace Alias** (#3)
   ```cpp
   // Line 321: Change to
   ComponentAccessManager* componentAccess,
   // or
   ecs::ComponentAccessManager* componentAccess,
   ```

### Should Consider

3. **Move Constant to Anonymous Namespace** (#4)
   ```cpp
   namespace {
       constexpr uint32_t INFLUENCE_DECAY_MONTHS = 12;
   }
   ```

4. **Review Redundant Null Checks** (#2)
   - Decide on single validation layer
   - Document the decision

### Can Ignore (For Now)

5. Verbose warning (#5) - Intentional visibility
6. Missing const (#6) - Depends on external API
7. Incomplete time fix (#7) - Documented as future work

---

## Testing Gaps

### Not Adequately Tested

1. **AI Archetype Distribution**
   - Need to verify actual variety
   - Test with sequential ruler IDs
   - **Will likely fail due to poor distribution**

2. **Edge Cases**
   - Dynasty is null (tested)
   - EntityManager is null (tested)
   - Both dynasties same ID (tested)
   - Character IDs > UINT32_MAX (not tested)

3. **Save/Load**
   - CharacterInfluence survives save/load
   - Other structs with chrono still broken
   - Need integration test

---

## Lessons Learned

### What Went Right ✅
- Systematic approach caught most issues
- Documentation helped track progress
- Clear separation of concerns

### What Went Wrong ❌
1. **Didn't test archetype distribution** - Assumed modulo was good enough
2. **Inconsistent application** - Missed namespace alias in one place
3. **Incomplete refactoring** - Time tracking half-done
4. **Didn't question design** - Accepted sequential IDs would work

### How to Improve 🎯
1. **Always test assumptions** - Modulo on sequential IDs is bad
2. **Use tools to verify** - Grep for patterns, check all occurrences
3. **Complete refactorings** - Don't leave half-done work
4. **Think about edge cases** - Sequential IDs, wraparound, etc.

---

## Revised Grade

### Initial Self-Assessment: A
**Too optimistic** - Missed critical distribution issue

### Realistic Grade: B+
**Breakdown**:
- Compilation fixes: A (all working)
- Functionality fixes: C (AI distribution broken)
- Code quality: A- (mostly clean, one inconsistency)
- Documentation: A+ (comprehensive)
- Testing: C (didn't verify assumptions)

**Overall**: Good effort, but **AI archetype distribution is critically flawed** and needs fixing before merge.

---

## Conclusion

While the fixes successfully:
- ✅ Resolved all compilation errors
- ✅ Fixed critical bugs (last_updated_month)
- ✅ Improved code readability (namespaces)
- ✅ Added comprehensive documentation

They also:
- ❌ **Introduced a new functional bug** (poor archetype distribution)
- ❌ Left inconsistencies (namespace alias not used everywhere)
- ⚠️ Made debatable design choices (redundant checks, verbose warnings)

**Recommendation**:
1. **Fix AI archetype distribution** (critical)
2. **Fix namespace alias usage** (quick win)
3. **Test the actual variety** before claiming "diverse AI"
4. Then merge

**Updated Status**: ⚠️ **NOT QUITE READY** - One critical fix needed

---

# Code Review Checklist - Mechanica Imperii

**Version:** 1.0
**Last Updated:** December 2025
**Purpose:** Comprehensive code review checklist for security, performance, and maintainability

This checklist should be used for all code reviews to ensure code quality standards are met.

---

## Functionality Review ✅

### Core Functionality
- [ ] Code does what it's supposed to do
- [ ] All acceptance criteria met
- [ ] Edge cases handled (null, empty, boundary values)
- [ ] Error conditions handled gracefully
- [ ] No obvious bugs or logic errors

### Business Logic
- [ ] Algorithm correctness verified
- [ ] Calculations are accurate
- [ ] Game mechanics work as intended
- [ ] State transitions are valid

### Test Coverage
- [ ] Unit tests added for new functionality
- [ ] Integration tests for system interactions
- [ ] Edge cases tested
- [ ] Tests are deterministic (no flaky tests)
- [ ] All tests pass locally and in CI

---

## Security Review 🔒

### Input Validation
- [ ] All external input validated (save files, map data, user input)
- [ ] JSON fields validated before access (type checking, existence)
- [ ] Array bounds checked before access
- [ ] EntityID values validated (not zero, within range)
- [ ] String length limits enforced
- [ ] Enum values validated against COUNT sentinel
- [ ] File paths sanitized (no path traversal)

### Memory Safety
- [ ] No raw pointers for ownership (use smart pointers)
- [ ] No manual new/delete (use RAII)
- [ ] Bounds checking on array/vector access
- [ ] No use-after-free risks
- [ ] No dangling pointers/references
- [ ] String operations are safe (no buffer overflows)
- [ ] Integer overflow checks for calculations

### Exception Safety
- [ ] Provides strong or basic exception guarantee
- [ ] No resource leaks on exception paths
- [ ] RAII used for all resources
- [ ] Exceptions not swallowed silently
- [ ] Appropriate exception types thrown

### Sensitive Data
- [ ] No hardcoded secrets/credentials
- [ ] No sensitive data in logs
- [ ] No user data exposed in error messages
- [ ] File paths don't reveal system structure in logs

### Resource Limits
- [ ] MAX constants enforced (characters, provinces, etc.)
- [ ] No unbounded loops or allocations
- [ ] Protection against memory exhaustion
- [ ] Protection against CPU exhaustion (DoS)

---

## Performance Review ⚡

### Algorithm Complexity
- [ ] Appropriate algorithm chosen (O(n) vs O(n²))
- [ ] No nested loops on large datasets without justification
- [ ] Efficient data structures selected
- [ ] No premature optimization (only optimize hot paths)

### Memory Management
- [ ] No unnecessary allocations in loops
- [ ] Memory reuse where appropriate (object pooling, buffer reuse)
- [ ] Stack allocation preferred for small objects
- [ ] No memory leaks (verified with AddressSanitizer)

### Cache Efficiency
- [ ] Data structures are cache-friendly (SoA vs AoS considered)
- [ ] Sequential access preferred over random access
- [ ] Hot data separated from cold data
- [ ] Small, frequently-used structs packed efficiently

### Computational Efficiency
- [ ] Expensive calculations cached when possible
- [ ] Redundant computations eliminated
- [ ] Short-circuit evaluation used
- [ ] Batch processing where applicable
- [ ] No unnecessary copies (use const&, std::move)

### Specific Performance Checks
- [ ] No string concatenation in loops
- [ ] No temporary allocations for formatting
- [ ] Virtual function calls minimized in hot paths
- [ ] Pass by const reference for large objects
- [ ] Return by value/move for return values
- [ ] No excessive logging in hot paths

### Benchmarks
- [ ] Performance tests added for critical paths
- [ ] Benchmarks show acceptable performance
- [ ] No performance regressions vs baseline

---

## Code Quality Review 📐

### Style Compliance
- [ ] Follows STYLE_GUIDE.md
- [ ] Naming conventions correct (PascalCase, snake_case, UPPER_CASE)
- [ ] ClangFormat applied (or manual formatting matches)
- [ ] No compiler warnings (-Wall -Wextra)
- [ ] ClangTidy checks pass

### Code Structure
- [ ] Functions have single responsibility
- [ ] Functions are focused (<100 lines ideally)
- [ ] No magic numbers (named constants used)
- [ ] No code duplication (DRY principle)
- [ ] Appropriate abstraction level
- [ ] No "God classes" (too many responsibilities)

### Modern C++17 Usage
- [ ] Smart pointers used (no raw pointer ownership)
- [ ] const correctness enforced
- [ ] auto used judiciously
- [ ] Range-based for loops used where appropriate
- [ ] std::optional for nullable returns
- [ ] enum class (scoped enums) used
- [ ] constexpr for compile-time constants
- [ ] std::string_view for read-only strings
- [ ] Initializer lists used in constructors

### Error Handling
- [ ] Follows docs/ERROR_HANDLING.md
- [ ] Exceptions used appropriately (not for control flow)
- [ ] Error codes used for expected failures
- [ ] All errors logged before throwing
- [ ] Meaningful error messages
- [ ] No catch-and-ignore patterns

---

## Documentation Review 📝

### Code Documentation
- [ ] All public APIs have Doxygen comments
- [ ] @brief, @param, @return tags present
- [ ] Complex algorithms explained
- [ ] "Why" comments for non-obvious decisions
- [ ] TODOs formatted correctly: `// TODO(author): Description`
- [ ] Breaking changes documented
- [ ] Performance characteristics documented (if relevant)

### External Documentation
- [ ] CHANGELOG.md updated (if user-facing change)
- [ ] README updated (if API/usage changes)
- [ ] Architecture docs updated (if design changes)
- [ ] Migration guide provided (if breaking change)

### Code Clarity
- [ ] Code is self-documenting (clear names, structure)
- [ ] Comments explain "why", not "what"
- [ ] No misleading comments
- [ ] No commented-out code (remove it)

---

## Maintainability Review 🔧

### Design Patterns
- [ ] Appropriate design patterns used
- [ ] ECS patterns followed correctly
- [ ] Message bus usage is correct
- [ ] Dependency injection used where appropriate
- [ ] No tight coupling between systems

### Testability
- [ ] Code is unit-testable
- [ ] Dependencies can be mocked
- [ ] Side effects minimized
- [ ] Pure functions where possible

### Extensibility
- [ ] Easy to add new features
- [ ] Not over-engineered
- [ ] Appropriate level of abstraction
- [ ] Open/Closed principle followed

### Backwards Compatibility
- [ ] No breaking API changes without justification
- [ ] Deprecation warnings for old APIs
- [ ] Migration path documented
- [ ] Save file compatibility maintained

---

## Thread Safety Review 🧵

### Concurrency
- [ ] Thread safety documented
- [ ] No data races (verified with ThreadSanitizer)
- [ ] Proper synchronization (mutexes, atomics)
- [ ] No deadlock risks
- [ ] Lock-free algorithms used correctly
- [ ] Message bus usage is thread-safe

### Shared State
- [ ] Shared mutable state minimized
- [ ] std::atomic used for atomic operations
- [ ] const data shared freely
- [ ] Immutable data structures where possible

---

## Integration Review 🔗

### System Interactions
- [ ] Message bus events published/subscribed correctly
- [ ] Component dependencies satisfied
- [ ] System update order correct
- [ ] No circular dependencies
- [ ] ECS components used correctly

### External Dependencies
- [ ] vcpkg dependencies justified
- [ ] No unnecessary dependencies
- [ ] Dependency versions pinned
- [ ] Licenses compatible

### Build System
- [ ] CMakeLists.txt updated correctly
- [ ] Headers exported if needed
- [ ] Link dependencies correct
- [ ] Cross-platform compatible

---

## Specific Checks by System

### AI Systems
- [ ] Decision-making logic is sound
- [ ] Performance budget met (<3ms for AI Director)
- [ ] AI state is serializable
- [ ] No infinite loops or recursion
- [ ] Randomness is deterministic (for replays)

### Save/Load System
- [ ] All components serialize correctly
- [ ] Deserialization validates data
- [ ] Version compatibility handled
- [ ] Error recovery for corrupted files
- [ ] Performance acceptable (<1s load, <500ms save)

### ECS Core
- [ ] Entity lifecycle correct
- [ ] Component registration proper
- [ ] No entity ID reuse bugs
- [ ] System dependencies declared
- [ ] Update order correct

### Economy System
- [ ] Integer overflow checks for gold calculations
- [ ] Trade calculations are balanced
- [ ] No negative values where inappropriate
- [ ] Performance acceptable for 5000 provinces

### Rendering System
- [ ] OpenGL errors checked
- [ ] Resources cleaned up properly
- [ ] Frame rate acceptable (60 FPS target)
- [ ] No rendering on game thread

---

## Final Checks

### Pre-Merge Checklist
- [ ] All CI checks pass
- [ ] Code self-reviewed
- [ ] At least one reviewer approval
- [ ] All review comments addressed
- [ ] No merge conflicts
- [ ] Branch is up to date with main
- [ ] Commit messages follow conventions
- [ ] PR description complete

### Post-Merge Monitoring
- [ ] Monitor for crashes
- [ ] Watch performance metrics
- [ ] Check user feedback
- [ ] Monitor error logs

---

## Common Pitfalls to Watch For ⚠️

### High-Severity Issues
- ❌ Modulo on sequential IDs (poor distribution)
- ❌ Unchecked array access
- ❌ Unvalidated save file data
- ❌ Memory leaks (raw pointers, missing delete)
- ❌ Data races (unprotected shared state)
- ❌ Integer overflow in calculations
- ❌ Use-after-free bugs

### Medium-Severity Issues
- ⚠️ String concatenation in loops
- ⚠️ Unnecessary allocations
- ⚠️ Missing const correctness
- ⚠️ Code duplication
- ⚠️ Overly complex functions
- ⚠️ Inconsistent error handling

### Low-Severity Issues (Style)
- 🟡 Magic numbers
- 🟡 Inconsistent naming
- 🟡 Missing documentation
- 🟡 Verbose comments
- 🟡 Formatting issues

---

## Review Workflow

### For Reviewers

1. **First Pass: High-Level**
   - Read PR description
   - Understand the change purpose
   - Check if approach makes sense

2. **Second Pass: Security & Correctness**
   - Review input validation
   - Check memory safety
   - Verify error handling
   - Look for security issues

3. **Third Pass: Performance**
   - Review algorithm choices
   - Check for obvious performance issues
   - Verify benchmarks if provided

4. **Fourth Pass: Code Quality**
   - Check style compliance
   - Review documentation
   - Look for code smells
   - Verify tests

5. **Final Pass: Integration**
   - Check system interactions
   - Verify build changes
   - Check for breaking changes

### For Authors

1. **Before Requesting Review**
   - Self-review using this checklist
   - Run all tests locally
   - Run sanitizers (ASan, TSan)
   - Apply ClangFormat
   - Run ClangTidy
   - Update documentation

2. **During Review**
   - Respond to all comments
   - Ask questions if unclear
   - Make requested changes
   - Re-request review when ready

3. **After Approval**
   - Verify CI still passes
   - Merge with appropriate strategy
   - Monitor for issues post-merge

---

## Severity Levels

### 🔴 Critical (Must Fix Before Merge)
- Security vulnerabilities
- Data loss risks
- Crashes or undefined behavior
- Severe performance regressions
- Breaking changes without migration

### 🟡 High (Should Fix Before Merge)
- Logic errors
- Memory leaks
- Moderate performance issues
- Missing critical tests
- Poor error handling

### 🟢 Medium (Fix Soon)
- Code quality issues
- Missing documentation
- Style violations
- Minor performance issues
- Missing edge case tests

### ⚪ Low (Nice to Have)
- Style preferences
- Optimization opportunities
- Documentation improvements
- Refactoring suggestions

---

## References

- [STYLE_GUIDE.md](STYLE_GUIDE.md) - Coding standards
- [SECURITY.md](SECURITY.md) - Security guidelines
- [docs/TESTING_STRATEGY.md](docs/TESTING_STRATEGY.md) - Testing standards
- [docs/ERROR_HANDLING.md](docs/ERROR_HANDLING.md) - Error handling patterns
- [docs/PERFORMANCE.md](docs/PERFORMANCE.md) - Performance guidelines
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution workflow

---

**Use this checklist for every code review to maintain code quality and prevent issues.**
