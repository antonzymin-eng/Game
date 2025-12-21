# GPU Map Renderer - Fix and Implementation Plan

**Date**: December 21, 2025
**Status**: Planning Phase
**Branch**: `claude/fix-map-rendering-WZB3K`

---

## Issues Identified

### ✅ Issue 1: TerrainType Enum Mismatch
**Status**: VERIFIED - Needs fix

**Current Code (WRONG):**
```cpp
// GPUMapRenderer.cpp:754-760
if (province->terrain_type == TerrainType::PLAINS) terrain_value = 10;
else if (province->terrain_type == TerrainType::FOREST) terrain_value = 20;
else if (province->terrain_type == TerrainType::MOUNTAINS) terrain_value = 30;
else if (province->terrain_type == TerrainType::DESERT) terrain_value = 40;
else if (province->terrain_type == TerrainType::SNOW) terrain_value = 50;  // ❌ DOESN'T EXIST
else if (province->terrain_type == TerrainType::WATER) terrain_value = 60; // ❌ DOESN'T EXIST
```

**Actual Enum (from MapData.h:65-75):**
```cpp
enum class TerrainType : uint8_t {
    PLAINS = 0,
    HILLS,
    MOUNTAINS,
    FOREST,
    DESERT,
    COAST,
    WETLAND,
    HIGHLANDS,
    UNKNOWN
};
```

**Fix**: Update to match actual enum values

---

### ✅ Issue 2: Missing Headers
**Status**: CONFIRMED

**Missing**:
```cpp
#include <chrono>  // Used at line 788 for performance timing
```

**Fix**: Add to includes section

---

### ✅ Issue 3: GLM Not Used
**Status**: CONFIRMED

**Problem**: GLM is added as dependency but manual matrix math is used instead

**Current (lines 813-823):**
```cpp
float projection[16] = {
    2.0f / (right - left), 0.0f, 0.0f, 0.0f,
    0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0.0f, 1.0f
};
```

**Fix**: Use GLM's `glm::ortho()` and `glm::value_ptr()`

---

### ✅ Issue 4: No OpenGL Error Checking
**Status**: CONFIRMED

**Problem**: OpenGL calls have no error checking

**Fix**: Add error checking macro and use throughout

---

### ✅ Issue 5: No Triangulation Validation
**Status**: CONFIRMED

**Current (line 716):**
```cpp
std::vector<uint32_t> local_indices = mapbox::earcut<uint32_t>(polygon_rings);
// No validation!
```

**Fix**: Check if result is empty and log warning

---

### ✅ Issue 6: Memory Inefficiency
**Status**: CONFIRMED

**Problem**: Large vectors are copied instead of moved

**Fix**: Use `std::move()` and reserve capacity

---

### ✅ Issue 7: Shader Path Resolution
**Status**: NEEDS IMPROVEMENT

**Current**: Hardcoded path attempts in `ReadShaderFile()`

**Fix**: Use proper path resolution relative to executable

---

### ✅ Issue 8: Texture Size Limits
**Status**: WEAK ERROR HANDLING

**Current (line 743):**
```cpp
if (id >= 256 * 256) {
    CORE_LOG_WARN("GPUMapRenderer", "Province ID " << id << " exceeds texture size (65536)");
    continue; // Silently skip
}
```

**Fix**: Better validation and reporting

---

### ✅ Issue 9: Multi-LOD Not Implemented
**Status**: PROMISED BUT MISSING

**Problem**: Documentation mentions multi-LOD but only single index buffer exists

**Fix**: Implement LOD system with multiple index buffers per zoom level

---

### ✅ Issue 10: CMakeLists Duplicate
**Status**: CONFIRMED (but user may have fixed)

**Problem**: `include(FetchContent)` at line 288, but already included at line 163

**Fix**: Check if still present, remove if so

---

### ✅ Issue 11: Not Integrated into main.cpp
**Status**: INTENTIONAL (but needs fixing)

**Problem**: GPU renderer compiled but not used

**Fix**: Add toggle and integrate rendering path

---

### ✅ Issue 12: No Baseline Performance Measurement
**Status**: NEEDED

**Problem**: Can't validate speedup claims without baseline

**Fix**: Add performance timing to existing MapRenderer

---

## Implementation Plan

### Phase 1: Fix Critical Issues (30 minutes)
**Priority: HIGH - These break compilation or runtime**

1. ✅ Fix TerrainType enum mapping
2. ✅ Add missing `#include <chrono>`
3. ✅ Fix CMakeLists.txt duplicate FetchContent (if present)
4. ✅ Add OpenGL error checking macro
5. ✅ Add triangulation validation

**Expected Outcome**: Code compiles and runs without crashes

---

### Phase 2: Improve Code Quality (45 minutes)
**Priority: MEDIUM - Improves robustness**

6. ✅ Replace manual matrix math with GLM
7. ✅ Fix memory efficiency (std::move)
8. ✅ Improve shader path resolution
9. ✅ Better texture size error handling

**Expected Outcome**: Code is production-quality

---

### Phase 3: Implement Multi-LOD (60 minutes)
**Priority: MEDIUM - Delivers on promise**

10. ✅ Design LOD index buffer structure
11. ✅ Generate LOD levels during upload
12. ✅ Switch index buffers based on zoom
13. ✅ Test LOD transitions

**Expected Outcome**: Smooth LOD transitions, better performance

---

### Phase 4: Integration (45 minutes)
**Priority: HIGH - Makes it usable**

14. ✅ Add performance measurement to existing MapRenderer
15. ✅ Add GPU renderer toggle in main.cpp
16. ✅ Wire up selection/hover events
17. ✅ Add UI controls for render mode

**Expected Outcome**: GPU renderer is usable in-game

---

### Phase 5: Testing & Validation (60 minutes)
**Priority: CRITICAL - Ensures correctness**

18. ✅ Test with 100, 500, 1000 provinces
19. ✅ Measure actual performance vs baseline
20. ✅ Visual comparison (screenshot diff)
21. ✅ Test all render modes
22. ✅ Test selection/hover
23. ✅ Test LOD transitions

**Expected Outcome**: Validated performance claims, no regressions

---

## Detailed Implementation Steps

### Step 1: Fix TerrainType Enum

**File**: `src/rendering/GPUMapRenderer.cpp:754-760`

**Change**:
```cpp
// BEFORE:
if (province->terrain_type == TerrainType::SNOW) terrain_value = 50;
else if (province->terrain_type == TerrainType::WATER) terrain_value = 60;

// AFTER:
else if (province->terrain_type == TerrainType::COAST) terrain_value = 50;
else if (province->terrain_type == TerrainType::WETLAND) terrain_value = 60;
else if (province->terrain_type == TerrainType::HIGHLANDS) terrain_value = 70;
else if (province->terrain_type == TerrainType::HILLS) terrain_value = 80;
else if (province->terrain_type == TerrainType::UNKNOWN) terrain_value = 0;
```

**Validation**: Compile without errors

---

### Step 2: Add Missing Headers

**File**: `src/rendering/GPUMapRenderer.cpp:1-15`

**Change**:
```cpp
// Add after existing includes:
#include <chrono>
```

**Validation**: Compile without warnings

---

### Step 3: Use GLM for Matrix Math

**File**: `src/rendering/GPUMapRenderer.cpp:813-823`

**Change**:
```cpp
// BEFORE:
void GPUMapRenderer::CalculateViewProjectionMatrix(const Camera2D& camera, float* matrix_out) {
    float half_width = (camera.viewport_width / camera.zoom) / 2.0f;
    float half_height = (camera.viewport_height / camera.zoom) / 2.0f;

    float left = camera.position.x - half_width;
    float right = camera.position.x + half_width;
    float bottom = camera.position.y - half_height;
    float top = camera.position.y + half_height;

    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0.0f, 1.0f
    };

    for (int i = 0; i < 16; ++i) {
        matrix_out[i] = projection[i];
    }
}

// AFTER:
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void GPUMapRenderer::CalculateViewProjectionMatrix(const Camera2D& camera, float* matrix_out) {
    float half_width = (camera.viewport_width / camera.zoom) / 2.0f;
    float half_height = (camera.viewport_height / camera.zoom) / 2.0f;

    float left = camera.position.x - half_width;
    float right = camera.position.x + half_width;
    float bottom = camera.position.y - half_height;
    float top = camera.position.y + half_height;

    glm::mat4 projection = glm::ortho(left, right, bottom, top);

    // Copy to output
    const float* matrix_ptr = glm::value_ptr(projection);
    for (int i = 0; i < 16; ++i) {
        matrix_out[i] = matrix_ptr[i];
    }
}
```

**Validation**: Test rendering, verify no visual changes

---

### Step 4: Add OpenGL Error Checking

**File**: `src/rendering/GPUMapRenderer.cpp`

**Add macro after includes:**
```cpp
#ifdef _DEBUG
#define CHECK_GL_ERROR() \
    { \
        GLenum err; \
        while ((err = glGetError()) != GL_NO_ERROR) { \
            CORE_LOG_ERROR("OpenGL", "Error 0x" << std::hex << err << " at " << __FILE__ << ":" << __LINE__); \
        } \
    }
#else
#define CHECK_GL_ERROR() // No-op in release
#endif
```

**Add checks after critical GL calls:**
```cpp
glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(index_count_), GL_UNSIGNED_INT, nullptr);
CHECK_GL_ERROR();

glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(ProvinceVertex), vertices.data(), GL_STATIC_DRAW);
CHECK_GL_ERROR();
```

**Validation**: Run in debug mode, check for GL errors

---

### Step 5: Add Triangulation Validation

**File**: `src/rendering/GPUMapRenderer.cpp:716`

**Change**:
```cpp
// BEFORE:
std::vector<uint32_t> local_indices = mapbox::earcut<uint32_t>(polygon_rings);

// AFTER:
std::vector<uint32_t> local_indices = mapbox::earcut<uint32_t>(polygon_rings);

if (local_indices.empty()) {
    CORE_LOG_WARN("GPUMapRenderer", "Triangulation failed for province "
                  << province->province_id << " (name: " << province->name << ")");
    CORE_LOG_WARN("GPUMapRenderer", "  Boundary points: " << province->boundary_points.size());
    continue; // Skip this province
}

// Validate triangle count is reasonable
if (local_indices.size() < 3 || local_indices.size() % 3 != 0) {
    CORE_LOG_ERROR("GPUMapRenderer", "Invalid triangle count for province "
                   << province->province_id << ": " << local_indices.size());
    continue;
}
```

**Validation**: Load provinces, check logs for warnings

---

### Step 6: Fix Memory Efficiency

**File**: `src/rendering/GPUMapRenderer.cpp:708-730`

**Change**:
```cpp
void GPUMapRenderer::TriangulateProvinces(...) {
    // Reserve capacity upfront
    size_t estimated_vertices = 0;
    size_t estimated_indices = 0;
    for (const auto* province : provinces) {
        if (province) {
            estimated_vertices += province->boundary_points.size();
            estimated_indices += province->boundary_points.size() * 3; // Rough estimate
        }
    }

    vertices.reserve(estimated_vertices);
    indices.reserve(estimated_indices);

    for (const auto* province : provinces) {
        // ... existing code ...

        // Use emplace_back instead of push_back
        for (const auto& pt : province->boundary_points) {
            vertices.emplace_back(ProvinceVertex{
                static_cast<float>(pt.x),
                static_cast<float>(pt.y),
                province->province_id,
                0.0f, 0.0f
            });
        }

        // Move indices instead of copy
        indices.insert(indices.end(), local_indices.begin(), local_indices.end());
    }
}
```

**Validation**: Profile memory usage, verify no copies

---

### Step 7: Improve Shader Path Resolution

**File**: `src/rendering/GPUMapRenderer.cpp:279-299`

**Change**:
```cpp
std::string GPUMapRenderer::ReadShaderFile(const std::string& filepath) {
    // Get executable directory
    std::filesystem::path exe_path = std::filesystem::current_path();

    // Try paths relative to executable
    std::vector<std::filesystem::path> search_paths = {
        exe_path / filepath,                    // ./shaders/map.vert
        exe_path / "bin" / filepath,           // ./bin/shaders/map.vert
        exe_path / ".." / filepath,            // ../shaders/map.vert
        exe_path / ".." / "bin" / filepath,    // ../bin/shaders/map.vert
        exe_path / ".." / ".." / filepath      // ../../shaders/map.vert
    };

    for (const auto& path : search_paths) {
        if (std::filesystem::exists(path)) {
            std::ifstream file(path);
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                CORE_LOG_INFO("GPUMapRenderer", "Loaded shader: " << path.string());
                return buffer.str();
            }
        }
    }

    CORE_LOG_ERROR("GPUMapRenderer", "Failed to find shader: " << filepath);
    CORE_LOG_ERROR("GPUMapRenderer", "Searched paths:");
    for (const auto& path : search_paths) {
        CORE_LOG_ERROR("GPUMapRenderer", "  - " << path.string());
    }

    return "";
}
```

**Add include**:
```cpp
#include <filesystem>
```

**Validation**: Test shader loading from different working directories

---

### Step 8: Implement Multi-LOD System

**File**: `include/map/render/GPUMapRenderer.h`

**Add to class:**
```cpp
private:
    struct LODLevel {
        size_t index_offset;     // Offset into index buffer
        size_t index_count;      // Number of indices for this LOD
        float min_zoom;          // Minimum zoom for this LOD
        float max_zoom;          // Maximum zoom for this LOD
    };

    std::vector<LODLevel> lod_levels_;
    GLuint ibo_lod0_;  // Simplified geometry for far zoom
    GLuint ibo_lod1_;  // Medium detail
    GLuint ibo_lod2_;  // High detail
    // ibo_ is used for maximum detail
```

**Implementation in .cpp**:
```cpp
void GPUMapRenderer::UploadProvinceData(...) {
    // Generate LOD 0 (strategic - very simple)
    std::vector<uint32_t> indices_lod0;
    GenerateLODIndices(provinces, 0, indices_lod0);  // Use simplified boundaries

    // Generate LOD 1 (regional - medium)
    std::vector<uint32_t> indices_lod1;
    GenerateLODIndices(provinces, 1, indices_lod1);

    // Generate LOD 2 (provincial - high)
    std::vector<uint32_t> indices_lod2;
    GenerateLODIndices(provinces, 2, indices_lod2);

    // LOD 3+ use full detail (indices)

    // Upload to separate IBOs
    glGenBuffers(1, &ibo_lod0_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_lod0_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_lod0.size() * sizeof(uint32_t),
                 indices_lod0.data(), GL_STATIC_DRAW);

    // ... repeat for LOD1, LOD2 ...

    // Configure LOD levels
    lod_levels_ = {
        {0, indices_lod0.size(), 0.0f, 0.3f},   // Strategic
        {0, indices_lod1.size(), 0.3f, 0.6f},   // Regional
        {0, indices_lod2.size(), 0.6f, 1.2f},   // Provincial
        {0, index_count_, 1.2f, 100.0f}         // Local/Tactical (full detail)
    };
}

void GPUMapRenderer::Render(const Camera2D& camera) {
    // Select appropriate LOD based on zoom
    GLuint active_ibo = ibo_;
    size_t active_count = index_count_;

    for (const auto& lod : lod_levels_) {
        if (camera.zoom >= lod.min_zoom && camera.zoom < lod.max_zoom) {
            active_ibo = GetIBOForLOD(lod);
            active_count = lod.index_count;
            break;
        }
    }

    // Bind and render
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, active_ibo);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(active_count), GL_UNSIGNED_INT, nullptr);
}
```

**Validation**: Test zoom in/out, verify LOD switches smoothly

---

### Step 9: Integrate into main.cpp

**File**: `apps/main.cpp`

**Add toggle variable (global or in game state):**
```cpp
// After includes, before main()
bool g_use_gpu_renderer = false;  // Toggle via UI or command line
```

**In initialization section (after MapRenderer::Initialize()):**
```cpp
// Initialize GPU renderer
auto gpu_map_renderer = std::make_unique<GPUMapRenderer>(entity_manager);
if (!gpu_map_renderer->Initialize()) {
    CORE_LOG_ERROR("Main", "Failed to initialize GPU map renderer - falling back to ImGui");
    g_use_gpu_renderer = false;
} else {
    CORE_LOG_INFO("Main", "GPU map renderer initialized successfully");

    // Upload province data
    std::vector<const ProvinceRenderComponent*> provinces;
    // ... collect provinces from ECS ...

    if (gpu_map_renderer->UploadProvinceData(provinces)) {
        CORE_LOG_INFO("Main", "Uploaded " << provinces.size() << " provinces to GPU");
        g_use_gpu_renderer = true;  // Enable by default if successful
    }
}
```

**In render loop:**
```cpp
// BEFORE main map rendering:
auto map_render_start = std::chrono::high_resolution_clock::now();

if (g_use_gpu_renderer && gpu_map_renderer) {
    gpu_map_renderer->Render(camera);
} else {
    map_renderer->Render();  // Fallback to ImGui
}

auto map_render_end = std::chrono::high_resolution_clock::now();
float map_render_ms = std::chrono::duration<float, std::milli>(map_render_end - map_render_start).count();

// Display in UI or log
if (performance_window_open) {
    ImGui::Text("Map Render: %.2f ms", map_render_ms);
    ImGui::Checkbox("Use GPU Renderer", &g_use_gpu_renderer);
}
```

**Validation**: Toggle between renderers, verify both work

---

### Step 10: Add Performance Measurement

**File**: `src/rendering/MapRenderer.cpp`

**Add timing to existing Render() method:**
```cpp
void MapRenderer::Render() {
    auto start_time = std::chrono::high_resolution_clock::now();

    // ... existing rendering code ...

    auto end_time = std::chrono::high_resolution_clock::now();
    last_render_time_ms_ = std::chrono::duration<float, std::milli>(end_time - start_time).count();
}
```

**Add member variable to header:**
```cpp
private:
    float last_render_time_ms_ = 0.0f;

public:
    float GetLastRenderTime() const { return last_render_time_ms_; }
```

**Validation**: Log both renderers' times, compare

---

## Testing Checklist

### Compilation Tests
- [ ] Code compiles without errors
- [ ] Code compiles without warnings
- [ ] All dependencies resolve correctly
- [ ] Shaders are copied to build directory

### Runtime Tests
- [ ] GPU renderer initializes without crashes
- [ ] Shaders load successfully
- [ ] Province data uploads without errors
- [ ] Map renders visually correct
- [ ] No OpenGL errors in debug mode

### Performance Tests
- [ ] Measure baseline (ImGui) with 500 provinces
- [ ] Measure GPU renderer with 500 provinces
- [ ] Measure baseline with 1000 provinces
- [ ] Measure GPU renderer with 1000 provinces
- [ ] Calculate actual speedup ratio
- [ ] Verify FPS improvement claims

### Feature Tests
- [ ] Selection highlighting works
- [ ] Hover effects work
- [ ] Multiple render modes work (political, terrain, trade)
- [ ] LOD transitions are smooth
- [ ] Zoom and pan are smooth
- [ ] No visual artifacts or gaps

### Edge Case Tests
- [ ] Province with 3 vertices (triangle)
- [ ] Province with 1000+ vertices (complex shape)
- [ ] Concave province (test triangulation)
- [ ] Self-intersecting province (should fail gracefully)
- [ ] Province ID > 65536 (exceeds texture size)

---

## Success Criteria

### Must Have (Blocking)
- ✅ Code compiles and runs
- ✅ No crashes or GL errors
- ✅ Visual output matches ImGui renderer
- ✅ At least 2x speedup over baseline

### Should Have (Important)
- ✅ 4-8x speedup over baseline
- ✅ Smooth LOD transitions
- ✅ All render modes working
- ✅ Selection/hover working

### Nice to Have (Optional)
- ⚪ 10x speedup
- ⚪ Border rendering implemented
- ⚪ Province names rendering

---

## Rollback Plan

If GPU renderer fails or causes issues:

1. Set `g_use_gpu_renderer = false` in main.cpp
2. Revert to ImGui rendering (already works)
3. Debug issues separately without blocking gameplay

**No breaking changes to existing systems.**

---

**Status**: Plan complete, ready for implementation
**Next**: Begin Phase 1 fixes
