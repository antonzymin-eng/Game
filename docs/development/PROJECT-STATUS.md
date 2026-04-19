# Project Status

**Status Date:** 2026-04-19  
**Source:** Repository snapshot in `/home/runner/work/Game/Game`

---

## Current Repository Reality

- Build system is CMake (`/home/runner/work/Game/Game/CMakeLists.txt`) with platform presets in `/home/runner/work/Game/Game/CMakePresets.json`.
- The main executable target remains `mechanica_imperii`.
- System source groups currently wired into the main target include:
  - Core/config/save/threading/ECS utilities
  - Administrative, military, population, province management, time, technology, economy, diplomacy
  - Scenario, gameplay, testing, realm, trade, data, news
  - Character and AI systems
  - UI, rendering, and map systems
- Repository scale (quick snapshot):
  - `src/*.cpp`: 172 files
  - `include/*.h`: 200 files
  - `tests/*.cpp`: 53 files

---

## Verification Run in This Session

Attempted local configure command:

```bash
cmake --preset linux-debug
```

Result in this environment:

- ❌ Configure failed because `sdl2` is not installed in the sandbox (`pkg-config` could not find `sdl2`).
- No additional build/test claims are made beyond this environment-specific dependency failure.

---

## Tracking Notes

- This file is the canonical rolling status tracker for the `docs/development` area.
- Dated snapshots remain available for historical context:
  - `/home/runner/work/Game/Game/docs/development/PROJECT-STATUS-2025-10-20.md`
  - `/home/runner/work/Game/Game/docs/development/PROJECT-STATUS-2025-10-21.md`
- Work log entries are tracked in:
  - `/home/runner/work/Game/Game/docs/development/WORK-SESSION-LOG.md`

