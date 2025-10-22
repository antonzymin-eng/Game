# 🤖 Claude AI Prompt Template for DiplomacySystem

## Quick Copy-Paste Prompts

### For Bundle A (Core Diplomacy) - RECOMMENDED START
```
I need you to implement Bundle A: Core Diplomacy for the DiplomacySystem.

Context:
- C++17 game project using ECS architecture
- DiplomacySystem compiles but has 41 TODOs (stub implementations)
- Use ComponentAccessManager for data access
- Follow existing code patterns

Tasks to complete:
1. Task 4: Embassy & Communication System (lines 331-344)
   - EstablishEmbassy()
   - RecallAmbassador()
   - SendDiplomaticGift()

2. Task 1: Treaty System (lines 302-368)
   - ProposeTradeAgreement()
   - ProcessTreatyCompliance()
   - UpdateTreatyStatus()
   - HandleTreatyViolation()

3. Task 9: Helper Methods (lines 509-533)
   - CalculateBaseOpinion()
   - CalculateAllianceValue()
   - CalculateWarScore()
   - FindBestCasusBelli()
   - Evaluate proposal methods

Requirements:
- Remove all TODOs in these methods
- Add proper error handling
- Use ComponentAccessManager::GetComponent()
- Log diplomatic events
- Return meaningful values (no placeholder 0.0 or false)

Files attached:
[Attach: DiplomacySystem.h, DiplomacySystem.cpp, DiplomacyComponents.h]
```

---

### For Bundle B (War & Peace)
```
I need you to implement Bundle B: War & Peace for the DiplomacySystem.

This builds on the Core Diplomacy implementation.

Tasks to complete:
1. Task 3: War Declaration System (lines 405-413)
   - ProcessWarDeclaration()
   - HandleAllyActivation()
   - ProcessPeaceNegotiation()

2. Task 2: Marriage Diplomacy (lines 318-325)
   - ArrangeMarriage()
   - ProcessMarriageEffects()

Requirements:
- Integrate with MilitarySystem (use stubs if methods don't exist)
- Create alliance through marriage option
- Handle ally call-to-arms
- Calculate war scores

Files attached:
[Attach: DiplomacySystem.cpp, DiplomacyComponents.h, MilitarySystem.h]
```

---

### For Bundle C (AI Diplomacy)
```
I need you to implement Bundle C: AI Diplomacy for the DiplomacySystem.

This adds autonomous diplomatic behavior for NPC realms.

Tasks to complete:
1. Task 5: AI Decision Making (lines 386-401)
   - ProcessAIDiplomacy()
   - EvaluateProposal()
   - GenerateAIDiplomaticActions()

2. Task 6: Personality System (lines 501, 537-551)
   - InitializeDiplomaticPersonalities()
   - ApplyPersonalityToOpinion()
   - GetWarLikelihood()
   - GetTradePreference()

3. Task 11: Relationship Updates (lines 368-380)
   - UpdateDiplomaticRelationships()
   - ProcessDiplomaticDecay()
   - CalculatePrestigeEffects()

Requirements:
- Implement all 8 personality types
- AI should make reasonable decisions
- Opinion should decay over time
- Personalities affect behavior

Files attached:
[Attach: DiplomacySystem.cpp, DiplomacyComponents.h, AIDirector.h]
```

---

### For Bundle D (Economic Integration)
```
I need you to implement Bundle D: Economic Integration for the DiplomacySystem.

This connects diplomacy to the economic system.

Tasks to complete:
1. Task 7: Trade Relations (lines 417-425)
   - UpdateTradeRelations()
   - CalculateTradeValue()
   - ProcessTradeDisputes()

2. Task 8: Intelligence System (lines 429-433)
   - ProcessDiplomaticIntelligence()
   - UpdateForeignRelationsKnowledge()

3. Task 10: Query Methods (lines 437-560)
   - GetAllRealms()
   - GetNeighboringRealms()
   - GetPotentialAllies()
   - GetPotentialEnemies()
   - GetBorderingRealms()

Requirements:
- Use EconomicSystem for trade data (stub if needed)
- Track economic interdependence
- Implement neighbor detection

Files attached:
[Attach: DiplomacySystem.cpp, EconomicSystem.h, RealmManager.h]
```

---

## 📁 Files to Attach

### Always include:
1. `/workspaces/Game/include/game/diplomacy/DiplomacySystem.h`
2. `/workspaces/Game/src/game/diplomacy/DiplomacySystem.cpp`
3. `/workspaces/Game/include/game/diplomacy/DiplomacyComponents.h`

### Bundle-specific:
- **Bundle B:** `include/game/military/MilitarySystem.h`
- **Bundle C:** `include/game/ai/AIDirector.h`
- **Bundle D:** `include/game/economy/EconomicSystem.h`

---

## ✅ Verification Commands

After Claude provides implementation:

```bash
# 1. Copy the updated code to the file
# 2. Compile
cd /workspaces/Game/build
make clean && make

# 3. Check for TODOs removed
grep -c "// TODO:" /workspaces/Game/src/game/diplomacy/DiplomacySystem.cpp

# 4. Check for errors
make 2>&1 | grep "error:"

# 5. Run the executable
./mechanica_imperii
```

---

## 🎯 Success Indicators

✅ **Compilation succeeds**
✅ **TODO count reduced** (41 → fewer)
✅ **No new warnings**
✅ **Executable runs**
✅ **Diplomatic actions logged**
✅ **Components updated correctly**

---

## 📝 Notes for Claude

- Use existing patterns from other systems (PopulationSystem, MilitarySystem)
- ComponentAccessManager::GetComponent() returns raw pointer (can be nullptr)
- Always check pointers before dereferencing
- Log important events using LogDiplomaticEvent()
- Opinion range: -100 to +100
- Trust range: 0.0 to 1.0
- Prestige difference affects diplomatic actions
- Use std::chrono for time calculations

