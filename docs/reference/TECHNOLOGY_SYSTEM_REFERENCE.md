# Technology System - Complete Reference

**Created:** January 2025  
**Status:** Consolidated from 3 source documents  
**Purpose:** Single authoritative reference for Technology System

---

## Table of Contents

1. [Overview & Architecture](#overview--architecture)
2. [Components](#components)
3. [Technology Definitions](#technology-definitions)
4. [Integration](#integration)
5. [Implementation Guide](#implementation-guide)

---

## Overview & Architecture

### System Hierarchy

```
EntityManager (Core Registry)
    ↓
ComponentAccessManager (Thread-Safe Access)
    ↓
Game Systems (Including TechnologySystem)
    ↓
Bridge Systems (Technology-Economic Integration)
```

### File Locations

| Component | Location | Status |
|-----------|----------|--------|
| **Tech Components** | `include/game/technology/TechnologyComponents.h` | ✅ COMPLETE |
| **Tech System** | `include/game/technology/TechnologySystem.h` | ⚠️ SCAFFOLD |
| **Tech System Impl** | `src/game/technology/TechnologySystem.cpp` | ⚠️ PARTIAL |
| **Tech-Economy Bridge** | `include/game/economy/TechnologyEconomicBridge.h` | ✅ DEFINED |
| **Configuration** | `config/GameConfig.json` | ✅ EXISTS |

### Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         ENTITY-COMPONENT-SYSTEM (ECS)                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   EntityManager (Core Registry)                                              │
│   └─ Manages all entities and their components                               │
│                                                                              │
│   ComponentAccessManager (Thread-Safe Wrapper)                               │
│   └─ Synchronizes access to EntityManager                                    │
│   └─ Provides read/write locks per component type                            │
│   └─ Handles deferred writes and frame synchronization                       │
│                                                                              │
│   MessageBus (Event System)                                                  │
│   └─ Decoupled communication between systems                                 │
│   └─ Subscribe/Publish pattern                                               │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                           GAME SYSTEMS LAYER                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  All Systems Implement ISystem Interface:                                    │
│  ├─ void Initialize()                                                        │
│  ├─ void Update(float delta_time)                                            │
│  ├─ void Shutdown()                                                          │
│  └─ ThreadingStrategy GetThreadingStrategy()                                 │
│                                                                              │
│  Core Systems:                                                               │
│  ├─ EconomicSystem              └─ Treasury, income, trades                  │
│  ├─ PopulationSystem            └─ Demographics, happiness                   │
│  ├─ MilitarySystem              └─ Armies, recruitment                       │
│  ├─ DiplomacySystem             └─ Relations, treaties                       │
│  ├─ TradeSystem                 └─ Trade routes, commerce                    │
│  ├─ TechnologySystem            └─ Research, innovation (NEW)                │
│  └─ TimeManagementSystem        └─ Game clock, speed control                 │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                      TECHNOLOGY SYSTEM IN DETAIL                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  TechnologySystem Class                                                      │
│  ├─ Initialize()                                                             │
│  │  └─ Register with MessageBus, load config                                 │
│  │                                                                           │
│  ├─ Update(float delta_time)                                                 │
│  │  ├─ UpdateResearchComponents()                                            │
│  │  │  └─ Calculate progress, check prerequisites, publish events            │
│  │  ├─ UpdateInnovationComponents()                                          │
│  │  │  └─ Random discovery, breakthrough chance                              │
│  │  ├─ UpdateKnowledgeComponents()                                           │
│  │  │  └─ Knowledge transmission, network effects                            │
│  │  └─ ProcessTechnologyEvents()                                             │
│  │     └─ Track discoveries, publish notifications                           │
│  │                                                                           │
│  └─ Component Management Methods                                             │
│     ├─ CreateResearchComponent(entity_id)                                    │
│     ├─ CreateInnovationComponent(entity_id)                                  │
│     ├─ CreateKnowledgeComponent(entity_id)                                   │
│     └─ CreateTechnologyEventsComponent(entity_id)                            │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Data Flow Diagram

```
Each Frame:

1. EconomicSystem processes treasury & calculates available budget
   └─ Posts to MessageBus: Monthly income calculated

2. TechnologyEconomicBridge observes economic changes
   ├─ Reads EconomicComponent from entity
   ├─ Reads ResearchComponent from entity
   └─ Updates TechnologyEconomicBridgeComponent
      ├─ Allocates monthly_research_budget
      └─ Detects crises (if applicable)

3. TechnologySystem updates all technology components
   ├─ UpdateResearchComponents()
   │  ├─ Reads monthly_research_budget
   │  ├─ Calculate research progress += (budget / tech_cost)
   │  ├─ Check prerequisites met
   │  └─ Post event if research completes
   │
   ├─ UpdateInnovationComponents()
   │  ├─ Roll for accidental discoveries
   │  └─ Post event if breakthrough occurs
   │
   └─ UpdateKnowledgeComponents()
      ├─ Knowledge transmission between connected realms
      ├─ Knowledge decay (loss_rate per month)
      └─ Update literacy effects

4. Bridge applies technology effects back to economy
   ├─ Read implemented technologies from research states
   ├─ Calculate TechnologyEconomicEffects
   └─ Apply to EconomicComponent (production bonus, etc.)

5. Other systems observe MessageBus for tech events
   ├─ MilitarySystem: Military tech affects unit effectiveness
   ├─ PopulationSystem: Academic tech affects literacy/education
   └─ TradeSystem: Naval tech affects trade efficiency
```

---

## Components

### 1. ResearchComponent

**Purpose:** Track technology research and development

#### Key Fields

```cpp
struct ResearchComponent : public Component<ResearchComponent> {
    // State Tracking
    std::unordered_map<TechnologyType, ResearchState> technology_states;
    /*  ResearchState values:
        - UNKNOWN: Not yet discovered as possibility
        - AVAILABLE: Known but not being researched
        - RESEARCHING: Active research in progress
        - DISCOVERED: Research complete, pending implementation
        - IMPLEMENTING: Being rolled out to realm
        - IMPLEMENTED: Fully integrated and providing benefits
    */
    
    std::unordered_map<TechnologyType, double> research_progress;
    // Progress: 0.0 (not started) to 1.0 (complete)
    
    TechnologyType current_focus;
    // Single focused tech receives 50% research bonus
    
    // Research Infrastructure
    int universities;              // Higher education institutions
    int libraries;                 // Knowledge repositories
    int workshops;                 // Practical research facilities
    int scholar_population;        // Dedicated researchers
    
    // Research Capacity
    double monthly_research_budget;       // Funding from treasury
    double base_research_efficiency;      // Base research speed
    double literacy_bonus;                // Bonus from educated population
    
    // Statistics
    int total_technologies_discovered;
    int total_technologies_implemented;
    double lifetime_research_investment;
};
```

#### Usage Example

```cpp
auto research_comp = component_access_.GetComponent<ResearchComponent>(realm_id);
if (research_comp.HasValue()) {
    auto& research = research_comp.GetValue();
    
    // Check if tech is available for research
    if (research.technology_states[PRINTING_PRESS] == ResearchState::AVAILABLE) {
        research.technology_states[PRINTING_PRESS] = ResearchState::RESEARCHING;
        research.research_progress[PRINTING_PRESS] = 0.0;
        research.current_focus = PRINTING_PRESS;
    }
}
```

### 2. InnovationComponent

**Purpose:** Native innovation and accidental invention

#### Key Fields

```cpp
struct InnovationComponent : public Component<InnovationComponent> {
    // Innovation Rates
    double innovation_rate;           // Base chance for discovery per month
    double breakthrough_chance;       // Chance for major breakthrough
    
    // Personnel
    int inventors;                    // Dedicated inventors
    int craftsmen_innovators;         // Skilled craftspeople innovating
    
    // Cultural Factors
    double cultural_openness;         // 0.0-1.0: Receptivity to new ideas
    double guild_resistance;          // Negative modifier from tradition
    double religious_restriction;     // Religious barriers to innovation
    
    // Support
    double royal_patronage;           // Crown funding multiplier
    double merchant_funding;          // Private sector investment
    
    // History
    std::vector<std::string> accidental_discoveries;
    int total_breakthroughs;
};
```

#### Innovation Calculation

```cpp
// Monthly innovation check
double effective_innovation_rate = 
    base_innovation_rate * 
    (1.0 + cultural_openness) *
    (1.0 - guild_resistance) *
    (1.0 - religious_restriction) *
    (1.0 + royal_patronage + merchant_funding);

if (RandomRoll() < effective_innovation_rate) {
    // Accidental discovery of random available technology
}

if (RandomRoll() < breakthrough_chance) {
    // Major breakthrough: instant research completion
}
```

### 3. KnowledgeComponent

**Purpose:** Knowledge preservation and transmission networks

#### Key Fields

```cpp
struct KnowledgeComponent : public Component<KnowledgeComponent> {
    // Infrastructure
    int manuscripts;                  // Preserved documents
    int scribes;                      // Copyists and translators
    int translators;                  // Language experts
    
    // Networks
    std::unordered_map<types::EntityID, double> knowledge_networks;
    // Map of connected realms → connection strength (0.0-1.0)
    
    std::unordered_set<types::EntityID> scholarly_exchanges;
    // Active exchange programs with other realms
    
    std::unordered_set<types::EntityID> trade_knowledge_routes;
    // Trade routes that also transmit knowledge
    
    // Population Education
    double literacy_rate;             // 0.0-1.0: % of population literate
    double knowledge_transmission_rate; // Speed of knowledge spread
    
    // Statistics
    int total_manuscripts_preserved;
    int total_knowledge_transmitted;
    std::vector<types::EntityID> knowledge_sources;  // Where knowledge came from
};
```

#### Knowledge Transmission

```cpp
// Knowledge spreads between connected realms
for (auto& [connected_realm, strength] : knowledge_networks) {
    auto their_research = GetComponent<ResearchComponent>(connected_realm);
    auto our_research = GetComponent<ResearchComponent>(our_realm);
    
    // They have tech we don't?
    for (auto& [tech, their_state] : their_research.technology_states) {
        if (their_state == ResearchState::IMPLEMENTED &&
            our_research.technology_states[tech] == ResearchState::UNKNOWN) {
            
            // Transmission chance based on network strength
            if (RandomRoll() < strength * knowledge_transmission_rate) {
                our_research.technology_states[tech] = ResearchState::AVAILABLE;
                // Now we can research it!
            }
        }
    }
}
```

### 4. TechnologyEventsComponent

**Purpose:** Historical tracking of discoveries and breakthroughs

#### Key Fields

```cpp
struct TechnologyEventsComponent : public Component<TechnologyEventsComponent> {
    // Recent Activity
    std::vector<std::string> recent_discoveries;     // Last 10 discoveries
    std::vector<std::string> research_breakthroughs; // Major breakthroughs
    
    // Historical Tracking
    std::unordered_map<TechnologyType, int64_t> discovery_dates;
    // When each tech was discovered (timestamp)
    
    std::unordered_map<TechnologyType, DiscoveryMethod> discovery_methods;
    /*  DiscoveryMethod values:
        - RESEARCHED: Normal research progression
        - INNOVATION: Accidental discovery
        - BREAKTHROUGH: Major breakthrough
        - TRANSMITTED: Learned from another realm
        - STOLEN: Espionage or conquest
    */
    
    // Reputation
    double technological_reputation;  // Fame as technology leader
    double innovation_prestige;       // Prestige from innovations
    
    // Statistics
    int first_discoverer_count;       // # of techs discovered first globally
    int early_adopter_count;          // # of techs adopted early
};
```

---

## Technology Definitions

### Technology ID Ranges

| Category | ID Range | Count | Era |
|----------|----------|-------|-----|
| **Agricultural** | 1000-1099 | 15+ | 1000-1400 AD |
| **Military** | 1100-1199 | 20+ | 1000-1600 AD |
| **Craft** | 1200-1299 | 15+ | 1200-1700 AD |
| **Administrative** | 1300-1399 | 10+ | 1100-1500 AD |
| **Academic** | 1400-1499 | 12+ | 1200-1600 AD |
| **Naval** | 1500-1599 | 10+ | 1300-1700 AD |

### Agricultural Technologies (1000-1099)

```cpp
enum TechnologyType {
    // Basic Agriculture (1000-1019)
    THREE_FIELD_SYSTEM = 1000,
    /*  Prerequisites: None
        Effects: +25% agricultural production
        Cost: 500 research points
        Implementation: 6 months
        Era: Available from game start
    */
    
    HEAVY_PLOW = 1001,
    /*  Prerequisites: None
        Effects: +20% food production from plains
        Cost: 400 research points
        Implementation: 4 months
        Era: 1000+ AD
    */
    
    HORSE_COLLAR = 1002,
    /*  Prerequisites: HEAVY_PLOW
        Effects: +15% plowing efficiency, +10% transport
        Cost: 600 research points
        Implementation: 8 months
        Era: 1100+ AD
    */
    
    WINDMILL = 1003,
    /*  Prerequisites: None
        Effects: +30% milling efficiency, enables pumping
        Cost: 800 research points
        Implementation: 12 months
        Era: 1150+ AD
    */
    
    CROP_ROTATION = 1004,
    /*  Prerequisites: THREE_FIELD_SYSTEM
        Effects: +20% agricultural yield, +soil health
        Cost: 700 research points
        Implementation: 24 months (full cycle)
        Era: 1200+ AD
    */
    
    // Advanced Agriculture (1020-1039)
    IRRIGATION_SYSTEMS = 1020,
    FERTILIZER_TECHNIQUES = 1021,
    SELECTIVE_BREEDING = 1022,
    // ... more agricultural techs
};
```

### Military Technologies (1100-1199)

```cpp
    // Armor (1100-1119)
    CHAINMAIL_ARMOR = 1100,
    /*  Prerequisites: None
        Effects: +25% infantry defense
        Cost: 800 research points
        Maintenance: +15% unit cost
        Era: 1000+ AD
    */
    
    PLATE_ARMOR = 1101,
    /*  Prerequisites: CHAINMAIL_ARMOR, BLAST_FURNACE
        Effects: +50% cavalry defense, +30% infantry defense
        Cost: 2000 research points
        Maintenance: +40% unit cost
        Era: 1350+ AD
    */
    
    // Weapons (1120-1139)
    CROSSBOW = 1120,
    /*  Prerequisites: None
        Effects: +40% ranged attack, -20% fire rate
        Cost: 600 research points
        Era: 1000+ AD
    */
    
    LONGBOW = 1121,
    /*  Prerequisites: None
        Effects: +30% range, +25% fire rate
        Cost: 500 research points
        Training: Requires 10 years
        Era: 1200+ AD
    */
    
    CANNONS = 1122,
    /*  Prerequisites: GUNPOWDER, BRONZE_CASTING
        Effects: Siege bonus, +200% fortification damage
        Cost: 3000 research points
        Era: 1400+ AD
    */
    
    // Fortifications (1140-1159)
    CASTLE_CONSTRUCTION = 1140,
    STAR_FORTS = 1141,
    // ... more military techs
```

### Craft Technologies (1200-1299)

```cpp
    // Metalworking (1200-1219)
    BLAST_FURNACE = 1200,
    /*  Prerequisites: None
        Effects: +100% iron production, enables steel
        Cost: 1500 research points
        Implementation: 18 months
        Era: 1400+ AD
    */
    
    ADVANCED_METALLURGY = 1201,
    /*  Prerequisites: BLAST_FURNACE
        Effects: +50% metal quality, +25% production
        Cost: 2000 research points
        Era: 1500+ AD
    */
    
    // Knowledge (1220-1239)
    PRINTING_PRESS = 1220,
    /*  Prerequisites: PAPER_MAKING, MECHANICAL_PRESS
        Effects: +200% knowledge transmission, -80% book cost
        Cost: 2500 research points
        Implementation: 24 months
        Era: 1450+ AD
    */
    
    MECHANICAL_CLOCK = 1221,
    /*  Prerequisites: ADVANCED_GEARS
        Effects: +navigation accuracy, +time tracking
        Cost: 1800 research points
        Era: 1300+ AD
    */
    
    // Commerce (1240-1259)
    DOUBLE_ENTRY_BOOKKEEPING = 1240,
    /*  Prerequisites: ARABIC_NUMERALS
        Effects: +30% tax collection efficiency
        Cost: 1000 research points
        Era: 1300+ AD
    */
```

### Administrative Technologies (1300-1399)

```cpp
    WRITTEN_LAW_CODES = 1300,
    /*  Prerequisites: None
        Effects: +20% administrative efficiency, +legitimacy
        Cost: 1200 research points
        Era: 1100+ AD
    */
    
    BUREAUCRATIC_ADMINISTRATION = 1301,
    /*  Prerequisites: WRITTEN_LAW_CODES
        Effects: +40% admin efficiency, +centralization
        Cost: 2000 research points
        Era: 1300+ AD
    */
    
    CENSUS_TECHNIQUES = 1302,
    /*  Prerequisites: BUREAUCRATIC_ADMINISTRATION
        Effects: +50% tax collection, +population tracking
        Cost: 1500 research points
        Era: 1400+ AD
    */
    
    TAX_COLLECTION_SYSTEM = 1303,
    /*  Prerequisites: WRITTEN_LAW_CODES
        Effects: +35% tax revenue, -corruption
        Cost: 1800 research points
        Era: 1200+ AD
    */
```

### Academic Technologies (1400-1499)

```cpp
    UNIVERSITY_SYSTEM = 1400,
    /*  Prerequisites: None
        Effects: +100% research speed, +literacy
        Cost: 2500 research points
        Maintenance: High (salaries)
        Era: 1200+ AD
    */
    
    SCHOLASTIC_METHOD = 1401,
    /*  Prerequisites: UNIVERSITY_SYSTEM
        Effects: +50% research efficiency, +education
        Cost: 1800 research points
        Era: 1300+ AD
    */
    
    EXPERIMENTAL_METHOD = 1402,
    /*  Prerequisites: SCHOLASTIC_METHOD
        Effects: +innovation rate +75%, enables rapid progress
        Cost: 3000 research points
        Era: 1600+ AD
    */
    
    SCIENTIFIC_INSTRUMENTS = 1403,
    /*  Prerequisites: EXPERIMENTAL_METHOD, PRECISION_METALWORKING
        Effects: +100% research accuracy, +breakthrough chance
        Cost: 2200 research points
        Era: 1600+ AD
    */
```

### Naval Technologies (1500-1599)

```cpp
    IMPROVED_SHIP_DESIGN = 1500,
    /*  Prerequisites: None
        Effects: +30% ship speed, +cargo capacity
        Cost: 1200 research points
        Era: 1200+ AD
    */
    
    NAVIGATION_INSTRUMENTS = 1501,
    /*  Prerequisites: None
        Effects: +50% exploration range, -shipwreck chance
        Cost: 1500 research points
        Era: 1300+ AD
    */
    
    COMPASS_NAVIGATION = 1502,
    /*  Prerequisites: NAVIGATION_INSTRUMENTS
        Effects: +100% navigation accuracy, enables ocean travel
        Cost: 1800 research points
        Era: 1400+ AD
    */
    
    NAVAL_ARTILLERY = 1503,
    /*  Prerequisites: CANNONS, IMPROVED_SHIP_DESIGN
        Effects: +200% naval combat, enables ship-to-ship combat
        Cost: 2500 research points
        Era: 1500+ AD
    */
```

---

## Integration

### Technology-Economic Bridge

**Primary Integration:** TechnologyEconomicBridge connects research and economy

#### Bidirectional Effects

**Technology → Economy:**
```cpp
struct TechnologyEconomicEffects {
    double production_efficiency = 1.0;    // Agricultural + Craft tech
    double trade_efficiency = 1.0;          // Naval + Craft tech
    double tax_efficiency = 1.0;            // Administrative tech
    double market_sophistication = 1.0;     // Academic + Craft tech
    double innovation_rate_modifier = 1.0;  // Academic tech
};

// Calculate effects from implemented technologies
if (HasTech(THREE_FIELD_SYSTEM)) {
    effects.production_efficiency *= 1.15;  // +15%
}
if (HasTech(BLAST_FURNACE)) {
    effects.production_efficiency *= 1.20;  // +20%
}
if (HasTech(IMPROVED_SHIP_DESIGN)) {
    effects.trade_efficiency *= 1.10;  // +10%
}
if (HasTech(TAX_COLLECTION_SYSTEM)) {
    effects.tax_efficiency *= 1.12;  // +12%
}
if (HasTech(UNIVERSITY_SYSTEM)) {
    effects.innovation_rate_modifier *= 1.25;  // +25%
}
```

**Economy → Technology:**
```cpp
// Treasury funds research
double research_budget = treasury * research_budget_percentage;  // typically 5%

// Trade routes spread knowledge
double knowledge_bonus = active_trade_routes * 0.02;  // +2% per route

// Wealth enables innovation
double innovation_bonus = (treasury / 100000.0) * 0.1;  // scales with wealth

// Stability affects research speed
double research_speed = base_speed * stability_modifier;
```

#### Crisis Detection

```cpp
// Research funding crisis
if (monthly_budget < minimum_research_budget * 0.3) {
    PublishEvent(ResearchFundingCrisis{realm_id});
    // Effects: Scholar unhappiness, slower research, brain drain risk
}

// Implementation crisis
if (treasury < tech_implementation_cost * 0.5) {
    PublishEvent(ImplementationCrisis{realm_id, tech_id});
    // Effects: Can't implement discovered technologies
}

// Brain drain event
if (scholar_funding < scholar_salaries * 0.4) {
    PublishEvent(BrainDrainEvent{realm_id});
    // Effects: Scholars emigrate to better-funded realms
}
```

#### Event Messages

```cpp
struct TechnologyBreakthroughEconomicImpact {
    types::EntityID realm_id;
    TechnologyType tech;
    double immediate_cost;           // Implementation cost
    double monthly_maintenance;      // Ongoing costs
    double expected_roi_years;       // Payback period
};

struct ResearchFundingCrisis {
    types::EntityID realm_id;
    double funding_deficit;
    int affected_scholars;
    double research_slowdown;        // % slower
};
```

### Diplomacy Integration

**Knowledge Transfer:**
```cpp
// Embassies enable knowledge sharing
if (HasEmbassy(realm_a, realm_b) && AreAllied(realm_a, realm_b)) {
    double transfer_rate = embassy_quality * 0.05;  // Up to 5% per month
    TransferKnowledgeAboutTech(realm_a, realm_b, random_tech, transfer_rate);
}

// Treaties can include tech sharing provisions
Treaty alliance_treaty;
alliance_treaty.provisions.push_back(TechSharingProvision{
    .techs_to_share = {PRINTING_PRESS, BLAST_FURNACE},
    .transfer_speed = 0.10  // 10% per month
});
```

### Trade Integration

**Merchants Spread Knowledge:**
```cpp
// Active trade routes boost research
for (auto& route : active_trade_routes) {
    if (route.IsActive()) {
        research_budget_bonus += monthly_trade_income * 0.02;
        knowledge_transmission_rate += 0.01;
    }
}

// Trade hubs become knowledge centers
if (IsTradeHub(province)) {
    knowledge_transmission_rate *= 1.5;
    innovation_rate *= 1.2;
}
```

### Population Integration

**Education and Innovation:**
```cpp
auto pop_comp = GetComponent<PopulationComponent>(province_id);

// Scholar population contributes to research
int total_scholars = pop_comp.scholar_population;
research_capacity += total_scholars * scholar_research_rate;

// Literacy affects research efficiency
double literacy_bonus = pop_comp.literacy_rate * 0.5;
research_efficiency *= (1.0 + literacy_bonus);

// Happiness affects innovation
double happiness_modifier = (pop_comp.average_happiness - 0.5) * 0.4;
innovation_rate *= (1.0 + happiness_modifier);
```

---

## Implementation Guide

### Setup Steps

#### 1. Create Technology Components

```cpp
// For each realm that can research
types::EntityID realm_id = CreateRealm("England");

// Add research component
ResearchComponent research;
research.universities = 2;
research.libraries = 5;
research.scholar_population = 100;
research.monthly_research_budget = 500.0;
research.base_research_efficiency = 1.0;
component_access_.GetEntityManager().AddComponent(realm_id, research);

// Add innovation component
InnovationComponent innovation;
innovation.innovation_rate = 0.05;  // 5% per month
innovation.breakthrough_chance = 0.01;  // 1% per month
innovation.cultural_openness = 0.7;
component_access_.GetEntityManager().AddComponent(realm_id, innovation);

// Add knowledge component
KnowledgeComponent knowledge;
knowledge.literacy_rate = 0.15;  // 15% literate
knowledge.knowledge_transmission_rate = 0.03;
component_access_.GetEntityManager().AddComponent(realm_id, knowledge);

// Add events component
TechnologyEventsComponent events;
component_access_.GetEntityManager().AddComponent(realm_id, events);
```

#### 2. Initialize Technology States

```cpp
// Set initial technology states
auto research_comp = GetComponentForWrite<ResearchComponent>(realm_id);

// Start with some basic techs already known
research_comp.technology_states[THREE_FIELD_SYSTEM] = ResearchState::AVAILABLE;
research_comp.technology_states[HEAVY_PLOW] = ResearchState::AVAILABLE;
research_comp.technology_states[CHAINMAIL_ARMOR] = ResearchState::IMPLEMENTED;

// Rest are unknown
for (int i = 1000; i < 1600; i++) {
    auto tech = static_cast<TechnologyType>(i);
    if (research_comp.technology_states.find(tech) == research_comp.technology_states.end()) {
        research_comp.technology_states[tech] = ResearchState::UNKNOWN;
    }
}
```

#### 3. Setup Technology-Economic Bridge

```cpp
// Create bridge component
TechnologyEconomicBridgeComponent bridge;
bridge.research_budget_percentage = 0.05;  // 5% of income
bridge.last_update_time = GetCurrentTime();
component_access_.GetEntityManager().AddComponent(realm_id, bridge);

// Subscribe to economic events
message_bus_.Subscribe<MonthlyIncomeCalculated>([this](const auto& event) {
    UpdateResearchBudgets(event.realm_id, event.monthly_income);
});
```

#### 4. Configuration

**config/GameConfig.json:**
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
  },
  "economic_bridge": {
    "update_interval_days": 1.0,
    "bridge_update_interval": 1.0,
    "research_budget_base_percentage": 0.05,
    "trade_knowledge_bonus_per_route": 0.02,
    "crisis_thresholds": {
      "funding_crisis_threshold": 0.3,
      "implementation_crisis_threshold": 0.5,
      "brain_drain_threshold": 0.4
    }
  }
}
```

### API Usage Examples

#### Starting Research

```cpp
types::EntityID realm_id = GetPlayerRealm();

// Get research component
auto research = GetComponentForWrite<ResearchComponent>(realm_id);

// Set focus technology (gets 50% bonus)
research.current_focus = PRINTING_PRESS;
research.technology_states[PRINTING_PRESS] = ResearchState::RESEARCHING;
research.research_progress[PRINTING_PRESS] = 0.0;

// Publish event
message_bus_.Publish(ResearchStartedEvent{realm_id, PRINTING_PRESS});
```

#### Checking Research Progress

```cpp
auto research = GetComponent<ResearchComponent>(realm_id);

if (research.technology_states[PRINTING_PRESS] == ResearchState::RESEARCHING) {
    double progress = research.research_progress[PRINTING_PRESS];
    std::cout << "Printing Press: " << (progress * 100) << "% complete\n";
    
    if (progress >= 1.0) {
        // Research complete!
        research.technology_states[PRINTING_PRESS] = ResearchState::DISCOVERED;
        message_bus_.Publish(TechnologyDiscoveredEvent{realm_id, PRINTING_PRESS});
    }
}
```

#### Implementing Technology

```cpp
// Check if can afford implementation
double impl_cost = CalculateImplementationCost(PRINTING_PRESS);
auto economic = GetComponent<EconomicComponent>(realm_id);

if (economic.treasury >= impl_cost) {
    // Deduct cost
    economic.treasury -= impl_cost;
    
    // Start implementation
    auto research = GetComponentForWrite<ResearchComponent>(realm_id);
    research.technology_states[PRINTING_PRESS] = ResearchState::IMPLEMENTING;
    
    // Schedule completion event (12 months later)
    ScheduleEvent(TechnologyImplementedEvent{realm_id, PRINTING_PRESS}, 
                  GetCurrentTime() + (12 * 30 * 24 * 3600));  // 12 months
}
```

#### Knowledge Transmission

```cpp
// Setup knowledge network between realms
auto knowledge = GetComponentForWrite<KnowledgeComponent>(england_id);
knowledge.knowledge_networks[france_id] = 0.5;  // 50% strength
knowledge.scholarly_exchanges.insert(france_id);

// Each frame, knowledge spreads
auto france_research = GetComponent<ResearchComponent>(france_id);
auto england_research = GetComponentForWrite<ResearchComponent>(england_id);

for (auto& [tech, state] : france_research.technology_states) {
    if (state == ResearchState::IMPLEMENTED &&
        england_research.technology_states[tech] == ResearchState::UNKNOWN) {
        
        double transmission_chance = 
            knowledge.knowledge_networks[france_id] * 
            knowledge.knowledge_transmission_rate;
            
        if (RandomRoll() < transmission_chance) {
            england_research.technology_states[tech] = ResearchState::AVAILABLE;
            message_bus_.Publish(KnowledgeTransmittedEvent{
                .source_realm = france_id,
                .target_realm = england_id,
                .technology = tech
            });
        }
    }
}
```

### System Update Loop

```cpp
void TechnologySystem::Update(float delta_time) {
    // 1. Update research progress
    auto entities = component_access_.GetEntitiesWithComponent<ResearchComponent>();
    for (auto entity : entities) {
        UpdateResearch(entity, delta_time);
    }
    
    // 2. Check for innovations
    auto innovators = component_access_.GetEntitiesWithComponent<InnovationComponent>();
    for (auto entity : innovators) {
        CheckInnovation(entity, delta_time);
    }
    
    // 3. Transmit knowledge
    auto knowledge_holders = component_access_.GetEntitiesWithComponent<KnowledgeComponent>();
    for (auto entity : knowledge_holders) {
        TransmitKnowledge(entity, delta_time);
    }
    
    // 4. Process technology events
    ProcessTechnologyEvents(delta_time);
}

void TechnologySystem::UpdateResearch(types::EntityID entity, float delta_time) {
    auto research = GetComponentForWrite<ResearchComponent>(entity);
    auto bridge = GetComponent<TechnologyEconomicBridgeComponent>(entity);
    
    if (!research.IsValid() || !bridge.HasValue()) return;
    
    double budget = bridge.GetValue().monthly_research_budget;
    double efficiency = research.Get().base_research_efficiency;
    
    // Update each technology being researched
    for (auto& [tech, state] : research.Get().technology_states) {
        if (state == ResearchState::RESEARCHING) {
            double cost = GetTechnologyCost(tech);
            double progress_per_second = (budget / cost) * efficiency / (30.0 * 24.0 * 3600.0);
            
            // Apply focus bonus
            if (tech == research.Get().current_focus) {
                progress_per_second *= 1.5;
            }
            
            research.Get().research_progress[tech] += progress_per_second * delta_time;
            
            // Check completion
            if (research.Get().research_progress[tech] >= 1.0) {
                CompleteTechnologyResearch(entity, tech);
            }
        }
    }
}
```

---

## Design Patterns Used

1. **ENTITY-COMPONENT-SYSTEM (ECS)** - Separates data (components) from logic (systems)

2. **BRIDGE PATTERN** - Integrates two systems without circular dependencies

3. **CALCULATOR PATTERN** - Use TechnologyCalculator for testable calculations

4. **MESSAGE/EVENT PATTERN** - Systems communicate via MessageBus for decoupling

5. **STRONG TYPES** - Use types::EntityID instead of raw uint32_t

6. **COMPOSITION OVER INHERITANCE** - Use multiple components instead of class hierarchies

---

## Revision History

| Date | Change | Author |
|------|--------|--------|
| Oct 31, 2025 | Initial implementation guide created | AI Assistant |
| Jan 2025 | Consolidated from 3 source documents | AI Assistant |

---

## Source Documents Consolidated

This reference was created by merging content from:
1. TECHNOLOGY_IMPLEMENTATION_GUIDE.md (549 lines)
2. TECHNOLOGY_SYSTEM_QUICK_REFERENCE.md (299 lines)
3. TECHNOLOGY_ARCHITECTURE_DIAGRAM.txt (ASCII diagram)

All original files archived to: `archive/2025-01-docs-cleanup/superseded/`

---

**For Questions:** Refer to source code in `include/game/technology/` and `src/game/technology/`  
**For Updates:** Modify this file and update "Last Updated" date  
**For Implementation:** Follow setup steps and API usage examples
