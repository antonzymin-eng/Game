# API Documentation Guide - Mechanica Imperii

**Version:** 1.0
**Last Updated:** December 2025

This document describes the API documentation system for Mechanica Imperii, including how to generate and contribute to API documentation.

---

## Table of Contents

1. [Documentation System](#documentation-system)
2. [Generating Documentation](#generating-documentation)
3. [Documentation Standards](#documentation-standards)
4. [Doxygen Syntax Reference](#doxygen-syntax-reference)
5. [Module Organization](#module-organization)
6. [Examples](#examples)
7. [Best Practices](#best-practices)

---

## 1. Documentation System

### Overview

Mechanica Imperii uses **Doxygen** for API documentation generation. Doxygen extracts documentation from specially formatted comments in source code and generates HTML documentation.

**Documentation Coverage:**
- All public classes and structs
- All public member functions
- All public enumerations
- Module/namespace organization
- Usage examples for complex APIs

**Generated Output:**
- HTML documentation (browsable)
- Class diagrams
- Collaboration diagrams
- File dependency graphs
- Cross-referenced source code

---

## 2. Generating Documentation

### 2.1 Prerequisites

**Install Doxygen:**

```bash
# Ubuntu/Debian
sudo apt-get install doxygen graphviz

# macOS
brew install doxygen graphviz

# Windows
# Download from: https://www.doxygen.nl/download.html
# Also install Graphviz: https://graphviz.org/download/
```

**Optional:**
- **Graphviz** - For generating diagrams (recommended)
- **LaTeX** - For PDF documentation (optional)

### 2.2 Generate HTML Documentation

```bash
# From project root directory
doxygen Doxyfile

# Output generated in: docs/api/html/
# Open in browser:
open docs/api/html/index.html        # macOS
xdg-open docs/api/html/index.html   # Linux
start docs/api/html/index.html      # Windows
```

### 2.3 CMake Integration

**Add documentation target to CMakeLists.txt:**

```cmake
# Find Doxygen
find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(DOXYGEN_FOUND)
    # Configure Doxyfile
    set(DOXYGEN_IN ${CMAKE_SOURCE_DIR}/Doxyfile)
    set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    # Add documentation target
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )

    message(STATUS "Doxygen found. Run 'make docs' to generate documentation.")
else()
    message(STATUS "Doxygen not found. API documentation will not be available.")
endif()
```

**Build documentation:**
```bash
cmake --build build --target docs
```

### 2.4 Continuous Documentation

**GitHub Actions (Optional):**

```yaml
# .github/workflows/docs.yml
name: Documentation

on:
  push:
    branches: [ main ]

jobs:
  docs:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install Doxygen
        run: sudo apt-get install doxygen graphviz
      - name: Generate documentation
        run: doxygen Doxyfile
      - name: Deploy to GitHub Pages
        uses: peaceiris/actions-gh-pages@v3
        with:
          github_token: ${{ secrets.GITHUB_TOKEN }}
          publish_dir: ./docs/api/html
```

---

## 3. Documentation Standards

### 3.1 What to Document

**Always document:**
- ✅ Public classes and structs
- ✅ Public member functions
- ✅ Public enumerations and constants
- ✅ Template parameters
- ✅ Function parameters and return values
- ✅ Exceptions that may be thrown
- ✅ Performance characteristics (if relevant)
- ✅ Thread safety guarantees

**Don't document:**
- ❌ Private implementation details (unless complex)
- ❌ Obvious getters/setters (unless behavior is non-trivial)
- ❌ Auto-generated code
- ❌ Test code (use regular comments instead)

### 3.2 Documentation Location

**Header files (.h):**
```cpp
// include/game/character/CharacterRelationships.h

/**
 * @file CharacterRelationships.h
 * @brief Character marriage and friendship system
 * @author Mechanica Imperii Team
 * @date November 2025
 */

#pragma once

namespace game {
namespace character {

/**
 * @brief Manages character relationships including marriages and friendships
 *
 * This component tracks all relationships for a character, including:
 * - Marriages (current and historical)
 * - Friendships with bond strength
 * - Rivalries
 * - Family relationships
 *
 * Relationships are bidirectional and must be maintained consistently
 * across both characters involved.
 *
 * @note This is an ECS component and should be accessed through EntityManager
 */
class CharacterRelationshipsComponent {
    // ...
};

}  // namespace character
}  // namespace game
```

**Implementation files (.cpp):**
- Implementation details (if complex)
- Algorithm explanations
- Performance notes

---

## 4. Doxygen Syntax Reference

### 4.1 Basic Comment Blocks

**C++ style (preferred):**
```cpp
/**
 * @brief Brief description (one line)
 *
 * Detailed description can span multiple lines
 * and provide additional context.
 *
 * @param param1 Description of first parameter
 * @param param2 Description of second parameter
 * @return Description of return value
 */
ReturnType FunctionName(Type1 param1, Type2 param2);
```

**Alternative style:**
```cpp
/// @brief Brief description
/// Detailed description
/// @param param1 First parameter
/// @return Return value
ReturnType FunctionName(Type1 param1);
```

### 4.2 Common Doxygen Tags

| Tag | Purpose | Example |
|-----|---------|---------|
| `@file` | File description | `@file CharacterRelationships.h` |
| `@brief` | Brief description | `@brief Calculate loyalty value` |
| `@param` | Parameter description | `@param character_id The character to process` |
| `@return` | Return value description | `@return Loyalty between 0.0 and 100.0` |
| `@throws` | Exception documentation | `@throws std::invalid_argument if ID invalid` |
| `@note` | Additional notes | `@note This function is thread-safe` |
| `@warning` | Warnings | `@warning Do not call from multiple threads` |
| `@see` | Cross-reference | `@see GetLoyalty()` |
| `@todo` | TODO items | `@todo Implement caching` |
| `@deprecated` | Deprecated APIs | `@deprecated Use CalculateLoyalty() instead` |
| `@since` | Version introduced | `@since v0.2.0` |
| `@pre` | Precondition | `@pre character_id must be valid` |
| `@post` | Postcondition | `@post Result is normalized [0,1]` |
| `@example` | Usage example | `@example examples/character_loyalty.cpp` |

### 4.3 Code Examples in Documentation

```cpp
/**
 * @brief Find character by ID
 *
 * @code
 * // Usage example
 * auto character = FindCharacter(EntityID{123});
 * if (character) {
 *     ProcessCharacter(*character);
 * } else {
 *     std::cerr << "Character not found\n";
 * }
 * @endcode
 *
 * @param id Character entity ID
 * @return Optional character (nullopt if not found)
 */
std::optional<Character> FindCharacter(EntityID id);
```

### 4.4 Lists in Documentation

**Bulleted lists:**
```cpp
/**
 * @brief Calculate influence based on relationships
 *
 * This function considers:
 * - Direct friendships (bond strength weighted)
 * - Marriage alliances
 * - Shared cultural ties
 * - Historical interactions
 *
 * @param character_id Character to evaluate
 * @return Influence score
 */
```

**Numbered lists:**
```cpp
/**
 * @brief Process character death
 *
 * Death processing follows these steps:
 * 1. Remove from active character list
 * 2. Transfer titles to heirs
 * 3. Update relationship networks
 * 4. Trigger AI notifications
 * 5. Record in historical events
 *
 * @param character_id Character that died
 */
```

### 4.5 Grouping and Modules

**Define modules:**
```cpp
/**
 * @defgroup character Character System
 * @brief Character management and relationships
 *
 * The character system manages all character-related data including:
 * - Character attributes (age, traits, skills)
 * - Relationships (marriages, friendships, rivalries)
 * - Family trees and dynasties
 * - Character AI decision-making
 */

/**
 * @ingroup character
 * @brief Character relationship component
 */
class CharacterRelationshipsComponent {
    // ...
};
```

**Group related functions:**
```cpp
class CharacterRelationships {
public:
    /// @name Marriage Management
    /// @{

    /** @brief Add new marriage */
    void AddMarriage(EntityID spouse_id);

    /** @brief Check if married to specific person */
    bool IsMarriedTo(EntityID other) const;

    /** @brief Get all spouses */
    std::vector<EntityID> GetSpouses() const;

    /// @}

    /// @name Friendship Management
    /// @{

    /** @brief Set relationship type */
    void SetRelationship(EntityID other, RelationshipType type);

    /** @brief Get friendship bond strength */
    double GetFriendshipBondStrength(EntityID other) const;

    /// @}
};
```

---

## 5. Module Organization

### 5.1 Recommended Module Structure

**Top-level modules:**

```cpp
/**
 * @defgroup core Core Systems
 * @brief Core engine functionality
 */

/**
 * @defgroup game Game Systems
 * @brief Gameplay logic and simulation
 */

/**
 * @defgroup render Rendering
 * @brief Graphics and visualization
 */

/**
 * @defgroup ui User Interface
 * @brief UI components and interactions
 */
```

**Submodules:**

```cpp
/**
 * @defgroup ecs Entity Component System
 * @ingroup core
 * @brief ECS architecture
 */

/**
 * @defgroup character Character System
 * @ingroup game
 * @brief Character management
 */

/**
 * @defgroup economy Economy System
 * @ingroup game
 * @brief Economic simulation
 */
```

### 5.2 Namespace Documentation

```cpp
/**
 * @namespace game
 * @brief Game simulation systems
 */
namespace game {

/**
 * @namespace game::character
 * @brief Character-related functionality
 */
namespace character {
    // Character classes
}

/**
 * @namespace game::economy
 * @brief Economic simulation
 */
namespace economy {
    // Economy classes
}

}  // namespace game
```

---

## 6. Examples

### 6.1 Complete Class Documentation

```cpp
/**
 * @file LoyaltyCalculator.h
 * @brief Character loyalty calculation system
 * @ingroup character
 */

#pragma once

#include "core/types/game_types.h"
#include <optional>

namespace game {
namespace character {

/**
 * @brief Calculates and manages character loyalty to their liege
 *
 * The loyalty system determines how likely a vassal is to remain loyal
 * to their liege or rebel. Loyalty is influenced by multiple factors:
 *
 * **Factors Affecting Loyalty:**
 * - Opinion of liege (-100 to +100)
 * - Cultural alignment (same culture provides bonus)
 * - Religious alignment (same faith provides bonus)
 * - Relationship bonds (friendships, marriages)
 * - Character traits (loyal, ambitious, etc.)
 * - Recent events (wars, gifts, insults)
 *
 * **Performance:**
 * Loyalty calculation is O(1) with cached relationship data.
 * Cache invalidated when opinion or relationships change.
 *
 * **Thread Safety:**
 * This class is not thread-safe. Access must be synchronized externally.
 *
 * @code
 * // Example usage
 * LoyaltyCalculator calculator;
 * double loyalty = calculator.Calculate(vassal_id, liege_id);
 * if (loyalty < LoyaltyCalculator::REBELLION_THRESHOLD) {
 *     HandlePotentialRebellion(vassal_id);
 * }
 * @endcode
 *
 * @see CharacterRelationships
 * @see OpinionSystem
 *
 * @since v0.1.0
 */
class LoyaltyCalculator {
public:
    /// Minimum loyalty value
    static constexpr double MIN_LOYALTY = 0.0;

    /// Maximum loyalty value
    static constexpr double MAX_LOYALTY = 100.0;

    /// Threshold below which rebellion is likely
    static constexpr double REBELLION_THRESHOLD = 25.0;

    /// Threshold above which character is considered loyal
    static constexpr double LOYAL_THRESHOLD = 75.0;

    /**
     * @brief Default constructor
     */
    LoyaltyCalculator() = default;

    /**
     * @brief Calculate loyalty of a character to their liege
     *
     * Computes loyalty based on opinion, culture, religion, and relationships.
     * Result is cached until invalidated by state changes.
     *
     * **Algorithm:**
     * 1. Calculate opinion modifier (weighted 40%)
     * 2. Calculate culture modifier (weighted 30%)
     * 3. Calculate relationship modifier (weighted 30%)
     * 4. Sum weighted modifiers
     * 5. Clamp to [MIN_LOYALTY, MAX_LOYALTY]
     *
     * @param character_id The vassal character (must be valid)
     * @param liege_id The liege character (must be valid)
     *
     * @return Loyalty value between MIN_LOYALTY and MAX_LOYALTY
     *
     * @throws std::invalid_argument if either ID is invalid (zero or not found)
     *
     * @pre character_id != 0
     * @pre liege_id != 0
     * @post MIN_LOYALTY <= result <= MAX_LOYALTY
     *
     * @note This method is called every game tick for all vassals
     * @warning Expensive if called without caching for large realms
     *
     * @see InvalidateCache()
     */
    double Calculate(types::EntityID character_id,
                    types::EntityID liege_id) const;

    /**
     * @brief Check if character is likely to rebel
     *
     * Convenience method that checks if loyalty is below REBELLION_THRESHOLD.
     *
     * @param character_id The character to evaluate
     * @return true if loyalty < REBELLION_THRESHOLD, false otherwise
     *
     * @see Calculate()
     */
    bool IsLikelyToRebel(types::EntityID character_id) const;

    /**
     * @brief Invalidate cached loyalty values
     *
     * Call this when opinion or relationships change to force recalculation.
     *
     * @param character_id Optional character ID to invalidate specific cache.
     *                     If nullopt, invalidates entire cache.
     */
    void InvalidateCache(std::optional<types::EntityID> character_id = std::nullopt);

private:
    /**
     * @brief Calculate opinion modifier to loyalty
     *
     * Opinion impact is non-linear: negative opinion has stronger effect
     * than positive opinion to create interesting rebellion dynamics.
     *
     * **Formula:**
     * - If opinion < 0: 50.0 + (opinion * 0.75)
     * - If opinion >= 0: 50.0 + (opinion * 0.5)
     *
     * @param opinion Character's opinion of liege [-100, +100]
     * @return Opinion modifier [0.0, 100.0]
     */
    double CalculateOpinionModifier(int opinion) const;

    /**
     * @brief Calculate culture modifier to loyalty
     *
     * @param char_id Vassal character ID
     * @param liege_id Liege character ID
     * @return Culture modifier [0.0, 100.0]
     */
    double CalculateCultureModifier(types::EntityID char_id,
                                   types::EntityID liege_id) const;

    /**
     * @brief Calculate relationship modifier to loyalty
     *
     * Friendships and marriages with liege increase loyalty.
     *
     * @param char_id Vassal character ID
     * @param liege_id Liege character ID
     * @return Relationship modifier [0.0, 100.0]
     */
    double CalculateRelationshipModifier(types::EntityID char_id,
                                        types::EntityID liege_id) const;

    /// Cached loyalty values (character_id -> loyalty)
    mutable std::unordered_map<types::EntityID, double> loyalty_cache_;

    /// Cache validity flag
    mutable bool cache_valid_ = false;
};

}  // namespace character
}  // namespace game
```

### 6.2 Function Template Documentation

```cpp
/**
 * @brief Find entity component of specific type
 *
 * Template function to retrieve a component from an entity.
 *
 * @tparam T Component type (must inherit from IComponent)
 *
 * @param entity_id Entity to query
 *
 * @return Pointer to component if exists, nullptr otherwise
 *
 * @code
 * auto* comp = FindComponent<CharacterRelationshipsComponent>(entity_id);
 * if (comp) {
 *     auto friends = comp->GetFriends();
 * }
 * @endcode
 */
template <typename T>
T* FindComponent(EntityID entity_id) {
    static_assert(std::is_base_of_v<IComponent, T>,
                  "T must inherit from IComponent");
    // Implementation
}
```

### 6.3 Enumeration Documentation

```cpp
/**
 * @brief Types of character relationships
 *
 * Defines the various relationship types that can exist between characters.
 * Each type has different gameplay effects and bond strength requirements.
 */
enum class RelationshipType : uint8_t {
    /**
     * Close friendship
     *
     * Requirements:
     * - Bond strength >= 25.0
     * - Positive opinion (>= 0)
     *
     * Effects:
     * - +10 opinion modifier
     * - Increased diplomatic cooperation
     * - AI favors character in decisions
     */
    FRIEND,

    /**
     * Personal rivalry
     *
     * Requirements:
     * - Bond strength >= 25.0
     * - Negative opinion (< 0)
     *
     * Effects:
     * - -20 opinion modifier
     * - Decreased diplomatic cooperation
     * - AI opposes character in decisions
     */
    RIVAL,

    /**
     * Romantic relationship
     *
     * Requirements:
     * - Bond strength >= 50.0
     * - Compatible orientations
     *
     * Effects:
     * - +25 opinion modifier
     * - May lead to marriage
     */
    LOVER,

    /** Character count sentinel (not a valid type) */
    COUNT
};
```

---

## 7. Best Practices

### 7.1 Writing Good Documentation

**✅ DO:**
```cpp
/**
 * @brief Calculate trade route profit based on distance and goods value
 *
 * Profit calculation considers:
 * - Distance between provinces (longer = more profit)
 * - Goods base value
 * - Trade efficiency modifiers (technology, infrastructure)
 * - Tariffs and taxes
 *
 * Formula: profit = (goods_value * distance_modifier * efficiency) - taxes
 *
 * @param route_id Trade route identifier
 * @return Profit in gold ducats, or 0 if route is unprofitable
 *
 * @note Performance: O(1) - values are cached
 * @see UpdateTradeRoutes() to recalculate all routes
 */
double CalculateTradeProfit(TradeRouteID route_id) const;
```

**❌ DON'T:**
```cpp
/**
 * Calculates profit
 * @param route_id route id
 * @return profit
 */
double CalculateTradeProfit(TradeRouteID route_id) const;  // Too vague!
```

### 7.2 Keeping Documentation Updated

**When code changes:**
1. ✅ Update documentation immediately
2. ✅ Review related documentation for impact
3. ✅ Add `@since` tags for new features
4. ✅ Mark deprecated APIs with `@deprecated`
5. ✅ Update examples if behavior changes

**Deprecation example:**
```cpp
/**
 * @brief Get all rivals (legacy method)
 *
 * @deprecated Since v0.3.0. Use GetSignificantRivals() instead.
 *             This method will be removed in v1.0.
 *
 * @return All characters with RIVAL relationship type
 *
 * @see GetSignificantRivals() for new API
 */
[[deprecated("Use GetSignificantRivals() instead")]]
std::vector<EntityID> GetRivals() const;
```

### 7.3 Cross-Referencing

**Link related APIs:**
```cpp
/**
 * @brief Calculate character loyalty
 *
 * @see OpinionSystem for opinion calculation
 * @see CharacterRelationships for relationship data
 * @see CultureSystem for cultural modifiers
 */
```

### 7.4 Performance Documentation

**Document complexity:**
```cpp
/**
 * @brief Find shortest path between provinces
 *
 * Uses Dijkstra's algorithm for pathfinding.
 *
 * **Complexity:**
 * - Time: O((V + E) log V) where V = provinces, E = borders
 * - Space: O(V)
 *
 * **Performance Notes:**
 * - Results are cached for frequently used paths
 * - Cache invalidated when map changes
 *
 * @param start Start province
 * @param end End province
 * @return Vector of province IDs forming path, empty if no path exists
 */
std::vector<ProvinceID> FindPath(ProvinceID start, ProvinceID end);
```

---

## Summary

### Documentation Checklist

When documenting APIs:

- [ ] File header with @file, @brief, @author
- [ ] Class/struct with @brief and detailed description
- [ ] All public methods documented
- [ ] @param for each parameter
- [ ] @return for non-void functions
- [ ] @throws for exceptions
- [ ] Usage @code examples for complex APIs
- [ ] Performance notes (@note) if relevant
- [ ] Thread safety documentation
- [ ] Cross-references (@see) to related APIs
- [ ] Module grouping (@ingroup)

### Quick Reference

```bash
# Generate documentation
doxygen Doxyfile

# View generated docs
open docs/api/html/index.html
```

---

**For questions about API documentation, consult this guide or the Doxygen manual: https://www.doxygen.nl/manual/**

**Version History:**
- v1.0 (December 2025) - Initial API documentation guide
