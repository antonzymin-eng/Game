# Diplomacy System - Complete Implementation Plan

**Project**: Mechanica Imperii Grand Strategy Game
**System**: Advanced Diplomacy & Sphere of Influence
**Total Duration**: 20-22 weeks
**Current Status**: Phase 1-2 Complete (25%), Phase 3 In Progress (32% of Phase 3), Phase 4-8 Pending (67%)

---

## Executive Summary

The diplomacy system transforms basic nation-to-nation relationships into a dynamic simulation of power projection, historical memory, trust building, and sphere competition. This plan details implementation across 8 phases covering memory, trust, influence, espionage, and AI enhancement.

### Current Foundation (✅ Implemented)
- **~60% base system**: DiplomacySystem with 25/41 methods implemented
- **Thread-safe**: MAIN_THREAD strategy + ThreadSafeMessageBus
- **ECS architecture**: Component-based with proper integration
- **8,000+ lines**: Existing diplomacy code in production

---

## Phase Overview

| Phase | System | Weeks | Status | Lines of Code (Est.) |
|-------|--------|-------|--------|----------------------|
| **1** | Memory & Historical Tracking | 2-3 | ✅ **COMPLETE** | ~1,400 |
| **2** | Trust & Reputation | 2 | ✅ **COMPLETE** | ~800 |
| **3** | Sphere of Influence | 4 | 🟡 **IN PROGRESS** (Week 1/4 done) | ~2,590/8,000 |
| **4** | Secret Diplomacy & Intelligence | 3 | ❌ **PENDING** | ~5,000 |
| **5** | Treaty Enhancement | 2 | ❌ **PENDING** | ~3,000 |
| **6** | Relationship Dynamics | 2 | ❌ **PENDING** | ~2,500 |
| **7** | AI Enhancement | 3 | ❌ **PENDING** | ~4,000 |
| **8** | Testing & Balance | 2 | ❌ **PENDING** | ~2,000 |

**Total**: 20-22 weeks, ~26,700 lines of new code

---

## Phase 1: Memory & Historical Tracking ✅ COMPLETE

### Objective
Create comprehensive memory system tracking diplomatic events with decay over time, influencing future decisions.

### Implementation Status: 100%

**Files Created:**
- `include/game/diplomacy/DiplomaticMemory.h` (344 lines)
- `src/game/diplomacy/DiplomaticMemory.cpp` (682 lines)
- `include/game/diplomacy/MemorySystem.h` (80 lines)
- `src/game/diplomacy/MemorySystem.cpp` (350 lines)

**Features Implemented:**
- ✅ 50+ Event types across 8 categories
- ✅ Event severity levels (Trivial → Critical)
- ✅ Time-based exponential decay
- ✅ Permanent memories (grudges, lasting bonds)
- ✅ Historical pattern detection (rivals, allies, betrayals)
- ✅ Milestone system (100-year peace, eternal alliance, dynastic union)
- ✅ EventMemory component for bilateral relationships
- ✅ Integration with DiplomaticState

**Event Categories:**
1. **MILITARY**: Wars, battles, military aid (10 event types)
2. **ECONOMIC**: Trade, gifts, loans, embargoes (9 event types)
3. **DIPLOMATIC**: Treaties, alliances, embassies (10 event types)
4. **PERSONAL**: Ruler relationships, friendships, betrayals (6 event types)
5. **DYNASTIC**: Marriages, successions, claims (7 event types)
6. **TERRITORIAL**: Border agreements, seizures (6 event types)
7. **RELIGIOUS**: Conversions, holy sites, crusades (6 event types)
8. **BETRAYAL**: Backstabs, abandonments, assassinations (5 event types)

**Key Algorithms:**
- Opinion decay: `weight = weight * (1 - decay_rate)^months`
- Memory pruning: Remove events with <5% impact
- Pattern detection: 3+ betrayals = grudge, 10+ positive events over 20 years = friendship

---

## Phase 2: Trust & Reputation System ✅ COMPLETE

### Objective
Implement multi-faceted trust system affecting diplomatic options and AI decision-making.

### Implementation Status: 100%

**Files Created:**
- `include/game/diplomacy/TrustSystem.h` (215 lines)
- `src/game/diplomacy/TrustSystem.cpp` (557 lines)

**Features Implemented:**
- ✅ 5 Trust Factors with independent tracking
- ✅ Weighted composite trust calculation
- ✅ Trust bounds (floor/ceiling based on history)
- ✅ Trust volatility assessment
- ✅ Trust rebuilding paths after betrayal
- ✅ Global trustworthiness reputation
- ✅ TrustComponent for ECS

**Trust Factors:**
1. **Treaty Compliance** (weight: 1.2): How well they honor agreements
2. **Military Reliability** (weight: 1.5): Reliability in wars/alliances
3. **Economic Reliability** (weight: 1.0): Trade reliability, debt repayment
4. **Personal Relationship** (weight: 0.8): Ruler-to-ruler trust
5. **Historical Behavior** (weight: 1.0): Long-term pattern

**Trust Calculation:**
```
overall_trust = Σ(factor.value * factor.weight) / Σ(factor.weight)
trust_bounds: [min_possible_trust, max_possible_trust]
```

**Trust Rebuilding Requirements:**
- Months of peace (typically 24+)
- Treaty compliance over time
- Diplomatic gifts (3+ required)
- Third-party mediation
- Shared military victories

---

## Phase 3: Sphere of Influence System 🟡 IN PROGRESS

### Objective
Implement multi-layered power projection where powerful nations exert pressure through military, economic, dynastic, religious, and cultural means.

### Duration: 4 weeks (Week 1/4 Complete)
### Priority: **HIGH** (Core gameplay feature)
### Status: **32% Complete** (~2,590/8,000 lines implemented)

**Implementation Plan**: See `PHASE3_SPHERE_OF_INFLUENCE_PLAN.md`
**Current Status**: See `PHASE3_STATUS.md`

### Implementation Progress

**✅ Week 1 Complete (Files Created):**
- `InfluenceComponents.h/cpp` (380 lines) - All data structures
- `InfluenceCalculator.h/cpp` (820 lines) - Influence calculation formulas
- `InfluenceSystem.h/cpp` (1,390 lines) - Core system manager

**❌ Week 2-4 Pending:**
- Propagation algorithms (partial implementation)
- Sphere conflict resolution
- Full integration with character/religion systems
- Serialization/deserialization
- Comprehensive testing

### Quick Overview

**Seven Influence Types:**
1. **Military** (2-4 hops): Fear-based compliance, garrison pressure
2. **Economic** (5-8 hops): Trade dependency, financial leverage
3. **Dynastic** (unlimited): Family ties, marriage networks
4. **Personal** (3-5 hops): Ruler friendships, character bonds
5. **Religious** (unlimited, same faith): Religious authority, clergy loyalty
6. **Cultural** (4-6 hops): Shared culture, identity politics
7. **Prestige** (global): International reputation, bandwagoning

**Key Features:**
- **Granular Targeting**: Influence specific vassals or characters within realms
- **Sphere Competition**: Detect competing powers, tension calculation, flashpoints
- **Autonomy Effects**: Reduced diplomatic freedom under strong influence
- **Propagation**: Distance-based decay along realm networks

**Files to Create:**
- `InfluenceSystem.h/cpp` (main system)
- `InfluenceComponents.h/cpp` (data structures)
- `InfluenceCalculator.h/cpp` (calculations)
- `InfluencePropagation.h/cpp` (geographic spread)
- `InfluenceConflict.h/cpp` (sphere competition)

---

## Phase 4: Secret Diplomacy & Intelligence ❌ PENDING

### Objective
Add espionage, secret treaties, hidden agendas, and information warfare.

### Duration: 3 weeks
### Dependencies: Phase 1-2 (Memory + Trust)

**Key Features:**
- Embassy system with intelligence gathering
- Secret treaties with discovery mechanics
- Intelligence operations (spy networks, counter-intel)
- Hidden agendas for AI realms
- Information propagation and rumor spreading

**Files to Create:**
- `EmbassySystem.h/cpp`
- `IntelligenceSystem.h/cpp`
- `SecretDiplomacy.h/cpp`
- `InformationPropagation.h/cpp`

---

## Phase 5: Treaty System Enhancement ❌ PENDING

### Objective
Complete treaty compliance tracking, enforcement mechanisms, and consequences.

### Duration: 2 weeks
### Dependencies: Phase 1-2

**Key Features:**
- Automated compliance monitoring
- Violation detection and severity assessment
- Multi-party treaties (coalitions, trade blocs)
- Treaty negotiation with counter-proposals
- Casus belli validation

**Files to Modify:**
- `DiplomacyComponents.h` (Treaty structure)
- `DiplomacySystem.cpp` (treaty methods)

**Files to Create:**
- `TreatyEnforcement.h/cpp`
- `TreatyNegotiation.h/cpp`

---

## Phase 6: Relationship Dynamics ❌ PENDING

### Objective
Create living, breathing diplomatic relationships that evolve naturally.

### Duration: 2 weeks
### Dependencies: Phase 1-2

**Key Features:**
- Opinion modifier system with tracking
- Natural opinion drift (rivals → hostile, distant → neutral)
- Third-party effects (enemy of my friend, etc.)
- Enhanced personality-driven behavior

**Files to Create:**
- `OpinionModifiers.h/cpp`
- `RelationshipDynamics.h/cpp`

---

## Phase 7: AI Enhancement & Integration ❌ PENDING

### Objective
Make AI use all diplomatic tools intelligently based on goals and situation.

### Duration: 3 weeks
### Dependencies: All previous phases

**Key Features:**
- Strategic goal integration (expansion, consolidation, survival)
- Weighted decision-making with multiple factors
- Diplomatic deception (lies, bluffs, backstabs)
- Coalition formation and coordination

**Files to Modify:**
- `DiplomaticAI.h/cpp`
- `NationAI.cpp` (integration)

---

## Phase 8: Testing & Balance ❌ PENDING

### Objective
Ensure system stability, performance, and fun gameplay.

### Duration: 2 weeks

**Activities:**
- Unit tests for all calculation methods
- Integration tests with AI/Economic/Military systems
- Performance profiling (<5ms average update)
- Balance tuning (decay rates, AI weights, costs)

**Files to Create:**
- `tests/test_influence_system.cpp`
- `tests/test_trust_rebuilding.cpp`
- `tests/test_sphere_competition.cpp`
- `tests/test_diplomatic_ai_integration.cpp`

---

## Architecture Decisions

### Component Design
```
DiplomacyComponent (existing)
  ├─ relationships (map<EntityID, DiplomaticState>)
  ├─ active_treaties (vector<Treaty>)
  └─ marriages (vector<DynasticMarriage>)

DiplomaticMemoryComponent (Phase 1 ✅)
  ├─ memories (map<EntityID, EventMemory>)
  └─ milestones (map<EntityID, MilestoneTracker>)

TrustComponent (Phase 2 ✅)
  ├─ trust_relationships (map<EntityID, TrustData>)
  └─ rebuilding_paths (map<EntityID, TrustRebuildingPath>)

InfluenceComponent (Phase 3 ❌)
  ├─ influence_projection (map<InfluenceType, double>)
  ├─ influenced_realms (map<EntityID, InfluenceState>)
  └─ sphere_conflicts (vector<InfluenceConflict>)
```

### Threading Strategy
- **Main Thread**: Core diplomacy updates, treaty processing
- **MAIN_THREAD**: All diplomacy systems (thread-safe after fix)
- **MessageBus**: ThreadSafeMessageBus for cross-system events

### Update Frequencies
- **Monthly**: Opinion decay, trust updates, influence propagation
- **Quarterly**: AI diplomacy evaluations, sphere conflict detection
- **Yearly**: Reputation recalculation, milestone checks, memory pruning

### Performance Targets
- **Update Time**: <5ms average per realm monthly update
- **Memory**: <100MB for 500 realms with full diplomatic data
- **Scalability**: O(n²) for n realms (acceptable for n ≤ 500)

---

## Integration Points

### Military System
- War declarations trigger memory events
- Ally activation uses trust calculation
- Military strength affects influence projection
- Battles create shared memories

### Economic System
- Trade volume creates economic influence
- Economic dependency affects autonomy
- Sanctions reduce influence
- Debt creates leverage

### AI System
- Strategic goals drive diplomatic actions
- Influence affects AI decision weights
- Trust determines proposal acceptance
- Memory informs long-term planning

### Character System
- Ruler relationships create personal influence
- Marriages establish dynastic influence
- Character traits affect trust rebuilding
- Council members can be compromised

---

## Success Metrics

### Technical
- ✅ All tests passing (90%+ coverage)
- ✅ <5ms average update time
- ✅ No memory leaks
- ✅ Thread-safe operations
- ✅ Saves/loads correctly

### Gameplay
- ✅ AI forms logical alliances
- ✅ Betrayals feel impactful
- ✅ Trust matters in negotiations
- ✅ Spheres of influence visible and meaningful
- ✅ Secrets create interesting situations
- ✅ Reputation affects available options

### Performance
- ✅ Handles 500 realms smoothly
- ✅ <100MB memory for diplomatic data
- ✅ Scales efficiently with realm count

---

## Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| Performance with 500+ realms | High | Cache calculations, use spatial partitioning |
| AI spam diplomacy | Medium | Cooldowns exist, tune them carefully |
| Save file bloat | Medium | Compress memory data, limit history depth |
| Thread safety issues | High | Use MAIN_THREAD, ComponentAccessManager correctly |
| Balance (too easy/hard) | Medium | Configurable parameters, iterative tuning |
| Sphere competition overwhelming | Low | Tune tension thresholds, limit simultaneous conflicts |
| Trust rebuilding too slow | Low | Balance monthly recovery rates |

---

## Development Workflow

### Per-Phase Process
1. **Design Review**: Verify data structures and architecture
2. **Incremental Implementation**: Small commits with build verification
3. **Unit Tests**: Write tests alongside implementation
4. **Integration**: Connect to existing systems
5. **Balance Pass**: Tune parameters for gameplay
6. **Documentation**: Update API docs and examples

### Commit Strategy
- Small, atomic commits after each feature
- Build verification before each commit
- Descriptive commit messages with context
- PR review for major features

### Testing Approach
- Unit tests for calculations and algorithms
- Integration tests for system interactions
- Stress tests for performance
- Playtesting for balance and fun

---

## Next Steps

### Immediate (This Sprint)
1. ✅ Create comprehensive documentation (this file)
2. ✅ Commit documentation to repository
3. ✅ Begin Phase 3: Sphere of Influence implementation
4. ✅ Create InfluenceComponents.h data structures
5. ✅ Implement and test basic influence types (partial)
6. 🟡 Complete Phase 3 Week 2-4 tasks (in progress)

### Short-term (Next 4 weeks)
- Complete Phase 3: Sphere of Influence system
- Implement all 7 influence types
- Add granular targeting (vassals/characters)
- Implement sphere competition and flashpoints
- Integration with existing diplomacy systems

### Medium-term (Weeks 5-12)
- Phase 4: Secret Diplomacy & Intelligence
- Phase 5: Treaty Enhancement
- Phase 6: Relationship Dynamics

### Long-term (Weeks 13-22)
- Phase 7: AI Enhancement
- Phase 8: Testing & Balance
- Full system integration
- Performance optimization
- Balance tuning

---

## References

- **Codebase**: `/home/user/Game/src/game/diplomacy/`
- **Documentation**: `/home/user/Game/docs/diplomacy/`
- **Tests**: `/home/user/Game/tests/`
- **Detailed Plans**: See individual phase documentation files

---

**Last Updated**: 2025-11-12
**Status**: Phase 1-2 complete, Phase 3 Week 1/4 complete (32%)
**Next Milestone**: Phase 3 Week 2-4 - Complete propagation, conflict resolution, and integration
