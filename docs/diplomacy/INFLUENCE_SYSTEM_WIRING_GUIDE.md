# InfluenceSystem Integration Wiring Guide

**Created**: November 15, 2025
**Status**: Complete - Integration wired and ready for use
**Phase**: Phase 3 - Sphere of Influence System

---

## Overview

This guide explains how to wire the InfluenceSystem with Character, Religion, and Province systems to enable full integration. After following this guide, the influence system will use:

- **Actual marriages** for dynastic influence (not placeholders)
- **Real friendships** for personal influence
- **Faith compatibility** for religious influence
- **Province borders** for geographic neighbor detection

---

## Architecture

### Integration Flow

```
Game Initialization
        ↓
1. Create Systems (Province, Religion, Character)
        ↓
2. Create Components (CharacterRelationships, ReligionComponents, etc.)
        ↓
3. Create InfluenceSystem
        ↓
4. Enable Integration (influence_system.EnableIntegration())
        ↓
5. Register System Managers (SetProvinceAdjacencyManager, SetReligionSystemData)
        ↓
6. Register Components as they're created
        ↓
7. Use InfluenceSystem normally
```

### Component Registration Pattern

```cpp
// When creating a character
CharacterRelationshipsComponent* relationships = CreateCharacterRelationships(char_id);
influence_system.RegisterCharacterRelationships(char_id, relationships);

// When destroying a character
influence_system.UnregisterCharacterRelationships(char_id);
```

---

## Step-by-Step Wiring Instructions

### Step 1: Include Headers

```cpp
#include "game/diplomacy/InfluenceSystem.h"
#include "game/character/CharacterRelationships.h"
#include "game/religion/ReligionComponents.h"
#include "game/province/ProvinceAdjacency.h"
```

### Step 2: Create Core Systems

Create the systems that provide data to the influence system:

```cpp
// Create province adjacency manager
province::ProvinceAdjacencyManager adjacency_manager;

// Register all provinces
for (auto province_id : all_provinces) {
    adjacency_manager.RegisterProvince(province_id);
}

// Add adjacencies (borders between provinces)
for (auto& [prov1, prov2, border_type] : province_borders) {
    adjacency_manager.AddAdjacency(prov1, prov2, border_type);
}

// Set ownership
for (auto& [province_id, realm_id] : province_ownership) {
    adjacency_manager.UpdateProvinceOwnership(province_id, realm_id);
}
```

```cpp
// Create religion system
religion::ReligionSystemData religion_data;
religion_data.InitializeDefaultFaiths();

// Register custom faiths if needed
auto custom_faith_id = religion_data.RegisterFaith("Custom Faith",
    religion::ReligionGroup::CUSTOM, "denomination_name");
```

### Step 3: Create InfluenceSystem and Enable Integration

```cpp
// Create influence system
InfluenceSystem influence_system;

// Enable integration (creates integration helper)
influence_system.EnableIntegration();

// Register system-level managers
influence_system.SetProvinceAdjacencyManager(&adjacency_manager);
influence_system.SetReligionSystemData(&religion_data);
```

### Step 4: Register Components as They're Created

When you create character, religion, or province components, register them:

#### Character Relationships

```cpp
// When creating a character
CharacterRelationshipsComponent* char_rel = new CharacterRelationshipsComponent(char_id);

// Add marriages, friendships, etc.
char_rel->AddMarriage(spouse_id, spouse_realm, spouse_dynasty, true /* alliance */);
char_rel->SetRelationship(friend_id, RelationshipType::FRIEND, 50 /* opinion */, 80.0 /* bond */);

// Register with influence system
influence_system.RegisterCharacterRelationships(char_id, char_rel);
```

#### Character Religion

```cpp
// When creating a character's religion component
CharacterReligionComponent* char_religion = new CharacterReligionComponent(char_id, faith_id);
char_religion->piety = 70.0;
char_religion->devotion = 60.0;

// Register with influence system
influence_system.RegisterCharacterReligion(char_id, char_religion);
```

#### Realm Religion

```cpp
// When creating a realm's religion component
RealmReligionComponent* realm_religion = new RealmReligionComponent(realm_id, state_faith);
realm_religion->tolerance = 50.0;
realm_religion->clergy_loyalty = 70.0;

// Register with influence system
influence_system.RegisterRealmReligion(realm_id, realm_religion);
```

### Step 5: Unregister Components When Destroyed

```cpp
// When a character is destroyed
influence_system.UnregisterCharacterRelationships(char_id);
influence_system.UnregisterCharacterReligion(char_id);

// When a realm is destroyed
influence_system.UnregisterRealmReligion(realm_id);
```

### Step 6: Verify Integration

```cpp
if (influence_system.IsIntegrationEnabled()) {
    // Integration is active - influence calculations will use real data
    std::cout << "Integration enabled!\n";
} else {
    // Missing components - still using placeholder logic
    std::cout << "Warning: Integration not fully enabled\n";
}
```

### Step 7: Use InfluenceSystem Normally

```cpp
// Initialize the system
influence_system.Initialize();

// Monthly updates
influence_system.MonthlyUpdate();

// Query influence
auto sphere = influence_system.GetSphereOfInfluence(realm_id);
auto autonomy = influence_system.GetRealmAutonomy(realm_id);
```

---

## Integration Points in Influence Calculations

The integration affects these calculations:

### 1. CalculateInfluenceBetween() - Dynastic Influence

**Before Integration**:
```cpp
// Placeholder: generic heir-based bonus
if (source_realm.heir != 0 && target_realm.heir != 0) {
    return 10.0;
}
```

**After Integration**:
```cpp
// Uses actual marriage ties from CharacterRelationshipsComponent
auto source_rel = GetCharacterRelationships(source_ruler);
auto target_rel = GetCharacterRelationships(target_ruler);

// Check for direct marriage
if (source_rel->IsMarriedTo(target_ruler)) {
    marriage_strength += 30.0;  // Direct marriage = strong tie
}

// Check for marriage to realm members
for (auto& marriage : source_rel->marriages) {
    if (marriage.realm_of_spouse == target_realm_id) {
        marriage_strength += 15.0;
        if (marriage.is_alliance) marriage_strength += 10.0;
    }
}

// Check family connections
if (source_rel->IsSiblingOf(target_ruler)) {
    marriage_strength += 20.0;
}
```

### 2. CalculateInfluenceBetween() - Personal Influence

**Before Integration**:
```cpp
// Only uses opinion from diplomacy system
double opinion_influence = (opinion + 100.0) / 200.0 * 50.0;
```

**After Integration**:
```cpp
// Uses friendship bonds and relationships
auto source_rel = GetCharacterRelationships(source_ruler);

if (source_rel->IsFriendsWith(target_ruler)) {
    double bond = source_rel->GetFriendshipBondStrength(target_ruler);
    total += (bond / 100.0) * 40.0;  // Scale bond to influence
}

auto relationship = source_rel->GetRelationship(target_ruler);
if (relationship) {
    // Opinion modifier
    total += ((relationship->opinion + 100.0) / 200.0) * 30.0;

    // Special relationship types
    switch (relationship->type) {
        case RelationshipType::FRIEND: total += 10.0; break;
        case RelationshipType::BLOOD_BROTHER: total += 20.0; break;
        case RelationshipType::RIVAL: total -= 15.0; break;
    }
}
```

### 3. CalculateInfluenceBetween() - Religious Influence

**Before Integration**:
```cpp
// Placeholder: fixed bonus if same religion
if (source.religion == target.religion) {
    return 20.0;
}
```

**After Integration**:
```cpp
// Uses ReligionSystemData for faith compatibility
auto source_ruler_religion = GetCharacterReligion(source_ruler);
auto target_realm_religion = GetRealmReligion(target_realm);

// Religious authority of source ruler
double authority = source_ruler_religion->GetReligiousAuthority();
total += (authority / 100.0) * 40.0;

// Faith compatibility bonuses
if (religion_data->AreSameFaith(source_faith, target_faith)) {
    total += 40.0;  // Same faith
} else if (religion_data->AreSameDenomination(source_faith, target_faith)) {
    total += 25.0;  // Same denomination
} else if (religion_data->AreSameReligionGroup(source_faith, target_faith)) {
    total += 10.0;  // Same religion group
}

// Bonus for controlling holy sites
total += source_realm_religion->owned_holy_sites.size() * 3.0;

// Clergy loyalty bonus
if (source_realm_religion->clergy_loyalty > 70.0) {
    total += 10.0;
}
```

### 4. GetAdjacentRealms() - Geographic Neighbors

**Before Integration**:
```cpp
// Placeholder: uses province count as heuristic
// No actual border checking
```

**After Integration**:
```cpp
// Uses ProvinceAdjacencyManager for real border detection
if (adjacency_manager->RealmsShareBorder(realm1, realm2)) {
    // Realms are actually neighbors!
    auto neighbors = adjacency_manager->GetNeighboringRealms(realm_id);
    adjacent.insert(adjacent.end(), neighbors.begin(), neighbors.end());
}
```

---

## Component Lifecycle Management

### Best Practices

1. **Register components immediately after creation**:
   ```cpp
   auto* component = CreateComponent();
   influence_system.RegisterComponent(id, component);
   ```

2. **Unregister before destruction**:
   ```cpp
   influence_system.UnregisterComponent(id);
   delete component;
   ```

3. **Enable integration once during initialization**:
   ```cpp
   // In game initialization
   influence_system.EnableIntegration();
   influence_system.SetProvinceAdjacencyManager(&adjacency_manager);
   influence_system.SetReligionSystemData(&religion_data);
   ```

4. **Check integration status before critical operations**:
   ```cpp
   if (!influence_system.IsIntegrationEnabled()) {
       // Log warning or fallback to placeholder logic
   }
   ```

### Integration Lifecycle

```
Game Start
    ↓
EnableIntegration() ──→ Creates InfluenceSystemIntegrationHelper
    ↓
SetProvinceAdjacencyManager() ──→ Helper stores reference
SetReligionSystemData() ──→ Helper stores reference
    ↓
[Game Loop]
    ↓
RegisterCharacterRelationships() ──→ Helper caches component
RegisterCharacterReligion() ──→ Helper caches component
RegisterRealmReligion() ──→ Helper caches component
    ↓
InfluenceSystem calculations ──→ Uses helper's integrated calculations
    ↓
[Component Destroyed]
    ↓
UnregisterCharacterRelationships() ──→ Helper removes from cache
    ↓
Game End
```

---

## Performance Considerations

### Memory Footprint

- **Integration Helper**: ~64 bytes + component caches
- **Component Caches**: O(n) where n = number of characters/realms
- **Expected overhead**: < 1 MB for typical game (1000 characters, 100 realms)

### Computation Cost

- **Registration**: O(1) - simple map insertion
- **Unregistration**: O(1) - simple map removal
- **Integrated Calculations**: Same complexity as placeholder, but more accurate
  - Marriage lookup: O(m) where m = number of marriages (typically 1-3)
  - Friendship lookup: O(1) - hash map
  - Faith lookup: O(1) - hash map
  - Neighbor lookup: O(1) - cached

### When to Use Integration

**Use integration when**:
- You have character, religion, or province systems implemented
- You want accurate influence calculations
- You care about diplomatic realism

**Placeholder logic is sufficient when**:
- Prototyping or testing
- Character/religion/province systems not yet implemented
- Performance is absolutely critical (though overhead is minimal)

---

## Troubleshooting

### Integration Not Enabled

**Problem**: `IsIntegrationEnabled()` returns false

**Solutions**:
1. Call `EnableIntegration()` first
2. Ensure `SetProvinceAdjacencyManager()` OR `SetReligionSystemData()` has been called
3. Check that at least one system manager is set

### Components Not Being Used

**Problem**: Influence calculations still use placeholder logic

**Solutions**:
1. Verify components are registered: `RegisterCharacterRelationships()`, etc.
2. Check that character IDs match between component and registration
3. Ensure realm IDs match for realm religion components

### Memory Leaks

**Problem**: Components not properly cleaned up

**Solutions**:
1. Call `Unregister*()` methods before destroying components
2. Use RAII pattern for component lifecycle
3. Consider using smart pointers for component ownership

### Influence Values Too Low

**Problem**: Integrated influence lower than expected

**Solutions**:
1. Check marriage data: `IsMarriedTo()`, `HasMarriageTiesTo()`
2. Verify friendship bonds: `GetFriendshipBondStrength()`
3. Ensure faith IDs match: `AreSameFaith()`
4. Confirm province adjacencies are set up correctly

---

## Example Integration Code

See `examples/influence_system_integration_example.cpp` for a complete working example.

Quick snippet:

```cpp
#include "game/diplomacy/InfluenceSystem.h"
#include "game/character/CharacterRelationships.h"
#include "game/religion/ReligionComponents.h"
#include "game/province/ProvinceAdjacency.h"

// Setup
InfluenceSystem influence_system;
influence_system.EnableIntegration();
influence_system.SetProvinceAdjacencyManager(&adjacency_manager);
influence_system.SetReligionSystemData(&religion_data);

// Register components
CharacterRelationshipsComponent char_rel(char_id);
influence_system.RegisterCharacterRelationships(char_id, &char_rel);

CharacterReligionComponent char_religion(char_id, faith_id);
influence_system.RegisterCharacterReligion(char_id, &char_religion);

RealmReligionComponent realm_religion(realm_id, state_faith);
influence_system.RegisterRealmReligion(realm_id, &realm_religion);

// Verify
if (influence_system.IsIntegrationEnabled()) {
    std::cout << "Integration active!\n";
}

// Use normally
influence_system.Initialize();
influence_system.MonthlyUpdate();
```

---

## Testing Integration

### Unit Test Checklist

- [ ] `EnableIntegration()` creates helper
- [ ] `SetProvinceAdjacencyManager()` registers manager
- [ ] `SetReligionSystemData()` registers data
- [ ] `RegisterCharacterRelationships()` adds to cache
- [ ] `RegisterCharacterReligion()` adds to cache
- [ ] `RegisterRealmReligion()` adds to cache
- [ ] `UnregisterCharacterRelationships()` removes from cache
- [ ] `UnregisterCharacterReligion()` removes from cache
- [ ] `UnregisterRealmReligion()` removes from cache
- [ ] `IsIntegrationEnabled()` returns true when managers set
- [ ] Dynastic influence uses actual marriages
- [ ] Personal influence uses friendships
- [ ] Religious influence uses faith compatibility
- [ ] Geographic neighbors use province borders

### Integration Test Scenarios

1. **Marriage Alliance**: Create marriage between rulers, verify dynastic influence increases
2. **Friendship Bond**: Add friendship, verify personal influence increases
3. **Faith Compatibility**: Same faith = +40, same denomination = +25, etc.
4. **Geographic Neighbors**: Adjacent provinces = neighbors detected

---

## Migration from Placeholder Logic

If you have existing code using placeholder logic:

### Before (Placeholder)

```cpp
InfluenceSystem influence_system;
influence_system.Initialize();
// Uses placeholder calculations
```

### After (Integrated)

```cpp
// Add integration setup
influence_system.EnableIntegration();
influence_system.SetProvinceAdjacencyManager(&adjacency_manager);
influence_system.SetReligionSystemData(&religion_data);

// Register components as they're created
for (auto& character : all_characters) {
    influence_system.RegisterCharacterRelationships(character.id, &character.relationships);
    influence_system.RegisterCharacterReligion(character.id, &character.religion);
}

for (auto& realm : all_realms) {
    influence_system.RegisterRealmReligion(realm.id, &realm.religion);
}

// Rest of code unchanged
influence_system.Initialize();
```

---

## API Reference

### InfluenceSystem Integration Methods

| Method | Purpose | When to Call |
|--------|---------|-------------|
| `EnableIntegration()` | Create integration helper | Once during initialization |
| `SetProvinceAdjacencyManager(manager)` | Register adjacency manager | After creating manager |
| `SetReligionSystemData(data)` | Register religion data | After initializing faiths |
| `RegisterCharacterRelationships(id, component)` | Add character relationships | When character created |
| `RegisterCharacterReligion(id, component)` | Add character religion | When character created |
| `RegisterRealmReligion(id, component)` | Add realm religion | When realm created |
| `UnregisterCharacterRelationships(id)` | Remove character relationships | Before destroying character |
| `UnregisterCharacterReligion(id)` | Remove character religion | Before destroying character |
| `UnregisterRealmReligion(id)` | Remove realm religion | Before destroying realm |
| `IsIntegrationEnabled()` | Check if integration active | Before critical operations |

---

## Conclusion

The InfluenceSystem integration is now fully wired and ready for use. By following this guide, you can:

1. ✅ Enable integration with Character, Religion, and Province systems
2. ✅ Register components as they're created
3. ✅ Use accurate, data-driven influence calculations
4. ✅ Maintain backward compatibility with placeholder logic

For questions or issues, refer to:
- `docs/diplomacy/INFLUENCE_SYSTEM_INTEGRATION_GUIDE.md` - Component implementation details
- `docs/CODE_REVIEW_INTEGRATION_SYSTEMS.md` - Code review and quality analysis
- `examples/influence_system_integration_example.cpp` - Working example code

**Integration Status**: ✅ COMPLETE AND READY FOR USE

---

**Document Version**: 1.0
**Last Updated**: November 15, 2025
**Author**: InfluenceSystem Development Team
