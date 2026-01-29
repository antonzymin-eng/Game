# Province Texture Renderer Implementation Plan

**Goal**: Replace complex triangulation-based GPU renderer with simple, scalable province ID texture approach.

**Why**: Current approach doesn't scale well and is overly complex. Province texture method handles 1,000-10,000 provinces with constant performance.

**Timeline**: ~5-7 days for full implementation

---

## Architecture Overview

### Current (To Be Replaced)
```
GPUMapRenderer (1,018 lines)
├── Triangulates each province with Earcut
├── Generates 3 LOD levels via mesh decimation
├── Uploads geometry to VBO/IBO per LOD
└── Renders triangles with province ID per vertex
```

**Problems**:
- Complex (1,018 lines)
- Runtime triangulation overhead
- LOD generation expensive
- Scales poorly with province count

### New (Province Texture)
```
TextureMapRenderer (~150 lines)
├── Pre-baked province ID texture (offline)
├── Single textured quad render
├── Fragment shader does province lookup per pixel
└── Tactical detail layer for battles (lazy-loaded)
```

**Benefits**:
- Simple (~150 lines)
- Zero runtime geometry processing
- Constant performance (1,000 provinces = 10,000 provinces)
- Perfect province boundaries

---

## Step-by-Step Implementation

### Phase 1: Offline Tools (Days 1-2)

#### Step 1.1: Create Province Texture Baker
**File**: `tools/bake_province_texture.py`
**Purpose**: Convert GeoJSON → Province ID texture (PNG)

**Algorithm**:
```python
1. Load GeoJSON province boundaries
2. Create empty image (4096x4096 or configurable)
3. For each province:
   - Rasterize polygon to image
   - Set pixel value = province ID (encode as RGB)
4. Save as PNG
```

**Input**: `data/maps/map_europe_combined.json`
**Output**: `data/textures/province_id_map.png`

**Dependencies**:
- `shapely` - Polygon operations
- `Pillow` (PIL) - Image manipulation
- `json` - GeoJSON parsing

**Estimated Effort**: 4-6 hours

**Acceptance Criteria**:
- [ ] Generates 4096x4096 texture from 133 province GeoJSON
- [ ] Each province has unique RGB color = ID
- [ ] Province boundaries are crisp (anti-aliasing off)
- [ ] Can handle up to 65,535 provinces (RGB encoding)

---

#### Step 1.2: Create Texture Validation Tool
**File**: `tools/validate_province_texture.py`
**Purpose**: Verify baked texture is correct

**Checks**:
```python
1. Count unique colors (should match province count)
2. Check for gaps (black pixels = unmapped areas)
3. Verify province IDs are sequential
4. Output coverage report
```

**Estimated Effort**: 1-2 hours

---

### Phase 2: Core Renderer (Days 2-3)

#### Step 2.1: Create TextureMapRenderer Header
**File**: `include/map/render/TextureMapRenderer.h`

```cpp
class TextureMapRenderer {
public:
    TextureMapRenderer(core::ecs::EntityManager& entity_manager);
    ~TextureMapRenderer();

    bool Initialize();
    void Render(const Camera2D& camera);
    void HandleInput();

    // Province data upload (updates color lookup table)
    bool UploadProvinceColors(const std::vector<const ProvinceRenderComponent*>& provinces);

    // Camera access
    Camera2D& GetCamera() { return camera_; }
    const Camera2D& GetCamera() const { return camera_; }

    // Selection
    void SetSelectedProvince(uint32_t id);
    void ClearSelection();
    uint32_t GetProvinceAtScreenPos(int x, int y);

private:
    core::ecs::EntityManager& entity_manager_;
    Camera2D camera_;

    // OpenGL objects
    GLuint vao_;                      // Screen quad VAO
    GLuint vbo_;                      // Screen quad vertices
    GLuint province_id_texture_;      // Province ID map (loaded from PNG)
    GLuint province_color_texture_;   // 1D texture: province ID → color
    GLuint shader_program_;

    // Shader uniforms
    GLint u_view_projection_;
    GLint u_province_id_map_;
    GLint u_province_colors_;
    GLint u_selected_province_;
    GLint u_hovered_province_;

    // State
    uint32_t selected_province_id_;
    uint32_t hovered_province_id_;
    size_t province_count_;

    // Initialization helpers
    bool LoadShaders();
    bool LoadProvinceTexture(const std::string& path);
    bool CreateScreenQuad();
    void CheckGLError(const char* file, int line);
};
```

**Estimated Effort**: 1 hour

---

#### Step 2.2: Implement TextureMapRenderer
**File**: `src/rendering/TextureMapRenderer.cpp`

**Key Methods**:

```cpp
bool Initialize() {
    // 1. Load shaders
    // 2. Load province ID texture from PNG
    // 3. Create screen quad geometry
    // 4. Create province color lookup texture (1D, 65536 pixels)
}

void Render(const Camera2D& camera) {
    // 1. Bind shader
    // 2. Set uniforms (view_projection, textures, selection)
    // 3. Bind province_id_texture (slot 0)
    // 4. Bind province_color_texture (slot 1)
    // 5. Draw screen quad (6 vertices)
}

bool UploadProvinceColors(provinces) {
    // 1. Create 1D array: colors[65536]
    // 2. For each province: colors[province.id] = province.color
    // 3. Upload to province_color_texture via glTexSubImage1D
}

uint32_t GetProvinceAtScreenPos(int x, int y) {
    // 1. Read pixel from province_id_texture at (x, y)
    // 2. Decode RGB → province ID
    // 3. Return province ID (for mouse picking)
}
```

**Estimated Effort**: 6-8 hours

---

#### Step 2.3: Create Province Shaders
**File**: `src/rendering/TextureMapRenderer.cpp` (embedded)

**Vertex Shader**:
```glsl
#version 330 core

layout(location = 0) in vec2 position;  // Screen quad vertices
layout(location = 1) in vec2 texcoord;  // UV coordinates

uniform mat4 view_projection;

out vec2 v_world_uv;

void main() {
    gl_Position = view_projection * vec4(position, 0.0, 1.0);
    v_world_uv = texcoord;
}
```

**Fragment Shader**:
```glsl
#version 330 core

in vec2 v_world_uv;

uniform sampler2D province_id_map;    // Province ID per pixel (RGB encoded)
uniform sampler1D province_colors;    // Color lookup: ID → RGBA
uniform uint selected_province_id;
uniform uint hovered_province_id;

out vec4 frag_color;

uint DecodeProvinceID(vec3 rgb) {
    // RGB → uint (supports up to 16.7M provinces)
    return uint(rgb.r * 255.0) +
           uint(rgb.g * 255.0) * 256 +
           uint(rgb.b * 255.0) * 65536;
}

void main() {
    // Sample province ID at this pixel
    vec3 id_pixel = texture(province_id_map, v_world_uv).rgb;
    uint province_id = DecodeProvinceID(id_pixel);

    // Look up province color
    vec4 base_color = texelFetch(province_colors, int(province_id), 0);

    // Apply selection highlight
    vec4 final_color = base_color;
    if (province_id == selected_province_id) {
        final_color = mix(base_color, vec4(1.0, 1.0, 1.0, 1.0), 0.3);
    } else if (province_id == hovered_province_id) {
        final_color = base_color * 1.2;
    }

    frag_color = final_color;
}
```

**Estimated Effort**: 2 hours

---

### Phase 3: Tactical Detail Layer (Days 3-4)

#### Step 3.1: Create DetailedTerrain Structure
**File**: `include/map/render/DetailedTerrain.h`

```cpp
struct DetailedTerrain {
    uint32_t province_id;

    // Terrain features (procedurally generated)
    std::vector<glm::vec2> forest_positions;
    std::vector<glm::vec2> hill_positions;
    std::vector<std::vector<glm::vec2>> river_paths;

    // Heightmap (optional, for advanced terrain)
    std::vector<float> elevation;  // 128x128 grid

    // Generation
    static DetailedTerrain* Generate(
        uint32_t province_id,
        TerrainType terrain_type,
        const std::vector<glm::vec2>& boundary,
        uint32_t seed
    );
};

class TerrainDetailManager {
public:
    void Update(const Camera2D& camera);
    void Render(const Camera2D& camera);

    DetailedTerrain* GetOrGenerate(uint32_t province_id);
    void UnloadDistant(const Camera2D& camera);

private:
    std::unordered_map<uint32_t, std::unique_ptr<DetailedTerrain>> loaded_;
    core::ecs::EntityManager& entity_manager_;
};
```

**Estimated Effort**: 3-4 hours

---

#### Step 3.2: Implement Procedural Terrain Generation
**File**: `src/rendering/DetailedTerrain.cpp`

```cpp
DetailedTerrain* DetailedTerrain::Generate(
    uint32_t province_id,
    TerrainType terrain_type,
    const std::vector<glm::vec2>& boundary,
    uint32_t seed)
{
    auto terrain = new DetailedTerrain();
    terrain->province_id = province_id;

    Random rng(seed);

    // Calculate province bounds
    auto bounds = CalculateBounds(boundary);

    // Generate features based on terrain type
    switch (terrain_type) {
        case TerrainType::FOREST:
            terrain->forest_positions = GenerateForestPoints(
                boundary, bounds, rng, density=0.3f
            );
            break;

        case TerrainType::MOUNTAINS:
            terrain->hill_positions = GenerateHillPoints(
                boundary, bounds, rng, count=10
            );
            terrain->elevation = GenerateHeightmap(
                bounds, rng, roughness=0.7f
            );
            break;

        case TerrainType::WETLAND:
            terrain->river_paths = GenerateRivers(
                boundary, bounds, rng, count=2
            );
            break;
    }

    return terrain;
}
```

**Estimated Effort**: 4-6 hours

---

#### Step 3.3: Implement Detail Rendering
**File**: `src/rendering/TerrainDetailManager.cpp`

```cpp
void TerrainDetailManager::Render(const Camera2D& camera) {
    // Only render details at high zoom
    if (camera.zoom < 3.0f) return;

    auto visible_provinces = GetVisibleProvinces(camera);

    for (auto province_id : visible_provinces) {
        auto* detail = GetOrGenerate(province_id);
        if (!detail) continue;

        // Render forests as sprite billboards
        for (auto& pos : detail->forest_positions) {
            if (camera.IsVisible(pos)) {
                DrawTreeSprite(pos, camera);
            }
        }

        // Render hills as textured quads
        for (auto& pos : detail->hill_positions) {
            if (camera.IsVisible(pos)) {
                DrawHillSprite(pos, camera);
            }
        }

        // Render rivers as line strips
        for (auto& river : detail->river_paths) {
            DrawRiverLine(river, camera);
        }
    }
}
```

**Estimated Effort**: 3-4 hours

---

### Phase 4: Seamless Zoom (Day 4)

#### Step 4.1: Implement Zoom Transition Logic
**File**: `src/rendering/TextureMapRenderer.cpp`

```cpp
void TextureMapRenderer::Render(const Camera2D& camera) {
    float zoom = camera.zoom;

    // Phase 1: Always render base province layer
    float province_alpha = CalculateProvinceAlpha(zoom);
    RenderProvinceLayer(camera, province_alpha);

    // Phase 2: Fade in tactical details at high zoom
    if (zoom > 2.0f) {
        float detail_alpha = CalculateDetailAlpha(zoom);
        terrain_detail_manager_->Render(camera, detail_alpha);
    }

    // Phase 3: Render units (always visible above certain zoom)
    if (zoom > 1.5f) {
        RenderUnits(camera);
    }
}

float CalculateProvinceAlpha(float zoom) {
    // Zoom 0-3: Provinces fully opaque
    // Zoom 3-6: Fade out to 30% (still visible for context)
    // Zoom 6+: 30% opacity
    if (zoom < 3.0f) return 1.0f;
    if (zoom > 6.0f) return 0.3f;
    return 1.0f - (zoom - 3.0f) / 3.0f * 0.7f;
}

float CalculateDetailAlpha(float zoom) {
    // Zoom 2-3: Fade in from 0% to 100%
    // Zoom 3+: Fully opaque
    if (zoom < 2.0f) return 0.0f;
    if (zoom > 3.0f) return 1.0f;
    return (zoom - 2.0f) / 1.0f;
}
```

**Estimated Effort**: 2-3 hours

---

### Phase 5: Integration (Day 5)

#### Step 5.1: Update CMakeLists.txt
**File**: `CMakeLists.txt`

```cmake
# Remove old GPU renderer
set(RENDER_SOURCES
    src/rendering/MapRenderer.cpp
    # src/rendering/GPUMapRenderer.cpp       # REMOVE
    # src/rendering/RenderingManager.cpp     # REMOVE
    src/rendering/TextureMapRenderer.cpp     # ADD
    src/rendering/DetailedTerrain.cpp        # ADD
    src/rendering/TerrainDetailManager.cpp   # ADD
    src/rendering/TerrainRenderer.cpp
    # ...
)

# Remove earcut dependency (no longer needed)
# FetchContent_Declare(earcut ...) # REMOVE

# Keep GLM (still needed for math)
```

**Estimated Effort**: 30 minutes

---

#### Step 5.2: Update main.cpp
**File**: `apps/main.cpp`

```cpp
// Replace include
#include "map/render/TextureMapRenderer.h"

// Update global
static std::unique_ptr<game::map::TextureMapRenderer> g_map_renderer;

// Update InitializeMapSystem()
static void InitializeMapSystem() {
    // Create renderer
    g_map_renderer = std::make_unique<game::map::TextureMapRenderer>(*g_entity_manager);

    // Initialize (loads province texture)
    if (!g_map_renderer->Initialize()) {
        throw std::runtime_error("Failed to initialize TextureMapRenderer");
    }

    // Load province data
    MapDataLoader::LoadProvincesECS("data/maps/map_europe_combined.json", *g_entity_manager);

    // Upload province colors to renderer
    std::vector<const ProvinceRenderComponent*> provinces;
    g_entity_manager->ForEachEntity([&](entity_id) {
        if (auto* p = GetComponent<ProvinceRenderComponent>(entity_id))
            provinces.push_back(p);
    });
    g_map_renderer->UploadProvinceColors(provinces);
}

// Render loop stays the same
if (g_map_renderer) {
    g_map_renderer->HandleInput();
    g_map_renderer->Render();
}
```

**Estimated Effort**: 1 hour

---

### Phase 6: Testing & Optimization (Days 6-7)

#### Step 6.1: Test with Current Map (133 Provinces)
**Checklist**:
- [ ] Map renders correctly
- [ ] Province colors match original
- [ ] Province boundaries are crisp
- [ ] Mouse picking works (click province → select)
- [ ] Zoom in/out smooth
- [ ] Performance: <1ms render time

**Estimated Effort**: 2-3 hours

---

#### Step 6.2: Create Large Test Map (1,000-5,000 Provinces)
**Approaches**:
1. Subdivide existing provinces (split each into 10-50 sub-provinces)
2. Generate synthetic grid map
3. Download larger GeoJSON (world map with detailed regions)

**Test Performance**:
- [ ] Render time still <1ms
- [ ] Texture baking completes in reasonable time (<5 minutes)
- [ ] No visual artifacts
- [ ] Province picking still works

**Estimated Effort**: 3-4 hours

---

#### Step 6.3: Optimize Tactical Detail Loading
**Profile**:
- How many provinces visible at max zoom?
- Detail generation time per province
- Memory usage for loaded details

**Optimize**:
- Cache recently used details
- Async generation on background thread
- LOD for detail features (fewer trees at medium zoom)

**Estimated Effort**: 4-6 hours

---

## File Structure

### New Files to Create
```
tools/
  ├── bake_province_texture.py          # Offline texture baker
  └── validate_province_texture.py      # Texture validation

include/map/render/
  ├── TextureMapRenderer.h              # Main renderer
  ├── DetailedTerrain.h                 # Tactical detail structures
  └── TerrainDetailManager.h            # Detail loading/rendering

src/rendering/
  ├── TextureMapRenderer.cpp            # Main renderer impl
  ├── DetailedTerrain.cpp               # Terrain generation
  └── TerrainDetailManager.cpp          # Detail management

data/textures/
  └── province_id_map.png               # Generated texture (4096x4096)
```

### Files to Modify
```
CMakeLists.txt                          # Update source files, remove earcut
apps/main.cpp                           # Use TextureMapRenderer
```

### Files to Delete (After Testing)
```
include/map/render/GPUMapRenderer.h
include/map/render/RenderingManager.h
include/map/render/Camera2D.h           # Can keep if reused
src/rendering/GPUMapRenderer.cpp
src/rendering/RenderingManager.cpp
```

---

## Estimated Total Effort

| Phase | Tasks | Hours | Days |
|-------|-------|-------|------|
| 1. Offline Tools | Province texture baker + validation | 6-8 | 1 |
| 2. Core Renderer | TextureMapRenderer + shaders | 9-11 | 1.5 |
| 3. Tactical Detail | DetailedTerrain + generation | 7-10 | 1.5 |
| 4. Seamless Zoom | Transition logic + blending | 2-3 | 0.5 |
| 5. Integration | CMake + main.cpp updates | 1.5 | 0.5 |
| 6. Testing | Current map + large map + optimization | 9-13 | 1.5 |
| **TOTAL** | | **34-46** | **5-7** |

---

## Risk Mitigation

### Risk 1: Province Texture Quality
**Problem**: Rasterized boundaries might look pixelated
**Mitigation**:
- Use high-res texture (8192x8192 if needed)
- Test multiple resolutions
- Consider SDF (signed distance field) for ultra-crisp borders

### Risk 2: Province ID Encoding Limits
**Problem**: RGB encoding maxes out at 16.7M provinces, might need more
**Mitigation**:
- Use RGBA encoding (4.3 billion provinces)
- Current RGB encoding handles any realistic map size

### Risk 3: Tactical Detail Generation Slow
**Problem**: Generating forests/rivers might lag when zooming in
**Mitigation**:
- Pre-generate and cache on first zoom
- Use background thread for generation
- Simple procedural algorithms (fast)

### Risk 4: Memory Usage with Large Maps
**Problem**: 10,000 provinces with details could use lots of RAM
**Mitigation**:
- Only load details for visible provinces (~20 max)
- Unload details when zoomed out
- Benchmark: 20 provinces × 1MB each = 20MB (acceptable)

---

## Success Criteria

### Minimum Viable Product (MVP)
- [ ] Renders 133 province map correctly
- [ ] Province selection works
- [ ] Zoom in/out smooth (no pop-in)
- [ ] Performance: <2ms render time
- [ ] Code: <500 lines total (vs 1,612 current)

### Full Feature Set
- [ ] Handles 5,000+ provinces with <2ms render time
- [ ] Tactical details (forests, hills, rivers) at high zoom
- [ ] Seamless transition from strategic → tactical
- [ ] Mouse picking accurate at all zoom levels
- [ ] Memory usage: <100MB for large maps

### Stretch Goals
- [ ] Multiple texture resolutions (LOD for textures)
- [ ] Province border rendering (outline shader)
- [ ] Animated rivers (flow shader effect)
- [ ] Dynamic province color updates (live game state changes)

---

## Migration Path

### Parallel Development (Recommended)
1. Keep old GPU renderer working
2. Build new TextureMapRenderer alongside
3. Add runtime toggle to switch between renderers
4. Test both side-by-side
5. Once validated, delete old renderer

### Clean Break (Risky)
1. Delete old GPU renderer immediately
2. Build new TextureMapRenderer
3. Fix issues as they arise
4. No fallback option

**Recommendation**: Parallel development for safety.

---

## Next Steps

**Immediate** (This Week):
1. Create `bake_province_texture.py` tool
2. Generate texture from current map
3. Validate texture quality

**Short-term** (Next Week):
1. Implement TextureMapRenderer core
2. Test with 133 provinces
3. Verify performance

**Medium-term** (Week 3):
1. Add tactical detail system
2. Test with large maps (1,000-5,000 provinces)
3. Optimize and polish

---

**Ready to start?** Begin with Step 1.1 (Province Texture Baker).
