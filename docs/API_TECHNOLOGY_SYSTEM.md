# Technology System API Documentation

**Version**: 1.0
**Created**: 2025-11-22
**System**: Technology System (Phase 3, System #006)

---

## Table of Contents

1. [Overview](#overview)
2. [Core Components](#core-components)
3. [System Initialization](#system-initialization)
4. [Technology Research](#technology-research)
5. [Innovation and Breakthroughs](#innovation-and-breakthroughs)
6. [Knowledge Transmission](#knowledge-transmission)
7. [Technology Effects](#technology-effects)
8. [Prerequisites System](#prerequisites-system)
9. [Technology Data](#technology-data)
10. [Integration Patterns](#integration-patterns)
11. [Examples](#examples)
12. [Best Practices](#best-practices)

---

## Overview

The Technology System manages medieval research, innovation, knowledge transmission, and technology progression. It provides a complete simulation of technological advancement from the early medieval period (1066) through the Renaissance.

### Key Features

- **60 Technologies** across 6 categories (Agricultural, Military, Craft, Administrative, Academic, Naval)
- **4-Component ECS Architecture** for flexible entity management
- **Prerequisite System** with historical accuracy
- **Dynamic Effects** that integrate with economy, military, and population systems
- **Knowledge Networks** for cultural transmission
- **Innovation Breakthroughs** for accelerated research

### Threading Model

- **Strategy**: MAIN_THREAD (single-threaded)
- **Rationale**: Uses raw component pointers for performance; sequential access prevents use-after-free
- **Update Frequency**: 1 Hz (once per second)

---

## Core Components

### 1. ResearchComponent

Manages research progress and infrastructure for an entity (province/nation).

```cpp
struct ResearchComponent {
    // Research state for each technology
    std::unordered_map<TechnologyType, ResearchState> technology_states;
    std::unordered_map<TechnologyType, double> research_progress;  // 0.0 - 1.0
    std::unordered_map<TechnologyType, double> implementation_level;  // 0.0 - 1.0

    // Research infrastructure
    uint32_t universities;
    uint32_t monasteries;
    uint32_t libraries;
    uint32_t workshops;
    uint32_t scholar_population;

    // Research budget and investment
    double monthly_research_budget;
    std::unordered_map<TechnologyCategory, double> category_investment;

    // Research modifiers
    double base_research_efficiency;  // 1.0 = normal
    double literacy_bonus;            // From population literacy
    double trade_network_bonus;       // From trade routes
    double stability_bonus;           // From economic stability

    // Focus system
    TechnologyType current_focus;     // Focused research gets bonus
    double focus_bonus;               // Default 0.5 (+50%)
};
```

**States**:
- `UNKNOWN`: Not yet available for research
- `AVAILABLE`: Can be researched (prerequisites met)
- `RESEARCHING`: Currently being researched
- `DISCOVERED`: Research complete, ready for implementation
- `IMPLEMENTING`: Being implemented in the province
- `IMPLEMENTED`: Fully implemented and providing benefits

**Usage**:
```cpp
// Get research component
auto* research = tech_system.GetResearchComponent(entity_id);

// Set research budget
research->monthly_research_budget = 1000.0;

// Distribute budget across categories
research->category_investment[TechnologyCategory::AGRICULTURAL] = 400.0;
research->category_investment[TechnologyCategory::MILITARY] = 300.0;
research->category_investment[TechnologyCategory::CRAFT] = 300.0;

// Focus on a specific technology (gets +50% research speed)
research->current_focus = TechnologyType::PRINTING_PRESS;
```

### 2. InnovationComponent

Manages innovation potential and breakthrough mechanics.

```cpp
struct InnovationComponent {
    // Innovation capacity
    double innovation_rate;           // Base innovation chance per update
    double breakthrough_chance;       // Chance for major breakthrough

    // Innovator population
    uint32_t inventors;
    uint32_t craftsmen;
    uint32_t scholars;

    // Innovation environment
    double cultural_openness;         // 0.0 - 1.0 (affects innovation chance)
    double innovation_encouragement;  // From ruler policies
    double guild_resistance;          // Guilds resist change (0.0 - 1.0)
    double religious_restriction;     // Religious limits on innovation

    // Funding
    double royal_patronage;           // Ruler's support for innovation
    double merchant_funding;          // Merchant investment

    // Innovation expertise by category
    std::unordered_map<TechnologyCategory, double> innovation_expertise;

    // Breakthrough tracking
    std::chrono::system_clock::time_point last_breakthrough;
    std::vector<TechnologyType> recent_discoveries;
};
```

**Usage**:
```cpp
// Get innovation component
auto* innovation = tech_system.GetInnovationComponent(entity_id);

// Encourage innovation through policy
innovation->cultural_openness = 0.8;          // Very open to new ideas
innovation->innovation_encouragement = 0.6;   // Moderate government support
innovation->guild_resistance = 0.3;           // Some guild opposition
innovation->religious_restriction = 0.2;      // Minor religious limits

// Provide funding
innovation->royal_patronage = 0.4;            // 40% bonus from ruler
innovation->merchant_funding = 0.3;           // 30% bonus from merchants

// Build expertise in specific areas
innovation->innovation_expertise[TechnologyCategory::CRAFT] = 0.9;
```

### 3. KnowledgeComponent

Manages knowledge preservation and transmission networks.

```cpp
struct KnowledgeComponent {
    // Manuscript and book system
    uint32_t manuscripts;
    uint32_t scribes;
    double book_production_capacity;
    double manuscript_durability;     // How well manuscripts are preserved

    // Knowledge quality
    double knowledge_preservation_quality;  // 0.0 - 1.0
    double knowledge_loss_rate;            // Monthly knowledge decay rate
    double knowledge_transmission_rate;     // How fast knowledge spreads

    // Literacy system
    double literacy_rate;
    std::vector<std::string> supported_languages;

    // Knowledge network
    double network_strength;                                 // Overall network effectiveness
    std::unordered_map<types::EntityID, double> knowledge_connections;  // Connected entities
    std::unordered_map<TechnologyType, double> specific_knowledge;      // Technology knowledge level

    // Cultural factors
    double cultural_knowledge_absorption;    // How well foreign knowledge is absorbed
    double foreign_knowledge_acceptance;     // Willingness to learn from others
};
```

**Usage**:
```cpp
// Get knowledge component
auto* knowledge = tech_system.GetKnowledgeComponent(entity_id);

// Build knowledge infrastructure
knowledge->manuscripts = 500;
knowledge->scribes = 50;
knowledge->book_production_capacity = 10.0;  // Books per month

// Improve preservation
knowledge->knowledge_preservation_quality = 0.8;  // Good preservation
knowledge->manuscript_durability = 0.9;           // Well-maintained

// Create knowledge network
knowledge->knowledge_connections[neighbor_id] = 0.7;  // Strong connection
knowledge->network_strength = 1.5;                     // Effective network

// Cultural openness
knowledge->cultural_knowledge_absorption = 0.8;   // Absorb 80% of foreign knowledge
knowledge->foreign_knowledge_acceptance = 0.7;    // 70% willing to learn
```

### 4. TechnologyEventsComponent

Tracks technology-related events and history.

```cpp
struct TechnologyEventsComponent {
    // Discovery tracking
    std::unordered_map<TechnologyType, std::chrono::system_clock::time_point> discovery_dates;
    std::unordered_map<TechnologyType, DiscoveryMethod> discovery_methods;
    std::unordered_map<TechnologyType, double> discovery_investments;

    // Event history
    std::vector<std::string> recent_discoveries;
    std::vector<std::string> innovation_attempts;
    std::vector<std::string> research_breakthroughs;

    // Reputation and prestige
    double technological_reputation;   // 0.0 - 10.0
    double scholarly_recognition;      // International recognition

    // Active research tracking
    size_t max_event_history;          // Maximum history size (default 100)
    int months_since_last_discovery;
    int months_since_last_innovation;
    int months_since_last_breakthrough;
};
```

**Usage**:
```cpp
// Get events component
auto* events = tech_system.GetTechnologyEventsComponent(entity_id);

// Check discovery history
if (events->discovery_dates.find(TechnologyType::PRINTING_PRESS) !=
    events->discovery_dates.end()) {
    auto date = events->discovery_dates[TechnologyType::PRINTING_PRESS];
    // Technology was discovered at 'date'
}

// Check reputation
double reputation = events->technological_reputation;  // 0.0 - 10.0 scale
```

---

## System Initialization

### Basic Initialization

```cpp
#include "game/technology/TechnologySystem.h"

// Create system
ComponentAccessManager component_manager;
ThreadSafeMessageBus message_bus;
TechnologySystem tech_system(component_manager, message_bus);

// Initialize system
tech_system.Initialize();
```

### Entity Initialization

```cpp
// Initialize all technology components for an entity
types::EntityID province_id = 1;
int starting_year = 1066;
double initial_budget = 500.0;

tech_system.InitializeTechnologyComponents(
    province_id,
    starting_year,
    initial_budget
);
```

This creates and initializes all 4 components:
- ResearchComponent with budget distributed across categories
- InnovationComponent with year-adjusted innovation rate
- KnowledgeComponent with basic network setup
- TechnologyEventsComponent with empty history

### Validation

```cpp
// Validate components are properly initialized
if (tech_system.ValidateTechnologyComponents(province_id)) {
    std::cout << "Technology components valid" << std::endl;
}

// Get detailed status
auto status = tech_system.GetTechnologyComponentStatus(province_id);
for (const auto& status_msg : status) {
    std::cout << status_msg << std::endl;
}
```

---

## Technology Research

### Starting Research

```cpp
// Get research component
auto* research = tech_system.GetResearchComponent(province_id);

// Check prerequisites
if (tech_system.CheckTechnologyPrerequisites(province_id,
    TechnologyType::PRINTING_PRESS)) {

    // Set technology to researching
    research->technology_states[TechnologyType::PRINTING_PRESS] =
        ResearchState::RESEARCHING;
    research->research_progress[TechnologyType::PRINTING_PRESS] = 0.0;

    // Focus on this technology for +50% speed
    research->current_focus = TechnologyType::PRINTING_PRESS;
}
```

### Research Progress Calculation

Research progress is calculated automatically during `Update()`:

```cpp
progress_increment = base_progress * efficiency

where:
  base_progress = 0.001 per second (0.1% per second)

  efficiency = base_research_efficiency
             * (1 + literacy_bonus)
             * (1 + trade_network_bonus)
             * (1 + stability_bonus)
             * (1 + focus_bonus if focused)
             * budget_factor

  budget_factor = min(1.0, monthly_research_budget / 100.0)
```

### Checking Research State

```cpp
auto state = research->technology_states[TechnologyType::PRINTING_PRESS];

switch (state) {
    case ResearchState::UNKNOWN:
        // Not yet available
        break;
    case ResearchState::AVAILABLE:
        // Can start researching
        break;
    case ResearchState::RESEARCHING:
        // Currently being researched
        double progress = research->research_progress[TechnologyType::PRINTING_PRESS];
        std::cout << "Progress: " << (progress * 100) << "%" << std::endl;
        break;
    case ResearchState::DISCOVERED:
        // Ready to implement
        break;
    case ResearchState::IMPLEMENTING:
        // Being implemented
        double impl_level = research->implementation_level[TechnologyType::PRINTING_PRESS];
        std::cout << "Implementation: " << (impl_level * 100) << "%" << std::endl;
        break;
    case ResearchState::IMPLEMENTED:
        // Fully active and providing benefits
        break;
}
```

---

## Innovation and Breakthroughs

### Innovation Mechanics

Innovation provides an alternative path to technology discovery, independent of focused research.

```cpp
// Calculate innovation chance (per update)
innovation_chance = innovation_rate * delta_time / 100.0
                  * cultural_openness
                  * innovation_encouragement
                  * (1.0 - guild_resistance)
                  * (1.0 - religious_restriction)
                  * (1.0 + royal_patronage + merchant_funding)
```

### Breakthroughs

When innovation succeeds, there's a chance for a breakthrough:

```cpp
if (random < breakthrough_chance) {
    // Major breakthrough occurred
    // Accelerates research on active technologies
    // Increases innovation_rate temporarily
    // Boosts scholarly_recognition
}
```

### Configuring Innovation Environment

```cpp
auto* innovation = tech_system.GetInnovationComponent(province_id);

// Create favorable innovation environment
innovation->cultural_openness = 0.9;          // Very open society
innovation->innovation_encouragement = 0.8;   // Strong government support
innovation->guild_resistance = 0.1;           // Minimal guild opposition
innovation->religious_restriction = 0.0;      // No religious limits

// This creates: ~7.8x multiplier to base innovation rate
```

---

## Knowledge Transmission

### Knowledge Networks

Knowledge spreads automatically between connected entities:

```cpp
// Create knowledge connection
auto* knowledge = tech_system.GetKnowledgeComponent(province_id);
types::EntityID neighbor = 2;

knowledge->knowledge_connections[neighbor] = 0.8;  // Strong connection (0.0 - 1.0)
```

### Transmission Calculation

```cpp
transmission_amount = knowledge_transmission_rate
                    * connection_strength
                    * delta_time / 30.0  // Monthly rate

for each technology:
  knowledge_gap = our_knowledge - target_knowledge

  if (knowledge_gap > 0):
    transfer = knowledge_gap * transmission_amount
             * target.cultural_knowledge_absorption

    target_knowledge += transfer * target.foreign_knowledge_acceptance
```

### Knowledge Decay

Without proper preservation, knowledge can be lost:

```cpp
// Monthly knowledge decay
decay_amount = knowledge_loss_rate * delta_time / 30.0

for each technology:
  knowledge_level *= (1.0 - decay_amount)

  // High preservation quality prevents excessive loss
  if (knowledge_preservation_quality > 0.7):
    knowledge_level = max(knowledge_level, 0.5)  // Keep at least 50%
```

### Manuscript System

```cpp
// Monthly manuscript production
manuscripts_produced = book_production_capacity
                     * scribes
                     * delta_time / 30.0

manuscripts += manuscripts_produced

// Annual manuscript decay
manuscript_decay = manuscripts
                 * (1.0 - manuscript_durability)
                 * delta_time / 365.0

manuscripts -= manuscript_decay
```

---

## Technology Effects

### Effect Application

Technologies provide bonuses to various game systems when implemented:

```cpp
#include "game/technology/TechnologyEffectApplicator.h"

TechnologyEffectApplicator applicator;

// Apply effects (called automatically when technology is implemented)
auto result = applicator.ApplyTechnologyEffects(
    entity_manager,
    message_bus,
    province_id,
    TechnologyType::PRINTING_PRESS,
    1.0  // Full implementation level
);

if (result.success) {
    std::cout << result.message << std::endl;
    std::cout << "Applied effects:" << std::endl;
    for (const auto& effect : result.applied_effects) {
        std::cout << "  " << effect << std::endl;
    }
}
```

### Effect Types

**Economic Effects**:
- `PRODUCTION_BONUS`: Increases resource production
- `TRADE_EFFICIENCY`: Improves trade income
- `TAX_EFFICIENCY`: Improves tax collection
- `FOOD_PRODUCTION`: Increases food output
- `INFRASTRUCTURE_QUALITY`: Improves infrastructure
- `BUILDING_COST_REDUCTION`: Reduces construction costs
- `MARKET_ACCESS`: Improves trade access

**Military Effects**:
- `MILITARY_STRENGTH`: Increases unit attack strength
- `MILITARY_DEFENSE`: Increases unit defense
- `MILITARY_MAINTENANCE`: Reduces upkeep costs
- `FORTIFICATION_STRENGTH`: Improves fort defense
- `NAVAL_STRENGTH`: Increases naval unit power
- `UNIT_COST_REDUCTION`: Reduces recruitment costs

**Technology Effects**:
- `RESEARCH_SPEED`: Increases research efficiency
- `INNOVATION_RATE`: Boosts innovation chance
- `KNOWLEDGE_TRANSMISSION`: Improves knowledge spread

**Population Effects**:
- `POPULATION_GROWTH`: Increases growth rate
- `HEALTH_IMPROVEMENT`: Reduces mortality
- `EDUCATION_QUALITY`: Increases literacy

**Administrative/Diplomatic Effects**:
- `ADMINISTRATIVE_CAPACITY`: Increases management capacity
- `DIPLOMATIC_REPUTATION`: Improves diplomatic standing

### Calculating Total Effects

```cpp
// Get sum of all active technology effects
auto total_effects = applicator.CalculateTotalEffects(
    entity_manager,
    province_id
);

// Check specific effect
double total_production_bonus = total_effects[EffectType::PRODUCTION_BONUS];
std::cout << "Total production bonus: +"
          << (total_production_bonus * 100) << "%" << std::endl;

// Get formatted summary
std::string summary = applicator.GetEffectSummary(entity_manager, province_id);
std::cout << summary << std::endl;
```

---

## Prerequisites System

### Checking Prerequisites

```cpp
#include "game/technology/TechnologyPrerequisites.h"

// Get prerequisites for a technology
auto prereqs = TechnologyPrerequisites::GetPrerequisites(
    TechnologyType::PLATE_ARMOR
);
// Returns: { CHAINMAIL_ARMOR, BLAST_FURNACE }

// Check if technology has prerequisites
bool has_prereqs = TechnologyPrerequisites::HasPrerequisites(
    TechnologyType::PRINTING_PRESS
);

// Check if all prerequisites are met
bool can_research = tech_system.CheckTechnologyPrerequisites(
    province_id,
    TechnologyType::PLATE_ARMOR
);

// Get missing prerequisites
auto missing = tech_system.GetMissingPrerequisites(
    province_id,
    TechnologyType::PLATE_ARMOR
);

for (auto tech : missing) {
    std::cout << "Missing: " << static_cast<int>(tech) << std::endl;
}
```

### Prerequisite Examples

```cpp
// Simple prerequisite
HORSE_COLLAR requires HEAVY_PLOW

// Multiple prerequisites
PLATE_ARMOR requires CHAINMAIL_ARMOR + BLAST_FURNACE
OCEAN_NAVIGATION requires COMPASS_NAVIGATION + NAVIGATION_INSTRUMENTS

// Technology chains
GUNPOWDER → CANNONS → STAR_FORTRESS
GUNPOWDER → ARQUEBUS → MUSKET
```

### Finding Unlocked Technologies

```cpp
// What technologies does this unlock?
auto unlocked = TechnologyPrerequisites::GetUnlockedTechnologies(
    TechnologyType::GUNPOWDER
);
// Returns technologies that have GUNPOWDER as a prerequisite
```

---

## Technology Data

### Technology Categories

```cpp
enum class TechnologyCategory {
    AGRICULTURAL = 0,   // Farming, food production
    MILITARY,           // Warfare, weapons, armor
    CRAFT,              // Crafts, manufacturing, tools
    ADMINISTRATIVE,     // Government, bureaucracy
    ACADEMIC,           // Science, education, research
    NAVAL,              // Ships, navigation, sea trade
    COUNT
};
```

### Technology List

**Agricultural (1001-1010)**:
- THREE_FIELD_SYSTEM, HEAVY_PLOW, HORSE_COLLAR
- WINDMILL, WATERMILL, CROP_ROTATION
- SELECTIVE_BREEDING, AGRICULTURAL_MANUAL
- IRRIGATION_SYSTEMS, NEW_WORLD_CROPS

**Military (1101-1110)**:
- CHAINMAIL_ARMOR, PLATE_ARMOR, CROSSBOW, LONGBOW
- GUNPOWDER, CANNONS, ARQUEBUS, MUSKET
- STAR_FORTRESS, MILITARY_ENGINEERING

**Craft (1201-1210)**:
- BLAST_FURNACE, WATER_POWERED_MACHINERY
- MECHANICAL_CLOCK, PRINTING_PRESS
- DOUBLE_ENTRY_BOOKKEEPING, PAPER_MAKING
- GLASS_MAKING, TEXTILE_MACHINERY
- ADVANCED_METALLURGY, PRECISION_INSTRUMENTS

**Administrative (1301-1310)**:
- WRITTEN_LAW_CODES, BUREAUCRATIC_ADMINISTRATION
- CENSUS_TECHNIQUES, TAX_COLLECTION_SYSTEMS
- DIPLOMATIC_PROTOCOLS, RECORD_KEEPING
- STANDARDIZED_WEIGHTS, POSTAL_SYSTEMS
- PROFESSIONAL_ARMY, STATE_MONOPOLIES

**Academic (1401-1410)**:
- SCHOLASTIC_METHOD, UNIVERSITY_SYSTEM
- VERNACULAR_WRITING, NATURAL_PHILOSOPHY
- MATHEMATICAL_NOTATION, EXPERIMENTAL_METHOD
- HUMANIST_EDUCATION, SCIENTIFIC_INSTRUMENTS
- OPTICAL_DEVICES, CARTOGRAPHY

**Naval (1501-1510)**:
- IMPROVED_SHIP_DESIGN, NAVIGATION_INSTRUMENTS
- COMPASS_NAVIGATION, NAVAL_ARTILLERY
- OCEAN_NAVIGATION, SHIPYARD_TECHNIQUES
- MARITIME_LAW, NAVAL_TACTICS
- LIGHTHOUSE_SYSTEMS, HARBOR_ENGINEERING

### Loading Technology Data

Technology data is loaded from JSON files:
- `data/definitions/technologies.json`: All technology definitions
- Contains historical data, costs, effects, prerequisites

---

## Integration Patterns

### Economy Integration

The TechnologyEconomicBridge provides bidirectional integration:

```cpp
#include "game/economy/TechnologyEconomicBridge.h"

TechnologyEconomicBridge bridge;
bridge.SetTechnologySystem(&tech_system);
bridge.SetEconomicSystem(&economic_system);
bridge.Initialize();

// Technology affects economy
auto tech_effects = bridge.CalculateTechnologyEffects(province_id);
bridge.ApplyTechnologyEffectsToEconomy(province_id, tech_effects);

// Economy affects technology
auto econ_contributions = bridge.CalculateEconomicContributions(province_id);
bridge.ApplyEconomicContributionsToTechnology(province_id, econ_contributions);

// Crisis detection
bridge.ProcessCrisisDetection(province_id);
```

### UI Integration

```cpp
#include "ui/TechnologyInfoWindow.h"

TechnologyInfoWindow tech_window(tech_system);

// Render technology window
tech_window.Render(province_id);
// Shows 4 tabs: Research, Tech Tree, Innovation, Knowledge Network
```

### Message Bus Integration

Subscribe to technology events:

```cpp
message_bus.Subscribe<TechnologyDiscoveryEvent>(
    [](const TechnologyDiscoveryEvent& event) {
        std::cout << "Technology discovered: "
                  << static_cast<int>(event.technology) << std::endl;
    }
);

message_bus.Subscribe<ResearchBreakthroughEvent>(
    [](const ResearchBreakthroughEvent& event) {
        std::cout << "Research breakthrough with magnitude: "
                  << event.breakthrough_magnitude << std::endl;
    }
);
```

---

## Examples

### Example 1: Basic Research Setup

```cpp
// Initialize province technology
types::EntityID province = 1;
tech_system.InitializeTechnologyComponents(province, 1066, 500.0);

// Set up research infrastructure
auto* research = tech_system.GetResearchComponent(province);
research->universities = 2;
research->libraries = 5;
research->scholar_population = 100;

// Configure budget
research->monthly_research_budget = 500.0;
research->category_investment[TechnologyCategory::CRAFT] = 300.0;
research->category_investment[TechnologyCategory::ACADEMIC] = 200.0;

// Start researching printing press
if (tech_system.CheckTechnologyPrerequisites(province, TechnologyType::PRINTING_PRESS)) {
    research->technology_states[TechnologyType::PRINTING_PRESS] = ResearchState::RESEARCHING;
    research->current_focus = TechnologyType::PRINTING_PRESS;
}
```

### Example 2: Knowledge Network

```cpp
// Province 1 and Province 2 share knowledge
auto* knowledge1 = tech_system.GetKnowledgeComponent(1);
auto* knowledge2 = tech_system.GetKnowledgeComponent(2);

// Establish bidirectional connection
knowledge1->knowledge_connections[2] = 0.7;  // Strong link
knowledge2->knowledge_connections[1] = 0.7;

// Set up knowledge infrastructure
knowledge1->manuscripts = 300;
knowledge1->scribes = 30;
knowledge1->knowledge_transmission_rate = 0.5;

// Cultural openness
knowledge2->cultural_knowledge_absorption = 0.8;
knowledge2->foreign_knowledge_acceptance = 0.9;

// Knowledge will now flow from Province 1 to Province 2
```

### Example 3: Innovation-Focused Strategy

```cpp
// Create an innovation-focused civilization
auto* innovation = tech_system.GetInnovationComponent(province);

// Maximize innovation environment
innovation->cultural_openness = 1.0;          // Completely open
innovation->innovation_encouragement = 1.0;   // Maximum support
innovation->guild_resistance = 0.0;           // No guilds
innovation->religious_restriction = 0.0;      // No restrictions
innovation->royal_patronage = 0.5;            // 50% royal funding
innovation->merchant_funding = 0.5;           // 50% merchant funding

// Build innovator population
innovation->inventors = 100;
innovation->craftsmen = 500;
innovation->scholars = 200;

// This creates maximum innovation potential
```

### Example 4: Checking Technology Benefits

```cpp
// What benefits does printing press provide?
auto effects = TechnologyEffectsDatabase::GetEffects(TechnologyType::PRINTING_PRESS);

for (const auto& effect : effects) {
    std::cout << effect.description
              << " (affects " << effect.affected_system << ")"
              << std::endl;
}
// Output:
// +50% knowledge dissemination (affects technology)
// +60% knowledge spread rate (affects technology)
// +30% innovation from information (affects technology)
// +15% cultural influence (affects diplomacy)
```

---

## Best Practices

### 1. Budget Management

```cpp
// Don't spread budget too thin
// Focus on 2-3 categories maximum

// Good:
research->category_investment[TechnologyCategory::CRAFT] = 400.0;
research->category_investment[TechnologyCategory::ACADEMIC] = 300.0;
research->category_investment[TechnologyCategory::ADMINISTRATIVE] = 200.0;

// Bad (too spread out):
for (int i = 0; i < static_cast<int>(TechnologyCategory::COUNT); ++i) {
    research->category_investment[static_cast<TechnologyCategory>(i)] = 100.0;
}
```

### 2. Research Focus

```cpp
// Always use research focus for +50% speed on your primary goal
research->current_focus = TechnologyType::PRINTING_PRESS;

// Change focus when priorities change
if (at_war) {
    research->current_focus = TechnologyType::GUNPOWDER;
}
```

### 3. Infrastructure Investment

```cpp
// Build infrastructure early - it compounds
// 1 university = +10% research efficiency
research->universities = 5;     // +50% total

// Don't neglect preservation
knowledge->knowledge_preservation_quality = 0.8;  // Prevents knowledge loss
```

### 4. Knowledge Networks

```cpp
// Connect to advanced neighbors
knowledge->knowledge_connections[advanced_neighbor] = 0.9;  // Strong link

// Be culturally open
knowledge->cultural_knowledge_absorption = 0.8;
knowledge->foreign_knowledge_acceptance = 0.9;

// This lets you learn faster from others
```

### 5. Innovation vs. Research

```cpp
// Use both paths for maximum speed:
// 1. Focused research on specific technologies
research->current_focus = TechnologyType::PRINTING_PRESS;

// 2. High innovation for random discoveries
innovation->cultural_openness = 0.9;
innovation->innovation_encouragement = 0.8;

// Research is predictable, innovation is opportunistic
```

### 6. Validation

```cpp
// Always validate after major changes
if (!tech_system.ValidateTechnologyComponents(province)) {
    // Something went wrong - check component status
    auto status = tech_system.GetTechnologyComponentStatus(province);
    for (const auto& msg : status) {
        std::cerr << msg << std::endl;
    }
}
```

### 7. Performance

```cpp
// The system updates at 1 Hz - don't query too frequently
// Cache component pointers within update loops

// Good:
auto* research = tech_system.GetResearchComponent(province);
for (auto& [tech, state] : research->technology_states) {
    // Use research pointer
}

// Bad:
for (auto& [tech, state] : tech_system.GetResearchComponent(province)->technology_states) {
    // Calls GetResearchComponent() every iteration
}
```

---

## Threading Considerations

**The technology system uses MAIN_THREAD strategy:**

```cpp
// ✅ Good: Single-threaded access
tech_system.Update(delta_time);

// ❌ Bad: Don't access from multiple threads
std::thread t1([&]() { tech_system.Update(delta_time); });
std::thread t2([&]() { tech_system.Update(delta_time); });

// ❌ Bad: Don't share component pointers between threads
auto* research = tech_system.GetResearchComponent(province);
std::thread t([&]() { research->monthly_research_budget = 1000.0; });
```

**Why MAIN_THREAD?**
- System uses raw component pointers for performance
- Sequential access prevents use-after-free issues
- 1 Hz update frequency makes single-threaded acceptable

---

## Error Handling

### Common Issues

**1. Missing Prerequisites**
```cpp
if (!tech_system.CheckTechnologyPrerequisites(province, tech)) {
    auto missing = tech_system.GetMissingPrerequisites(province, tech);
    // Research missing prerequisites first
}
```

**2. Insufficient Budget**
```cpp
auto* research = tech_system.GetResearchComponent(province);
if (research->monthly_research_budget < 100.0) {
    // Research will be very slow
    // Consider increasing budget
}
```

**3. Component Not Found**
```cpp
auto* research = tech_system.GetResearchComponent(province);
if (!research) {
    // Entity not initialized
    tech_system.InitializeTechnologyComponents(province);
}
```

**4. Invalid Component State**
```cpp
if (!tech_system.ValidateTechnologyComponents(province)) {
    // Components have invalid values
    // Check status for details
    auto status = tech_system.GetTechnologyComponentStatus(province);
}
```

---

## See Also

- [Technology System Validation Report](../TECHNOLOGY_SYSTEM_VALIDATION_REPORT.md)
- [Technology System Test Report](../tests/phase3/system-006-technology-test-report.md)
- [Economy Integration](./API_ECONOMY_SYSTEM.md)
- [ECS Architecture](./API_ECS_ARCHITECTURE.md)

---

**Last Updated**: 2025-11-22
**Version**: 1.0
**Author**: Claude Code
