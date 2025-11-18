# Faction System - Usage Guide

**Version:** 1.0
**Date:** November 18, 2025

This guide explains how to use and integrate the faction system in Mechanica Imperii.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [System Integration](#system-integration)
3. [Basic Usage](#basic-usage)
4. [Advanced Usage](#advanced-usage)
5. [Configuration](#configuration)
6. [Event Handling](#event-handling)
7. [Examples](#examples)

---

## Quick Start

### 1. Add FactionSystem to Your Game

```cpp
#include "game/faction/FactionSystem.h"
#include "game/faction/FactionComponents.h"

// In your game initialization:
auto faction_system = std::make_unique<game::faction::FactionSystem>(
    access_manager,
    message_bus
);

faction_system->Initialize();
```

### 2. Initialize Factions for a Province

```cpp
// For a new province entity:
game::types::EntityID province_id = 1234;

// Create default factions (Nobility, Clergy, Merchants, Military, Burghers, Peasants)
faction_system->InitializeFactions(province_id);
```

### 3. Update Each Frame

```cpp
// In your game loop:
float delta_time = /* your frame time */;
faction_system->Update(delta_time);

// The system will automatically process monthly updates
```

---

## System Integration

### Registering with SystemManager

```cpp
// In your main game setup:
system_manager.RegisterSystem(std::move(faction_system));
```

### Component Access

```cpp
// Get faction component for a province:
auto* factions = access_manager.GetComponent<game::faction::ProvincialFactionsComponent>(province_id);

if (factions) {
    std::cout << "Faction Stability: " << factions->faction_stability << std::endl;
    std::cout << "Revolt Risk: " << factions->revolt_risk << std::endl;
    std::cout << "Dominant Faction: " << faction_system->GetDominantFaction(province_id) << std::endl;
}
```

---

## Basic Usage

### Querying Faction State

```cpp
// Get specific faction's metrics
double influence = faction_system->GetFactionInfluence(province_id, FactionType::NOBILITY);
double loyalty = faction_system->GetFactionLoyalty(province_id, FactionType::CLERGY);
double satisfaction = faction_system->GetFactionSatisfaction(province_id, FactionType::PEASANTS);
double revolt_risk = faction_system->GetRevoltRisk(province_id, FactionType::MILITARY);

// Get angry factions
std::vector<FactionType> angry = faction_system->GetAngryFactions(province_id);
for (auto faction_type : angry) {
    std::string name = TypeRegistry::FactionTypeToString(faction_type);
    std::cout << "WARNING: " << name << " faction is angry!" << std::endl;
}

// Get dominant faction
FactionType dominant = faction_system->GetDominantFaction(province_id);
std::cout << "Dominant faction: " << TypeRegistry::FactionTypeToString(dominant) << std::endl;
```

### Adjusting Faction Metrics

```cpp
// Increase nobility satisfaction due to a royal ceremony
faction_system->AdjustSatisfaction(
    province_id,
    FactionType::NOBILITY,
    0.15,  // +15% satisfaction
    "Royal ceremony held"
);

// Decrease peasant loyalty due to harsh taxation
faction_system->AdjustLoyalty(
    province_id,
    FactionType::PEASANTS,
    -0.10,  // -10% loyalty
    "Heavy taxation"
);

// Increase merchant influence due to trade boom
faction_system->AdjustInfluence(
    province_id,
    FactionType::MERCHANTS,
    0.05,  // +5% influence
    "Trade boom"
);
```

### Managing Demands

```cpp
// Fulfill a faction's demand
faction_system->FulfillDemand(
    province_id,
    FactionType::BURGHERS,
    "self_governance"  // Demand type
);

// Reject a faction's demand
faction_system->RejectDemand(
    province_id,
    FactionType::CLERGY,
    "religious_authority"  // Demand type
);
```

### Handling Revolts

```cpp
// Check if revolt is likely
double risk = faction_system->GetRevoltRisk(province_id, FactionType::PEASANTS);
if (risk > 0.7) {
    std::cout << "WARNING: High revolt risk!" << std::endl;
}

// Manually resolve a revolt (typically called by your conflict resolution system)
bool revolt_succeeded = /* your battle resolution */;
faction_system->ResolveRevolt(
    province_id,
    FactionType::PEASANTS,
    revolt_succeeded
);
```

---

## Advanced Usage

### Managing Coalitions

```cpp
// Form a coalition between factions
faction_system->FormCoalition(
    FactionType::NOBILITY,
    FactionType::MILITARY,
    "Military alliance to defend the realm"
);

// Dissolve a coalition
faction_system->DissolveCoalition(
    FactionType::MERCHANTS,
    FactionType::BURGHERS,
    "Trade dispute"
);

// Check coalition compatibility
double compatibility = faction_system->CalculateCoalitionCompatibility(
    FactionType::CLERGY,
    FactionType::NOBILITY
);
// Returns 0.7 (natural allies)
```

### National-Level Operations

```cpp
// Update national faction metrics (aggregates from all provinces)
game::types::EntityID nation_id = 0;  // Assuming entity 0 is the nation
faction_system->UpdateNationalFactionMetrics(nation_id);

// Process national-level demands
faction_system->ProcessNationalDemands(nation_id);

// Access national component
auto* national_factions = access_manager.GetComponent<game::faction::NationalFactionsComponent>(nation_id);
if (national_factions) {
    // Get national satisfaction for clergy
    double clergy_national_sat = national_factions->national_satisfaction[FactionType::CLERGY];

    // Get most influential faction nationwide
    FactionType most_influential = national_factions->GetMostInfluentialFaction();

    // Get most dissatisfied faction nationwide
    FactionType most_angry = national_factions->GetMostDissatisfiedFaction();
}
```

### Custom Faction Creation

```cpp
// Create a custom faction manually
game::faction::FactionData custom_faction(
    FactionID(999),
    FactionType::INTELLECTUAL_CIRCLE,
    "The Enlightened Society"
);

custom_faction.influence = 0.25;
custom_faction.loyalty = 0.9;
custom_faction.satisfaction = 0.8;
custom_faction.militancy = 0.1;
custom_faction.cohesion = 0.95;
custom_faction.wealth_level = 0.4;

// Add to province
faction_system->AddFaction(province_id, custom_faction);
```

---

## Configuration

### Tuning Faction Parameters

```cpp
// Get current configuration
auto config = faction_system->GetConfiguration();

// Modify parameters
game::faction::FactionSystemConfig new_config = config;

// Make revolts less likely
new_config.revolt_base_chance = 0.02;  // Down from 0.05
new_config.revolt_risk_threshold = 0.8;  // Up from 0.7

// Make demands more frequent
new_config.demand_base_rate = 0.15;  // Up from 0.1
new_config.demand_rate_if_dissatisfied = 0.4;  // Up from 0.3

// Adjust satisfaction decay
new_config.satisfaction_decay_rate = 0.005;  // Down from 0.01

// Apply new configuration
faction_system->SetConfiguration(new_config);
```

### Faction-Specific Tuning

```cpp
// Customize faction defaults
config.faction_militancy[FactionType::PEASANTS] = 0.8;  // More militant peasants
config.faction_influence_base[FactionType::MERCHANTS] = 0.6;  // Stronger merchants
config.faction_cohesion_base[FactionType::NOBILITY] = 0.9;  // More unified nobility

faction_system->SetConfiguration(config);
```

---

## Event Handling

### Subscribing to Faction Events

```cpp
// Subscribe to influence changes
message_bus.Subscribe<game::faction::FactionInfluenceChangeEvent>(
    [](const game::faction::FactionInfluenceChangeEvent& event) {
        std::cout << "Faction " << static_cast<int>(event.faction)
                  << " influence changed from " << event.old_influence
                  << " to " << event.new_influence
                  << " (" << event.reason << ")" << std::endl;
    }
);

// Subscribe to faction demands
message_bus.Subscribe<game::faction::FactionDemandEvent>(
    [](const game::faction::FactionDemandEvent& event) {
        std::cout << "Faction " << static_cast<int>(event.faction)
                  << " demands: " << event.demand_description << std::endl;
        if (event.is_ultimatum) {
            std::cout << "  *** THIS IS AN ULTIMATUM! ***" << std::endl;
        }
    }
);

// Subscribe to revolts
message_bus.Subscribe<game::faction::FactionRevoltEvent>(
    [](const game::faction::FactionRevoltEvent& event) {
        std::cout << "REVOLT! Faction " << static_cast<int>(event.faction)
                  << " has rebelled!" << std::endl;
        std::cout << "  Strength: " << event.revolt_strength << std::endl;
        std::cout << "  Reason: " << event.revolt_reason << std::endl;
    }
);

// Subscribe to satisfaction changes
message_bus.Subscribe<game::faction::FactionSatisfactionChangeEvent>(
    [](const game::faction::FactionSatisfactionChangeEvent& event) {
        double change = event.new_satisfaction - event.old_satisfaction;
        if (change < -0.1) {
            std::cout << "Major dissatisfaction in faction "
                      << static_cast<int>(event.faction) << std::endl;
        }
    }
);
```

### Triggering System Events

```cpp
// Notify faction system of game events
faction_system->HandleEconomicChange(province_id, 150.0);  // Economic growth
faction_system->HandleMilitaryEvent(province_id, true);    // Military victory
faction_system->HandleAdministrativeEvent("reform", province_id);  // Administrative reform
faction_system->HandlePolicyChange(province_id, "tax_increase");  // Tax policy change
```

---

## Examples

### Example 1: Player Makes a Decision

```cpp
void OnPlayerDecision(DecisionType decision, EntityID province_id) {
    switch (decision) {
        case DecisionType::ECONOMIC_TAX_RATE:
            // Increased taxes
            faction_system->HandlePolicyChange(province_id, "tax_increase");
            break;

        case DecisionType::ADMIN_BUREAUCRACY_REFORM:
            // Administrative reform
            faction_system->HandleAdministrativeEvent("reform", province_id);
            faction_system->AdjustSatisfaction(province_id, FactionType::BUREAUCRATS, 0.15, "Reform enacted");
            break;

        case DecisionType::POPULATION_RELIGIOUS_TOLERANCE:
            // Religious tolerance policy
            faction_system->HandlePolicyChange(province_id, "religious_tolerance");
            break;
    }
}
```

### Example 2: Checking for Political Instability

```cpp
bool IsProvinceUnstable(EntityID province_id, FactionSystem& faction_system) {
    auto* factions = access_manager.GetComponent<ProvincialFactionsComponent>(province_id);
    if (!factions) return false;

    // Check multiple stability indicators
    bool high_revolt_risk = factions->revolt_risk > 0.6;
    bool low_stability = factions->faction_stability < 0.4;
    bool many_angry_factions = faction_system->GetAngryFactions(province_id).size() >= 3;
    bool low_loyalty = factions->average_faction_loyalty < 0.3;

    return high_revolt_risk || low_stability || many_angry_factions || low_loyalty;
}

void CheckAllProvinces() {
    auto provinces = access_manager.GetAllEntitiesWithComponent<ProvincialFactionsComponent>();

    for (auto province_id : provinces) {
        if (IsProvinceUnstable(province_id, *faction_system)) {
            std::cout << "Province " << province_id << " is UNSTABLE!" << std::endl;

            // Get specific problems
            auto angry = faction_system->GetAngryFactions(province_id);
            for (auto faction : angry) {
                std::cout << "  - Angry faction: "
                          << TypeRegistry::FactionTypeToString(faction) << std::endl;
            }
        }
    }
}
```

### Example 3: AI Decision Making Based on Factions

```cpp
void AIConsiderDemands(EntityID province_id, FactionSystem& faction_system) {
    auto* demands_comp = access_manager.GetComponent<FactionDemandsComponent>(province_id);
    if (!demands_comp || demands_comp->pending_demands.empty()) {
        return;  // No demands to consider
    }

    for (size_t i = 0; i < demands_comp->pending_demands.size(); ++i) {
        const auto& demand = demands_comp->pending_demands[i];

        // Get faction making the demand
        double influence = faction_system->GetFactionInfluence(province_id, demand.faction);
        double loyalty = faction_system->GetFactionLoyalty(province_id, demand.faction);
        double revolt_risk = faction_system->GetRevoltRisk(province_id, demand.faction);

        // AI decision logic
        bool should_fulfill = false;

        if (demand.is_ultimatum) {
            // Always consider ultimatums seriously
            should_fulfill = (revolt_risk > 0.7) || (influence > 0.5);
        } else if (demand.urgency > 0.7) {
            // Urgent demands from powerful factions
            should_fulfill = (influence > 0.4);
        } else {
            // Normal demands - balance influence vs loyalty
            should_fulfill = (influence > 0.6 && loyalty < 0.4);
        }

        if (should_fulfill) {
            faction_system->FulfillDemand(province_id, demand.faction, demand.demand_type);
            std::cout << "AI fulfilled demand: " << demand.demand_description << std::endl;
        } else {
            faction_system->RejectDemand(province_id, demand.faction, demand.demand_type);
            std::cout << "AI rejected demand: " << demand.demand_description << std::endl;
        }
    }
}
```

### Example 4: UI Display

```cpp
void DisplayFactionStatus(EntityID province_id) {
    auto* factions = access_manager.GetComponent<ProvincialFactionsComponent>(province_id);
    if (!factions) return;

    std::cout << "=== PROVINCE FACTION STATUS ===" << std::endl;
    std::cout << "Stability: " << (factions->faction_stability * 100) << "%" << std::endl;
    std::cout << "Political Tension: " << (factions->political_tension * 100) << "%" << std::endl;
    std::cout << "Revolt Risk: " << (factions->revolt_risk * 100) << "%" << std::endl;
    std::cout << std::endl;

    for (const auto& faction : factions->factions) {
        std::cout << "--- " << faction.faction_name << " ---" << std::endl;
        std::cout << "  Influence:    " << (faction.influence * 100) << "%" << std::endl;
        std::cout << "  Loyalty:      " << (faction.loyalty * 100) << "%" << std::endl;
        std::cout << "  Satisfaction: " << (faction.satisfaction * 100) << "%" << std::endl;
        std::cout << "  Power:        " << (faction.GetEffectivePower() * 100) << "%" << std::endl;
        std::cout << "  Revolt Risk:  " << (faction.GetRevoltRisk() * 100) << "%" << std::endl;

        if (faction.IsAngry()) {
            std::cout << "  ⚠️  STATUS: ANGRY" << std::endl;
        } else if (faction.IsContent()) {
            std::cout << "  ✓  STATUS: CONTENT" << std::endl;
        }

        if (!faction.active_demands.empty()) {
            std::cout << "  Active Demands:" << std::endl;
            for (const auto& demand : faction.active_demands) {
                std::cout << "    - " << demand << std::endl;
            }
        }
        std::cout << std::endl;
    }
}
```

### Example 5: Save/Load Integration

```cpp
// Serialization
Json::Value SaveFactionSystem(const FactionSystem& system) {
    return system.Serialize(1);  // Version 1
}

// Deserialization
void LoadFactionSystem(FactionSystem& system, const Json::Value& data) {
    system.Deserialize(data, 1);
}

// Component serialization (handled by EntityManager)
// The FactionData::ToJson() and FromJson() methods are automatically used
```

---

## Best Practices

### 1. **Always Check for Null Components**
```cpp
auto* factions = access_manager.GetComponent<ProvincialFactionsComponent>(province_id);
if (!factions) {
    // Handle missing component
    return;
}
// Safe to use factions
```

### 2. **Use Event-Driven Updates**
```cpp
// Good: Use event handlers
faction_system->HandleEconomicChange(province_id, economic_delta);

// Avoid: Directly modifying faction state
// factions->faction[0].satisfaction = 0.9;  // BAD!
```

### 3. **Monitor Angry Factions**
```cpp
// Check every game year
if (current_year != last_check_year) {
    auto angry = faction_system->GetAngryFactions(province_id);
    if (!angry.empty()) {
        // Take action to appease factions
    }
    last_check_year = current_year;
}
```

### 4. **Balance Concessions and Rejections**
```cpp
// Track fulfillment ratio
int demands_fulfilled = 0;
int demands_rejected = 0;

// Try to maintain ~60% fulfillment rate
if (demands_fulfilled / (demands_fulfilled + demands_rejected) < 0.6) {
    // Fulfill next demand
} else {
    // Can afford to reject
}
```

### 5. **Use Configuration for Game Balance**
```cpp
// Easy mode: less revolt risk
auto easy_config = faction_system->GetConfiguration();
easy_config.revolt_base_chance = 0.01;
easy_config.revolt_risk_threshold = 0.9;

// Hard mode: more volatile factions
auto hard_config = faction_system->GetConfiguration();
hard_config.revolt_base_chance = 0.10;
hard_config.revolt_risk_threshold = 0.5;
hard_config.satisfaction_decay_rate = 0.02;
```

---

## Troubleshooting

### Factions Not Updating
- Ensure `FactionSystem::Update()` is called each frame
- Check that `monthly_update_interval` hasn't been set too high
- Verify components are properly initialized

### Unexpected Revolts
- Check `revolt_risk_threshold` and `revolt_base_chance` configuration
- Monitor satisfaction and loyalty levels
- Review recent demand rejections

### Coalitions Not Forming
- Check `coalition_formation_threshold` (default: 0.6)
- Verify compatibility matrix in `CalculateCoalitionCompatibility()`
- Ensure `NationalFactionsComponent` exists on nation entity

### Memory Issues
- Limit faction event history sizes (configurable in components)
- Clean up old demands periodically
- Use entity pooling for provinces

---

## Performance Tips

1. **Batch Provincial Updates**: The system already does this monthly
2. **Limit Active Factions**: Default 6 per province is optimal
3. **Event Filtering**: Only subscribe to events you need
4. **Cached Queries**: Cache frequently accessed faction data
5. **Lazy Recalculation**: Metrics only recalculate when state changes

---

## Integration Checklist

- [ ] FactionSystem added to SystemManager
- [ ] FactionSystem::Initialize() called
- [ ] FactionSystem::Update() called each frame
- [ ] Factions initialized for all provinces
- [ ] Event subscriptions configured
- [ ] Event handlers connected to game systems
- [ ] UI updated to display faction information
- [ ] AI updated to consider faction demands
- [ ] Save/load system handles faction components
- [ ] Configuration tuned for game balance
- [ ] Testing completed for edge cases

---

**Next Steps:**
- See [FACTION_SYSTEM_VALIDATION.md](./FACTION_SYSTEM_VALIDATION.md) for validation report
- See design docs for faction system architecture
- See `FactionComponents.h` for complete component reference
- See `FactionSystem.h` for complete API reference

---

**Document Version:** 1.0
**Last Updated:** November 18, 2025
**Author:** Claude (AI Assistant)
