# DiplomacySystem & InfluenceSystem Integration Documentation

## Overview

This directory contains comprehensive documentation of the integration between DiplomacySystem and InfluenceSystem in the game engine. These systems work together to manage diplomatic relationships and sphere of influence mechanics.

## Documentation Files

### 1. integration-analysis.md (COMPREHENSIVE - PRIMARY SOURCE)
**Size**: ~2000 lines | **Purpose**: Complete architectural analysis

Contains:
- Complete DiplomacySystem class structure and all methods
- Opinion modifier system with implementation details
- InfluenceSystem architecture and the seven influence types
- InfluenceCalculator static functions
- All existing integration points with code snippets
- Autonomy and diplomatic freedom calculations
- Bridge system pattern overview
- Missing implementations and TODOs
- Data flow analysis
- Summary of current state

**When to use**: Need full understanding of how the systems work

### 2. integration-quick-reference.md (LOOKUP - QUICK START)
**Size**: ~300 lines | **Purpose**: Quick reference guide

Contains:
- File locations (all relevant files organized by system)
- Key integration points with line numbers
- Opinion modifier system structure and methods
- Seven influence types table (range, decay, sensitivity)
- Monthly update cycle flow
- Configuration keys from GameConfig
- Data structure hierarchies
- Missing implementations checklist
- Example code snippets
- Key differences table

**When to use**: Need to quickly find something specific, or want an overview

### 3. integration-data-flow.txt (VISUAL - DIAGRAMS)
**Size**: ~300 lines | **Purpose**: Visual representation of data flow

Contains:
- ASCII diagrams showing opinion modifier flow
- Detailed example of opinion affecting influence calculation
- Distance decay tables for all seven influence types
- Monthly update cycle synchronization diagram
- Autonomy calculation breakdown with real numbers
- Integration through GetDiplomaticState() call chain
- System initialization and setup flow
- Opinion range mapping
- Modifier decay schedule (60 months timeline)
- Data structure hierarchy diagrams

**When to use**: Need to visualize how data flows between systems

### 4. ../INTEGRATION_SUMMARY.txt (EXECUTIVE SUMMARY)
**Size**: ~400 lines | **Purpose**: Concise overview with findings

Contains:
- Key findings for each system
- Integration points summary
- Opinion modifier system details
- Autonomy system details
- Missing integrations checklist
- Implementation recommendations (4 priority levels)
- File organization summary
- Testing considerations
- Configuration parameters
- Conclusion

**When to use**: Need high-level understanding or executive summary

## Key Concepts

### Opinion System (DiplomacySystem)
- **Range**: [-100, +100]
- **Storage**: OpinionModifier vector with decay
- **Decay**: Exponential (5% per month) for weights, linear for baseline drift
- **Usage**: Applied as relationship_modifier to influence calculations

### Influence System (InfluenceSystem)
- **Seven types**: Military, Economic, Dynastic, Personal, Religious, Cultural, Prestige
- **Calculation**: Base strength × distance_modifier × relationship_modifier
- **Distance decay**: Type-specific (0% to 40% per hop)
- **Opinion sensitivity**: Military and Personal (100%), Economic (50%), Others (0%)

### Autonomy
- **Formula**: 1.0 - (total_influence / 200.0)
- **Range**: [0.0, 1.0] (0% = puppet state, 100% = fully independent)
- **Calculation**: Monthly from total incoming influence
- **Impact**: Should affect diplomatic decisions (mostly TODO)

## Integration Flow

```
DiplomaticState::opinion [-100, +100]
    ↓
InfluenceSystem::GetDiplomaticState()
    ↓
InfluenceCalculator (uses opinion in calculations)
    ↓
InfluenceSource::relationship_modifier = 1.0 + (opinion / 200.0)
    ↓
effective_strength = base × distance × relationship
    ↓
Total influence on realm
    ↓
autonomy = 1.0 - (total / 200.0)
```

## File Locations

### Header Files
| System | File | Lines |
|--------|------|-------|
| Diplomacy | `include/game/diplomacy/DiplomacySystem.h` | 180 |
| Diplomacy | `include/game/diplomacy/DiplomacyComponents.h` | 335 |
| Influence | `include/game/diplomacy/InfluenceSystem.h` | 387 |
| Influence | `include/game/diplomacy/InfluenceComponents.h` | 325 |
| Influence | `include/game/diplomacy/InfluenceCalculator.h` | 291 |
| Bridge | `include/game/bridge/DiplomacyEconomicBridge.h` | 476 |

### Implementation Files
| System | File |
|--------|------|
| Diplomacy | `src/game/diplomacy/DiplomacySystem_minimal.cpp` |
| Diplomacy | `src/game/diplomacy/DiplomacyComponents.cpp` |
| Influence | `src/game/diplomacy/InfluenceSystem.cpp` |
| Influence | `src/game/diplomacy/InfluenceComponents.cpp` |
| Influence | `src/game/diplomacy/InfluenceCalculator.cpp` |
| Bridge | `src/game/bridge/DiplomacyEconomicBridge.cpp` |

## How Opinion Affects Influence

### Example
1. Diplomacy: Realm A and B have opinion = -5 (slight disagreement)
2. Influence calculation:
   - Military strength base = 65
   - Modifier = 1.0 + (-5/200) = 0.975
   - Effective = 65 × 0.975 = 63.4
   - Result: 2.5% reduction in influence

### Opinion Sensitivities
- **Military**: 100% opinion sensitive (full modifier applied)
- **Economic**: 50% opinion sensitive (only half modifier applied)
- **Personal**: 100% opinion sensitive
- **Others**: Not directly opinion sensitive

## Autonomy Thresholds

| Autonomy | Status | Effect |
|----------|--------|--------|
| 0.9-1.0 | Fully Independent | Can make all diplomatic decisions freely |
| 0.5-0.9 | Mostly Independent | Some diplomatic constraints |
| 0.3-0.5 | Dependent | Limited diplomatic freedom |
| 0.1-0.3 | Heavily Dominated | Very restricted choices |
| 0.0-0.1 | Puppet State | Almost no autonomy |

## Important Constants

### Opinion System
- Max opinion: 100
- Min opinion: -100
- Modifier decay: 5% per month (0.95^months)
- Modifier removal threshold: weight < 0.05

### Influence System
- Total influence cap: 200 (half = 1.0 autonomy)
- Military+Economic cap: 150 (affects diplomatic freedom)
- Max hops for propagation: 10
- Min influence threshold: 5.0 (below this ignored)
- Monthly decay rate: 2%

## Testing

See INTEGRATION_SUMMARY.txt for complete testing checklist.

Key tests:
- Opinion modifiers decay correctly
- Autonomy calculated from total influence
- Opinion affects influence calculations
- Monthly synchronization works
- SetDiplomacySystem connects properly

## Next Steps

### Priority 1: Make Autonomy Affect Diplomacy
- Modify proposal acceptance based on autonomy
- Prevent war declarations when autonomy too low
- Add autonomy penalties to actions

### Priority 2: Event System
- Implement NotifyInfluenceChange events
- Create autonomy threshold triggers
- Wire up event handlers

### Priority 3: Feedback Loops
- Conflict outcomes affect autonomy
- Opinion-autonomy bidirectional effects
- Historical data influences calculations

### Priority 4: Testing & Verification
- Unit tests for each component
- Integration tests for full flow
- Configuration validation

## References

- See `include/game/diplomacy/DiplomacyComponents.h` for DiplomaticState structure
- See `include/game/diplomacy/InfluenceComponents.h` for InfluenceSource structure
- See `src/game/diplomacy/InfluenceComponents.cpp` lines 114-142 for autonomy calculation
- See `src/game/diplomacy/DiplomacyComponents.cpp` lines 345-456 for opinion modifiers

## Questions?

- How opinion modifiers work? → See integration-quick-reference.md
- How influence calculation works? → See integration-data-flow.txt section 2
- What's missing in integration? → See INTEGRATION_SUMMARY.txt under MISSING INTEGRATIONS
- Where is code X? → See integration-quick-reference.md FILE LOCATIONS section
