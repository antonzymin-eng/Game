# Map Rendering Architecture - LOD Strategy

**Date**: October 21, 2025  
**Status**: Design Specification  
**Implementation**: Pending

---

## Overview

The map uses a **Level-of-Detail (LOD) rendering system** with viewport culling for efficiency. The rendering detail increases progressively as the player zooms in, from abstract state boundaries to detailed terrain and units.

---

## LOD Hierarchy (5 Levels)

### **LOD 0: Strategic View (Furthest Zoom Out)**
**Zoom Range**: 0-20%  
**Visibility**: States/large regions only

**Rendered Elements**:
- State/realm boundaries (thick lines)
- State fill colors (owner-based)
- State names (large font)
- Major capitals (large dots)

**Hidden**:
- Provinces
- Cities
- Terrain features
- All detail

**Performance**: 10-50 polygons on screen


### **LOD 1: Regional View**
**Zoom Range**: 20-40%  
**Visibility**: States + Provinces

**Rendered Elements**:
- State boundaries (medium lines)
- Province boundaries (thin lines)
- Province fill colors (stylized)
- Province names (medium font)
- Major capitals (medium dots)

**Hidden**:
- Cities
- Terrain features
- Buildings
- Units

**Performance**: 50-200 polygons on screen


### **LOD 2: Provincial View (Default Game View)**
**Zoom Range**: 40-60%  
**Visibility**: Provinces + Major Features

**Rendered Elements**:
- Province boundaries (detailed)
- Province colors (terrain-influenced stylized)
- Province names
- **Major cities** (stylized icons, pop > 50k)
- **Major terrain features** (mountains, major rivers - stylized)
- Province capitals (dots)
- Trade routes (lines between provinces)

**Hidden**:
- Small cities/towns
- Minor terrain
- Buildings
- Roads
- Units (except army stacks)

**Performance**: 200-500 polygons + icons


### **LOD 3: Local View**
**Zoom Range**: 60-80%  
**Visibility**: Detailed Province View

**Rendered Elements**:
- Province boundaries
- All cities and towns (icon size by population)
- All terrain features (stylized)
  - Mountains (triangle icons)
  - Forests (tree cluster icons)
  - Rivers (blue lines)
  - Lakes (blue polygons)
- **Roads** (thin lines between cities)
- **Army units** (stylized military icons with counters)
- Resource icons (mines, farms, etc.)

**Hidden**:
- Actual terrain textures
- Buildings
- Individual units

**Performance**: 500-2000 polygons + many icons


### **LOD 4: Tactical View (Maximum Zoom - Top 2 Levels)**
**Zoom Range**: 80-100%  
**Visibility**: Terrain and Units

**Rendered Elements**:
- **Actual terrain rendering** (heightmap-based)
  - Elevation shading
  - Terrain textures (grass, forest, mountain, water)
  - Natural terrain colors
- **Buildings** (3D models or sprites)
  - Cities show actual buildings
  - Farms, mines, castles, etc.
- **Roads** (textured paths)
- **Individual military units** (sprites/models)
  - Unit formations visible
  - Unit types distinguishable
  - Movement animations
- Detailed water rendering

**Hidden**:
- State/province names (unless hovering)
- Abstract icons replaced by actual visuals

**Performance**: 2000-5000+ polygons + sprites/models

---

## Viewport Culling Strategy

### Spatial Partitioning
```cpp
// Quadtree or Grid-based spatial index
class SpatialIndex {
    // Only render provinces/features within viewport bounds
    std::vector<EntityID> GetVisibleProvinces(Rect viewport, int lod_level);
    std::vector<Feature> GetVisibleFeatures(Rect viewport, int lod_level);
};
```

### Culling Rules
1. **Frustum Culling**: Check if province bounds intersect viewport
2. **LOD-based Culling**: Higher LOD = more aggressive culling of small features
3. **Occlusion**: Don't render fully hidden provinces (covered by others)
4. **Distance Culling**: Features beyond certain distance from viewport center are culled

### Performance Targets
- **LOD 0-1**: < 1ms render time
- **LOD 2-3**: < 5ms render time
- **LOD 4**: < 16ms render time (60 FPS)

---

## Rendering Components

### Core Classes

```cpp
// Main renderer with LOD management
class MapRenderer {
    int current_lod_level;
    Camera2D camera;
    SpatialIndex spatial_index;
    
    void Render();
    void UpdateLOD(); // Calculate LOD based on zoom
    void RenderProvinces(int lod);
    void RenderFeatures(int lod);
    void RenderUnits(int lod);
};

// Terrain rendering for LOD 4
class TerrainRenderer {
    void RenderHeightmap(Province* province);
    void RenderTerrainTextures(Province* province);
    void RenderWater(Province* province);
};

// Efficient viewport management
class ViewportCuller {
    Rect viewport_bounds;
    std::vector<EntityID> visible_provinces;
    
    void UpdateViewport(Camera2D camera);
    bool IsVisible(Province* province);
    bool IsFeatureVisible(Feature* feature, int lod);
};
```

### Data Structures

```cpp
// Province rendering data (cached)
struct ProvinceRenderData {
    EntityID province_id;
    std::vector<Vector2> boundary_points;
    Vector2 center_position;
    Color fill_color;
    Rect bounding_box;
    
    // LOD-specific cached data
    std::vector<Vector2> simplified_boundary_lod1;
    std::vector<Vector2> simplified_boundary_lod2;
    Mesh terrain_mesh_lod4; // Only for highest zoom
};

// Feature rendering (cities, terrain, etc)
struct FeatureRenderData {
    FeatureType type; // CITY, MOUNTAIN, FOREST, etc
    Vector2 position;
    int lod_min; // Minimum LOD to show
    int lod_max; // Maximum LOD to show
    IconType icon; // For stylized rendering
    Mesh mesh;     // For terrain rendering
};
```

---

## Rendering Pipeline

### Frame Render Loop
```cpp
void MapRenderer::Render() {
    // 1. Update viewport and LOD
    UpdateViewport();
    UpdateLOD();
    
    // 2. Viewport culling
    visible_provinces = viewport_culler->GetVisible(camera);
    visible_features = viewport_culler->GetVisibleFeatures(camera, current_lod);
    
    // 3. Render by LOD level
    switch(current_lod) {
        case 0: RenderLOD0_Strategic(); break;
        case 1: RenderLOD1_Regional(); break;
        case 2: RenderLOD2_Provincial(); break;
        case 3: RenderLOD3_Local(); break;
        case 4: RenderLOD4_Tactical(); break;
    }
    
    // 4. Render UI overlays
    RenderProvinceNames();
    RenderSelection();
    RenderTooltips();
}
```

### LOD Transition
- Smooth fade between LOD levels (opacity blending)
- Hysteresis to prevent flickering (LOD up threshold != LOD down threshold)
- Preload next LOD data during transitions

---

## Implementation Files

### Priority 1: Core Rendering (CRITICAL)
```
src/rendering/MapRenderer.cpp
  - Main rendering loop
  - LOD management
  - Province polygon rendering
  - Camera controls

src/rendering/ViewportCuller.cpp
  - Spatial culling
  - Visibility testing
  - Performance optimization

include/map/MapData.h (implement)
  - Province boundary data
  - Feature positions
  - Spatial index
```

### Priority 2: Terrain System (HIGH)
```
src/rendering/TerrainRenderer.cpp
  - LOD 4 terrain rendering
  - Heightmap rendering
  - Texture application

src/game/map/ProvinceGeometry.cpp
  - Province boundary generation
  - Mesh creation
  - LOD simplification

src/game/map/GeographicUtils.cpp
  - Coordinate transformations
  - Distance calculations
  - Spatial queries
```

### Priority 3: Feature Rendering (MEDIUM)
```
src/game/map/MapSystem.cpp
  - Feature management
  - City/terrain feature data
  - Icon/sprite management

src/game/map/SpatialIndex.cpp
  - Quadtree or grid structure
  - Fast spatial queries
  - Viewport intersection tests
```

---

## Data Requirements

### Province Data (JSON Format)
```json
{
  "provinces": [
    {
      "id": 1,
      "name": "Wessex",
      "owner_realm": 10,
      "boundary": [
        {"x": 100, "y": 200},
        {"x": 150, "y": 180},
        // ... polygon points
      ],
      "center": {"x": 125, "y": 190},
      "terrain_type": "plains",
      "features": [
        {
          "type": "city",
          "name": "Winchester",
          "position": {"x": 120, "y": 195},
          "population": 75000,
          "lod_min": 2
        },
        {
          "type": "mountain",
          "position": {"x": 140, "y": 185},
          "lod_min": 2
        }
      ]
    }
  ]
}
```

### LOD Configuration
```json
{
  "lod_config": {
    "levels": [
      {
        "level": 0,
        "zoom_min": 0.0,
        "zoom_max": 0.2,
        "render_provinces": false,
        "render_cities": false,
        "render_terrain": false
      },
      // ... more levels
      {
        "level": 4,
        "zoom_min": 0.8,
        "zoom_max": 1.0,
        "render_provinces": true,
        "render_cities": true,
        "render_terrain": true,
        "render_buildings": true,
        "render_units": true
      }
    ]
  }
}
```

---

## Performance Optimization

### Caching Strategy
1. **Province Meshes**: Pre-generate and cache all LOD levels
2. **Boundary Simplification**: Use Douglas-Peucker algorithm for lower LODs
3. **Texture Atlases**: Pack all icons/sprites into single texture
4. **Batch Rendering**: Group provinces by color/type for instanced rendering

### Memory Budget
- **LOD 0-1**: ~10 MB (state boundaries only)
- **LOD 2-3**: ~50 MB (all provinces + major features)
- **LOD 4**: ~200 MB (terrain meshes + textures)
- **Total Max**: ~260 MB for rendering data

### Rendering Techniques
- **Instanced Rendering**: For repeated icons (cities, units)
- **Texture Atlasing**: All icons in one texture
- **Mesh Simplification**: Different detail levels per LOD
- **Frustum Culling**: Only render visible provinces
- **Dirty Flags**: Only update changed provinces

---

## Integration with Game Systems

### ECS Component Integration
```cpp
// Attach to province entities
struct ProvinceRenderComponent : public Component<ProvinceRenderComponent> {
    ProvinceRenderData render_data;
    std::vector<FeatureRenderData> features;
    bool needs_update;
};

// MapRenderer accesses via ComponentAccessManager
void MapRenderer::UpdateProvinceColors() {
    auto provinces = entity_manager->GetEntitiesWithComponent<ProvinceRenderComponent>();
    for (auto& entity_id : provinces) {
        auto render = entity_manager->GetComponent<ProvinceRenderComponent>(entity_id);
        auto owner = entity_manager->GetComponent<OwnerComponent>(entity_id);
        render->render_data.fill_color = GetRealmColor(owner->realm_id);
    }
}
```

### Real-time Updates
- Province ownership changes → Update colors
- Population growth → Update city icon sizes
- Construction complete → Add building to LOD 4
- Army movement → Update unit positions

---

## Development Phases

### Phase 1: Basic LOD 2 Rendering (3-4 days)
**Goal**: See provinces on screen with basic interaction

- [x] Document architecture (this file)
- [ ] Implement MapRenderer.cpp basics
- [ ] Create 10 test provinces with boundaries
- [ ] Implement ViewportCuller.cpp
- [ ] Basic camera controls (pan/zoom)
- [ ] Province selection on click
- [ ] Simple LOD 2 rendering (polygons + names)

### Phase 2: Full LOD System (4-5 days)
**Goal**: All 5 LOD levels functional

- [ ] Implement LOD 0-1 (strategic/regional)
- [ ] Implement LOD 3 (local with features)
- [ ] Add major cities and terrain features
- [ ] Smooth LOD transitions
- [ ] Performance optimization

### Phase 3: Terrain Rendering (5-7 days)
**Goal**: LOD 4 with actual terrain

- [ ] Implement TerrainRenderer.cpp
- [ ] Heightmap generation/loading
- [ ] Terrain textures
- [ ] Building rendering
- [ ] Unit sprites/models

### Phase 4: Polish (2-3 days)
**Goal**: Production quality

- [ ] Visual effects (shadows, borders)
- [ ] Better color schemes
- [ ] Icon improvements
- [ ] Performance profiling
- [ ] Memory optimization

**Total Estimated Time**: 2-3 weeks for complete system

---

## Next Immediate Actions

1. **Create test province data** (JSON with 10 provinces)
2. **Implement MapRenderer.cpp** (LOD 2 only first)
3. **Implement ViewportCuller.cpp** (basic frustum culling)
4. **Test rendering loop** with hardcoded data
5. **Add camera controls** (WASD pan, mouse wheel zoom)

---

*Document Status: Design Complete - Ready for Implementation*  
*Last Updated: October 21, 2025*
