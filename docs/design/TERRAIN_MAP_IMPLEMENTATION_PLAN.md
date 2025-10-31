# Terrain Map Implementation Plan - LOD 4 (Maximum Zoom)

**Date Created:** October 31, 2025
**Status:** Planning Phase
**Related Documents:**
- `docs/design/MECHANICA_IMPERII_EXPANDED_DESIGN.md` - Original design vision
- `docs/architecture/MAP-RENDERING-ARCHITECTURE.md` - Current LOD architecture

---

## Original Design Vision

From the Expanded Design Document, Section 2: Geography & World:

> **"Terrain grid at 1m zoom, stylized map at higher zoom levels."**
>
> - High-res Europe map (4901×4251)
> - Weather, fog of war, line of sight mechanics

This refers to implementing **LOD 4: Tactical View** for:
- **Military campaigns** - detailed terrain for battles and army movements
- **Naval campaigns** - coastal features, ports, naval battles

---

## Current Implementation Status

### ✅ Completed (LOD 0-3)
- **LOD 0:** Strategic View (states/realms only)
- **LOD 1:** Regional View (provinces visible)
- **LOD 2:** Provincial View (cities, major terrain features)
- **LOD 3:** Local View (all cities, terrain icons, armies, roads)

### ⚠️ Pending (LOD 4)
- **LOD 4:** Tactical View (terrain grid, heightmap, detailed units)

**Current Code Status:**
- Architecture designed in `docs/architecture/MAP-RENDERING-ARCHITECTURE.md:103-127`
- `TerrainRenderer` class specified but not implemented
- Placeholder in MapRenderer.cpp for LOD 4 rendering

---

## LOD 4 Requirements - Detailed Breakdown

### Zoom Level
- **Activation:** 80-100% zoom (closest zoom)
- **Map Resolution:** 1 meter per grid cell at maximum zoom
- **Coverage:** High-res Europe map (4901×4251 pixels)

### Core Rendering Features

#### 1. Terrain Grid System
**Requirements:**
- Grid-based terrain representation (1m per cell at max zoom)
- Heightmap data for elevation
- Terrain types: plains, forest, mountain, water, hills, marsh, desert, tundra

**Data Structure:**
```cpp
struct TerrainCell {
    Vector2i grid_position;
    float elevation;         // Height in meters
    TerrainType type;        // Grass, forest, mountain, etc.
    float vegetation_density; // 0.0-1.0
    bool navigable;          // Can armies pass?
    WeatherState weather;    // Current weather effect
};
```

#### 2. Heightmap Rendering
**Requirements:**
- Elevation shading (darker = lower, lighter = higher)
- Terrain textures blended by type
- Normal mapping for 3D appearance
- Water rendering with shorelines

**Technical Approach:**
- Load or generate heightmap data per province
- Use OpenGL shader for elevation-based coloring
- Blend textures based on terrain type and elevation
- Dynamic lighting for time-of-day effects (optional)

#### 3. Buildings & Structures
**Requirements:**
- Cities show actual building sprites/models
- Fortifications (castles, walls, towers)
- Economic buildings (farms, mines, workshops)
- Military structures (barracks, depots)
- Naval structures (ports, shipyards, docks)

**Rendering:**
- Sprite-based for initial implementation
- 3D models for future enhancement
- LOD for building detail (simple icons → detailed sprites)

#### 4. Military Units
**Requirements:**
- Individual unit sprites (not just army stacks)
- Unit formations visible (line, column, square)
- Unit types distinguishable (infantry, cavalry, artillery)
- Movement animations
- Combat animations (optional)

**Data Integration:**
- Read from `MilitaryComponents.h` unit data
- Position units based on army location and formation
- Show unit strength with visual indicators

#### 5. Naval Units & Features
**Requirements:**
- Ship sprites/models (galleys, cogs, frigates, ships-of-the-line)
- Naval formations
- Ports and harbors with docking
- Coastal terrain features (cliffs, beaches, shallows)
- Blockade visualization

**Special Considerations:**
- Water depth rendering (deep blue → shallow cyan)
- Shoals and reefs as hazards
- Wind direction visualization for age of sail

#### 6. Weather & Environmental Effects
**Requirements:**
- Rain/snow visual effects
- Fog (reduces visibility)
- Storms (affects naval movement)
- Seasonal changes (snow in winter, green in summer)

**Implementation:**
- Particle effects for precipitation
- Fog overlay with adjustable density
- Seasonal texture swaps or color modulation

#### 7. Fog of War & Line of Sight
**Requirements:**
- Unexplored areas (black/dark gray)
- Explored but not visible (greyed out)
- Currently visible (full color and detail)
- LOS calculated from unit positions and terrain

**Technical Approach:**
- Grid-based visibility calculation
- Terrain elevation affects LOS (hills provide vision bonus)
- Forests reduce visibility range
- Real-time updates as units move

---

## Implementation Phases

### Phase 1: Basic Terrain Grid (3-4 days)
**Goal:** Render a basic terrain grid with heightmap at LOD 4

- [ ] Create `TerrainRenderer` class
- [ ] Implement heightmap data structure (per-province)
- [ ] Generate or load test heightmap data
- [ ] Render elevation-based coloring
- [ ] Implement terrain type textures (grass, forest, mountain, water)
- [ ] Add zoom threshold to activate LOD 4
- [ ] Test with existing 12 test provinces

**Files to Create/Modify:**
- `include/map/render/TerrainRenderer.h` (new)
- `src/rendering/TerrainRenderer.cpp` (new)
- `include/map/TerrainData.h` (new - data structures)
- `src/map/TerrainDataGenerator.cpp` (new - procedural generation)
- `src/rendering/MapRenderer.cpp` (modify LOD 4 case)

### Phase 2: Buildings & Structures (2-3 days)
**Goal:** Show buildings and fortifications at LOD 4

- [ ] Define building sprite/model data structure
- [ ] Load building positions from province data
- [ ] Render cities as building clusters
- [ ] Render fortifications (castles, walls)
- [ ] Render economic structures (farms, mines)
- [ ] Add building tooltips

**Files to Create/Modify:**
- `include/map/BuildingRenderData.h` (new)
- `src/rendering/BuildingRenderer.cpp` (new)
- `data/test_provinces.json` (add building data)

### Phase 3: Military & Naval Units (3-4 days)
**Goal:** Render individual units and formations

- [ ] Create unit sprite system
- [ ] Implement formation rendering (line, column, square)
- [ ] Integrate with `MilitaryComponents` data
- [ ] Render naval units and formations
- [ ] Add unit movement animations
- [ ] Show unit strength indicators

**Files to Create/Modify:**
- `include/map/render/UnitRenderer.h` (new)
- `src/rendering/UnitRenderer.cpp` (new)
- `assets/sprites/units/` (sprite assets)
- Integration with `include/game/military/MilitaryComponents.h`

### Phase 4: Weather & Environmental Effects (2-3 days)
**Goal:** Add weather, seasons, and visual effects

- [ ] Implement weather system integration
- [ ] Rain/snow particle effects
- [ ] Fog rendering
- [ ] Seasonal texture variations
- [ ] Time-of-day lighting (optional)

**Files to Create/Modify:**
- `include/map/render/WeatherRenderer.h` (new)
- `src/rendering/WeatherRenderer.cpp` (new)
- Shader files for weather effects

### Phase 5: Fog of War & Line of Sight (3-4 days)
**Goal:** Implement visibility and exploration mechanics

- [ ] Grid-based fog of war data structure
- [ ] LOS calculation algorithm
- [ ] Terrain-based visibility modifiers
- [ ] Real-time visibility updates
- [ ] Render fog overlay (black/grey/clear)
- [ ] Save/load fog of war state

**Files to Create/Modify:**
- `include/map/FogOfWar.h` (new)
- `src/map/FogOfWarSystem.cpp` (new)
- `include/map/LineOfSight.h` (new)
- `src/map/LineOfSightCalculator.cpp` (new)

### Phase 6: Polish & Optimization (2-3 days)
**Goal:** Performance and visual quality improvements

- [ ] Performance profiling
- [ ] Texture atlasing for terrain tiles
- [ ] Mesh caching and instancing
- [ ] Visual polish (shadows, borders, effects)
- [ ] Memory optimization
- [ ] Smooth LOD 3 ↔ LOD 4 transitions

---

## Technical Requirements

### Data Requirements

#### Heightmap Data
**Format:** 2D array of elevation values per province
```json
{
  "province_id": 1,
  "heightmap": {
    "width": 100,
    "height": 100,
    "data": [0.0, 0.1, 0.2, ...], // Normalized 0.0-1.0
    "min_elevation": 0.0,
    "max_elevation": 500.0
  }
}
```

#### Terrain Texture Assets
- `grass.png` - plains terrain
- `forest.png` - forested areas
- `mountain.png` - mountainous terrain
- `water.png` - rivers, lakes, seas
- `hills.png` - hilly terrain
- `marsh.png` - marshland/swamps
- `desert.png` - arid regions (for expansion)
- `tundra.png` - frozen regions

#### Building Sprites
- City sizes: small (< 10k), medium (10k-50k), large (50k+)
- Fortifications: castle, fortress, tower, walls
- Economic: farm, mine, workshop, market
- Military: barracks, depot, training ground
- Naval: port, shipyard, dock

#### Unit Sprites
**Military:**
- Infantry (levy, professional, musketeer)
- Cavalry (light, heavy, dragoon)
- Artillery (bombard, cannon, field gun)

**Naval:**
- Galley, cog, carrack, frigate, ship-of-the-line

### Performance Targets
- **LOD 4 Render Time:** < 16ms (60 FPS)
- **Terrain Grid Size:** Dynamically culled based on viewport
- **Memory Budget:** ~200 MB for terrain meshes + textures
- **Viewport Culling:** Only render visible terrain grid cells

### Shader Requirements
- Heightmap elevation shader (vertex displacement or color modulation)
- Terrain texture blending shader (multi-texture)
- Water shader (animated, reflective)
- Fog of war overlay shader (alpha blending)
- Weather effect shaders (rain, snow, fog)

---

## Integration with Existing Systems

### Map Rendering System
**File:** `src/rendering/MapRenderer.cpp`
- Add LOD 4 case in `Render()` method
- Call `TerrainRenderer::Render()` for terrain grid
- Call `BuildingRenderer::Render()` for structures
- Call `UnitRenderer::Render()` for military/naval units
- Apply weather and fog of war overlays

### Military System
**Files:** `include/game/military/MilitaryComponents.h`, `src/game/military/MilitarySystem.cpp`
- Read unit positions from `MilitaryUnitComponent`
- Read formation data from `MilitaryFormationComponent`
- Update unit positions in real-time during movement

### Population System
**Files:** `include/game/population/PopulationComponents.h`
- Use population data to determine city building density
- Larger cities = more building sprites

### Economic System
**Files:** `include/game/economy/EconomicComponents.h`
- Read building data from `BuildingComponent`
- Show active production buildings at LOD 4

### Weather System
**Status:** Not yet implemented
**Future Integration:**
- Create weather system that affects terrain rendering
- Weather impacts military movement and visibility
- Seasonal changes affect terrain textures

---

## Testing Strategy

### Unit Tests
- Heightmap data loading and generation
- Terrain type determination
- LOS calculation algorithms
- Fog of war visibility updates

### Integration Tests
- LOD 3 ↔ LOD 4 transitions
- Performance at various zoom levels
- Rendering with different province counts
- Memory usage under load

### Visual Tests
- Terrain rendering quality
- Building placement accuracy
- Unit formation correctness
- Weather effect appearance
- Fog of war transitions

---

## Milestones & Timeline

| Phase | Duration | Dependencies | Completion Criteria |
|-------|----------|--------------|---------------------|
| Phase 1: Basic Terrain Grid | 3-4 days | None | Heightmap renders, textures blend |
| Phase 2: Buildings & Structures | 2-3 days | Phase 1 | Buildings visible, tooltips work |
| Phase 3: Military & Naval Units | 3-4 days | Phase 1 | Units render, formations correct |
| Phase 4: Weather & Environment | 2-3 days | Phase 1 | Weather effects visible |
| Phase 5: Fog of War & LOS | 3-4 days | Phase 1, 3 | Visibility calculated, fog renders |
| Phase 6: Polish & Optimization | 2-3 days | All | 60 FPS, smooth transitions |

**Total Estimated Time:** 15-21 days (3-4 weeks)

---

## Next Steps

1. **Review and Approve Plan** - Confirm scope and priorities
2. **Set Up Asset Pipeline** - Prepare terrain textures and sprites
3. **Begin Phase 1** - Implement TerrainRenderer and basic heightmap
4. **Iterative Development** - Complete each phase, test, then proceed

---

## Open Questions

1. **Heightmap Data Source:**
   - Generate procedurally based on terrain type?
   - Load from external data files?
   - Hybrid approach (templates + noise)?

2. **Building Sprite Style:**
   - Realistic sprites?
   - Stylized/iconic representations?
   - 3D models or 2D sprites?

3. **Unit Animation:**
   - Static sprites or animated?
   - How detailed should formations be?

4. **Performance Priorities:**
   - Target 60 FPS at all zoom levels?
   - Acceptable to drop to 30 FPS at max zoom with many units?

5. **Scope for Initial Implementation:**
   - Full feature set or MVP (minimum viable product)?
   - Which phases are essential vs. nice-to-have?

---

*This document provides a comprehensive plan for implementing the terrain grid system at maximum zoom (LOD 4) for military and naval campaigns, as envisioned in the original Expanded Design Document.*
