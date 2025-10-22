# Git Baseline Tag Instructions
**Date:** October 22, 2025  
**Tag:** v1.0.0-build-ready-2025-10-22

---

## Purpose

This tag marks a reproducible baseline after:
- CMake presets integration (Ninja + Visual Studio)
- Build system cleanup (duplicates removed, target-scoped includes)
- Documentation alignment (BUILD.md, README.md, AI_CONTEXT.md updated)
- vcpkg manifest dependency management

---

## Tag Commands

### Create Annotated Tag

```bash
# Navigate to repository root
cd /path/to/Game

# Create annotated tag with message
git tag -a v1.0.0-build-ready-2025-10-22 -m "Build system modernization - Oct 22, 2025

Changes:
- CMake presets for Windows (Ninja + Visual Studio) and Linux
- Target-scoped includes (no global pollution)
- vcpkg manifest (vcpkg.json) for reproducible dependencies
- Documentation updates (BUILD.md, README.md, AI_CONTEXT.md)
- Fixed: duplicate options, variable mismatches, output directories

Status: 18 systems operational, Windows requires cmake reconfigure"

# Verify tag
git tag -l -n9 v1.0.0-build-ready-2025-10-22

# Push tag to remote (if applicable)
git push origin v1.0.0-build-ready-2025-10-22
```

---

## Tag Details

**Systems Operational:** 18 total
- Core: 4 (ECS, Threading, Save, Config)
- Game: 8 (Economy, Population, Military, Admin, Diplomacy, Tech, Time, Province)
- AI: 5 (Director, Nation, Character, Attention, Info Propagation)
- Render: 1 (Map with LOD 0-3)

**Build Presets:**
- `windows-debug` / `windows-release` (Ninja)
- `windows-vs-debug` / `windows-vs-release` (Visual Studio)
- `dev` (Ninja with tests)
- `linux-debug` / `linux-release` (Ninja)

**Key Files:**
- CMakeLists.txt (target-scoped, 18 systems)
- CMakePresets.json (7 presets)
- vcpkg.json (6 dependencies: SDL2, glad, jsoncpp, openssl, lz4, imgui)
- BUILD.md (preset-based instructions)
- README.md (updated status, preset table)
- AI_CONTEXT.md (complete context with presets)

---

## Reproducibility

To reproduce this exact build state:

```bash
# Clone and checkout tag
git clone <repo-url>
cd Game
git checkout v1.0.0-build-ready-2025-10-22

# Set vcpkg environment
export VCPKG_ROOT="/path/to/vcpkg"  # or $env:VCPKG_ROOT on Windows

# Configure and build
cmake --preset windows-release  # or linux-release
cmake --build --preset windows-release

# Expected output
build/windows-release/bin/mechanica_imperii.exe
```

---

## Verification Checklist

After applying this tag, verify:
- [ ] CMake configuration completes without errors
- [ ] Build completes successfully (all 18 systems)
- [ ] Executable runs: `./build/<preset>/bin/mechanica_imperii`
- [ ] Dependencies auto-installed via vcpkg manifest
- [ ] WindowsCleanup.h force-included (no macro conflicts)
- [ ] Output directory is `bin/` (not `Release/` or `Debug/`)

---

## Related Documentation

- BUILD.md - Complete build instructions
- ARCHITECTURE.md - Technical architecture (with SystemType enum note)
- AI_CONTEXT.md - Full project context for AI assistants
- README.md - Project overview and quick start

---

## Notes

**Windows Build:**
- Requires CMake reconfigure after applying these changes
- Run: `cmake --preset windows-release` to apply CMakeLists.txt fixes

**Linux Build:**
- Fully operational, no additional steps

**Future Tags:**
- Next milestone: LOD 4 terrain rendering implementation
- CI/CD integration with GitHub Actions
