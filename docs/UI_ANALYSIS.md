# UI Analysis: Non-Functional Buttons and Empty Windows

**Analysis Date:** 2025-12-02
**Branch:** claude/analyze-ui-issues-01E1SNDDf9yEZTK1FBcEr51X

## Executive Summary

Out of 78+ buttons found in the UI, **36 buttons (46%)** have TODO/placeholder implementations or are non-functional. Additionally, **7 windows/components** are either completely empty or contain only minimal stub content.

---

## 1. COMPLETELY EMPTY WINDOWS (Priority: High)

These components have empty Render() and Update() methods with only placeholder comments:

### 1.1 SimpleProvincePanel
- **File:** `src/ui/SimpleProvincePanel.cpp`
- **Status:** Completely empty
- **Impact:** Province information cannot be displayed in simple mode

### 1.2 AdministrativeUI
- **File:** `src/ui/AdministrativeUI.cpp`
- **Status:** Completely empty
- **Impact:** Administrative interface unusable

### 1.3 PerformanceWindow
- **File:** `src/ui/PerformanceWindow.cpp`
- **Status:** Completely empty
- **Impact:** No performance metrics visible (debugging tool)

### 1.4 Toast Notification System
- **File:** `src/ui/Toast.cpp`
- **Status:** Show() only logs to console, RenderAll() is empty
- **Impact:** No visual feedback for game events

---

## 2. MINIMAL/STUB CONTENT WINDOWS (Priority: Medium)

### 2.1 RealmWindow
- **File:** `src/ui/RealmWindow.cpp`
- **Status:** Has tab structure but only placeholder text
- **Tabs:** Dynasty, Succession, Court, Vassals
- **Impact:** Realm management interface non-functional

### 2.2 BattleViewerWindow
- **File:** `src/ui/BattleViewerWindow.cpp`
- **Status:** Header notes "placeholder implementation"
- **Impact:** Battle details cannot be viewed properly

### 2.3 NationOverviewWindow
- **File:** `src/ui/NationOverviewWindow.cpp`
- **Status:** Partial - some tabs have "Placeholder data" comments
- **Impact:** National overview shows incomplete information

---

## 3. NON-FUNCTIONAL BUTTONS BY COMPONENT

### 3.1 InGameHUD Pause Menu (5 buttons)
**File:** `src/ui/InGameHUD.cpp`

| Button | Line | Status | Issue |
|--------|------|--------|-------|
| Save Game | 179 | TODO | `// TODO: Show save dialog` |
| Load Game | 186 | TODO | `// TODO: Show load dialog` |
| Settings | 193 | TODO | `// TODO: Open settings window` |
| Exit to Main Menu | 203 | TODO | `// TODO: Confirm and exit to menu` |
| Resume (ESC) | N/A | Works | Functional |

**Impact:** Pause menu is mostly non-functional except Resume button

### 3.2 GameControlPanel (2 buttons)
**File:** `src/ui/GameControlPanel.cpp`

| Button | Line | Status | Issue |
|--------|------|--------|-------|
| Save | 150 | TODO | `// TODO: Trigger save` |
| Load | 159 | TODO | `// TODO: Trigger load` |

**Impact:** Cannot save/load from main control panel

### 3.3 SettingsWindow Advanced Features (5 buttons)
**File:** `src/ui/SettingsWindow.cpp`

| Button | Line | Status | Issue |
|--------|------|--------|-------|
| Test Music | 176 | TODO | `// TODO: Play test music` |
| Test Sound Effect | 180 | TODO | `// TODO: Play test SFX` |
| Clear All Autosaves | 223 | TODO | `// TODO: Implement clear autosaves` |
| Reset Configuration | 332 | TODO | `// TODO: Reset config files` |
| Clear Cache | 340 | TODO | `// TODO: Clear game cache` |

**Additional Issues:**
- Line 149: Master volume not applied to audio system
- Line 158: Music volume not applied to audio system
- Line 167: SFX volume not applied to audio system

**Impact:** Settings cannot be fully applied; advanced features unavailable

### 3.4 EconomyWindow (4 buttons)
**File:** `src/ui/EconomyWindow.cpp`

| Button | Line | Status | Issue |
|--------|------|--------|-------|
| Borrow Money | 119 | TODO | `// TODO: Implement borrow money dialog` |
| Emergency Tax | 128 | TODO | `// TODO: Implement emergency tax` |
| Send Gift | 137 | TODO | `// TODO: Implement send gift dialog` |
| Build | 340 | TODO | `// TODO: Implement building construction` |

**Additional Issues:**
- Line 152: Treasury graph not implemented
- Line 223: Tax rate slider doesn't apply to economic system
- Line 364: Construction queue display missing
- Line 377: Province development interface missing

**Impact:** Economic actions cannot be executed

### 3.5 MilitaryWindow (1 button + data issues)
**File:** `src/ui/MilitaryWindow.cpp`

| Button | Line | Status | Issue |
|--------|------|--------|-------|
| Recruit | 436 | TODO | `// TODO: Implement recruitment` |

**Additional Issues:**
- Line 88: Naval support not implemented
- Line 101: Morale calculation placeholder
- Line 107: Professionalism tracking missing
- Line 119: Active battles data placeholder
- Line 159: Army units list placeholder
- Line 460: Recruitment queue display missing
- Line 475: Active battles list missing

**Impact:** Military recruitment and naval management unavailable

### 3.6 DiplomacyWindow (5 buttons)
**File:** `src/ui/DiplomacyWindow.cpp`

| Button | Line | Status | Issue |
|--------|------|--------|-------|
| Break Treaty | 120 | TODO | `// TODO: Implement break treaty` |
| Propose Alliance | 142 | TODO | `// TODO: Open nation selector for alliance` |
| Propose Trade Deal | 146 | TODO | `// TODO: Open nation selector for trade` |
| Propose Peace | 150 | TODO | `// TODO: Open peace negotiation dialog` |
| Declare War | 193 | TODO | `// TODO: Open nation selector and CB selection dialog` |

**Additional Issues:**
- Line 59: Portrait integration placeholder
- Line 171: Active wars display incomplete

**Impact:** All diplomatic actions are non-functional

### 3.7 AdministrativeWindow (7 buttons)
**File:** `src/ui/AdministrativeWindow.cpp`

| Button | Status | Note |
|--------|--------|------|
| Enact Administrative Reforms | Placeholder | Shows placeholder message |
| Dismiss (per official) | Placeholder | Shows placeholder message |
| Appoint Official | Placeholder | Shows placeholder message |
| Expand Bureaucracy (+5 Clerks) | Placeholder | Shows placeholder message |
| Improve Record Keeping | Placeholder | Shows placeholder message |
| Establish Court | Placeholder | Shows placeholder message |
| Appoint Judge | Placeholder | Shows placeholder message |

**Impact:** Administrative actions show placeholder responses

### 3.8 TradeSystemWindow (1 button)
**File:** `src/ui/TradeSystemWindow.cpp`

| Button | Line | Status | Issue |
|--------|------|--------|-------|
| Establish Route | 497 | TODO | `// TODO: Call trade_system_.EstablishTradeRoute()` |

**Impact:** Cannot establish new trade routes

### 3.9 SaveLoadDialog (1 issue)
**File:** `src/ui/SaveLoadDialog.cpp`

| Issue | Line | Status |
|-------|------|--------|
| File scanning | 202 | TODO: `// TODO: Scan save directory for .sav files` |

**Impact:** Cannot see saved game files in dialog

---

## 4. SUMMARY STATISTICS

| Category | Count | Percentage |
|----------|-------|------------|
| Total UI Components | 27 | 100% |
| Completely Empty Components | 4 | 15% |
| Minimal/Stub Components | 3 | 11% |
| Total Buttons Found | 78+ | 100% |
| Working Buttons | 42 | 54% |
| Non-Functional Buttons | 36 | 46% |
| Total TODO Comments | 45+ | N/A |

---

## 5. FULLY FUNCTIONAL COMPONENTS ✓

These components are complete and working:

1. **MainMenuUI** - All menu items functional
2. **GameControlPanel** - Speed controls working (save/load pending)
3. **WindowManager** - Window management fully functional
4. **LeftSidebar** - All icon buttons working
5. **MapModeSelector** - All map modes working
6. **NationSelector** - START GAME working
7. **TechnologyInfoWindow** - Most complete game window
8. **SaveLoadDialog** - Save/Load operations work (file scanning TODO)

---

## 6. PRIORITY RECOMMENDATIONS

### High Priority (Core Gameplay)
1. **EconomyWindow buttons** - Economic actions critical to gameplay
2. **DiplomacyWindow buttons** - Diplomatic actions essential
3. **MilitaryWindow Recruit button** - Military recruitment needed
4. **InGameHUD pause menu** - Basic game functions (save/load/settings/exit)
5. **Toast notification system** - User feedback essential

### Medium Priority (Management)
6. **RealmWindow content** - Dynasty and succession management
7. **SettingsWindow audio controls** - Settings need to apply properly
8. **GameControlPanel save/load** - Duplicate of pause menu but visible

### Low Priority (Advanced/Debug)
9. **AdministrativeWindow placeholders** - Replace placeholder messages with real logic
10. **TradeSystemWindow Establish Route** - Trade system API integration
11. **PerformanceWindow** - Debug/development tool
12. **SimpleProvincePanel** - Alternative to existing ProvinceInfoWindow
13. **AdministrativeUI** - May be redundant with AdministrativeWindow
14. **BattleViewerWindow details** - Battle system integration
15. **SettingsWindow advanced buttons** - Maintenance tools

---

## 7. MISSING SUPPORTING SYSTEMS

Several non-functional buttons require supporting UI components that don't exist yet:

1. **Nation Selector Dialog** - Needed for:
   - Propose Alliance
   - Propose Trade Deal
   - Declare War

2. **Confirmation Dialogs** - Needed for:
   - Exit to Main Menu
   - Break Treaty
   - Clear Autosaves
   - Reset Configuration

3. **Specialized Dialogs** - Needed for:
   - Borrow Money
   - Send Gift
   - Peace Negotiation
   - Casus Belli Selection
   - Building Construction

4. **Integration with Settings** - Needed for:
   - Open Settings from pause menu
   - Apply audio settings to audio system

---

## 8. FILES REQUIRING ATTENTION

### Empty Files (Immediate Action)
- `src/ui/SimpleProvincePanel.cpp`
- `src/ui/AdministrativeUI.cpp`
- `src/ui/PerformanceWindow.cpp`
- `src/ui/Toast.cpp`

### Files with Multiple TODOs (Prioritize)
- `src/ui/InGameHUD.cpp` (5 TODOs)
- `src/ui/EconomyWindow.cpp` (7 TODOs)
- `src/ui/MilitaryWindow.cpp` (5 TODOs)
- `src/ui/DiplomacyWindow.cpp` (6 TODOs)
- `src/ui/SettingsWindow.cpp` (10 TODOs)

### Files with Placeholder Content
- `src/ui/RealmWindow.cpp`
- `src/ui/BattleViewerWindow.cpp`
- `src/ui/AdministrativeWindow.cpp`

---

## 9. RECOMMENDED IMPLEMENTATION ORDER

Based on impact and dependencies:

1. **Phase 1 - Critical Gameplay**
   - Toast notification system (feedback mechanism)
   - EconomyWindow: Borrow Money, Emergency Tax
   - DiplomacyWindow: Nation selector infrastructure
   - MilitaryWindow: Recruit button

2. **Phase 2 - Core Features**
   - InGameHUD: Save/Load/Settings/Exit from pause menu
   - DiplomacyWindow: Alliance, Trade Deal, War Declaration
   - EconomyWindow: Build button and construction queue
   - SettingsWindow: Apply audio settings properly

3. **Phase 3 - Management**
   - RealmWindow: Dynasty, Succession, Court tabs
   - MilitaryWindow: Naval support, recruitment queue
   - TradeSystemWindow: Establish Route
   - SaveLoadDialog: File scanning

4. **Phase 4 - Polish**
   - AdministrativeWindow: Replace placeholders with real logic
   - BattleViewerWindow: Complete battle integration
   - SettingsWindow: Advanced maintenance buttons
   - NationOverviewWindow: Complete placeholder data

5. **Phase 5 - Optional**
   - PerformanceWindow: Debug metrics
   - SimpleProvincePanel: Alternative province view
   - AdministrativeUI: Evaluate if needed vs AdministrativeWindow

---

## 10. NOTES

- The UI architecture is solid with good separation of concerns
- WindowManager system works well
- Most display-only functionality works correctly
- Primary gap is in action buttons that modify game state
- Many buttons require supporting dialog systems before implementation
- Some components may be redundant (SimpleProvincePanel vs ProvinceInfoWindow, AdministrativeUI vs AdministrativeWindow)

**End of Analysis**
