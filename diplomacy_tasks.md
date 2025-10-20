# DiplomacySystem - Implementation Task List

## 📋 Overview
**Current Status:** ✅ **BUNDLE A COMPLETE** - 28 TODOs remaining (13 completed)
**Last Updated:** October 20, 2025
**Build Status:** ✅ Clean compilation (9.6MB executable)

**Files Involved:**
- `include/game/diplomacy/DiplomacySystem.h` (213 lines)
- `src/game/diplomacy/DiplomacySystem.cpp` (main implementation) ✅ **BUNDLE A COMPLETE**
- `include/game/diplomacy/DiplomacyComponents.h` (data structures)
- `src/game/diplomacy/DiplomacyComponents.cpp` (component implementation)
- `src/game/diplomacy/DiplomacySystemSerialization.cpp` (save/load)

**Progress:**
- ✅ **Bundle A (Core Diplomacy)**: 13/13 methods implemented
- ⏳ **Bundle B (War & Peace)**: 0/5 methods implemented
- ⏳ **Bundle C (AI & Updates)**: 0/6 methods implemented
- ⏳ **Bundle D (Advanced Features)**: 0/7 methods implemented

---

## ✅ **COMPLETED: Bundle A - Core Diplomacy** (13 methods)

### Embassy System ✅ COMPLETE
**Status:** All 3 methods implemented and tested
```cpp
✅ EstablishEmbassy() - Creates relationships, +10 opinion, +5% trust
✅ RecallAmbassador() - -15 opinion, -10% trust, tracks incidents
✅ SendDiplomaticGift() - Dynamic opinion bonus (5-30), +2% trust
```

### Treaty System ✅ COMPLETE
**Status:** All 4 methods implemented and tested
```cpp
✅ ProposeTradeAgreement() - Creates treaties using AddTreaty() helper
✅ ProcessTreatyCompliance() - Compliance checking with 1% decay
✅ UpdateTreatyStatus() - Expiration and broken treaty detection
✅ HandleTreatyViolation() - -30 opinion, -30% trust, reputation hit
```

### Helper Calculation Methods ✅ COMPLETE
**Status:** All 7 methods implemented and tested
```cpp
✅ CalculateBaseOpinion() - Comprehensive opinion calculation (-100 to +100)
✅ CalculateAllianceValue() - Military & reputation assessment (0.0-1.0)
✅ CalculateWarScore() - Relative strength comparison (0.0-1.0)
✅ FindBestCasusBelli() - War justification logic (enum return)
✅ EvaluateAllianceProposal() - AI acceptance calculation (0.0-1.0)
✅ EvaluateTradeProposal() - Trade evaluation (0.0-1.0)
✅ EvaluateMarriageProposal() - Marriage acceptance (0.0-1.0)
```

### Supporting Implementation ✅ COMPLETE
```cpp
✅ LogDiplomaticEvent() - Logging utility for all diplomatic actions
```

**Bundle A Implementation Notes:**
- Used component helper methods: ModifyOpinion(), GetRelationship(), AddTreaty(), HasTreatyType()
- Correct API usage: `game::military::MilitaryComponent`, `GetTotalGarrisonStrength()`
- Proper enum values: `TreatyType::NON_AGGRESSION`, `CasusBelli::BROKEN_TREATY`
- All methods follow const correctness patterns
- Comprehensive null checking and error handling

---

## 🎯 High Priority Tasks (Core Functionality)

### Task 1: Treaty System Implementation ✅ **COMPLETE**
**Status:** ✅ All methods implemented (Bundle A)
**Files:** `DiplomacySystem.cpp` (lines 302-368)
**Complexity:** Medium
**Dependencies:** None
**TODOs resolved:** 4/4 ✅
```cpp
// Implement:
- ProposeTradeAgreement() - line 302
- ProcessTreatyCompliance() - line 350
- UpdateTreatyStatus() - line 356
- HandleTreatyViolation() - line 362
```
**Expected deliverables:**
- Functional treaty proposal system
- Treaty compliance checking
- Treaty expiration handling
- Violation consequences

---

### Task 2: Marriage Diplomacy System ⏳ **PENDING** (Bundle B)
**Files:** `DiplomacySystem.cpp` (lines 318-325)
**Complexity:** Medium
**Dependencies:** Character system (may need stubs)
**TODOs to resolve:** 2/2
```cpp
// Implement:
- ArrangeMarriage() - line 318 ⏳
- ProcessMarriageEffects() - line 325 ⏳
```
**Expected deliverables:**
- Marriage proposal logic
- Alliance creation through marriage
- Dynastic claim tracking
- Opinion modifiers from marriages

---

### Task 3: War Declaration & Peace System ⏳ **PENDING** (Bundle B)
**Files:** `DiplomacySystem.cpp` (lines 405-413)
**Complexity:** High
**Dependencies:** Military system integration
**TODOs to resolve:** 3/3
```cpp
// Implement:
- ProcessWarDeclaration() - line 405 ⏳
- HandleAllyActivation() - line 409 ⏳
- ProcessPeaceNegotiation() - line 413 ⏳
```
**Expected deliverables:**
- War declaration with casus belli
- Ally call-to-arms system
- Peace treaty negotiation
- War score calculation (✅ helper method complete)

---

### Task 4: Embassy & Communication System ✅ **COMPLETE** (Bundle A)
**Status:** ✅ All methods implemented
**Files:** `DiplomacySystem.cpp` (lines 331-344)
**Complexity:** Low
**Dependencies:** None
**TODOs to resolve:** 3
```cpp
// Implement:
- EstablishEmbassy() - line 331
- RecallAmbassador() - line 338
- SendDiplomaticGift() - line 344
```
**Expected deliverables:**
- Embassy establishment logic
- Ambassador tracking
- Diplomatic gift system with opinion bonuses

---

## 🤖 Medium Priority Tasks (AI & Automation)

### Task 5: AI Diplomacy Decision Making
**Files:** `DiplomacySystem.cpp` (lines 386-401)
**Complexity:** High
**Dependencies:** AI personality system
**TODOs to resolve:** 3
```cpp
// Implement:
- ProcessAIDiplomacy() - line 386
- EvaluateProposal() - line 392
- GenerateAIDiplomaticActions() - line 401
```
**Expected deliverables:**
- AI proposal evaluation system
- Personality-based decision making
- Automated diplomatic action generation

---

### Task 6: Diplomatic Personality System
**Files:** `DiplomacySystem.cpp` (lines 501, 537-551)
**Complexity:** Medium
**Dependencies:** None
**TODOs to resolve:** 4
```cpp
// Implement:
- InitializeDiplomaticPersonalities() - line 501
- ApplyPersonalityToOpinion() - line 537
- GetWarLikelihood() - line 541
- GetTradePreference() - line 551
```
**Expected deliverables:**
- 8 personality types (Aggressive, Diplomatic, etc.)
- Personality effects on opinion
- War/peace likelihood modifiers
- Trade preference calculations

---

## 💰 Low Priority Tasks (Trade & Economy)

### Task 7: Trade Relations Integration
**Files:** `DiplomacySystem.cpp` (lines 417-425)
**Complexity:** Medium
**Dependencies:** EconomicSystem integration
**TODOs to resolve:** 3
```cpp
// Implement:
- UpdateTradeRelations() - line 417
- CalculateTradeValue() - line 421
- ProcessTradeDisputes() - line 425
```
**Expected deliverables:**
- Trade volume tracking
- Economic interdependence calculation
- Trade dispute resolution

---

## 🔍 Advanced Tasks (Intelligence & Analysis)

### Task 8: Intelligence Gathering System
**Files:** `DiplomacySystem.cpp` (lines 429-433)
**Complexity:** Medium
**Dependencies:** None
**TODOs to resolve:** 2
```cpp
// Implement:
- ProcessDiplomaticIntelligence() - line 429
- UpdateForeignRelationsKnowledge() - line 433
```
**Expected deliverables:**
- Information gathering mechanics
- Foreign relations visibility system
- Espionage integration hooks

---

## 🧮 Foundation Tasks (Helper Methods)

### Task 9: Calculation Helper Methods
**Files:** `DiplomacySystem.cpp` (lines 509-533)
**Complexity:** Low-Medium
**Dependencies:** None
**TODOs to resolve:** 7
```cpp
// Implement:
- CalculateBaseOpinion() - line 509
- CalculateAllianceValue() - line 513
- CalculateWarScore() - line 517
- FindBestCasusBelli() - line 521
- EvaluateAllianceProposal() - line 525
- EvaluateTradeProposal() - line 529
- EvaluateMarriageProposal() - line 533
```
**Expected deliverables:**
- Opinion calculation formulas
- Proposal evaluation algorithms
- War score computation
- Casus belli validation

---

### Task 10: Query & Data Access Methods
**Files:** `DiplomacySystem.cpp` (lines 437-560)
**Complexity:** Low
**Dependencies:** ECS system queries
**TODOs to resolve:** 5
```cpp
// Implement:
- GetAllRealms() - line 437
- GetNeighboringRealms() - line 442
- GetPotentialAllies() - line 447
- GetPotentialEnemies() - line 452
- GetBorderingRealms() - line 560
```
**Expected deliverables:**
- Realm query methods
- Neighbor detection
- Alliance/enemy identification

---

### Task 11: Relationship Updates
**Files:** `DiplomacySystem.cpp` (lines 368-380)
**Complexity:** Medium
**Dependencies:** None
**TODOs to resolve:** 3
```cpp
// Implement:
- UpdateDiplomaticRelationships() - line 368
- ProcessDiplomaticDecay() - line 374
- CalculatePrestigeEffects() - line 380
```
**Expected deliverables:**
- Opinion decay over time
- Relationship status updates
- Prestige impact on diplomacy

---

## 📦 Suggested Task Bundles for AI Implementation

### Bundle A: "Core Diplomacy" (Recommended Start)
**Estimated Time:** 2-3 hours
**Files to provide:**
- `DiplomacySystem.h` (full file)
- `DiplomacySystem.cpp` (full file)
- `DiplomacyComponents.h` (full file)

**Tasks included:**
- Task 4 (Embassy System) - Easy warm-up
- Task 1 (Treaty System) - Core functionality
- Task 9 (Helper Methods) - Foundation

**Why start here:** Establishes foundation without complex dependencies

---

### Bundle B: "War & Peace"
**Estimated Time:** 3-4 hours
**Files to provide:**
- `DiplomacySystem.cpp` (full file)
- `MilitarySystem.h` (for integration reference)

**Tasks included:**
- Task 3 (War System)
- Task 2 (Marriage System)
- Related helper methods

**Why this bundle:** Natural logical grouping, military integration

---

### Bundle C: "AI Diplomacy"
**Estimated Time:** 4-5 hours
**Files to provide:**
- `DiplomacySystem.cpp` (full file)
- `AIDirector.h` (for integration reference)

**Tasks included:**
- Task 5 (AI Decision Making)
- Task 6 (Personality System)
- Task 11 (Relationship Updates)

**Why this bundle:** Makes NPCs behave diplomatically

---

### Bundle D: "Economic Integration"
**Estimated Time:** 2-3 hours
**Files to provide:**
- `DiplomacySystem.cpp` (full file)
- `EconomicSystem.h` (for integration reference)

**Tasks included:**
- Task 7 (Trade Relations)
- Task 8 (Intelligence System)
- Task 10 (Query Methods)

**Why this bundle:** Connects diplomacy to economy

---

## 📝 Instructions for AI Implementation

### What to provide Claude:
1. **Primary file:** `src/game/diplomacy/DiplomacySystem.cpp` (full)
2. **Header file:** `include/game/diplomacy/DiplomacySystem.h` (full)
3. **Components:** `include/game/diplomacy/DiplomacyComponents.h` (full)
4. **Task specification:** One bundle from above
5. **Context:** "Implement Bundle A: Core Diplomacy - resolve all TODOs in Tasks 1, 4, and 9"

### Expected from Claude:
- Complete implementations (no new TODOs)
- Proper error handling
- Integration with ComponentAccessManager
- Const-correctness
- Comments explaining logic
- Validation methods

### Testing after implementation:
```bash
cd /workspaces/Game/build
make clean && make
./mechanica_imperii  # Should compile and run
```

---

## �� Implementation Notes

### Current Architecture:
- Uses ComponentAccessManager for ECS data access
- DiplomacyComponent stores per-realm diplomatic state
- MessageBus for event communication
- Stub implementations return safe defaults

### Key Design Patterns:
- Component-based data storage
- Event-driven state changes
- Opinion-based relationship system
- Treaty as contracts with compliance tracking

### Integration Points:
- **MilitarySystem**: War declarations, unit movements
- **EconomicSystem**: Trade values, resource flow
- **AIDirector**: Automated decision making
- **RealmManager**: Realm data and borders
- **CharacterAI**: Marriage candidates, personalities

---

## ✅ Success Criteria

Each task is complete when:
1. ✅ All TODOs removed
2. ✅ Compiles without warnings
3. ✅ No commented placeholder code
4. ✅ Proper error handling
5. ✅ Integration tests pass
6. ✅ Diplomatic actions have visible effects
7. ✅ AI makes reasonable diplomatic decisions

---

## 📊 Current Implementation Status

```
Total TODOs: 41
Lines of Code: ~600
Commented Code: 25 lines
Completion: ~40% (structure exists, logic missing)

Priority Distribution:
- High Priority: 17 TODOs (Core features)
- Medium Priority: 14 TODOs (AI/Trade)
- Low Priority: 10 TODOs (Helpers/Queries)
```

---

**Recommendation:** Start with **Bundle A (Core Diplomacy)** to establish foundation, then proceed to **Bundle B (War & Peace)** for immediate gameplay impact.
