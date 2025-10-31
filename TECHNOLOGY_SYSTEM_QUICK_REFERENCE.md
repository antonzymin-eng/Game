# Technology System - Quick Reference

## System Hierarchy

```
EntityManager (Core Registry)
    ↓
ComponentAccessManager (Thread-Safe Access)
    ↓
Game Systems (Including TechnologySystem)
    ↓
Bridge Systems (Technology-Economic Integration)
```

## Key File Locations

| Component | Location | Status |
|-----------|----------|--------|
| Tech Components | `include/game/technology/TechnologyComponents.h` | COMPLETE |
| Tech System | `include/game/technology/TechnologySystem.h` | SCAFFOLD |
| Tech System Impl | `src/game/technology/TechnologySystem.cpp` | PARTIAL |
| Tech-Economy Bridge | `include/game/economy/TechnologyEconomicBridge.h` | DEFINED |
| Configuration | `config/GameConfig.json` | EXISTS |

## Core Components (4 Components)

### 1. ResearchComponent
- **Purpose:** Track technology research and development
- **Key Fields:**
  - `technology_states` - Current state of each tech (UNKNOWN, AVAILABLE, RESEARCHING, DISCOVERED, IMPLEMENTING, IMPLEMENTED)
  - `research_progress` - 0.0 to 1.0 completion
  - `current_focus` - Single tech with 50% research bonus
  - `universities`, `libraries`, `workshops` - Infrastructure
  - `monthly_research_budget` - Funding from treasury

### 2. InnovationComponent
- **Purpose:** Native innovation and invention
- **Key Fields:**
  - `innovation_rate` - Chance for accidental discovery
  - `breakthrough_chance` - Major improvements
  - `inventors`, `craftsmen_innovators` - Personnel
  - `guild_resistance`, `religious_restriction` - Obstacles
  - `royal_patronage`, `merchant_funding` - Support

### 3. KnowledgeComponent
- **Purpose:** Knowledge preservation and transmission networks
- **Key Fields:**
  - `manuscripts`, `scribes` - Infrastructure
  - `knowledge_transmission_rate` - Speed of spread
  - `scholarly_exchanges`, `trade_knowledge_routes` - Networks
  - `literacy_rate` - Population education level

### 4. TechnologyEventsComponent
- **Purpose:** Historical tracking of discoveries and breakthroughs
- **Key Fields:**
  - `recent_discoveries`, `research_breakthroughs`
  - `discovery_dates`, `discovery_methods` - Historical tracking
  - `technological_reputation` - Prestige

## Technology Definitions (50+ Technologies)

```
Agricultural (1000-1099)      Military (1100-1199)       Craft (1200-1299)
├─ THREE_FIELD_SYSTEM         ├─ CHAINMAIL_ARMOR         ├─ BLAST_FURNACE
├─ HEAVY_PLOW                 ├─ PLATE_ARMOR             ├─ PRINTING_PRESS
├─ HORSE_COLLAR               ├─ CROSSBOW                ├─ MECHANICAL_CLOCK
├─ WINDMILL                   ├─ LONGBOW                 ├─ DOUBLE_ENTRY_BOOKKEEPING
└─ CROP_ROTATION              └─ CANNONS                 └─ ADVANCED_METALLURGY

Administrative (1300-1399)    Academic (1400-1499)       Naval (1500-1599)
├─ WRITTEN_LAW_CODES          ├─ UNIVERSITY_SYSTEM       ├─ IMPROVED_SHIP_DESIGN
├─ BUREAUCRATIC_ADMIN         ├─ SCHOLASTIC_METHOD       ├─ NAVIGATION_INSTRUMENTS
├─ CENSUS_TECHNIQUES          ├─ EXPERIMENTAL_METHOD     ├─ COMPASS_NAVIGATION
└─ TAX_COLLECTION             └─ SCIENTIFIC_INSTRUMENTS  └─ NAVAL_ARTILLERY
```

## System Integration Points

### Primary: Technology-Economy Bridge
```
Research Budget ← Treasury
   ↓
TechnologyEconomicEffects:
├─ production_efficiency (Agricultural+Craft tech)
├─ trade_efficiency (Naval+Craft tech)
├─ tax_efficiency (Administrative tech)
├─ market_sophistication (Academic+Craft tech)
└─ innovation_rate_modifier (Academic tech)
   ↓
Economic Benefits Applied
```

### Secondary: Diplomacy Bridge
- Knowledge transfer through embassies (DiplomacyEconomicBridge)
- Tech espionage in wars
- Tech sharing in alliances

### Secondary: Trade Bridge
- Merchants spread knowledge (+% research)
- Naval techs boost trade efficiency
- Trade routes provide research funding

### Tertiary: Population Bridge
- Scholar availability affects innovation
- Literacy rate affects research speed
- Population growth supports universities

## State Flow for a Technology

```
UNKNOWN
  ↓ (prerequisite techs discovered)
AVAILABLE
  ↓ (research focus or automatic)
RESEARCHING (progress 0.0 → 1.0)
  ↓ (research completes)
DISCOVERED
  ↓ (implementation begins, costs treasury)
IMPLEMENTING (0.0 → 1.0)
  ↓ (implementation completes)
IMPLEMENTED (effects active)
```

## How to Add Technology to Game

1. **Define in TechnologyComponents.h**
   ```cpp
   NEW_TECH_NAME = 1234,
   ```

2. **Create in database**
   - Add to TechnologyDefinition database
   - Set prerequisites, costs, historical year
   - Define effects mapping

3. **Add to configuration (GameConfig.json)**
   - Set research cost multiplier
   - Set implementation time
   - Configure effect magnitudes

4. **Let bridges handle integration**
   - Economic system automatically applies budget
   - Bridges auto-calculate effects
   - No manual integration needed

## Configuration Example

```json
{
  "technology": {
    "base_research_cost": 1000.0,
    "literacy_requirement_base": 0.1,
    "implementation_cost_multiplier": 100.0,
    "implementation_months": 12,
    
    "agricultural_tech_production_bonus": 0.15,
    "craft_tech_production_bonus": 0.20,
    "academic_tech_innovation_bonus": 0.25,
    
    "research_budget_percentage": 0.05,
    "funding_crisis_threshold": 0.3
  }
}
```

## Event Messages (for other systems to listen)

```cpp
// Posted by Technology System
TechnologyBreakthroughEconomicImpact {
    affected_entity, technology, economic_impact, efficiency_gain
}

ResearchFundingCrisis {
    affected_entity, funding_shortfall, research_slowdown
}

BrainDrainEvent {
    affected_entity, scholars_lost, innovation_penalty
}

TechnologyImplementationComplete {
    affected_entity, technology, total_cost, efficiency_bonus
}
```

## Memory Layout (per Realm)

```
RealmComponent [Core Realm Data]
    ↓
ResearchComponent [Research Progress]
    + InnovationComponent [Native Innovation]
    + KnowledgeComponent [Knowledge Networks]
    + TechnologyEventsComponent [Event History]
    ↓
EconomicComponent [Treasury, Income, etc.]
    ↓
TechnologyEconomicBridgeComponent [Cross-System Effects]
```

## Performance Considerations

- **Update Frequency:** Can be configured (typically 1-10 Hz)
- **Thread Strategy:** Can run in MAIN_THREAD, THREAD_POOL, or HYBRID
- **Memory:** ~500 bytes per realm for tech state
- **Bandwidth:** Minimal inter-system communication via MessageBus

## Testing Strategy

```
1. Component Creation
   ✓ ResearchComponent initializes correctly
   ✓ InnovationComponent pools set up
   ✓ Knowledge networks connect properly

2. Research Mechanics
   [ ] Progress calculation matches formula
   [ ] Cost scaling correct
   [ ] Prerequisite checking works

3. Knowledge Transfer
   [ ] Trade routes increase research
   [ ] Embassies enable espionage
   [ ] Immigration brings scholars

4. Bridge Integration
   [ ] Budget drawn from treasury
   [ ] Effects applied to economy
   [ ] Crises detected and published

5. Serialization
   [ ] Components save/load correctly
   [ ] History preserved
   [ ] Bridge state persists
```

## Common Implementation Tasks

### Add Research Progress Calculation
```cpp
double CalculateMonthlyResearchProgress(
    double budget,
    double efficiency_modifiers,
    double tech_cost,
    double focus_bonus) {
    // Logic here
}
```

### Add Technology Effect Application
```cpp
void ApplyTechnologyEffects(
    EntityID realm,
    TechnologyType tech,
    ResearchComponent& research) {
    // Modify economy/trade/military
}
```

### Add Crisis Detection
```cpp
bool DetectFundingCrisis(
    double monthly_budget,
    double required_budget,
    double crisis_threshold) {
    return monthly_budget < required_budget * crisis_threshold;
}
```

## Architectural Decision Records

### Why Components for Technology?
- Decouples research state from realm (easier to save/load)
- Multiple techs can be researched in parallel
- Allows tech to attach to provinces (future feature)

### Why Bridge System?
- Economics needs to know about tech (for effects)
- Tech needs to know about economics (for budget)
- Bidirectional dependency needs mediator
- Avoids circular dependencies between systems

### Why Repository Pattern for Trade?
- Reduces boilerplate component access
- Centralizes component creation logic
- Improves testability
- Should consider using for Technology too

## Next Steps for Implementation

1. **Implement ResearchComponent update** - Calculate progress each frame
2. **Implement InnovationComponent update** - Random discovery chance
3. **Implement KnowledgeComponent update** - Knowledge decay and transmission
4. **Complete Bridge integration** - Apply effects to economy
5. **Add serialization** - Save/load tech state
6. **Add UI** - Display research progress
7. **Add tests** - Unit and integration tests
