# Security Policy - Mechanica Imperii

**Version:** 1.0
**Last Updated:** December 2025
**Project:** Mechanica Imperii - Historical Grand Strategy Game

---

## Table of Contents

1. [Reporting Vulnerabilities](#reporting-vulnerabilities)
2. [Supported Versions](#supported-versions)
3. [Security Practices](#security-practices)
4. [Input Validation](#input-validation)
5. [Memory Safety](#memory-safety)
6. [Dependency Management](#dependency-management)
7. [Build Security](#build-security)
8. [Runtime Security](#runtime-security)
9. [Secure Coding Guidelines](#secure-coding-guidelines)
10. [Security Testing](#security-testing)

---

## 1. Reporting Vulnerabilities

### How to Report

If you discover a security vulnerability in Mechanica Imperii, please report it responsibly:

**📧 Email:** [To be configured - security@project.org]

**Please include:**
- Description of the vulnerability
- Steps to reproduce
- Potential impact assessment
- Suggested fix (if available)
- Your contact information for follow-up

### Response Timeline

- **Initial Response:** Within 48 hours of report
- **Vulnerability Assessment:** Within 1 week
- **Fix Timeline:** Depends on severity (critical issues prioritized)
- **Disclosure:** Coordinated disclosure after fix is available

### What to Expect

1. We will acknowledge your report promptly
2. We will investigate and validate the issue
3. We will develop and test a fix
4. We will release the fix and publicly credit you (if desired)
5. We will coordinate disclosure timing with you

---

## 2. Supported Versions

| Version | Supported          | Status             |
|---------|-------------------|--------------------|
| Main    | ✅ Yes            | Active development |
| 0.x.x   | ⚠️ Partial        | Bug fixes only     |
| < 0.1   | ❌ No             | Unsupported        |

**Note:** As this is a pre-release project, security patches will be applied to the main development branch and backported as needed.

---

## 3. Security Practices

### 3.1 Security-First Development

All code contributions must:
- ✅ Pass security-focused code review
- ✅ Include input validation for user-facing code
- ✅ Handle errors gracefully without exposing internals
- ✅ Use memory-safe practices (smart pointers, RAII)
- ✅ Be tested with sanitizers (ASan, TSan, UBSan)

### 3.2 Defense in Depth

Multiple layers of security:
1. **Input Validation** - Validate all external data
2. **Memory Safety** - Use modern C++ idioms
3. **Bounds Checking** - Validate array/vector access
4. **Exception Handling** - Catch and handle errors properly
5. **Dependency Security** - Keep libraries updated
6. **Build Hardening** - Enable security compiler flags

---

## 4. Input Validation

### 4.1 Save File Loading

**Threat:** Malicious save files could exploit deserialization bugs

**Mitigations:**
```cpp
// ✅ GOOD: Validate all JSON fields before use
bool CharacterRelationships::Deserialize(const std::string& data) {
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::istringstream stream(data);

    // Parse with error handling
    std::string errors;
    if (!Json::parseFromStream(builder, stream, &root, &errors)) {
        LOG_ERROR("Failed to parse character relationships: " + errors);
        return false;
    }

    // Validate required fields exist
    if (!root.isMember("character_id") || !root["character_id"].isUInt()) {
        LOG_ERROR("Invalid character_id in save data");
        return false;
    }

    // Bounds checking on values
    auto char_id = root["character_id"].asUInt();
    if (char_id == 0 || char_id > MAX_ENTITY_ID) {
        LOG_ERROR("Character ID out of valid range: " + std::to_string(char_id));
        return false;
    }

    // Validate array sizes to prevent memory exhaustion
    if (root.isMember("children") && root["children"].isArray()) {
        if (root["children"].size() > MAX_CHILDREN_PER_CHARACTER) {
            LOG_ERROR("Too many children in save data");
            return false;
        }
    }

    return true;
}
```

**Validation Checklist:**
- [ ] All JSON fields validated before access
- [ ] Type checking (isUInt, isString, isArray, etc.)
- [ ] Range validation for numeric values
- [ ] Array size limits enforced
- [ ] String length limits enforced
- [ ] Enum values validated against COUNT sentinel
- [ ] EntityID values checked for validity

### 4.2 Map Data Validation

**Threat:** Malicious GeoJSON could cause crashes or memory exhaustion

**Mitigations:**
```cpp
// ✅ GOOD: Validate map data bounds
bool LoadMapData(const Json::Value& geojson) {
    // Check feature count
    if (!geojson.isMember("features") || !geojson["features"].isArray()) {
        return false;
    }

    size_t feature_count = geojson["features"].size();
    if (feature_count > MAX_PROVINCES) {
        LOG_ERROR("Map contains too many provinces: " + std::to_string(feature_count));
        return false;
    }

    // Validate each province
    for (const auto& feature : geojson["features"]) {
        if (!ValidateProvinceData(feature)) {
            return false;
        }
    }

    return true;
}

bool ValidateProvinceData(const Json::Value& province) {
    // Validate coordinates are in valid range
    // Latitude: -90 to 90, Longitude: -180 to 180
    // Prevent out-of-bounds rendering issues

    // Validate polygon complexity (prevent DoS via complex geometries)
    if (polygon_points > MAX_POLYGON_POINTS) {
        return false;
    }

    return true;
}
```

### 4.3 User Input Sanitization

**Threat:** User input in character names, realm names could contain malicious data

**Mitigations:**
```cpp
// ✅ GOOD: Sanitize user text input
std::string SanitizeUserInput(const std::string& input, size_t max_length = 100) {
    // Length check
    if (input.length() > max_length) {
        return input.substr(0, max_length);
    }

    // Remove or escape potentially dangerous characters
    std::string sanitized;
    sanitized.reserve(input.length());

    for (char c : input) {
        // Allow alphanumeric, spaces, basic punctuation
        if (std::isalnum(c) || c == ' ' || c == '-' || c == '\'') {
            sanitized += c;
        }
    }

    return sanitized;
}
```

---

## 5. Memory Safety

### 5.1 Smart Pointers

**Always use RAII and smart pointers:**

```cpp
// ✅ GOOD: Automatic memory management
class ProvinceManager {
    std::unique_ptr<Province> CreateProvince(int id) {
        return std::make_unique<Province>(id);
    }

    std::vector<std::unique_ptr<Province>> provinces_;
};

// ❌ BAD: Manual memory management
Province* CreateProvince(int id) {
    return new Province(id);  // Who owns this? When is it deleted?
}
```

### 5.2 Bounds Checking

**Always validate array/vector access:**

```cpp
// ✅ GOOD: Bounds checking
std::optional<Character> GetCharacter(size_t index) const {
    if (index >= characters.size()) {
        return std::nullopt;
    }
    return characters[index];
}

// ✅ GOOD: Use .at() which throws on out-of-bounds
try {
    auto& character = characters.at(index);
    ProcessCharacter(character);
} catch (const std::out_of_range& e) {
    LOG_ERROR("Character index out of range: " + std::to_string(index));
    return false;
}

// ❌ BAD: Unchecked array access
auto& character = characters[index];  // Could crash if index >= size
```

### 5.3 String Safety

**Prevent buffer overflows:**

```cpp
// ✅ GOOD: Use std::string
std::string character_name = "Character Name";
character_name += " the Great";  // Safe, automatic resizing

// ❌ BAD: C-style strings (avoid)
char name[100];
strcpy(name, character_name.c_str());  // Buffer overflow risk!
```

### 5.4 Integer Overflow

**Check for overflow in calculations:**

```cpp
// ✅ GOOD: Check for overflow
bool SafeAdd(int a, int b, int& result) {
    if (a > 0 && b > INT_MAX - a) {
        return false;  // Would overflow
    }
    if (a < 0 && b < INT_MIN - a) {
        return false;  // Would underflow
    }
    result = a + b;
    return true;
}

// Use for resource calculations
if (!SafeAdd(current_gold, income, new_gold)) {
    LOG_ERROR("Gold calculation would overflow");
    new_gold = INT_MAX;  // Cap at maximum
}
```

### 5.5 Use-After-Free Prevention

**Never store raw pointers to temporary objects:**

```cpp
// ❌ BAD: Dangling pointer
const Character* GetCharacter(EntityID id) {
    auto it = characters.find(id);
    if (it != characters.end()) {
        return &it->second;  // Pointer becomes invalid if map modified!
    }
    return nullptr;
}

// ✅ GOOD: Return by value or shared_ptr
std::optional<Character> GetCharacter(EntityID id) {
    auto it = characters.find(id);
    if (it != characters.end()) {
        return it->second;  // Copy, safe
    }
    return std::nullopt;
}
```

---

## 6. Dependency Management

### 6.1 Dependency Inventory

**Current Dependencies (via vcpkg):**
- SDL2 - Windowing and input
- OpenGL (glad) - Graphics rendering
- JsonCpp - JSON parsing
- OpenSSL - Cryptography (if used for checksums)
- LZ4 - Compression
- ImGui - UI framework

### 6.2 Dependency Security Practices

**Pinned Baseline:**
```json
// vcpkg.json
{
  "builtin-baseline": "3508985146f1b1d248c67ead13f8f54be5b4f5da",
  "dependencies": [
    "sdl2",
    "glad",
    "jsoncpp",
    "openssl",
    "lz4",
    "imgui"
  ]
}
```

**Security Procedures:**
1. **Monthly Audits** - Check for known vulnerabilities
2. **Update Process:**
   - Review changelogs for security fixes
   - Test updates in isolation before merging
   - Update vcpkg baseline deliberately
3. **Vulnerability Tracking:**
   - Monitor CVE databases for dependencies
   - Subscribe to security advisories for key libraries

### 6.3 Dependency Update Policy

**When to Update:**
- ✅ Critical security vulnerabilities (CVSS >= 7.0)
- ✅ High-priority security fixes
- ⚠️ Medium vulnerabilities (evaluate impact)
- ❌ Routine updates (schedule quarterly)

**Update Process:**
1. Review security advisory
2. Check if vulnerability affects our usage
3. Test update in feature branch
4. Run full test suite with sanitizers
5. Deploy update after validation

---

## 7. Build Security

### 7.1 Compiler Security Flags

**CMake Configuration:**
```cmake
# Enable security features
if(MSVC)
    # Windows: Control Flow Guard, SDL checks
    add_compile_options(/GS /sdl /guard:cf)
    add_link_options(/GUARD:CF /DYNAMICBASE /NXCOMPAT)
else()
    # Linux/GCC/Clang: Stack protection, position-independent code
    add_compile_options(
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
        -fPIC
    )
endif()

# Release builds: Disable debug features
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(NDEBUG)
    # Strip symbols (separate debug symbols)
    if(UNIX)
        add_link_options(-Wl,--strip-debug)
    endif()
endif()
```

### 7.2 Sanitizer Configuration

**Enable sanitizers in testing builds:**

```cmake
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()
```

**Usage:**
```bash
# Build with AddressSanitizer
cmake -B build -DENABLE_ASAN=ON
cmake --build build

# Run tests with sanitizer
./build/tests/run_all_tests
```

---

## 8. Runtime Security

### 8.1 Crash Handling

**Secure crash reporting (no sensitive data):**

```cpp
// ✅ GOOD: Crash handler logs safely
void CrashHandler::HandleCrash(int signal) {
    // Log crash information WITHOUT sensitive data
    LOG_ERROR("Crash detected: signal " + std::to_string(signal));
    LOG_ERROR("Version: " + GetVersion());
    LOG_ERROR("Platform: " + GetPlatform());

    // Stack trace (sanitize paths)
    auto stack = GetSanitizedStackTrace();
    LOG_ERROR("Stack trace: " + stack);

    // DON'T log:
    // - User save data
    // - Character names (could be personally identifiable)
    // - Absolute file paths (reveal system structure)

    // Graceful shutdown
    CleanupResources();
    std::abort();
}
```

### 8.2 Resource Limits

**Prevent resource exhaustion:**

```cpp
// ✅ GOOD: Limit resource creation
class GameSession {
    static constexpr size_t MAX_CHARACTERS = 3000;
    static constexpr size_t MAX_PROVINCES = 5000;
    static constexpr size_t MAX_NATIONS = 500;

    bool CreateCharacter(const CharacterData& data) {
        if (characters.size() >= MAX_CHARACTERS) {
            LOG_WARNING("Character limit reached");
            return false;
        }
        // Create character...
    }
};
```

### 8.3 Logging Security

**Don't log sensitive information:**

```cpp
// ✅ GOOD: Safe logging
LOG_INFO("User loaded save file: " + SanitizeFilename(filename));
LOG_INFO("Character count: " + std::to_string(character_count));

// ❌ BAD: Exposes user data
LOG_INFO("User save data: " + save_json.toStyledString());
LOG_INFO("User file path: " + absolute_path);
```

---

## 9. Secure Coding Guidelines

### 9.1 Validation Checklist

For every new feature:
- [ ] All user input validated
- [ ] Bounds checking on array/vector access
- [ ] No raw pointers for ownership
- [ ] Exception handling for external operations (file I/O, JSON parsing)
- [ ] Integer overflow checks for calculations
- [ ] String length limits enforced
- [ ] Resource limits respected
- [ ] No hardcoded secrets/credentials
- [ ] Error messages don't expose internals

### 9.2 Common Vulnerabilities to Avoid

| Vulnerability | Description | Prevention |
|---------------|-------------|------------|
| **Buffer Overflow** | Writing past array bounds | Use std::vector, bounds checking |
| **Integer Overflow** | Arithmetic exceeds type limits | Use safe math functions, check before operations |
| **Use-After-Free** | Accessing freed memory | Use smart pointers, RAII |
| **Null Pointer Deref** | Dereferencing nullptr | Use std::optional, validate pointers |
| **Path Traversal** | Accessing files outside allowed directories | Sanitize file paths, validate against whitelist |
| **Deserialization** | Malicious data in save files | Validate all JSON fields, enforce limits |
| **Resource Exhaustion** | Creating unlimited objects | Enforce MAX constants, resource pooling |

### 9.3 Code Review Security Checklist

During code reviews, verify:
- [ ] Input validation present for external data
- [ ] No unchecked array indexing
- [ ] Smart pointers used for ownership
- [ ] Exception handling for error cases
- [ ] No secrets in code
- [ ] Sanitizers pass (ASan, TSan, UBSan)
- [ ] No obvious logic errors in security-critical code

---

## 10. Security Testing

### 10.1 Sanitizer Testing

**Run tests with all sanitizers:**

```bash
# AddressSanitizer - Memory errors
cmake -B build-asan -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-asan
./build-asan/tests/run_all_tests

# ThreadSanitizer - Race conditions
cmake -B build-tsan -DENABLE_TSAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tsan
./build-tsan/tests/run_all_tests

# UndefinedBehaviorSanitizer - Undefined behavior
cmake -B build-ubsan -DENABLE_UBSAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-ubsan
./build-ubsan/tests/run_all_tests
```

### 10.2 Fuzzing (Future)

**Planned fuzzing targets:**
- Save file deserialization
- JSON parsing
- Map data loading
- Character data validation

### 10.3 Static Analysis

**Tools to integrate:**
- **Clang-Tidy** - Security checks (`cert-*`, `bugprone-*`)
- **Cppcheck** - Additional static analysis
- **SonarQube** - Code quality and security scanning

### 10.4 Security Test Cases

**Create tests for:**
- Malformed JSON input
- Out-of-bounds array access
- Integer overflow scenarios
- Null pointer handling
- Invalid EntityID references
- Resource limit enforcement

**Example Security Test:**
```cpp
TEST(SecurityTests, RejectMalformedSaveData) {
    CharacterRelationshipsComponent comp;

    // Test 1: Invalid JSON
    EXPECT_FALSE(comp.Deserialize("invalid json {{{"));

    // Test 2: Missing required fields
    EXPECT_FALSE(comp.Deserialize("{}"));

    // Test 3: Out-of-range values
    std::string malicious = R"({"character_id": 999999999})";
    EXPECT_FALSE(comp.Deserialize(malicious));

    // Test 4: Array size attack
    std::string huge_array = GenerateHugeArrayJSON(1000000);
    EXPECT_FALSE(comp.Deserialize(huge_array));
}
```

---

## Security Contacts

**Security Team:** [To be configured]
**Email:** [security@project.org]
**PGP Key:** [If applicable]

---

## Version History

- **v1.0** (December 2025) - Initial security policy
  - Vulnerability reporting process
  - Input validation guidelines
  - Memory safety practices
  - Dependency management procedures
  - Build security configuration

---

## References

- [CWE Top 25 Most Dangerous Software Weaknesses](https://cwe.mitre.org/top25/)
- [OWASP Top 10](https://owasp.org/www-project-top-ten/)
- [C++ Core Guidelines - Safety](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-safety)
- [SEI CERT C++ Coding Standard](https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=88046682)

**For security questions or to report vulnerabilities, contact the security team immediately.**
