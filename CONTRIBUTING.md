# Contributing to Mechanica Imperii

Thank you for your interest in contributing to **Mechanica Imperii**, a historical grand strategy game covering 1000-1900 AD! This document provides guidelines for contributing to the project.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Development Workflow](#development-workflow)
3. [Code Standards](#code-standards)
4. [Testing Requirements](#testing-requirements)
5. [Pull Request Process](#pull-request-process)
6. [Commit Message Guidelines](#commit-message-guidelines)
7. [Code Review Process](#code-review-process)
8. [Branch Strategy](#branch-strategy)
9. [Issue Reporting](#issue-reporting)
10. [Development Environment](#development-environment)

---

## 1. Getting Started

### Prerequisites

**Required Tools:**
- **C++17 compatible compiler:**
  - GCC 9+ or Clang 10+ (Linux)
  - MSVC 2019+ (Windows)
- **CMake 3.20+**
- **vcpkg** (for dependency management)
- **Git**

**Optional Tools:**
- **ClangFormat** - Code formatting
- **Clang-Tidy** - Static analysis
- **Doxygen** - Documentation generation
- **Google Test** - Advanced testing features

### Initial Setup

1. **Fork and Clone:**
   ```bash
   git clone https://github.com/YOUR_USERNAME/Game.git
   cd Game
   ```

2. **Install Dependencies:**
   ```bash
   # Linux
   sudo apt-get install build-essential cmake git

   # Windows
   # Install Visual Studio 2019+ with C++ tools
   # Install CMake from cmake.org
   ```

3. **Setup vcpkg:**
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git
   ./vcpkg/bootstrap-vcpkg.sh  # Linux
   ./vcpkg/bootstrap-vcpkg.bat # Windows

   # Integrate with your system
   ./vcpkg/vcpkg integrate install
   ```

4. **Build the Project:**
   ```bash
   # See BUILD.md for detailed platform-specific instructions
   cmake --preset=default
   cmake --build build
   ```

5. **Run Tests:**
   ```bash
   cd build
   ctest --output-on-failure
   ```

For complete build instructions, see [BUILD.md](BUILD.md).

---

## 2. Development Workflow

### Workflow Overview

```
1. Create issue/check existing issues
2. Create feature branch from main
3. Make changes following code standards
4. Write tests for new functionality
5. Run tests and sanitizers locally
6. Commit changes with descriptive messages
7. Push to your fork
8. Create pull request
9. Address review feedback
10. Merge after approval
```

### Feature Branch Workflow

**Always work on feature branches:**

```bash
# Update your main branch
git checkout main
git pull upstream main

# Create feature branch
git checkout -b feature/character-diplomacy
# or
git checkout -b fix/save-corruption-bug
# or
git checkout -b refactor/ecs-performance

# Make changes, commit, push
git add .
git commit -m "Add diplomacy influence calculation"
git push origin feature/character-diplomacy
```

**Branch naming convention:**
- `feature/description` - New features
- `fix/description` - Bug fixes
- `refactor/description` - Code refactoring
- `docs/description` - Documentation updates
- `test/description` - Test additions/improvements

---

## 3. Code Standards

### 3.1 Style Guide

**All code must follow [STYLE_GUIDE.md](STYLE_GUIDE.md).**

**Key requirements:**
- **Naming:**
  - Classes/Structs: `PascalCase`
  - Functions: `PascalCase`
  - Variables: `snake_case`
  - Constants: `UPPER_SNAKE_CASE`
- **Formatting:**
  - 4 spaces indentation (no tabs)
  - 120 character line limit
  - K&R brace style
- **Modern C++17:**
  - Smart pointers (no raw pointers for ownership)
  - `const` correctness
  - Range-based for loops
  - `std::optional` for nullable returns
  - `enum class` (scoped enums)

### 3.2 Documentation Requirements

**All public APIs must have Doxygen comments:**

```cpp
/**
 * @brief Calculate character loyalty to their liege
 *
 * Loyalty is influenced by opinion, culture, religion, and relationships.
 * This calculation is performance-critical and called every game tick.
 *
 * @param character_id The vassal character
 * @param liege_id The liege to whom loyalty is calculated
 * @return Loyalty value between 0.0 and 100.0
 * @throws std::invalid_argument if either ID is invalid
 *
 * @note Performance: O(1) with cached values
 * @see ModifyLoyalty() for direct loyalty modifications
 */
double CalculateLoyalty(EntityID character_id, EntityID liege_id);
```

**Comment the "why", not the "what":**

```cpp
// ❌ BAD
// Increment counter
counter++;

// ✅ GOOD
// Filter insignificant relationships that don't impact diplomatic calculations
// Threshold of 25.0 determined by gameplay balancing (see PR #123)
if (bond_strength >= SIGNIFICANT_BOND_THRESHOLD) {
    AddToInfluenceNetwork(character);
}
```

### 3.3 Error Handling

**Follow [docs/ERROR_HANDLING.md](docs/ERROR_HANDLING.md):**

- Use exceptions for exceptional conditions
- Use `std::optional`/error codes for expected failures
- Always log errors before throwing
- Provide strong or basic exception safety
- Never catch and ignore exceptions

### 3.4 Security

**Follow [SECURITY.md](SECURITY.md):**

- Validate all input from external sources (save files, user input, map data)
- Use bounds checking on array/vector access
- No magic numbers - use named constants
- Check for integer overflow in calculations
- Use smart pointers (no manual memory management)

---

## 4. Testing Requirements

### 4.1 Test Coverage

**All PRs must include tests. See [docs/TESTING_STRATEGY.md](docs/TESTING_STRATEGY.md).**

**Minimum coverage requirements:**
- **Critical systems (ECS, Save, Core):** 90%+
- **Game systems (AI, Economy, Diplomacy):** 85%+
- **Other systems:** 80%+

### 4.2 Required Tests

**For new features:**
- [ ] Unit tests for business logic
- [ ] Integration tests for system interactions
- [ ] Performance tests (if performance-critical)
- [ ] Edge case and boundary condition tests

**For bug fixes:**
- [ ] Regression test demonstrating the bug
- [ ] Test verifying the fix

### 4.3 Running Tests Locally

**Before submitting PR:**

```bash
# Build tests
cmake --build build --target tests

# Run all tests
cd build
ctest --output-on-failure

# Run specific test
./tests/test_character_relationships

# Run with AddressSanitizer
cmake -B build-asan -DENABLE_ASAN=ON
cmake --build build-asan
./build-asan/tests/run_all_tests

# Run with ThreadSanitizer (for concurrent code)
cmake -B build-tsan -DENABLE_TSAN=ON
cmake --build build-tsan
./build-tsan/tests/run_tsan_tests
```

**All tests must pass before submitting PR.**

### 4.4 Test Quality

**Good test characteristics:**
- **Fast** - Unit tests < 1ms each
- **Isolated** - No dependencies on external state
- **Deterministic** - Same result every time
- **Readable** - Clear intent
- **Maintainable** - Easy to update

**Example:**
```cpp
TEST(CharacterRelationships, GetFriends_WithSignificantBond_ReturnsFriend) {
    // Arrange
    CharacterRelationshipsComponent comp(EntityID{1});
    EntityID friend_id{2};
    comp.SetRelationship(friend_id, RelationshipType::FRIEND,
                        /*opinion=*/50, /*bond=*/30.0);

    // Act
    auto friends = comp.GetFriends();

    // Assert
    ASSERT_EQ(friends.size(), 1);
    EXPECT_EQ(friends[0], friend_id);
}
```

---

## 5. Pull Request Process

### 5.1 Creating a Pull Request

1. **Push your feature branch to your fork:**
   ```bash
   git push origin feature/your-feature
   ```

2. **Create PR on GitHub:**
   - Go to the main repository
   - Click "New Pull Request"
   - Select your feature branch
   - Fill out the PR template

3. **PR Title Format:**
   ```
   [Type] Brief description

   Examples:
   [Feature] Add character diplomacy influence system
   [Fix] Resolve save file corruption on Windows
   [Refactor] Optimize ECS component lookup
   [Docs] Update architecture documentation
   ```

4. **PR Description Template:**
   ```markdown
   ## Summary
   Brief description of changes (1-3 sentences)

   ## Changes Made
   - Bullet list of specific changes
   - Include files modified
   - Mention any breaking changes

   ## Testing
   - [ ] Unit tests added/updated
   - [ ] Integration tests added/updated
   - [ ] Performance tests added (if applicable)
   - [ ] All tests pass locally
   - [ ] Tested with ASan/TSan (if applicable)

   ## Related Issues
   Closes #123
   Related to #456

   ## Screenshots (if UI changes)
   [Include screenshots if relevant]

   ## Checklist
   - [ ] Code follows STYLE_GUIDE.md
   - [ ] Documentation updated (if needed)
   - [ ] CHANGELOG.md updated (if user-facing change)
   - [ ] No compiler warnings
   - [ ] Tests pass
   ```

### 5.2 PR Requirements

**Before your PR will be reviewed:**

- [ ] All CI checks pass (build + tests on Windows and Linux)
- [ ] Code follows style guide
- [ ] Tests included and passing
- [ ] No merge conflicts with main
- [ ] PR description complete
- [ ] Code is self-reviewed

### 5.3 Automated Checks

**Your PR must pass:**
- ✅ **Build** - Compiles on Windows (MSVC) and Linux (GCC/Clang)
- ✅ **Tests** - All unit and integration tests pass
- ✅ **Static Analysis** - Clang-Tidy checks pass
- ✅ **Format** - ClangFormat validation (optional initially)
- ✅ **Sanitizers** - ASan/TSan pass (for relevant code)

---

## 6. Commit Message Guidelines

### 6.1 Commit Message Format

**Use conventional commit format:**

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Example:**
```
feat(character): Add loyalty calculation based on relationships

Implements loyalty system that considers:
- Opinion of liege
- Cultural/religious alignment
- Relationship bonds (friendships, marriages)
- Historical events

Performance: O(1) with cached relationship data.

Closes #234
```

### 6.2 Commit Types

- `feat`: New feature
- `fix`: Bug fix
- `refactor`: Code refactoring (no behavior change)
- `perf`: Performance improvement
- `test`: Adding or updating tests
- `docs`: Documentation changes
- `style`: Code style/formatting changes
- `build`: Build system or dependencies
- `ci`: CI/CD changes

### 6.3 Commit Scope (Optional)

Examples: `character`, `economy`, `ai`, `ecs`, `save`, `render`, `ui`

### 6.4 Commit Best Practices

**✅ DO:**
```bash
# Atomic commits (one logical change)
git commit -m "feat(economy): Add trade route wealth calculation"

# Descriptive messages
git commit -m "fix(save): Validate JSON schema before deserialization

Prevents crashes from malformed save files by validating
all required fields exist and have correct types.

Fixes #456"

# Reference issues
git commit -m "feat(diplomacy): Implement marriage alliance system

Closes #123"
```

**❌ DON'T:**
```bash
# Vague messages
git commit -m "fix stuff"
git commit -m "wip"

# Multiple unrelated changes
git commit -m "Add feature X, fix bug Y, refactor Z"

# Huge commits (thousands of lines)
# Instead, break into logical atomic commits
```

---

## 7. Code Review Process

### 7.1 Review Checklist

**Reviewers will check:**

**Functionality:**
- [ ] Code does what it's supposed to do
- [ ] Edge cases handled
- [ ] No obvious bugs

**Code Quality:**
- [ ] Follows STYLE_GUIDE.md
- [ ] No magic numbers (named constants used)
- [ ] Functions are focused (single responsibility)
- [ ] Appropriate use of modern C++17 features

**Testing:**
- [ ] Tests included for new functionality
- [ ] Tests actually test the code
- [ ] Tests are clear and maintainable
- [ ] Tests pass locally and in CI

**Security:**
- [ ] Input validation present (external data)
- [ ] No buffer overflow risks
- [ ] Memory safety (smart pointers, bounds checking)
- [ ] No hardcoded credentials

**Performance:**
- [ ] No obvious performance issues
- [ ] Appropriate data structures
- [ ] Thread safety for concurrent code
- [ ] Performance tests for critical paths

**Documentation:**
- [ ] Public APIs documented (Doxygen)
- [ ] Complex logic explained
- [ ] CHANGELOG.md updated (user-facing changes)
- [ ] README updated (if needed)

### 7.2 Addressing Feedback

**Respond to review comments:**

1. **Accept feedback gracefully** - Reviews improve code quality
2. **Ask questions** if you don't understand feedback
3. **Make requested changes** or provide reasoning if you disagree
4. **Push updates** to the same branch (updates PR automatically)
5. **Notify reviewers** when ready for re-review

**Example response:**
```markdown
> Consider using std::optional here instead of raw pointer

Good catch! Changed to std::optional in commit abc123.

> This function is getting long, consider splitting

I've refactored it into three smaller functions:
- ValidateInput()
- ProcessData()
- StoreResult()

Updated in commit def456.
```

### 7.3 Review Timeline

- **Initial review:** Within 2-3 days
- **Follow-up reviews:** Within 1-2 days
- **Approval and merge:** After 1-2 approvals from maintainers

---

## 8. Branch Strategy

### 8.1 Branch Types

**Main Branches:**
- `main` - Production-ready code
- `develop` - Integration branch (if used)

**Supporting Branches:**
- `feature/*` - New features
- `fix/*` - Bug fixes
- `hotfix/*` - Critical production fixes
- `release/*` - Release preparation

### 8.2 Branching Rules

**For contributors:**
```bash
# Always branch from main
git checkout main
git pull upstream main
git checkout -b feature/your-feature

# Keep your branch updated
git fetch upstream
git rebase upstream/main

# Push to your fork
git push origin feature/your-feature
```

**For maintainers:**
- Features merge to `main` via PR
- Hotfixes merge to `main` and backport if needed
- Tag releases with semantic versioning

### 8.3 Merge Strategy

**Use squash merge for features:**
- Keeps main branch history clean
- Preserves full history in PR

**Use merge commit for releases:**
- Preserves version history

---

## 9. Issue Reporting

### 9.1 Bug Reports

**Use the bug report template:**

```markdown
**Describe the bug**
Clear description of what the bug is.

**To Reproduce**
Steps to reproduce:
1. Load save file 'xyz.sav'
2. Click on character '...'
3. See error

**Expected behavior**
What you expected to happen.

**Actual behavior**
What actually happened.

**Screenshots**
If applicable, add screenshots.

**Environment:**
- OS: [e.g., Windows 10, Ubuntu 22.04]
- Compiler: [e.g., MSVC 2022, GCC 11]
- Version: [e.g., v0.2.3]

**Additional context**
Any other relevant information.

**Logs**
```
Paste relevant log output here
```
```

### 9.2 Feature Requests

**Use the feature request template:**

```markdown
**Is your feature request related to a problem?**
Clear description of the problem.

**Describe the solution you'd like**
What you want to happen.

**Describe alternatives you've considered**
Other solutions you've thought about.

**Additional context**
Any other relevant information.

**Implementation ideas (optional)**
Technical ideas for implementing this feature.
```

---

## 10. Development Environment

### 10.1 Recommended IDE Setup

**Visual Studio Code:**
```json
// .vscode/settings.json
{
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
  "editor.formatOnSave": true,
  "editor.rulers": [120],
  "files.trimTrailingWhitespace": true,
  "files.insertFinalNewline": true
}
```

**Visual Studio 2022:**
- Install C++ workload
- Install CMake tools
- Configure ClangFormat extension

**CLion:**
- Built-in CMake support
- Configure ClangFormat
- Enable Clang-Tidy

### 10.2 Code Formatting

**Install ClangFormat:**
```bash
# Linux
sudo apt-get install clang-format

# Windows
# Install with Visual Studio or LLVM
```

**Format before committing:**
```bash
# Format single file
clang-format -i src/game/character/CharacterRelationships.cpp

# Format all changed files
git diff --name-only | grep -E '\.(cpp|h)$' | xargs clang-format -i
```

### 10.3 Static Analysis

**Run Clang-Tidy:**
```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
clang-tidy src/game/character/CharacterRelationships.cpp -p build
```

---

## Community Guidelines

### Code of Conduct

Be respectful and professional:
- Be welcoming to newcomers
- Provide constructive feedback
- Focus on code, not people
- Respect different viewpoints
- No harassment or discrimination

### Getting Help

**Resources:**
- **Documentation:** See [docs/](docs/) folder
- **Architecture:** See [ARCHITECTURE.md](ARCHITECTURE.md)
- **Build issues:** See [BUILD.md](BUILD.md)
- **Discord/Forum:** [Link when available]

**Questions:**
- Check existing issues first
- Ask in discussion forum
- Create issue if bug/feature request

---

## Recognition

Contributors are recognized in:
- CHANGELOG.md credits
- GitHub contributors page
- Annual acknowledgments

---

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (see LICENSE file when added).

---

## Summary Checklist

Before submitting a PR:

- [ ] Code follows STYLE_GUIDE.md
- [ ] Tests added and passing
- [ ] Documentation updated
- [ ] Commit messages follow conventions
- [ ] PR description complete
- [ ] No merge conflicts
- [ ] CI checks pass
- [ ] Code self-reviewed

---

**Thank you for contributing to Mechanica Imperii!**

For questions, contact the maintainers or open a discussion issue.

**Version History:**
- v1.0 (December 2025) - Initial contributing guidelines
