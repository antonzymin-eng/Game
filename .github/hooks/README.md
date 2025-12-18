# Pre-commit Hooks Setup Guide

This directory contains pre-commit hooks for automatic code quality checks.

## Quick Start

Choose ONE of the following methods:

### Method 1: Pre-commit Framework (Recommended)

The pre-commit framework provides a comprehensive set of hooks with automatic updates.

**Install:**
```bash
# Install pre-commit (requires Python)
pip install pre-commit

# Install the hooks
pre-commit install

# Optional: Run on all files to test
pre-commit run --all-files
```

**Usage:**
Hooks run automatically on `git commit`. To skip hooks (not recommended):
```bash
git commit --no-verify
```

**Configuration:** `.pre-commit-config.yaml` in project root

### Method 2: Simple Git Hook (Alternative)

A minimal hook that only runs clang-format.

**Install:**
```bash
# Copy the hook to your .git/hooks directory
cp .github/hooks/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

**Requirements:**
- clang-format-18 must be installed: `sudo apt-get install clang-format-18`

**Usage:**
Automatically runs on `git commit`. To skip:
```bash
git commit --no-verify
```

## What the Hooks Do

### Pre-commit Framework (.pre-commit-config.yaml)
1. **clang-format** - Formats C++ code according to .clang-format
2. **trailing-whitespace** - Removes trailing whitespace
3. **end-of-file-fixer** - Ensures files end with newline
4. **check-yaml** - Validates YAML syntax
5. **check-json** - Validates JSON syntax
6. **check-added-large-files** - Prevents commits of large files (>1MB)
7. **check-case-conflict** - Detects case conflicts
8. **check-merge-conflict** - Detects merge conflict markers
9. **mixed-line-ending** - Enforces LF line endings
10. **editorconfig-checker** - Validates .editorconfig compliance
11. **cmake-format** - Formats CMakeLists.txt files

### Simple Hook (.github/hooks/pre-commit)
1. **clang-format** - Formats staged C++ files only

## File Exclusion

**IMPORTANT:** clang-format does NOT natively support `.clang-format-ignore` files.

Unlike tools like ESLint (`.eslintignore`) or Prettier (`.prettierignore`), clang-format has no built-in ignore file mechanism. Files must be excluded through other means.

### How to Exclude Files

**Method 1: Pre-commit Framework (Recommended)**

Edit `.pre-commit-config.yaml` and add an `exclude` pattern to the clang-format hook:

```yaml
- repo: https://github.com/pre-commit/mirrors-clang-format
  rev: v18.1.3
  hooks:
    - id: clang-format
      types_or: [c++, c]
      args: ['-i']
      exclude: '^(archive/|build/|vcpkg/|vcpkg_installed/|docs/api/)'  # Excluded paths
```

**Current exclusions:**
- `archive/` - Archived/deprecated code
- `build/` - Build artifacts
- `vcpkg/` and `vcpkg_installed/` - Package manager files
- `docs/api/` - Generated documentation

**Method 2: Simple Git Hook**

The simple hook in `.github/hooks/pre-commit` only formats files returned by `git diff --cached`, so it naturally excludes untracked files. It uses null-delimited output to safely handle filenames with spaces.

**Method 3: CI Workflow**

The CI workflow in `.github/workflows/code-quality.yml` only checks files in `src/` and `include/` directories:

```bash
find src include -name "*.cpp" -o -name "*.h"
```

This approach naturally excludes `archive/`, `build/`, `vcpkg/`, etc.

### Why No .clang-format-ignore?

This is a known limitation of clang-format. The tool only reads `.clang-format` configuration files and has no native ignore file support. Various workarounds exist:
- Wrapper scripts that filter files before calling clang-format
- Pre-commit framework with exclude patterns (our approach)
- Limiting `find` commands to specific directories (CI approach)

## Troubleshooting

### Hook is slow
The pre-commit framework can be slow on first run. Subsequent runs are cached and much faster.

### Hook fails with "command not found"
Install required tools:
```bash
# For clang-format
sudo apt-get install clang-format-18

# For pre-commit framework
pip install pre-commit
```

### Want to temporarily skip hooks
```bash
git commit --no-verify
```

### Update pre-commit hooks
```bash
pre-commit autoupdate
```

### Run hooks manually
```bash
# Pre-commit framework
pre-commit run --all-files

# Simple hook (run from project root)
.github/hooks/pre-commit
```

## Best Practices

1. **Install hooks early** - Set up before making changes
2. **Don't skip hooks** - They catch issues before CI does
3. **Keep hooks updated** - Run `pre-commit autoupdate` monthly
4. **Fix issues, don't skip** - If hooks fail, fix the issue
5. **Test before committing** - Run `pre-commit run --all-files` after big changes

## Comparison

| Feature | Pre-commit Framework | Simple Hook |
|---------|---------------------|-------------|
| Setup Complexity | Medium | Low |
| Coverage | Comprehensive (11 checks) | Minimal (1 check) |
| Speed | Slower (first run) | Fast |
| Dependencies | Python + pip | clang-format only |
| Auto-updates | Yes | No |
| Recommended for | Teams, large projects | Solo devs, simple needs |

## Integration with CI

Both methods complement CI workflows in `.github/workflows/code-quality.yml`:
- **Hooks**: Fast feedback during development
- **CI**: Authoritative checks before merge

Hooks use the same tools and configurations as CI for consistency.

## Disabling Specific Checks

### Pre-commit Framework
Edit `.pre-commit-config.yaml`:
```yaml
# Comment out unwanted hooks
# - id: trailing-whitespace
```

### Simple Hook
Edit `.github/hooks/pre-commit` and modify the script.

## Team Recommendations

**For teams:** Use pre-commit framework
- Ensures consistency across all developers
- Comprehensive checks catch more issues
- Auto-updates keep tools current

**For solo developers:** Either method works
- Simple hook if you only want formatting
- Pre-commit framework for comprehensive checks

## Questions?

See:
- Pre-commit framework: https://pre-commit.com
- Project style guide: `STYLE_GUIDE.md`
- Contribution guide: `CONTRIBUTING.md`
