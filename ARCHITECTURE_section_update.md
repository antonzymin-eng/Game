# ARCHITECTURE.md - Section Update
# Location: Line 128-151
# Date: October 22, 2025

---

### System Types (from `game_types.h`)

```cpp
enum class SystemType : uint8_t {
    INVALID = 0,
    ECS_FOUNDATION,
    MESSAGE_BUS,
    THREADING,
    ECONOMICS,
    MILITARY,
    DIPLOMACY,
    POPULATION,
    CONSTRUCTION,
    TECHNOLOGY,
    TRADE,
    PROVINCIAL_GOVERNANCE,
    REALM_MANAGEMENT,
    TIME_MANAGEMENT,
    AI_DIRECTOR,
    INFORMATION_PROPAGATION,
    ATTENTION_MANAGER,
    MAX_SYSTEM_TYPE
};
```

**Note:** Enum values like `CONSTRUCTION` and `TRADE` represent planned systems not yet included in the current CMake build. Currently implemented systems (18 total) are documented in README.md and AI_CONTEXT.md.

---

## Instructions for Manual Update

Replace lines 128-151 in `/mnt/project/ARCHITECTURE.md` with the content above.

The only change is the addition of the **Note:** paragraph after the enum definition.

**Why this change:**
- Clarifies that not all enum values are currently implemented
- Prevents confusion when comparing enum to CMake source lists
- Points to authoritative documentation for implemented systems
