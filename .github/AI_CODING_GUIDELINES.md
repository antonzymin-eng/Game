# AI Coding Guidelines for Mechanica Imperii

This document provides instructions for AI assistants (Claude, GitHub Copilot, ChatGPT, etc.) working on this codebase.

---

## 🎯 Quick Reference for AI Assistants

**Before generating any code, AI assistants MUST:**

1. ✅ Read `STYLE_GUIDE.md` for coding standards
2. ✅ Check `CONTRIBUTING.md` for workflow requirements
3. ✅ Review `SECURITY.md` for security practices
4. ✅ Follow `docs/ERROR_HANDLING.md` for exception handling
5. ✅ Adhere to patterns in existing codebase

**Auto-validation:** All generated code will be checked by:
- ClangFormat (formatting)
- ClangTidy (static analysis)
- Pre-commit hooks (if installed)
- CI workflows (on PR)

---

## 📋 Mandatory Standards Checklist

When generating C++ code for this project, AI assistants MUST follow:

### ✅ Naming Conventions (STYLE_GUIDE.md §1)

```cpp
// ✅ CORRECT
class CharacterSystem {                    // PascalCase for classes
private:
    EntityID character_id;                 // snake_case, NO prefix (new code)
    std::string character_name;            // NO m_ prefix

    // Legacy code may have m_ prefix - don't modify existing
    ComponentAccessManager& m_access_manager;  // OK in legacy files

public:
    void UpdateCharacter();                // PascalCase for methods
    static constexpr int MAX_CHARACTERS = 1000;  // UPPER_CASE for constants
};

// ❌ WRONG
class character_system {                   // ❌ Wrong case
private:
    EntityID m_character_id;               // ❌ Don't use m_ in new code
    EntityID CharacterId;                  // ❌ Wrong case for variables
public:
    void update_character();               // ❌ Wrong case for methods
    static constexpr int maxCharacters = 1000;  // ❌ Wrong case for constants
};
```

### ✅ Namespace Indentation (STYLE_GUIDE.md §1.6)

```cpp
// ✅ CORRECT - 4-space indentation
namespace game {
    namespace character {  // 4-space indent
        class CharacterSystem {
            // class content
        };
    }  // namespace character
}  // namespace game

// ❌ WRONG - No indentation
namespace game {
namespace character {  // ❌ No indent
    class CharacterSystem {
        // class content
    };
}  // namespace character
}  // namespace game
```

### ✅ Modern C++17 Features (STYLE_GUIDE.md §3)

```cpp
// ✅ CORRECT - Use modern C++17
auto result = CalculateLoyalty(char_id);           // ✅ auto for obvious types
if (auto opt_value = GetOptional(); opt_value) {   // ✅ std::optional
    ProcessValue(*opt_value);
}

std::vector<int> values{1, 2, 3};                  // ✅ Uniform initialization
for (const auto& value : values) {                 // ✅ Range-based for
    Process(value);
}

// ❌ WRONG - Old-style C++
std::optional<int> result = CalculateLoyalty(char_id);  // ❌ Redundant type
int* ptr = GetValue();                                   // ❌ Raw pointer
if (ptr != NULL) {                                       // ❌ Use nullptr
    ProcessValue(*ptr);
    delete ptr;                                          // ❌ Manual memory mgmt
}
```

### ✅ Error Handling (ERROR_HANDLING.md)

```cpp
// ✅ CORRECT - Use exceptions for errors, logging for warnings
void LoadCharacter(EntityID id) {
    if (id == 0) {
        throw std::invalid_argument("Character ID cannot be zero");
    }

    auto data = FetchData(id);
    if (!data.has_value()) {
        CORE_LOG_ERROR("CharacterSystem", "Failed to load character: " + std::to_string(id));
        throw std::runtime_error("Character data not found");
    }

    CORE_LOG_INFO("CharacterSystem", "Loaded character: " + std::to_string(id));
}

// ❌ WRONG - Silent failures
void LoadCharacter(EntityID id) {
    if (id == 0) {
        return;  // ❌ Silent failure - caller doesn't know it failed
    }

    auto data = FetchData(id);
    if (!data.has_value()) {
        std::cout << "Error loading character\n";  // ❌ Use logging, not cout
        return;  // ❌ Silent failure
    }
}
```

### ✅ Security Practices (SECURITY.md)

```cpp
// ✅ CORRECT - Validate inputs, bounds checking
bool ProcessInput(const std::string& user_input, int index) {
    // Input validation
    if (user_input.empty() || user_input.size() > MAX_INPUT_SIZE) {
        CORE_LOG_ERROR("Input", "Invalid input size");
        return false;
    }

    // Bounds checking
    if (index < 0 || index >= static_cast<int>(data.size())) {
        CORE_LOG_ERROR("Input", "Index out of bounds");
        return false;
    }

    // Sanitize input (example)
    std::string sanitized = SanitizeInput(user_input);
    return ProcessSanitized(sanitized, index);
}

// ❌ WRONG - No validation
bool ProcessInput(const std::string& user_input, int index) {
    return data[index].Process(user_input);  // ❌ No bounds check, no validation
}
```

### ✅ Documentation (API.md)

```cpp
// ✅ CORRECT - Complete Doxygen documentation
/**
 * @brief Calculate character loyalty based on relationships and traits
 *
 * Loyalty is influenced by:
 * - Opinion of liege (-100 to 100)
 * - Cultural/religious alignment
 * - Relationship bonds with liege
 * - Character traits (Loyal, Ambitious, etc.)
 *
 * @param character_id The character whose loyalty to calculate (must be valid)
 * @param liege_id The liege to whom loyalty is calculated (must be valid)
 * @return Loyalty value between 0.0 and 100.0
 * @throws std::invalid_argument if character_id or liege_id is invalid
 *
 * @note This is called every game tick - performance critical
 * @warning Modifying this may affect game balance
 *
 * @see ModifyLoyalty() for direct loyalty adjustments
 * @see GetOpinion() for opinion calculation details
 */
double CalculateLoyalty(EntityID character_id, EntityID liege_id);

// ❌ WRONG - No documentation
double CalculateLoyalty(EntityID character_id, EntityID liege_id);
```

---

## 🚀 How to Prompt AI Assistants

### Method 1: Reference Standards Explicitly

**Good Prompt:**
```
Please implement a new CharacterTrait system following:
- STYLE_GUIDE.md naming conventions (PascalCase classes, snake_case variables)
- ERROR_HANDLING.md guidelines (exceptions for errors, logging for info)
- SECURITY.md input validation requirements
- Modern C++17 features (smart pointers, std::optional, auto)

Make sure to:
- Add Doxygen documentation for all public methods
- Use 4-space namespace indentation
- NO m_ prefix for new member variables
- Include comprehensive error handling
```

**Better Prompt:**
```
I need to add a CharacterTrait system. Before you start:

1. Read these files:
   - STYLE_GUIDE.md (sections 1-4)
   - docs/ERROR_HANDLING.md
   - SECURITY.md (input validation section)
   - Look at src/game/character/CharacterRelationships.cpp for existing patterns

2. Follow these specific requirements:
   - Class name: CharacterTraitsComponent
   - Members: trait_id (EntityID), trait_values (vector<int>)
   - Methods: AddTrait(), RemoveTrait(), HasTrait()
   - Throw exceptions for invalid trait IDs
   - Log all trait modifications
   - Add full Doxygen documentation

3. After generating the code, verify it follows:
   - Naming conventions (no m_ prefix)
   - 4-space namespace indentation
   - Proper error handling
   - Input validation

Generate the header file first, then the implementation.
```

### Method 2: Provide Example Code

**Include Reference:**
```
Please implement a TradeRoute system similar to CharacterRelationships.
Base your implementation on: include/game/character/CharacterRelationships.h

Key patterns to follow from that file:
- Struct for data (like Marriage, Rivalry)
- Component class with EntityID
- snake_case members WITHOUT m_ prefix
- Full Doxygen comments
- Validation in all methods
```

### Method 3: Include Standards in System Prompt

**For Custom GPTs or Claude Projects:**
```markdown
You are an expert C++ developer working on Mechanica Imperii, a historical
grand strategy game using C++17.

MANDATORY: Before generating any code, follow these standards:

1. Naming (STYLE_GUIDE.md):
   - Classes/Structs: PascalCase
   - Functions/Methods: PascalCase
   - Variables: snake_case (NO m_ prefix in new code)
   - Constants: UPPER_CASE
   - Namespaces: lowercase

2. Indentation (STYLE_GUIDE.md):
   - Nested namespaces: 4-space indent
   - Code blocks: 4 spaces
   - No tabs

3. Modern C++17:
   - Use auto for obvious types
   - Use smart pointers (std::unique_ptr, std::shared_ptr)
   - Use std::optional instead of pointers for nullable values
   - Use range-based for loops
   - Use constexpr for compile-time constants

4. Error Handling (ERROR_HANDLING.md):
   - Throw exceptions for errors (invalid_argument, runtime_error)
   - Use CORE_LOG_* macros for logging (not cout/cerr)
   - Validate all inputs
   - Never fail silently

5. Documentation (API.md):
   - Add Doxygen comments to ALL public methods
   - Include @param, @return, @throws, @note
   - Explain WHY, not just WHAT

6. Security (SECURITY.md):
   - Validate all user input
   - Bounds check all array/vector access
   - Sanitize strings before processing
   - Check for integer overflow in calculations

Always ask if you're unsure about project conventions.
```

---

## 🤖 AI-Specific Instructions

### For GitHub Copilot

Create `.github/copilot-instructions.md`:

```markdown
# Copilot Instructions for Mechanica Imperii

## Code Style
- Follow STYLE_GUIDE.md for all naming and formatting
- Use snake_case for variables (NO m_ prefix)
- Use PascalCase for classes and methods
- Indent nested namespaces by 4 spaces

## Patterns to Learn
- Study src/game/character/CharacterRelationships.cpp
- Study include/game/character/CharacterRelationships.h
- Follow logging patterns in src/core/logging/Logger.h

## Always Include
- Doxygen documentation for public methods
- Input validation in all public methods
- Error logging for failures
- Bounds checking for array access

## Never Do
- Don't use m_ prefix for new member variables
- Don't use raw pointers (use smart pointers)
- Don't use NULL (use nullptr)
- Don't use cout/cerr (use CORE_LOG_*)
- Don't silently ignore errors
```

### For ChatGPT / Claude

**Start each session with:**
```
I'm working on Mechanica Imperii, a C++17 game project.

Please read these standards before helping me code:
1. /home/user/Game/STYLE_GUIDE.md
2. /home/user/Game/CONTRIBUTING.md
3. /home/user/Game/SECURITY.md
4. /home/user/Game/docs/ERROR_HANDLING.md

Key rules:
- New code: NO m_ prefix on member variables
- Namespaces: 4-space indentation
- Methods: PascalCase, Variables: snake_case
- Always use exceptions + logging for errors
- Add Doxygen docs to all public methods

Confirm you've understood these before we start.
```

---

## ✅ Validation Workflow

### After AI Generates Code

**Step 1: Run ClangFormat**
```bash
# Format the generated file
clang-format-18 -i path/to/generated/file.cpp

# Check if formatting changed anything
git diff path/to/generated/file.cpp
# If there are changes, the AI didn't follow formatting rules
```

**Step 2: Run ClangTidy**
```bash
# Analyze the file
clang-tidy-18 -p build path/to/generated/file.cpp

# Look for warnings about:
# - Naming violations
# - Modernization suggestions (use nullptr, auto, etc.)
# - Bug-prone patterns
```

**Step 3: Manual Review Checklist**

```markdown
## AI-Generated Code Review Checklist

### Naming Conventions
- [ ] Classes use PascalCase
- [ ] Methods use PascalCase
- [ ] Variables use snake_case
- [ ] Constants use UPPER_CASE
- [ ] NO m_ prefix on new member variables

### Code Quality
- [ ] Namespaces indented by 4 spaces
- [ ] Modern C++17 features used (auto, smart pointers, std::optional)
- [ ] No raw pointers for ownership
- [ ] nullptr used (not NULL)

### Error Handling
- [ ] Exceptions thrown for invalid inputs
- [ ] CORE_LOG_* macros used (not cout/cerr)
- [ ] Input validation in all public methods
- [ ] No silent failures

### Documentation
- [ ] Doxygen comments on all public methods
- [ ] @param for all parameters
- [ ] @return documented
- [ ] @throws documented
- [ ] Example usage provided (if complex)

### Security
- [ ] Input validation (size, range, format)
- [ ] Bounds checking on array/vector access
- [ ] No potential buffer overflows
- [ ] Integer overflow checks where needed

### Testing
- [ ] Unit tests included (if requested)
- [ ] Edge cases tested
- [ ] Error cases tested
```

**Step 4: Run Pre-commit Hooks**
```bash
# If using pre-commit framework
pre-commit run --files path/to/generated/file.cpp

# This will automatically:
# - Format code (clang-format)
# - Check YAML/JSON syntax
# - Remove trailing whitespace
# - Validate EditorConfig compliance
```

---

## 🎓 Training AI on Your Codebase

### Provide Good Examples

**Show AI existing code patterns:**
```
Here's how we implement components in this project.
Look at this example and follow the same pattern:

[Include CharacterRelationships.h as reference]

Now create a similar component for CharacterTraits.
```

### Correct AI Mistakes

**When AI generates wrong code, correct explicitly:**
```
Your code has these issues:

1. ❌ You used `int m_character_id;`
   ✅ Should be: `int character_id;` (no m_ prefix in new code)

2. ❌ You used `namespace game {\nnamespace character {`
   ✅ Should indent: `namespace game {\n    namespace character {`

3. ❌ You used `std::cout << "Error"`
   ✅ Should use: `CORE_LOG_ERROR("Component", "Error message")`

Please regenerate following STYLE_GUIDE.md.
```

### Create Project-Specific Snippets

**For VS Code, create `.vscode/ai-snippets.md`:**

```markdown
# Common Patterns for AI to Follow

## Component Class Template
```cpp
namespace game {
    namespace system {
        /**
         * @brief [Brief description]
         *
         * [Detailed description]
         */
        class SystemComponent {
        private:
            EntityID entity_id;
            std::vector<DataType> data;

        public:
            explicit SystemComponent(EntityID id);

            /**
             * @brief [Method description]
             * @param param [Description]
             * @return [Description]
             * @throws std::invalid_argument if param is invalid
             */
            bool MethodName(ParamType param);
        };
    }  // namespace system
}  // namespace game
```

## Error Handling Pattern
```cpp
void Function(ParamType param) {
    if (!IsValid(param)) {
        CORE_LOG_ERROR("System", "Invalid parameter: " + ToString(param));
        throw std::invalid_argument("Parameter validation failed");
    }

    try {
        // Operation
    } catch (const std::exception& e) {
        CORE_LOG_ERROR("System", "Operation failed: " + std::string(e.what()));
        throw;
    }
}
```
```

---

## 📚 Documentation to Provide AI

**Always give AI access to:**

1. **Core Standards** (Required reading)
   - `STYLE_GUIDE.md`
   - `CONTRIBUTING.md`
   - `SECURITY.md`
   - `docs/ERROR_HANDLING.md`

2. **Example Code** (Reference implementations)
   - `include/game/character/CharacterRelationships.h`
   - `src/game/character/CharacterRelationships.cpp`
   - Any file in the subsystem you're working on

3. **Configuration Files** (For auto-formatting)
   - `.clang-format`
   - `.clang-tidy`
   - `.editorconfig`

4. **Testing Standards**
   - `docs/TESTING_STRATEGY.md`
   - Example test files

---

## 🛠️ Tool Integration

### Use AI-Aware CI

Your CI workflow (`.github/workflows/code-quality.yml`) will catch:
- Formatting violations (ClangFormat)
- Static analysis issues (ClangTidy)
- Documentation warnings (Doxygen)
- Compiler warnings
- EditorConfig violations

**Advantage:** AI-generated code gets the same scrutiny as human code

### IDE Integration

**Configure your IDE to auto-format on save:**

**VS Code (`.vscode/settings.json`):**
```json
{
    "editor.formatOnSave": true,
    "C_Cpp.clang_format_path": "/usr/bin/clang-format-18",
    "C_Cpp.clang_format_style": "file"
}
```

**CLion:**
- Settings → Editor → Code Style → C/C++
- Import `.clang-format` file
- Enable "Reformat code on save"

---

## 💡 Pro Tips

### 1. **Create a Code Review Template**

`.github/PULL_REQUEST_TEMPLATE.md`:
```markdown
## AI-Generated Code Checklist

If this PR includes AI-generated code, confirm:
- [ ] AI read STYLE_GUIDE.md before generating
- [ ] Code passed clang-format check
- [ ] Code passed clang-tidy check
- [ ] All public methods have Doxygen documentation
- [ ] No m_ prefix on new member variables
- [ ] Namespaces indented correctly (4 spaces)
- [ ] Error handling follows ERROR_HANDLING.md
- [ ] Input validation added for all public methods
```

### 2. **Use Git Hooks**

Install the pre-commit hook (`.github/hooks/pre-commit`):
```bash
cp .github/hooks/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

Now AI-generated code will be auto-formatted before commit!

### 3. **Incremental Validation**

Don't generate entire files at once. Instead:
1. Generate header skeleton → validate
2. Generate method implementations one by one → validate each
3. Generate tests → validate

### 4. **AI Code Review**

Ask AI to review its own code:
```
You just generated CharacterTraits.cpp.

Please review it against:
1. STYLE_GUIDE.md - check naming, indentation
2. ERROR_HANDLING.md - check exception usage
3. SECURITY.md - check input validation

List any violations you find.
```

---

## 🎯 Summary: Quick Enforcement Checklist

**Before AI generates code:**
- [ ] Provide STYLE_GUIDE.md, CONTRIBUTING.md, SECURITY.md
- [ ] Show example files from the same subsystem
- [ ] Specify naming conventions explicitly
- [ ] Specify error handling requirements

**After AI generates code:**
- [ ] Run `clang-format-18 -i <file>`
- [ ] Run `clang-tidy-18 -p build <file>`
- [ ] Check for Doxygen documentation
- [ ] Verify naming conventions (no m_ prefix)
- [ ] Verify namespace indentation (4 spaces)
- [ ] Test the code works

**For team consistency:**
- [ ] Add standards to PR template
- [ ] Install pre-commit hooks
- [ ] Let CI catch violations
- [ ] Review AI-generated code like any other code

---

## 🔗 References

- **Project Standards:** `STYLE_GUIDE.md`, `CONTRIBUTING.md`, `SECURITY.md`
- **Tool Configs:** `.clang-format`, `.clang-tidy`, `.editorconfig`
- **Pre-commit Hooks:** `.github/hooks/README.md`
- **CI Workflows:** `.github/workflows/code-quality.yml`
- **Testing Guide:** `docs/TESTING_STRATEGY.md`

---

**Last Updated:** 2025-12-16
**Maintainer:** Project maintainers
**Questions:** See CONTRIBUTING.md for contact information
