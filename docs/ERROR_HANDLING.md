# Error Handling Standards - Mechanica Imperii

**Version:** 1.0
**Last Updated:** December 2025
**Applies To:** All C++17 code in Mechanica Imperii project

---

## Table of Contents

1. [Error Handling Philosophy](#error-handling-philosophy)
2. [Exception Policy](#exception-policy)
3. [Error Codes vs Exceptions](#error-codes-vs-exceptions)
4. [Logging Standards](#logging-standards)
5. [Error Recovery Strategies](#error-recovery-strategies)
6. [Assertions and Contracts](#assertions-and-contracts)
7. [System-Specific Error Handling](#system-specific-error-handling)
8. [Error Handling Patterns](#error-handling-patterns)
9. [Testing Error Paths](#testing-error-paths)

---

## 1. Error Handling Philosophy

### Core Principles

**Fail Fast, Fail Safe**
- Detect errors as early as possible
- Prevent propagation of invalid state
- Graceful degradation when possible
- Never silently ignore errors

**User Experience First**
- Users should never see crashes
- Error messages should be actionable
- Save game data before critical failures
- Log detailed diagnostics for developers

**Defense in Depth**
```
Layer 1: Input Validation    → Prevent errors at boundaries
Layer 2: Assertions          → Catch programmer errors early (debug)
Layer 3: Exceptions          → Handle exceptional conditions
Layer 4: Error Codes         → Handle expected failures
Layer 5: Logging             → Record all errors for diagnosis
Layer 6: Crash Handler       → Last resort for unrecoverable errors
```

---

## 2. Exception Policy

### 2.1 When to Use Exceptions

**✅ Use exceptions for:**
- Truly exceptional conditions (not expected to occur regularly)
- Constructor failures (no return value)
- Unrecoverable errors requiring stack unwinding
- Violation of contracts/preconditions
- Resource allocation failures
- Deserialization errors from untrusted data

**❌ Don't use exceptions for:**
- Expected failures (file not found, network timeout)
- Control flow (instead of if/else)
- Performance-critical hot paths
- Optional/nullable return values
- Simple validation failures

### 2.2 Exception Types

**Standard Library Exceptions (Prefer These):**
```cpp
#include <stdexcept>

// Logic errors (programmer mistakes)
std::invalid_argument  - Invalid function argument
std::logic_error       - General logic error
std::out_of_range      - Array/vector access out of bounds

// Runtime errors (external conditions)
std::runtime_error     - General runtime error
std::bad_alloc         - Memory allocation failed
std::system_error      - OS/system call failed
```

**Example - Input Validation:**
```cpp
void CharacterRelationships::SetRelationship(EntityID other_char,
                                             RelationshipType type,
                                             int opinion,
                                             double bond) {
    // Validate inputs - throw on programmer error
    if (other_char == 0) {
        throw std::invalid_argument("Cannot set relationship with invalid character ID");
    }

    if (opinion < MIN_OPINION || opinion > MAX_OPINION) {
        throw std::out_of_range("Opinion must be between " +
                                std::to_string(MIN_OPINION) + " and " +
                                std::to_string(MAX_OPINION));
    }

    // Normal operation
    // ...
}
```

### 2.3 Custom Exceptions (When Needed)

**Create custom exceptions for domain-specific errors:**

```cpp
namespace game {

// Base exception for all game errors
class GameException : public std::runtime_error {
public:
    explicit GameException(const std::string& message)
        : std::runtime_error(message) {}
};

// Specific game exceptions
class SaveFileCorruptedException : public GameException {
public:
    explicit SaveFileCorruptedException(const std::string& filename)
        : GameException("Save file corrupted: " + filename)
        , filename_(filename) {}

    const std::string& GetFilename() const { return filename_; }

private:
    std::string filename_;
};

class EntityNotFoundException : public GameException {
public:
    explicit EntityNotFoundException(EntityID id)
        : GameException("Entity not found: " + std::to_string(id))
        , entity_id_(id) {}

    EntityID GetEntityID() const { return entity_id_; }

private:
    EntityID entity_id_;
};

}  // namespace game
```

### 2.4 Exception Safety Guarantees

**Provide strong or basic exception safety:**

```cpp
class CharacterRelationshipsComponent {
public:
    // Strong guarantee: Either succeeds completely or leaves state unchanged
    void AddMarriage(EntityID spouse_id, EntityID realm, EntityID dynasty) {
        // Validate first (may throw, but doesn't modify state)
        if (spouse_id == 0) {
            throw std::invalid_argument("Invalid spouse ID");
        }

        // Create new marriage (may throw, but local variable)
        Marriage marriage(spouse_id, realm, dynasty);

        // Commit change (no-throw operation)
        marriages.push_back(std::move(marriage));
        current_spouse = spouse_id;
    }

    // Basic guarantee: State is valid but may be changed if throws
    void ModifyBondStrength(EntityID other_char, double delta) {
        auto it = relationships.find(other_char);
        if (it == relationships.end()) {
            throw EntityNotFoundException(other_char);
        }

        // State modified even if clamping throws (basic guarantee)
        it->second.bond_strength += delta;
        it->second.bond_strength = Clamp(it->second.bond_strength,
                                         MIN_BOND, MAX_BOND);
    }
};
```

### 2.5 Catching Exceptions

**Catch by const reference:**
```cpp
try {
    ProcessCharacter(character_id);
} catch (const EntityNotFoundException& e) {
    // Handle specific exception
    CORE_LOG_ERROR("CharacterSystem",
                   "Character not found: " + std::to_string(e.GetEntityID()));
    return false;
} catch (const GameException& e) {
    // Handle general game exceptions
    CORE_LOG_ERROR("CharacterSystem", "Game error: " + std::string(e.what()));
    return false;
} catch (const std::exception& e) {
    // Catch-all for standard exceptions
    CORE_LOG_ERROR("CharacterSystem", "Unexpected error: " + std::string(e.what()));
    return false;
}
```

**Never catch and ignore:**
```cpp
// ❌ BAD: Silent failure
try {
    SaveGame(filename);
} catch (...) {
    // Ignored - user thinks game saved but it didn't!
}

// ✅ GOOD: Log and inform user
try {
    SaveGame(filename);
} catch (const std::exception& e) {
    CORE_LOG_ERROR("SaveSystem", "Failed to save game: " + std::string(e.what()));
    ShowUserError("Could not save game. Please try again.");
    return false;
}
```

---

## 3. Error Codes vs Exceptions

### 3.1 When to Use Error Codes

**✅ Use error codes (return values) for:**
- Expected failures that occur during normal operation
- Performance-critical code paths
- Optional operations where failure is acceptable
- C-style APIs or cross-language boundaries

**Common Patterns:**

**Pattern 1: Boolean return + out parameter**
```cpp
bool FindCharacter(EntityID id, Character& out_character) {
    auto it = characters.find(id);
    if (it == characters.end()) {
        return false;  // Expected failure
    }
    out_character = it->second;
    return true;
}

// Usage
Character character;
if (FindCharacter(id, character)) {
    ProcessCharacter(character);
} else {
    // Handle not found
}
```

**Pattern 2: std::optional for nullable returns**
```cpp
std::optional<Character> FindCharacter(EntityID id) {
    auto it = characters.find(id);
    if (it == characters.end()) {
        return std::nullopt;  // No character found
    }
    return it->second;
}

// Usage
if (auto character = FindCharacter(id)) {
    ProcessCharacter(*character);
} else {
    // Handle not found
}
```

**Pattern 3: std::expected (C++23 or use third-party)**
```cpp
// Future: C++23 std::expected
std::expected<Character, ErrorCode> FindCharacter(EntityID id);

// Current: Use tl::expected or similar
tl::expected<Character, std::string> LoadCharacterData(const std::string& filename) {
    if (!FileExists(filename)) {
        return tl::unexpected("File not found: " + filename);
    }

    Character character;
    if (!ParseCharacterFile(filename, character)) {
        return tl::unexpected("Failed to parse character file");
    }

    return character;
}
```

### 3.2 Decision Matrix

| Condition | Use Exception | Use Error Code |
|-----------|---------------|----------------|
| Failure is rare (<1%) | ✅ Yes | ❌ No |
| Failure is expected | ❌ No | ✅ Yes |
| In constructor | ✅ Yes | ❌ Can't return |
| Performance-critical | ❌ No | ✅ Yes |
| Unrecoverable error | ✅ Yes | ❌ No |
| Optional operation | ❌ No | ✅ Yes |
| Cross-language boundary | ❌ No | ✅ Yes |

---

## 4. Logging Standards

### 4.1 Log Levels

**Your project uses `core::logging::Logger.h` with these levels:**

```cpp
enum class LogLevel {
    Trace,      // Extremely verbose, development only
    Debug,      // Detailed information for debugging
    Info,       // General informational messages
    Warn,       // Warning conditions that should be investigated
    Error,      // Error conditions that affect functionality
    Critical,   // Critical errors requiring immediate attention
    Off         // Logging disabled
};
```

### 4.2 When to Log at Each Level

**Trace (LogLevel::Trace):**
- Function entry/exit in hot paths
- Message bus events (when enabled)
- ECS lifecycle events (when enabled)
- Performance-critical loop iterations

```cpp
CORE_LOG_TRACE("AIDirector", "ProcessCharacter id=" + std::to_string(char_id));
```

**Debug (LogLevel::Debug):**
- State changes in systems
- Algorithm decision points
- Resource allocation/deallocation
- Cache hits/misses

```cpp
CORE_LOG_DEBUG("EconomySystem",
               "Trade route created: " + std::to_string(route_id) +
               " wealth_delta=" + std::to_string(wealth_change));
```

**Info (LogLevel::Info):**
- System initialization/shutdown
- Major game events (war, marriage, death)
- Save/load operations
- Configuration changes

```cpp
CORE_LOG_INFO("GameEngine", "Game initialized successfully");
CORE_LOG_INFO("SaveSystem", "Game saved to: " + filename);
```

**Warn (LogLevel::Warn):**
- Recoverable errors
- Performance degradation
- Deprecated API usage
- Resource limits approaching

```cpp
CORE_LOG_WARN("AIDirector",
              "Character count approaching limit: " +
              std::to_string(character_count) + "/" +
              std::to_string(MAX_CHARACTERS));
```

**Error (LogLevel::Error):**
- Operation failures
- Data validation failures
- Resource allocation failures
- Exception catches

```cpp
CORE_LOG_ERROR("SaveSystem",
               "Failed to save game: " + std::string(e.what()));
```

**Critical (LogLevel::Critical):**
- System failures
- Data corruption detected
- Unrecoverable errors before crash
- Security violations

```cpp
CORE_LOG_CRITICAL("SaveSystem",
                  "Save file corrupted, data integrity compromised: " + filename);
```

### 4.3 Logging Best Practices

**✅ DO:**
```cpp
// Include context (system, entity IDs, relevant values)
CORE_LOG_ERROR("CharacterSystem",
               "Invalid relationship bond strength: " +
               std::to_string(bond) + " for character " +
               std::to_string(char_id));

// Use structured logging with stream builder for complex messages
CORE_STREAM_INFO("EconomySystem")
    << "Province " << province_id
    << " wealth changed from " << old_wealth
    << " to " << new_wealth
    << " (delta: " << (new_wealth - old_wealth) << ")";

// Log before throwing exceptions
CORE_LOG_ERROR("CharacterRelationships",
               "Cannot add marriage: invalid spouse ID " +
               std::to_string(spouse_id));
throw std::invalid_argument("Invalid spouse ID");
```

**❌ DON'T:**
```cpp
// Vague messages without context
CORE_LOG_ERROR("System", "Error occurred");

// Logging sensitive user data
CORE_LOG_INFO("SaveSystem", "User data: " + save_json.dump());

// Logging in tight loops without throttling
for (const auto& character : characters) {
    CORE_LOG_TRACE("Loop", "Processing " + std::to_string(character.id));
}

// Duplicate logging (exception already logged elsewhere)
try {
    DoSomething();  // Already logs internally
} catch (const std::exception& e) {
    CORE_LOG_ERROR("System", e.what());  // Duplicate!
    throw;
}
```

### 4.4 Log Format

**Your project uses this format:**
```
[YYYY-MM-DD HH:MM:SS.µµµµµµ][LEVEL][System] Message
```

**Example:**
```
[2025-12-16 14:32:45.123456][INFO][GameEngine] Game initialized successfully
[2025-12-16 14:32:45.234567][ERROR][SaveSystem] Failed to load save file: corrupted.sav
[2025-12-16 14:32:45.345678][WARN][AIDirector] Character count: 2950/3000
```

### 4.5 Logging Macros Reference

**Available macros from your Logger.h:**

```cpp
// Simple string logging
CORE_LOG_TRACE(system, message)
CORE_LOG_DEBUG(system, message)
CORE_LOG_INFO(system, message)
CORE_LOG_WARN(system, message)
CORE_LOG_ERROR(system, message)
CORE_LOG_CRITICAL(system, message)

// Stream-based logging (lazy evaluation)
CORE_STREAM_TRACE(system) << "message"
CORE_STREAM_DEBUG(system) << "message"
CORE_STREAM_INFO(system) << "message"
CORE_STREAM_WARN(system) << "message"
CORE_STREAM_ERROR(system) << "message"
CORE_STREAM_CRITICAL(system) << "message"

// Legacy backwards-compatible functions
LogInfo(system, message)
LogWarning(system, message)
LogError(system, message)
LogDebug(system, message)
```

---

## 5. Error Recovery Strategies

### 5.1 Graceful Degradation

**Continue operation with reduced functionality:**

```cpp
bool EconomySystem::Update() {
    try {
        CalculateTradeRoutes();
    } catch (const std::exception& e) {
        CORE_LOG_ERROR("EconomySystem",
                       "Failed to calculate trade routes: " + std::string(e.what()));
        // Degrade gracefully: use previous tick's trade routes
        CORE_LOG_WARN("EconomySystem",
                      "Using previous trade route data (degraded mode)");
    }

    // Continue with other economy calculations
    try {
        UpdateProvinceWealth();
    } catch (const std::exception& e) {
        CORE_LOG_ERROR("EconomySystem",
                       "Failed to update province wealth: " + std::string(e.what()));
        return false;  // Can't continue without this
    }

    return true;
}
```

### 5.2 Retry with Backoff

**For transient failures (I/O, network):**

```cpp
bool SaveGame(const std::string& filename, int max_retries = 3) {
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        try {
            // Attempt save
            WriteSaveFile(filename);
            CORE_LOG_INFO("SaveSystem",
                          "Game saved successfully to " + filename);
            return true;

        } catch (const std::ios_base::failure& e) {
            CORE_LOG_WARN("SaveSystem",
                          "Save attempt " + std::to_string(attempt + 1) +
                          " failed: " + std::string(e.what()));

            if (attempt < max_retries - 1) {
                // Wait before retrying (exponential backoff)
                int delay_ms = (1 << attempt) * 100;  // 100ms, 200ms, 400ms
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        }
    }

    CORE_LOG_ERROR("SaveSystem",
                   "Failed to save game after " + std::to_string(max_retries) +
                   " attempts");
    return false;
}
```

### 5.3 Fallback Mechanisms

**Use default values when optional data fails to load:**

```cpp
Configuration LoadConfiguration(const std::string& config_file) {
    Configuration config;

    try {
        config = ParseConfigFile(config_file);
        CORE_LOG_INFO("ConfigSystem", "Loaded configuration from " + config_file);

    } catch (const std::exception& e) {
        CORE_LOG_WARN("ConfigSystem",
                      "Failed to load config, using defaults: " +
                      std::string(e.what()));
        config = GetDefaultConfiguration();
    }

    return config;
}
```

### 5.4 Transaction-Like Operations

**All-or-nothing for critical state changes:**

```cpp
bool CharacterSystem::ProcessDeath(EntityID character_id) {
    // Take snapshot for rollback
    auto snapshot = CreateCharacterSnapshot(character_id);

    try {
        // Step 1: Update relationships
        RemoveCharacterRelationships(character_id);

        // Step 2: Transfer titles
        TransferCharacterTitles(character_id);

        // Step 3: Update AI state
        NotifyAIOfDeath(character_id);

        // Step 4: Mark as dead
        MarkCharacterDead(character_id);

        CORE_LOG_INFO("CharacterSystem",
                      "Character " + std::to_string(character_id) + " died");
        return true;

    } catch (const std::exception& e) {
        CORE_LOG_ERROR("CharacterSystem",
                       "Failed to process death, rolling back: " +
                       std::string(e.what()));

        // Rollback to snapshot
        RestoreCharacterSnapshot(snapshot);
        return false;
    }
}
```

---

## 6. Assertions and Contracts

### 6.1 Debug vs Release Assertions

**Debug-only assertions (removed in release):**
```cpp
#include <cassert>

void ProcessCharacter(const Character& character) {
    // Programmer contract: character must be valid
    assert(character.id != 0 && "Character ID cannot be zero");
    assert(character.age >= 0 && "Character age cannot be negative");

    // Normal processing
    // ...
}
```

**Runtime assertions (checked in release):**
```cpp
void ProcessCharacter(const Character& character) {
    if (character.id == 0) {
        CORE_LOG_CRITICAL("CharacterSystem", "Invalid character ID: 0");
        throw std::logic_error("Character ID cannot be zero");
    }

    if (character.age < 0) {
        CORE_LOG_CRITICAL("CharacterSystem",
                          "Invalid character age: " + std::to_string(character.age));
        throw std::logic_error("Character age cannot be negative");
    }

    // Normal processing
}
```

### 6.2 Preconditions and Postconditions

**Document and enforce contracts:**

```cpp
/**
 * @brief Calculate loyalty of vassal to liege
 *
 * @pre character_id must be valid (checked with assertion)
 * @pre liege_id must be valid (checked with assertion)
 * @post Result is in range [0.0, 100.0]
 *
 * @param character_id The vassal character
 * @param liege_id The liege character
 * @return Loyalty value between 0.0 and 100.0
 */
double CalculateLoyalty(EntityID character_id, EntityID liege_id) {
    // Preconditions
    assert(character_id != 0 && "Invalid character ID");
    assert(liege_id != 0 && "Invalid liege ID");

    double loyalty = ComputeLoyalty(character_id, liege_id);

    // Postcondition
    assert(loyalty >= 0.0 && loyalty <= 100.0 && "Loyalty out of range");

    return loyalty;
}
```

### 6.3 Invariants

**Check class invariants:**

```cpp
class CharacterRelationshipsComponent {
public:
    void AddMarriage(EntityID spouse_id, EntityID realm, EntityID dynasty) {
        CheckInvariants();  // Before

        marriages.emplace_back(spouse_id, realm, dynasty);
        current_spouse = spouse_id;

        CheckInvariants();  // After
    }

private:
    void CheckInvariants() const {
        #ifndef NDEBUG
        // Invariant: current_spouse must be in marriages list
        if (current_spouse != 0) {
            bool found = false;
            for (const auto& marriage : marriages) {
                if (marriage.spouse == current_spouse) {
                    found = true;
                    break;
                }
            }
            assert(found && "current_spouse not in marriages list");
        }

        // Invariant: All bond strengths in valid range
        for (const auto& [id, rel] : relationships) {
            assert(rel.bond_strength >= MIN_BOND_STRENGTH &&
                   rel.bond_strength <= MAX_BOND_STRENGTH &&
                   "Bond strength out of range");
        }
        #endif
    }

    std::vector<Marriage> marriages;
    EntityID current_spouse{0};
    std::unordered_map<EntityID, CharacterRelationship> relationships;
};
```

---

## 7. System-Specific Error Handling

### 7.1 Save/Load System

**Critical: Must handle corrupted data gracefully**

```cpp
std::optional<GameState> LoadGame(const std::string& filename) {
    try {
        // Validate file exists and readable
        if (!FileExists(filename)) {
            CORE_LOG_ERROR("SaveSystem", "Save file not found: " + filename);
            return std::nullopt;
        }

        // Parse JSON
        Json::Value root = ParseJsonFile(filename);

        // Validate save file version
        if (!ValidateSaveVersion(root)) {
            CORE_LOG_ERROR("SaveSystem",
                           "Incompatible save file version: " + filename);
            return std::nullopt;
        }

        // Deserialize game state
        GameState state;
        if (!DeserializeGameState(root, state)) {
            CORE_LOG_ERROR("SaveSystem",
                           "Failed to deserialize game state from " + filename);
            return std::nullopt;
        }

        // Validate state integrity
        if (!ValidateGameState(state)) {
            CORE_LOG_ERROR("SaveSystem",
                           "Game state integrity check failed: " + filename);
            return std::nullopt;
        }

        CORE_LOG_INFO("SaveSystem", "Successfully loaded game from " + filename);
        return state;

    } catch (const Json::Exception& e) {
        CORE_LOG_ERROR("SaveSystem",
                       "JSON parsing error: " + std::string(e.what()));
        return std::nullopt;
    } catch (const std::exception& e) {
        CORE_LOG_ERROR("SaveSystem",
                       "Unexpected error loading game: " + std::string(e.what()));
        return std::nullopt;
    }
}
```

### 7.2 AI Systems

**Errors should not crash game loop:**

```cpp
void AIDirector::Update() {
    for (const auto& nation_id : active_nations) {
        try {
            ProcessNation(nation_id);
        } catch (const std::exception& e) {
            CORE_LOG_ERROR("AIDirector",
                           "Error processing nation " + std::to_string(nation_id) +
                           ": " + std::string(e.what()));
            // Continue with next nation - don't let one failure stop entire AI
        }
    }
}
```

### 7.3 Rendering System

**Rendering errors shouldn't crash game:**

```cpp
void MapRenderer::Render() {
    try {
        // Setup OpenGL state
        SetupRenderState();

        // Render provinces
        RenderProvinces();

        // Render borders
        RenderBorders();

    } catch (const std::exception& e) {
        CORE_LOG_ERROR("MapRenderer",
                       "Rendering error: " + std::string(e.what()));
        // Show error texture or blank screen instead of crashing
        RenderErrorState();
    }
}
```

---

## 8. Error Handling Patterns

### 8.1 RAII for Resource Management

**Always use RAII to prevent resource leaks:**

```cpp
class FileHandle {
public:
    explicit FileHandle(const std::string& filename) {
        file_ = std::fopen(filename.c_str(), "rb");
        if (!file_) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
    }

    ~FileHandle() {
        if (file_) {
            std::fclose(file_);
        }
    }

    // Non-copyable
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    FILE* Get() const { return file_; }

private:
    FILE* file_ = nullptr;
};

// Usage - file automatically closed even if exception thrown
void ProcessFile(const std::string& filename) {
    FileHandle file(filename);  // RAII - auto cleanup
    // ... process file ...
    // File closed automatically on scope exit (normal or exception)
}
```

### 8.2 Error Accumulation

**Collect multiple errors before failing:**

```cpp
struct ValidationResult {
    bool is_valid = true;
    std::vector<std::string> errors;

    void AddError(const std::string& error) {
        is_valid = false;
        errors.push_back(error);
    }
};

ValidationResult ValidateCharacter(const Character& character) {
    ValidationResult result;

    if (character.id == 0) {
        result.AddError("Character ID cannot be zero");
    }

    if (character.name.empty()) {
        result.AddError("Character name cannot be empty");
    }

    if (character.age < 0 || character.age > 120) {
        result.AddError("Character age out of valid range: " +
                       std::to_string(character.age));
    }

    // Return all errors, not just first one
    return result;
}

// Usage
auto validation = ValidateCharacter(character);
if (!validation.is_valid) {
    for (const auto& error : validation.errors) {
        CORE_LOG_ERROR("CharacterSystem", error);
    }
    throw std::invalid_argument("Character validation failed");
}
```

---

## 9. Testing Error Paths

### 9.1 Test Exception Handling

**Verify exceptions are thrown correctly:**

```cpp
TEST(CharacterRelationships, SetRelationship_InvalidCharacterID_ThrowsException) {
    CharacterRelationshipsComponent comp(EntityID{1});

    EXPECT_THROW(
        comp.SetRelationship(EntityID{0}, RelationshipType::FRIEND, 50, 50.0),
        std::invalid_argument
    );
}

TEST(CharacterRelationships, SetRelationship_OpinionOutOfRange_ThrowsException) {
    CharacterRelationshipsComponent comp(EntityID{1});

    EXPECT_THROW(
        comp.SetRelationship(EntityID{2}, RelationshipType::FRIEND, 150, 50.0),
        std::out_of_range
    );
}
```

### 9.2 Test Error Recovery

**Verify graceful degradation:**

```cpp
TEST(SaveSystem, LoadGame_CorruptedFile_ReturnsNullopt) {
    std::string corrupted_file = CreateCorruptedSaveFile();

    auto result = LoadGame(corrupted_file);

    EXPECT_FALSE(result.has_value());
    // Verify error was logged (check log output)
}

TEST(EconomySystem, Update_TradeRouteFails_ContinuesWithDegradedMode) {
    EconomySystem system;
    InjectTradeRouteFailure();  // Simulate failure

    bool result = system.Update();

    EXPECT_TRUE(result);  // Should still return true (degraded mode)
    EXPECT_TRUE(system.IsInDegradedMode());
}
```

---

## Summary

### Error Handling Checklist

When writing new code:

- [ ] Validate all inputs (especially at API boundaries)
- [ ] Use exceptions for exceptional conditions
- [ ] Use error codes/optional for expected failures
- [ ] Log errors at appropriate level
- [ ] Provide strong or basic exception safety guarantee
- [ ] Use RAII for all resource management
- [ ] Never catch and ignore exceptions
- [ ] Test error paths as thoroughly as success paths
- [ ] Document error conditions in function comments
- [ ] Ensure user never sees crashes (graceful degradation)

### Quick Reference

```cpp
// Exceptions - for exceptional conditions
if (invalid_input) {
    CORE_LOG_ERROR("System", "Error description");
    throw std::invalid_argument("Error message");
}

// Error codes - for expected failures
std::optional<T> FindItem(ID id) {
    if (!exists) return std::nullopt;
    return item;
}

// Logging - always log errors
CORE_LOG_ERROR("System", "Context: " + details);

// RAII - automatic cleanup
std::unique_ptr<Resource> resource = AcquireResource();
// Auto cleanup on scope exit

// Assertions - debug checks
assert(value >= 0 && "Value must be non-negative");
```

---

**For questions about error handling, consult this document or ask the development team.**

**Version History:**
- v1.0 (December 2025) - Initial error handling standards document
