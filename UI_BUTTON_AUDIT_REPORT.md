# Nation Systems UI Button Audit Report
**Date:** 2025-12-01
**Purpose:** Identify all buttons in nation system UI windows that exist but have no implementation

---

## Executive Summary

Analyzed 9 nation system UI windows and found **13 non-functional buttons** across 4 windows. Additionally, 1 window (RealmWindow) is completely non-functional with only placeholder tabs.

---

## Detailed Findings by Window

### 1. DiplomacyWindow.cpp ❌
**Location:** `src/ui/DiplomacyWindow.cpp`
**Status:** 5 non-functional buttons

#### Treaties Tab:
- **"Break Treaty" button** (line 119)
  - TODO comment: `// TODO: Implement break treaty`
  - Expected: Call `diplomacy_system_.BreakTreaty(current_player_entity_, treaty_id)`

- **"Propose Alliance" button** (line 141)
  - TODO comment: `// TODO: Open nation selector for alliance`
  - No system call implemented

- **"Propose Trade Deal" button** (line 145)
  - TODO comment: `// TODO: Open nation selector for trade`
  - No system call implemented

- **"Propose Peace" button** (line 149)
  - TODO comment: `// TODO: Open peace negotiation dialog`
  - No system call implemented

#### Wars Tab:
- **"Declare War" button** (line 192)
  - TODO comment: `// TODO: Open nation selector and CB selection dialog`
  - Expected: Call `diplomacy_system_.DeclareWar(current_player_entity_, target_nation, casus_belli)`

---

### 2. EconomyWindow.cpp ❌
**Location:** `src/ui/EconomyWindow.cpp`
**Status:** 5 non-functional buttons/controls

#### Treasury Tab:
- **"Borrow Money" button** (line 118)
  - TODO comment: `// TODO: Implement borrow money dialog`
  - No implementation

- **"Emergency Tax" button** (line 127)
  - TODO comment: `// TODO: Implement emergency tax`
  - Comment indicates: "Add money but reduce stability"

- **"Send Gift" button** (line 136)
  - TODO comment: `// TODO: Implement send gift dialog`
  - Comment indicates: "Show nation selector and amount"

#### Income Tab:
- **Tax Rate Slider** (lines 220-225)
  - TODO comment: `// TODO: Apply tax rate to economic system`
  - Expected: Call `economic_system_.SetTaxRate(current_player_entity_, tax_rate_slider_)`
  - Slider updates local variable but doesn't apply changes

#### Buildings Tab:
- **"Build" button** (line 339)
  - TODO comment: `// TODO: Implement building construction`
  - Expected: Call `economic_system_.StartConstruction(current_player_entity_, building_type)`
  - Appears for each building type: Workshop, Market, Barracks, Temple, University

---

### 3. MilitaryWindow.cpp ❌
**Location:** `src/ui/MilitaryWindow.cpp`
**Status:** 1 non-functional button (repeated for each unit type)

#### Recruitment Tab:
- **"Recruit" button** (line 435)
  - TODO comment: `// TODO: Implement recruitment`
  - Expected: Call `military_system_.RecruitUnit(current_player_entity_, unit_type, recruit_count)`
  - Non-functional for all 5 unit types: Infantry, Cavalry, Archers, Knights, Siege Equipment

---

### 4. TradeSystemWindow.cpp ❌
**Location:** `src/ui/TradeSystemWindow.cpp`
**Status:** 1 non-functional button

#### Opportunities Tab:
- **"Establish Route" button** (line 494)
  - TODO comment: `// TODO: Call trade_system_.EstablishTradeRoute() with parsed parameters`
  - Shows placeholder message instead of establishing route
  - Button appears for each trade opportunity found

---

### 5. AdministrativeWindow.cpp ✅
**Location:** `src/ui/AdministrativeWindow.cpp`
**Status:** ALL BUTTONS FUNCTIONAL

This window is exemplary - all buttons have working implementations:
- ✅ "Enact Administrative Reforms" → calls `administrative_system_.ProcessAdministrativeReforms()`
- ✅ "Dismiss" official → calls `administrative_system_.DismissOfficial()`
- ✅ "Appoint Official" → calls `administrative_system_.AppointOfficial()`
- ✅ "Expand Bureaucracy" → calls `administrative_system_.ExpandBureaucracy()`
- ✅ "Improve Record Keeping" → calls `administrative_system_.ImproveRecordKeeping()`
- ✅ "Establish Court" → calls `administrative_system_.EstablishCourt()`
- ✅ "Appoint Judge" → calls `administrative_system_.AppointJudge()`
- ✅ Governance type combo → calls `administrative_system_.UpdateGovernanceType()`

---

### 6. TechnologyInfoWindow.cpp ℹ️
**Location:** `src/ui/TechnologyInfoWindow.cpp`
**Status:** Display-only window (no buttons)

This is an informational window showing:
- Research progress and statistics
- Technology tree visualization
- Innovation metrics
- Knowledge network data

No interactive buttons requiring implementation.

---

### 7. TradeSystemWindow.cpp (Display Tabs) ℹ️
**Location:** `src/ui/TradeSystemWindow.cpp`
**Status:** Mostly display-only

Most tabs are informational (Trade Routes, Trade Hubs, Market Analysis).
Only the Opportunities tab has the non-functional "Establish Route" button documented above.

---

### 8. PopulationInfoWindow.cpp ℹ️
**Location:** `src/ui/PopulationInfoWindow.cpp`
**Status:** Display-only window (no buttons)

This is a purely informational window showing:
- Population statistics and demographics
- Employment data
- Culture and religion distribution

No interactive buttons.

---

### 9. RealmWindow.cpp ⚠️
**Location:** `src/ui/RealmWindow.cpp`
**Status:** COMPLETELY NON-FUNCTIONAL

This window has 4 tabs but ALL are empty placeholders:
- **Dynasty Tab** - Only displays: "Dynasty information and family tree"
- **Succession Tab** - Only displays: "Succession laws and heirs"
- **Court Tab** - Only displays: "Court members and advisors"
- **Vassals Tab** - Only displays: "Vassal realms and their status"

No actual functionality, data display, or buttons implemented.

---

### 10. NationOverviewWindow.cpp ℹ️
**Location:** `src/ui/NationOverviewWindow.cpp`
**Status:** Display-only window (no buttons)

Shows nation statistics with placeholder data in tabs:
- Economy tab
- Military tab
- Diplomacy tab

No interactive buttons.

---

## Summary Statistics

| Window | Total Buttons | Non-Functional | Functional | Status |
|--------|--------------|----------------|------------|--------|
| DiplomacyWindow | 5 | 5 | 0 | ❌ |
| EconomyWindow | 5 | 5 | 0 | ❌ |
| MilitaryWindow | 5 | 5 | 0 | ❌ |
| TradeSystemWindow | 1 | 1 | 0 | ❌ |
| AdministrativeWindow | 8 | 0 | 8 | ✅ |
| TechnologyInfoWindow | 0 | 0 | 0 | ℹ️ |
| PopulationInfoWindow | 0 | 0 | 0 | ℹ️ |
| RealmWindow | 0 | N/A | 0 | ⚠️ |
| NationOverviewWindow | 0 | 0 | 0 | ℹ️ |

**Total Non-Functional Buttons:** 16 (counting each unit type recruit button separately)
**Total Functional Buttons:** 8 (all in AdministrativeWindow)

---

## Recommendations

### High Priority
1. **Diplomacy System Integration** - 5 critical diplomatic actions are non-functional
2. **Economic Actions** - Core economic tools (loans, taxes, buildings) need implementation
3. **Military Recruitment** - Essential military function is blocked

### Medium Priority
4. **Trade Route Establishment** - Manual trade route creation is non-functional
5. **Realm Window** - Entire window needs implementation from scratch

### Low Priority (Already Working)
- AdministrativeWindow is fully functional and can serve as a reference implementation
- Display-only windows are working as intended

---

## Code Quality Notes

- All non-functional buttons have clear TODO comments indicating what needs to be implemented
- Most buttons have tooltips explaining expected functionality
- The AdministrativeWindow demonstrates the correct pattern for implementing functional buttons
- Button handlers are properly structured, just missing the system calls

---

## Next Steps

1. Prioritize implementation based on gameplay impact
2. Use AdministrativeWindow as a reference for proper button implementation
3. Ensure all system backends support the expected operations before implementing UI calls
4. Add user feedback (toasts/notifications) when operations succeed or fail
5. Consider implementing RealmWindow as a completely new feature or removing it if not needed
