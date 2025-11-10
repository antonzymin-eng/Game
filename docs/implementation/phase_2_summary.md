# Phase 2: Trust & Reputation System - Executive Summary

## Overview
**Duration:** 3 weeks (Days 4-24 of overall implementation)
**Goal:** Implement complete Trust & Reputation systems that integrate with Phase 1's Memory system

---

## Quick Reference

### Week 1: Trust System (Days 1-7)
**Objective:** Build multi-factor trust system with rebuilding mechanics

**Key Deliverables:**
- `TrustSystem.h/cpp` - Core trust mechanics
- `TrustRebuildingStrategies.h/cpp` - Path-based trust recovery
- `TrustSystemManager.cpp` - System coordinator
- `TrustComponent.cpp` - ECS component with serialization
- **Tests:** Unit tests for all trust operations

**Trust Factors:**
1. Treaty Compliance (weight: 2.0)
2. Military Reliability (weight: 1.5)
3. Economic Reliability (weight: 1.2)
4. Personal Relationship (weight: 1.0)
5. Historical Behavior (weight: 1.8)

**Key Mechanics:**
- Trust ranges: 0.0 (no trust) to 1.0 (complete trust)
- Trust bounds: Min/max limits based on history
- Stability detection: Fragile vs Solid trust
- Rebuilding paths: Strategy-based recovery after violations

---

### Week 2: Reputation System (Days 8-14)
**Objective:** Create global reputation system affecting all relationships

**Key Deliverables:**
- `ReputationSystem.h/cpp` - Core reputation mechanics
- `ReputationComponent.cpp` - ECS component
- `ReputationSystemManager.cpp` - System manager
- **Tests:** Reputation calculation and propagation tests

**Reputation Types (10):**
1. Military Honor
2. Treaty Keeper
3. Economic Reliability
4. Diplomatic Integrity
5. Aggressive Expansionist
6. Defensive Pacifist
7. Generous Ally
8. Backstabber
9. Crusader
10. Pragmatist

**Key Mechanics:**
- Global reputation scores (-1.0 to +1.0)
- Certainty tracking (how established is reputation)
- Primary reputations (top 3 defining traits)
- Reputation propagation (news spreads to observers)
- Global trust level (affects all relationships)

---

### Week 3: Integration & Polish (Days 15-21)
**Objective:** Complete integration, UI support, optimization, and documentation

**Key Deliverables:**
- `DiplomacySystemIntegrator.h/cpp` - Full system integration
- `DiplomacyUIData.h/cpp` - UI data structures
- Performance optimization
- Comprehensive documentation
- Final validation tests

**Integration Pipeline:**
```
Diplomatic Event
    ↓
Memory System (records event)
    ↓
Trust System (updates trust factors)
    ↓
Reputation System (updates global reputation)
    ↓
Diplomatic State (opinion, modifiers, relationships)
```

**Performance Targets:**
- Monthly update: < 10ms for 50 realms (2500 relationships)
- Memory queries: < 0.1ms
- Trust calculations: < 0.5ms per relationship
- Reputation updates: < 1ms per realm

---

## System Architecture

### Component Relationships
```
DiplomaticMemoryComponent (Phase 1)
    ↓ (events influence)
TrustComponent (Phase 2 Week 1)
    ↓ (trust affects)
ReputationComponent (Phase 2 Week 2)
    ↓ (reputation modifies)
DiplomaticState (base diplomacy)
```

### Data Flow
1. **Event Occurs** → DiplomaticEvent created
2. **Memory Records** → EventMemory stores with decay
3. **Trust Updates** → Trust factors modified
4. **Reputation Updates** → Global reputation recalculated
5. **Propagation** → News spreads to observers
6. **State Updates** → Opinion modifiers applied

---

## Example: Treaty Violation Cascade

```cpp
// 1. Event Creation
DiplomaticEvent event(EventType::TREATY_VIOLATED, realm_a, realm_b);

// 2. Memory System
memory_system.RecordEvent(event);
// - Adds to event_history
// - Marks as permanent memory
// - Opinion impact: -50

// 3. Trust System
trust_system.ModifyTrust(realm_a, realm_b,
    TrustFactorType::TREATY_COMPLIANCE, -0.3);
// - Trust drops from 0.7 to 0.4
// - Trust ceiling lowered to 0.8
// - Triggers rebuilding path

// 4. Reputation System
reputation_system.ProcessEvent(event);
// - "Treaty Keeper" score drops
// - "Backstabber" score increases (if repeated)
// - Global trust drops

// 5. Propagation
// - Realm C hears about violation
// - Realm C's opinion of Realm A drops by 30% of direct impact

// 6. UI Update
ui_data = CreateRelationshipUI(realm_b, realm_a);
// - Shows trust: 0.4 (Fragile)
// - Shows grudge warning
// - Shows rebuilding requirements
```

---

## Trust Rebuilding Example

### Scenario: Broken Alliance

**Initial State:**
- Trust: 0.2 (after betrayal)
- Trust Ceiling: 0.6 (permanently lowered)
- Relationship: Enemy

**Rebuilding Path Created:**
```cpp
MilitaryBetrayalRecovery strategy;
TrustRebuildingPath path = strategy.CreateRebuildingPath(...);
```

**Requirements:**
1. ✓ Fight alongside in one war (+0.15 trust)
2. ✗ Provide significant military aid (+0.10 trust)
3. ✗ Publicly acknowledge betrayal (+0.05 trust)
4. ✗ Provide hostages/guarantees (+0.10 trust)
5. ✗ 60 months of peace (currently: 12/60)
6. ✗ Send 5 gifts (currently: 0/5)

**Natural Recovery:** +0.003/month (very slow)

**Timeline:** 5-10 years minimum to reach cautious trust (0.4)

---

## Key Features

### 1. Multi-Factor Trust
- Different aspects of trustworthiness
- Weighted calculation
- Individual factor trends
- Historical tracking

### 2. Trust Bounds
```cpp
min_possible_trust: 0.0 → 0.3 (after deep cooperation)
max_possible_trust: 1.0 → 0.6 (after betrayal)
```

### 3. Reputation Certainty
```cpp
// Consistent behavior = high certainty
treaty_keeper.certainty = 0.85  // Well-established
treaty_keeper.supporting_events = 15
treaty_keeper.contradicting_events = 1

// Inconsistent = low certainty
pragmatist.certainty = 0.35  // Uncertain
pragmatist.supporting_events = 5
pragmatist.contradicting_events = 5
```

### 4. Reputation Propagation
```cpp
// Major event spreads to observers
if (event.severity >= EventSeverity::MODERATE) {
    PropagateToObservers(event);
    // Impact = 30% of direct effect
}
```

### 5. Memory-Trust-Reputation Integration
```cpp
// Historical events set trust bounds
if (memory.betrayals_count >= 3) {
    trust_data.max_possible_trust = 0.7;
}

// Trust influences reputation
if (trust_level < 0.3 && violations > 2) {
    reputation.backstabber_score -= 0.25;
}
```

---

## Testing Strategy

### Unit Tests (Day 1-14)
- Trust factor calculations
- Trust trend detection
- Reputation scoring
- Certainty calculations
- Serialization

### Integration Tests (Day 15-18)
- Memory → Trust integration
- Trust → Reputation flow
- Event propagation
- Full pipeline tests

### Performance Tests (Day 18-19)
- Scalability (10, 50, 100 realms)
- Monthly update benchmarks
- Memory profiling
- Cache effectiveness

### Validation Tests (Day 20-21)
- Complete scenarios
- Save/load integrity
- Edge cases
- Backward compatibility

---

## UI Data Structures

### Trust Breakdown
```cpp
struct TrustBreakdown {
    double overall_trust;
    vector<FactorBreakdown> factors;  // Individual trust factors
    double min_possible_trust;
    double max_possible_trust;
    bool is_fragile;
    bool is_solid;
    string stability_description;
};
```

### Reputation Breakdown
```cpp
struct ReputationBreakdown {
    double global_trust;
    vector<ReputationEntry> reputations;
    vector<string> primary_traits;  // Top 3
    ReputationStats stats;
};
```

### Memory Breakdown
```cpp
struct MemoryBreakdown {
    int total_positive_events;
    int total_negative_events;
    vector<MemorableMoment> major_events;  // Top 10
    vector<MemorableMoment> recent_events; // Last 20
    bool has_grudge;
    bool has_friendship;
    string relationship_summary;
};
```

---

## Success Criteria

### Functional ✓
- [x] Trust tracks 5 factors
- [x] Trust rebuilding works
- [x] 10+ reputation types
- [x] Reputation propagates
- [x] Full integration

### Performance ⚠
- [ ] < 10ms monthly update (50 realms)
- [ ] No memory leaks
- [ ] Stable 1000+ year games

### Quality ⚠
- [ ] 90%+ code coverage
- [ ] All tests passing
- [ ] Complete documentation
- [ ] No critical bugs

### User Experience ⚠
- [ ] Clear trust UI
- [ ] Clear reputation UI
- [ ] Meaningful feedback
- [ ] Intuitive rebuilding

---

## Files Created

### Headers (include/game/diplomacy/)
- `TrustSystem.h` (890 lines)
- `TrustRebuildingStrategies.h` (120 lines)
- `ReputationSystem.h` (450 lines)
- `DiplomacySystemIntegrator.h` (110 lines)
- `DiplomacyUIData.h` (200 lines)

### Implementation (src/game/diplomacy/)
- `TrustSystem.cpp` (650 lines)
- `TrustRebuildingStrategies.cpp` (180 lines)
- `TrustSystemManager.cpp` (320 lines)
- `TrustComponent.cpp` (280 lines)
- `ReputationSystem.cpp` (420 lines)
- `ReputationComponent.cpp` (350 lines)
- `ReputationSystemManager.cpp` (280 lines)
- `DiplomacySystemIntegrator.cpp` (240 lines)
- `DiplomacyUIDataFactory.cpp` (280 lines)

### Tests (tests/game/diplomacy/)
- `TrustSystemTests.cpp` (150 lines)
- `TrustSystemIntegrationTests.cpp` (120 lines)
- `ReputationSystemTests.cpp` (140 lines)
- `SystemIntegrationTests.cpp` (100 lines)
- `FinalValidationTests.cpp` (80 lines)

### Documentation (docs/)
- `systems/trust_system.md`
- `systems/reputation_system.md`
- `systems/performance_optimization.md`
- `systems/trust_reputation_user_guide.md`

**Total:** ~5,000 lines of code + documentation

---

## Phase 3 Preview

After completing Phase 2, Phase 3 will build on this foundation:

**AI Decision Making:**
- Use trust levels for alliance decisions
- Consider reputation when selecting trade partners
- Risk assessment based on trust

**Dynamic Events:**
- Trust-based event triggers
- Reputation-influenced outcomes
- Memory-driven storylines

**Advanced Diplomacy:**
- Coalition systems
- Alliance networks
- Mediation and arbitration

**Historical Chronicles:**
- Legendary relationships
- Famous betrayals
- Alliance histories

---

## Quick Start for Developers

### 1. Read Phase 1 Plan First
Understanding Memory system is prerequisite

### 2. Start with Week 1 Day 1
Trust Factor implementation is foundation

### 3. Run Tests Frequently
Each day has specific test requirements

### 4. Review Integration Points
Week 3 ties everything together

### 5. Check Performance Early
Don't wait until Week 3 for optimization

---

## Common Issues & Solutions

### Issue: Trust not updating
**Solution:** Check trust factor weights and calculation order

### Issue: Reputation not propagating
**Solution:** Verify event severity thresholds

### Issue: Performance slow
**Solution:** Enable caching, check ECS access patterns

### Issue: Save/load broken
**Solution:** Verify all Serialize/Deserialize implementations

### Issue: UI data incomplete
**Solution:** Check factory method implementations

---

## Contact & Support

For questions about this plan:
- Review detailed plan: `docs/implementation/phase_2_trust_reputation_plan.md`
- Check Phase 1 plan: `docs/implementation/phase_1_memory_tracking_plan.md` (from request)
- Refer to architecture docs in `docs/systems/`

---

**Last Updated:** 2025-11-10
**Version:** 1.0
**Status:** Ready for Implementation
