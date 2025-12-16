# C++ Code Style Guide - Mechanica Imperii

**Version:** 1.0
**Last Updated:** December 2025
**Applies To:** All C++17 code in Mechanica Imperii project

---

## Table of Contents

1. [Naming Conventions](#naming-conventions)
2. [Formatting Standards](#formatting-standards)
3. [File Organization](#file-organization)
4. [Documentation Standards](#documentation-standards)
5. [Modern C++17 Practices](#modern-c17-practices)
6. [Code Quality Principles](#code-quality-principles)
7. [Examples](#examples)

---

## 1. Naming Conventions

### 1.1 General Principles
- **Be descriptive**: Names should clearly communicate purpose
- **Avoid abbreviations** unless widely understood (e.g., `id`, `AI`, `ECS`)
- **Use full words**: `character_count` not `char_cnt`

### 1.2 Classes, Structs, and Types

**Style:** PascalCase

```cpp
class CharacterRelationshipsComponent;
struct Marriage;
struct CharacterRelationship;
enum class MarriageType;
using EntityID = uint32_t;
```

### 1.3 Functions and Methods

**Style:** PascalCase

```cpp
void AddMarriage(EntityID spouse_id);
bool IsMarriedTo(EntityID other_char) const;
std::vector<EntityID> GetFriends() const;
double CalculateLoyalty(EntityID character_id);
```

**Naming Guidelines:**
- Verbs or verb phrases for actions: `Calculate`, `Update`, `Process`
- Boolean functions start with `Is`, `Has`, `Can`, `Should`: `IsMarriedTo`, `HasChildren`
- Getters use `Get` prefix: `GetFriends()`, `GetBondStrength()`
- Setters use `Set` prefix: `SetRelationship()`
- Modifiers use action verbs: `ModifyBondStrength()`, `AddChild()`

### 1.4 Variables and Members

**Style:** snake_case

```cpp
int character_id;
double bond_strength;
std::chrono::system_clock::time_point marriage_date;
types::EntityID current_spouse;
std::vector<EntityID> children;
```

**Member Variables:**
- No Hungarian notation
- No prefixes (no `m_`, `_`, etc.)
- Use meaningful names that indicate purpose

### 1.5 Constants and Enumerators

**Constants:** UPPER_SNAKE_CASE

```cpp
static constexpr double MIN_BOND_STRENGTH = 0.0;
static constexpr double MAX_BOND_STRENGTH = 100.0;
static constexpr int MAX_PROVINCES = 5000;
```

**Enum Values:** UPPER_CASE

```cpp
enum class MarriageType : uint8_t {
    NORMAL,
    MATRILINEAL,
    POLITICAL,
    SECRET,
    MORGANATIC,
    COUNT  // Sentinel value for enum size
};
```

### 1.6 Namespaces

**Style:** lowercase, may use underscores for multi-word

```cpp
namespace game {
namespace character {
    // Character-related code
}
}

namespace core {
namespace ecs {
    // ECS framework code
}
}
```

**Guidelines:**
- Use nested namespaces for logical organization
- Never use `using namespace` in headers
- Prefer explicit namespace qualification in implementation files for clarity

### 1.7 File Names

**Style:** Match the primary class name

```cpp
CharacterRelationships.h    // Header for CharacterRelationshipsComponent
CharacterRelationships.cpp  // Implementation
EconomySystem.h             // Header for EconomySystem
```

**Guidelines:**
- Use PascalCase for filenames
- Header extension: `.h`
- Implementation extension: `.cpp`
- Test files: `test_` prefix with descriptive name (e.g., `test_character_relationships.cpp`)

---

## 2. Formatting Standards

### 2.1 Indentation

- **Use 4 spaces** (no tabs)
- Indent each level consistently
- Configure your editor to convert tabs to spaces

```cpp
class MyClass {
public:
    void MyFunction() {
        if (condition) {
            DoSomething();
        }
    }
};
```

### 2.2 Line Length

- **Maximum 120 characters** per line
- Break long lines logically at natural boundaries
- Align continuation lines for readability

```cpp
// Good: Function parameters broken across lines
void AddMarriage(types::EntityID spouse_id,
                 types::EntityID spouse_realm,
                 types::EntityID spouse_dynasty,
                 bool creates_alliance = false);

// Good: Long conditions
if (character.bond_strength >= SIGNIFICANT_BOND_THRESHOLD &&
    character.is_active &&
    character.type == RelationshipType::FRIEND) {
    // ...
}
```

### 2.3 Braces and Spacing

**Opening Brace Style:** Same line (K&R style)

```cpp
// Classes, functions, control structures
class MyClass {
    void MyFunction() {
        if (condition) {
            // code
        } else {
            // code
        }
    }
};

// Exception: Namespace braces on new line is acceptable
namespace game {
namespace character {

}  // namespace character
}  // namespace game
```

**Spacing:**
```cpp
// Spaces after control keywords
if (condition)
for (auto& item : container)
while (running)

// No spaces inside parentheses
DoSomething(arg1, arg2);  // Good
DoSomething( arg1, arg2 );  // Bad

// Spaces around operators
int result = a + b * c;
bool valid = x > 0 && y < 100;

// No space before function calls
Calculate();  // Good
Calculate ();  // Bad
```

### 2.4 Blank Lines and Visual Separation

```cpp
// Use blank lines to separate logical sections
class CharacterRelationshipsComponent {
public:
    // Marriage management
    void AddMarriage(...);
    bool IsMarriedTo(...) const;

    // Relationship management
    void SetRelationship(...);
    std::optional<CharacterRelationship> GetRelationship(...) const;

private:
    std::vector<Marriage> marriages;
    std::unordered_map<EntityID, CharacterRelationship> relationships;
};
```

**Section Separators:** Use consistent comment blocks

```cpp
// ============================================================================
// Marriage Management
// ============================================================================

// ========================================================================
// Relationship Management
// ========================================================================
```

### 2.5 Include Order

**Standard Order:**
1. Related header (for .cpp files)
2. C system headers
3. C++ standard library headers
4. Third-party library headers
5. Project headers

```cpp
// CharacterRelationships.cpp
#include "game/character/CharacterRelationships.h"  // Related header first

#include <chrono>        // C++ standard library
#include <optional>
#include <vector>

#include <json/json.h>   // Third-party

#include "core/ECS/IComponent.h"  // Project headers
#include "core/types/game_types.h"
```

**Alphabetize** within each section.

---

## 3. File Organization

### 3.1 Header File Structure

```cpp
// ============================================================================
// FileName.h - Brief description of purpose
// Created: Date - Context or phase
// Location: include/path/to/FileName.h
// ============================================================================

#pragma once

// Includes (in order: C, C++ std, third-party, project)
#include <vector>
#include "core/types/game_types.h"

namespace game {
namespace subsystem {

// Forward declarations (if needed)
class SomeClass;

// Enums and constants
enum class MyEnum : uint8_t {
    VALUE_ONE,
    VALUE_TWO
};

// Main class declaration
class MyClass {
public:
    // Public constants
    static constexpr int MAX_VALUE = 100;

    // Constructors and destructors
    MyClass();
    explicit MyClass(int value);
    ~MyClass() = default;

    // Public methods (grouped logically)
    void DoSomething();
    int GetValue() const;

private:
    // Private methods
    void InternalHelper();

    // Member variables (at end)
    int value_;
    std::vector<int> data_;
};

}  // namespace subsystem
}  // namespace game
```

### 3.2 Implementation File Structure

```cpp
// Created: Date
// Location: src/path/to/FileName.cpp
// Purpose: Brief description

#include "path/to/FileName.h"

#include <algorithm>
#include "other/dependencies.h"

namespace game {
namespace subsystem {

// Anonymous namespace for file-local helpers
namespace {

// Helper functions only used in this file
void InternalHelper() {
    // ...
}

}  // anonymous namespace

// Class implementation
MyClass::MyClass() : value_(0) {
    // ...
}

void MyClass::DoSomething() {
    // ...
}

}  // namespace subsystem
}  // namespace game
```

### 3.3 Header Guards

**Use `#pragma once`** (modern, simpler, less error-prone)

```cpp
#pragma once

// Header content
```

**Avoid traditional include guards** unless compiler compatibility requires:
```cpp
// Only if #pragma once not supported
#ifndef GAME_CHARACTER_RELATIONSHIPS_H
#define GAME_CHARACTER_RELATIONSHIPS_H

// Header content

#endif  // GAME_CHARACTER_RELATIONSHIPS_H
```

---

## 4. Documentation Standards

### 4.1 File Headers

Every file should have a header comment:

```cpp
// ============================================================================
// CharacterRelationships.h - Character Marriage and Friendship System
// Created: November 15, 2025 - Phase 3: Influence System Integration
// Location: include/game/character/CharacterRelationships.h
// ============================================================================
```

### 4.2 Doxygen Comments for Public APIs

**Use Doxygen-style comments** for all public classes, methods, and functions:

```cpp
/**
 * @brief Calculates character loyalty based on relationships
 *
 * Loyalty is influenced by opinion, traits, culture, and relationship bonds.
 * The calculation uses the bond strength with the liege and applies modifiers
 * based on shared culture and dynasty ties.
 *
 * @param character_id The character whose loyalty to calculate
 * @param liege_id The liege to whom loyalty is calculated
 * @return Loyalty value between 0.0 and 100.0
 * @throws std::invalid_argument if character_id or liege_id is invalid
 *
 * @note This method is performance-critical and called every game tick
 * @see ModifyLoyalty() for direct loyalty modifications
 */
double CalculateLoyalty(EntityID character_id, EntityID liege_id);
```

**Doxygen Tags to Use:**
- `@brief` - Short one-line description
- `@param` - Parameter description (include name, purpose)
- `@return` - Return value description
- `@throws` - Exceptions that may be thrown
- `@note` - Additional important information
- `@warning` - Critical warnings about usage
- `@see` - Cross-references to related functions
- `@example` - Usage example for complex APIs

### 4.3 Inline Comments

**Comment the "Why", Not the "What"**

```cpp
// Bad: Obvious comment
// Increment counter
counter++;

// Good: Explains reasoning
// Use bond strength threshold to filter insignificant relationships
// that don't meaningfully impact diplomatic calculations
if (bond_strength >= SIGNIFICANT_BOND_THRESHOLD) {
    AddToInfluenceNetwork(character);
}
```

**Document Breaking Changes and Design Decisions:**

```cpp
/**
 * Get all rivals of this character
 *
 * BREAKING CHANGE (Phase 3): Now filters by bond strength threshold.
 * Only returns rivals with significant bond strength (>= 25.0).
 * For all rivalries regardless of strength, use GetAllRivals().
 *
 * @return Characters with RIVAL relationship >= SIGNIFICANT_BOND_THRESHOLD
 */
std::vector<EntityID> GetRivals() const;
```

### 4.4 TODO/FIXME Format

```cpp
// TODO(username): Brief description of what needs to be done
// FIXME(username): Brief description of bug or issue
// NOTE(username): Important information for future developers
// HACK(username): Explanation of why this workaround exists

// Example:
// TODO(alice): Implement decay for inactive friendships after 5 years
// FIXME(bob): Marriage validation doesn't check for incest properly
// NOTE(charlie): This threshold was chosen after playtest balancing
// HACK(dave): Workaround for JsonCpp's handling of int64 - remove when upgraded
```

### 4.5 Complex Algorithm Documentation

For non-trivial algorithms, provide context:

```cpp
/**
 * Calculate influence propagation using modified PageRank algorithm
 *
 * Algorithm Overview:
 * 1. Build friendship graph with bond strength as edge weights
 * 2. Apply iterative PageRank with damping factor 0.85
 * 3. Normalize scores by network size
 * 4. Cache results for 30 game days (performance optimization)
 *
 * Performance: O(n * m) where n = characters, m = avg friendships
 *
 * References:
 * - Page et al. (1998) "The PageRank Citation Ranking"
 * - Adapted for weighted, undirected graphs
 */
void CalculateInfluenceNetwork();
```

---

## 5. Modern C++17 Practices

### 5.1 Use Smart Pointers

**Prefer RAII and smart pointers over raw pointers:**

```cpp
// Good: Unique ownership
std::unique_ptr<Character> character = std::make_unique<Character>();

// Good: Shared ownership (use sparingly)
std::shared_ptr<Province> province = std::make_shared<Province>();

// Avoid: Raw pointers (only for non-owning references)
Character* raw_ptr = character.get();  // OK for non-owning
```

### 5.2 Use `const` Liberally

```cpp
// Const member functions
std::vector<EntityID> GetFriends() const;  // Doesn't modify state

// Const parameters
void Process(const Character& character);  // Won't modify argument

// Const variables
const double MAX_LOYALTY = 100.0;
```

### 5.3 Prefer `auto` for Complex Types

```cpp
// Good: Auto for iterators and complex types
auto it = relationships.find(character_id);
auto relationship = GetRelationship(other_char);

// Good: Explicit types for clarity
int count = 0;
double loyalty = 50.0;
EntityID id = 123;

// Avoid: Auto for simple types (reduces clarity)
auto x = 5;  // Better: int x = 5;
```

### 5.4 Use Range-Based For Loops

```cpp
// Good: Range-based for
for (const auto& marriage : marriages) {
    ProcessMarriage(marriage);
}

// Good: Structured bindings (C++17)
for (const auto& [char_id, relationship] : relationships) {
    UpdateBondStrength(char_id, relationship.bond_strength);
}

// Avoid: Traditional for loop unless index needed
for (size_t i = 0; i < marriages.size(); ++i) {
    ProcessMarriage(marriages[i]);  // Verbose
}
```

### 5.5 Use `std::optional` for Nullable Returns

```cpp
// Good: Optional for values that may not exist
std::optional<CharacterRelationship> GetRelationship(EntityID other_char) const {
    auto it = relationships.find(other_char);
    if (it != relationships.end()) {
        return it->second;
    }
    return std::nullopt;
}

// Usage
if (auto rel = GetRelationship(char_id)) {
    // Use rel.value() or *rel
    ProcessRelationship(*rel);
}
```

### 5.6 Use Enum Classes (Scoped Enums)

```cpp
// Good: Type-safe enum class
enum class MarriageType : uint8_t {
    NORMAL,
    MATRILINEAL,
    POLITICAL
};

MarriageType type = MarriageType::NORMAL;  // Must use scope

// Avoid: Unscoped enums
enum MarriageTypeOld {
    NORMAL,  // Pollutes global namespace
    MATRILINEAL
};
```

### 5.7 Use `constexpr` for Compile-Time Constants

```cpp
// Good: Compile-time constants
static constexpr double MIN_BOND_STRENGTH = 0.0;
static constexpr int MAX_CHARACTERS = 3000;

// Compile-time functions
constexpr int Square(int x) {
    return x * x;
}
```

### 5.8 Prefer STL Algorithms Over Raw Loops

```cpp
// Good: STL algorithms
auto it = std::find_if(marriages.begin(), marriages.end(),
    [spouse_id](const Marriage& m) { return m.spouse == spouse_id; });

// Good: Count with predicate
int friend_count = std::count_if(relationships.begin(), relationships.end(),
    [](const auto& pair) { return pair.second.type == RelationshipType::FRIEND; });

// Use: Algorithms improve readability and intent
```

### 5.9 Use Initializer Lists

```cpp
// Good: Member initializer lists (efficient)
Marriage::Marriage(EntityID spouse_id, EntityID realm, EntityID dynasty)
    : spouse(spouse_id)
    , realm_of_spouse(realm)
    , spouse_dynasty(dynasty)
    , marriage_date(std::chrono::system_clock::now())
{}

// Good: Uniform initialization
std::vector<int> values{1, 2, 3, 4, 5};
Character character{id, name, birth_date};
```

### 5.10 Default and Delete Special Members

```cpp
class MyClass {
public:
    // Explicitly default constructors
    MyClass() = default;
    ~MyClass() = default;

    // Delete unwanted operations
    MyClass(const MyClass&) = delete;
    MyClass& operator=(const MyClass&) = delete;

    // Enable move semantics
    MyClass(MyClass&&) = default;
    MyClass& operator=(MyClass&&) = default;
};
```

---

## 6. Code Quality Principles

### 6.1 Single Responsibility Principle

Each class/function should have one clear purpose:

```cpp
// Good: Focused responsibility
class CharacterLoyaltyCalculator {
    double Calculate(EntityID character_id, EntityID liege_id);
};

class CharacterRelationshipManager {
    void AddFriend(EntityID char1, EntityID char2);
    void RemoveFriend(EntityID char1, EntityID char2);
};

// Avoid: God classes with too many responsibilities
```

### 6.2 Avoid Magic Numbers

```cpp
// Bad: Magic numbers
if (bond_strength > 25.0) { ... }

// Good: Named constants
static constexpr double SIGNIFICANT_BOND_THRESHOLD = 25.0;
if (bond_strength > SIGNIFICANT_BOND_THRESHOLD) { ... }
```

### 6.3 Keep Functions Short and Focused

```cpp
// Good: Small, focused functions
void ProcessCharacter(Character& character) {
    UpdateRelationships(character);
    CalculateLoyalty(character);
    UpdateInfluence(character);
}

// Avoid: 200-line functions doing everything
```

### 6.4 Error Handling

```cpp
// Use exceptions for exceptional conditions
if (!IsValidCharacterId(id)) {
    throw std::invalid_argument("Invalid character ID: " + std::to_string(id));
}

// Use std::optional for nullable values
std::optional<Character> FindCharacter(EntityID id);

// Use error codes for expected failures (e.g., file I/O)
bool SaveGame(const std::string& filename);
```

### 6.5 Const Correctness

```cpp
class CharacterRelationships {
public:
    // Const methods don't modify state
    std::vector<EntityID> GetFriends() const;
    bool IsMarriedTo(EntityID other) const;

    // Non-const methods modify state
    void AddMarriage(EntityID spouse);
    void ModifyBondStrength(EntityID other, double delta);

private:
    // Mutable for caching in const methods (use sparingly)
    mutable std::optional<std::vector<EntityID>> cached_friends_;
};
```

---

## 7. Examples

### 7.1 Complete Class Example

```cpp
// ============================================================================
// LoyaltyCalculator.h - Character Loyalty Calculation System
// Created: December 2025 - Economy Phase
// Location: include/game/character/LoyaltyCalculator.h
// ============================================================================

#pragma once

#include "core/types/game_types.h"
#include <optional>

namespace game {
namespace character {

/**
 * @brief Calculates and manages character loyalty to their liege
 *
 * This class implements the loyalty calculation algorithm that considers:
 * - Opinion of liege
 * - Cultural and religious alignment
 * - Relationship bonds (friendships, marriages)
 * - Trait modifiers
 * - Historical events
 */
class LoyaltyCalculator {
public:
    // Loyalty thresholds for gameplay
    static constexpr double MIN_LOYALTY = 0.0;
    static constexpr double MAX_LOYALTY = 100.0;
    static constexpr double REBELLION_THRESHOLD = 25.0;
    static constexpr double LOYAL_THRESHOLD = 75.0;

    LoyaltyCalculator() = default;
    ~LoyaltyCalculator() = default;

    /**
     * @brief Calculate loyalty of a character to their liege
     *
     * @param character_id The vassal character
     * @param liege_id The liege character
     * @return Loyalty value between MIN_LOYALTY and MAX_LOYALTY
     * @throws std::invalid_argument if either ID is invalid
     */
    double Calculate(types::EntityID character_id, types::EntityID liege_id) const;

    /**
     * @brief Check if character is likely to rebel
     *
     * @param character_id The character to evaluate
     * @return true if loyalty is below REBELLION_THRESHOLD
     */
    bool IsLikelyToRebel(types::EntityID character_id) const;

private:
    /**
     * @brief Calculate opinion modifier to loyalty
     *
     * Opinion impact is non-linear: negative opinion has stronger effect
     * than positive opinion to make rebellions more likely.
     */
    double CalculateOpinionModifier(int opinion) const;

    double CalculateCultureModifier(types::EntityID char_id,
                                    types::EntityID liege_id) const;
    double CalculateRelationshipModifier(types::EntityID char_id,
                                         types::EntityID liege_id) const;
};

}  // namespace character
}  // namespace game
```

### 7.2 Complete Implementation Example

```cpp
// Created: December 2025
// Location: src/game/character/LoyaltyCalculator.cpp
// Purpose: Implementation of loyalty calculation system

#include "game/character/LoyaltyCalculator.h"

#include <algorithm>
#include <cmath>
#include "core/ecs/EntityManager.h"
#include "game/character/CharacterRelationships.h"

namespace game {
namespace character {

namespace {

// Anonymous namespace for internal constants
constexpr double OPINION_WEIGHT = 0.4;
constexpr double CULTURE_WEIGHT = 0.3;
constexpr double RELATIONSHIP_WEIGHT = 0.3;

/**
 * Clamp value between min and max
 */
double Clamp(double value, double min, double max) {
    return std::max(min, std::min(max, value));
}

}  // anonymous namespace

double LoyaltyCalculator::Calculate(types::EntityID character_id,
                                    types::EntityID liege_id) const {
    // Validate inputs
    if (character_id == 0 || liege_id == 0) {
        throw std::invalid_argument("Invalid character or liege ID");
    }

    // Calculate component modifiers
    double opinion_mod = CalculateOpinionModifier(GetOpinion(character_id, liege_id));
    double culture_mod = CalculateCultureModifier(character_id, liege_id);
    double relationship_mod = CalculateRelationshipModifier(character_id, liege_id);

    // Weighted sum
    double loyalty = (opinion_mod * OPINION_WEIGHT) +
                     (culture_mod * CULTURE_WEIGHT) +
                     (relationship_mod * RELATIONSHIP_WEIGHT);

    // Clamp to valid range
    return Clamp(loyalty, MIN_LOYALTY, MAX_LOYALTY);
}

bool LoyaltyCalculator::IsLikelyToRebel(types::EntityID character_id) const {
    auto loyalty = Calculate(character_id, GetLiege(character_id));
    return loyalty < REBELLION_THRESHOLD;
}

double LoyaltyCalculator::CalculateOpinionModifier(int opinion) const {
    // Non-linear scaling: negative opinion has stronger impact
    // This creates interesting rebellion dynamics
    if (opinion < 0) {
        return 50.0 + (opinion * 0.75);  // Negative opinions hurt more
    } else {
        return 50.0 + (opinion * 0.5);   // Positive opinions help less
    }
}

}  // namespace character
}  // namespace game
```

---

## References and Additional Resources

### Official Style Guides
- **Google C++ Style Guide**: https://google.github.io/styleguide/cppguide.html
- **LLVM Coding Standards**: https://llvm.org/docs/CodingStandards.html
- **C++ Core Guidelines**: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines

### C++17 Resources
- **cppreference.com**: Comprehensive C++ reference
- **Modern C++ Features**: https://github.com/AnthonyCalandra/modern-cpp-features

### Tools
- **ClangFormat**: Automated code formatting (see `.clang-format`)
- **Clang-Tidy**: Static analysis and linting (see `.clang-tidy`)
- **Doxygen**: API documentation generation (see `docs/Doxyfile`)

---

## Enforcement

This style guide is enforced through:
1. **Automated formatting** - ClangFormat on save/commit
2. **Static analysis** - Clang-Tidy checks in CI pipeline
3. **Code reviews** - All PRs reviewed against this guide
4. **CI/CD checks** - Automated style validation

For questions or clarifications, refer to existing codebase examples or consult the development team.

---

**Document Version History:**
- v1.0 (December 2025) - Initial style guide based on project conventions
