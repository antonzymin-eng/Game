# GPU Renderer Integration Plan

**Date**: January 30, 2026
**Status**: Planning
**Estimated Time**: 2 days
**Branch**: `claude/gpu-renderer-integration-plan-71TI2`

---

## Executive Summary

A complete GPU renderer already exists in branch `claude/fix-map-rendering-WZB3K`. This plan focuses on:

1. **Day 1**: Merge existing GPU renderer and verify functionality
2. **Day 2**: Benchmark both renderers with scientific methodology + make data-driven decision

---

## Existing GPU Renderer (Branch: `claude/fix-map-rendering-WZB3K`)

### Already Implemented

| Component | File | Status |
|-----------|------|--------|
| GPUMapRenderer class | `include/map/render/GPUMapRenderer.h` | ✅ Complete |
| Implementation | `src/rendering/GPUMapRenderer.cpp` | ✅ Complete |
| Province fill shader | `shaders/map.vert`, `shaders/map.frag` | ✅ Complete |
| Border shader | `shaders/border.vert`, `shaders/border.frag` | ✅ Complete |
| Multi-LOD system | 3 LOD levels with index buffers | ✅ Complete |
| Province textures | Color + metadata textures | ✅ Complete |
| Triangulation | Using mapbox/earcut.hpp | ✅ Complete |

### Features Already Available

```cpp
class GPUMapRenderer {
    // Render modes
    enum class RenderMode {
        POLITICAL,    // Province colors by owner
        TERRAIN,      // Colors by terrain type
        TRADE,        // Colors by trade network
        RELIGION,     // Colors by dominant religion
        CULTURE       // Colors by culture group
    };

    // Selection/hover highlighting
    void SetSelectedProvince(uint32_t province_id);
    void SetHoveredProvince(uint32_t province_id);

    // Multi-LOD rendering
    static constexpr int LOD_COUNT = 3;
    GLuint lod_ibos_[LOD_COUNT];

    // Statistics
    size_t GetVertexCount();
    size_t GetCurrentTriangleCount();
    int GetCurrentLODLevel();
    float GetLastRenderTime();
};
```

### Known Issues (from `docs/GPU_RENDERER_FIX_PLAN.md`)

| Issue | Status | Description |
|-------|--------|-------------|
| TerrainType enum mismatch | ✅ Fixed | Updated to match actual enum values |
| Missing `<chrono>` header | ✅ Fixed | Added to includes |
| Manual matrix math | ✅ Fixed | Now uses GLM |
| No OpenGL error checking | ✅ Fixed | Added CHECK_GL_ERROR macro |
| No triangulation validation | ✅ Fixed | Added empty result check |
| Memory inefficiency | ✅ Fixed | Using std::move |

### Commit History

```
6d85132 fix: Fix 4 issues from critique of commit a2f10e1
a2f10e1 fix: Fix 2 minor issues from critique of commit 1e7ef91
1e7ef91 fix: Fix 5 issues from critique of commit f5077ce
f5077ce fix: Fix 5 critical issues from critique of commit 8a547b9
8a547b9 fix: Add comprehensive validation and statistics to LOD generation
f61b2c5 fix: Fix 5 critical validation and debugging issues in LOD system
04d994e fix: Implement proper shared-vertex LOD system
88a7b0a fix: Fix 4 critical multi-LOD implementation issues
a1bf8bb fix: Comprehensive GPU renderer fixes - all 8 critical issues resolved
7fc321a fix: Implement GPU renderer fixes and integration
aa504a0 feat: Add GPU-accelerated OpenGL map renderer prototype
```

---

## Day 1: Merge & Verify GPU Renderer (4-6 hours)

### Objective
Merge the existing GPU renderer from `claude/fix-map-rendering-WZB3K` into main and verify it works correctly.

### Tasks

#### Task 1: Review Changes (1 hour)
```bash
# Compare branches
git diff origin/main origin/claude/fix-map-rendering-WZB3K --stat

# Review GPU renderer files specifically
git show origin/claude/fix-map-rendering-WZB3K:include/map/render/GPUMapRenderer.h
git show origin/claude/fix-map-rendering-WZB3K:src/rendering/GPUMapRenderer.cpp
```

#### Task 2: Merge Branch (30 minutes)
```bash
# Create integration branch from main
git checkout -b integrate-gpu-renderer origin/main

# Merge GPU renderer branch
git merge origin/claude/fix-map-rendering-WZB3K

# Resolve any conflicts
# Build and test
```

#### Task 3: Build Verification (1 hour)
```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run existing tests
ctest --output-on-failure
```

#### Task 4: Manual Testing (2 hours)
- [ ] Launch game with GPU renderer enabled
- [ ] Verify provinces render correctly
- [ ] Test all render modes (Political, Terrain, Trade, Religion, Culture)
- [ ] Test province selection highlighting
- [ ] Test province hover highlighting
- [ ] Test LOD transitions (zoom in/out)
- [ ] Test camera panning
- [ ] Check for visual artifacts or glitches
- [ ] Verify no crashes on startup/shutdown

#### Task 4b: Visual Regression Testing (1 hour)

Capture screenshots with both renderers and compare:

```cpp
// Screenshot capture utility
void CaptureScreenshot(const std::string& filename) {
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    std::vector<uint8_t> pixels(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Flip vertically (OpenGL origin is bottom-left)
    // Save as PNG using stb_image_write or similar
    stbi_flip_vertically_on_write(1);
    stbi_write_png(filename.c_str(), width, height, 4, pixels.data(), width * 4);
}
```

**Test scenarios to capture:**

| Scenario | Camera Position | Zoom | Filename |
|----------|-----------------|------|----------|
| Overview | (0, 0) | 0.1 | `test_overview_{renderer}.png` |
| Provincial | (500, 300) | 0.5 | `test_provincial_{renderer}.png` |
| Zoomed | (200, 150) | 0.9 | `test_zoomed_{renderer}.png` |
| Selection | Selected province | 0.5 | `test_selection_{renderer}.png` |
| Edge case | Province at screen edge | 0.5 | `test_edge_{renderer}.png` |

**Comparison script:**
```python
#!/usr/bin/env python3
from PIL import Image
import numpy as np

def compare_images(img1_path, img2_path, threshold=0.01):
    img1 = np.array(Image.open(img1_path))
    img2 = np.array(Image.open(img2_path))

    if img1.shape != img2.shape:
        return False, "Size mismatch"

    diff = np.abs(img1.astype(float) - img2.astype(float))
    diff_ratio = np.sum(diff > 10) / diff.size  # Pixels with >10 difference

    if diff_ratio > threshold:
        # Save diff image for debugging
        diff_img = Image.fromarray((diff * 10).astype(np.uint8))
        diff_img.save(f"diff_{img1_path}")
        return False, f"Diff ratio: {diff_ratio:.2%}"

    return True, f"Match within {threshold:.1%} tolerance"

# Compare all test scenarios
scenarios = ['overview', 'provincial', 'zoomed', 'selection', 'edge']
for s in scenarios:
    ok, msg = compare_images(f'test_{s}_imgui.png', f'test_{s}_gpu.png')
    print(f"{s}: {'PASS' if ok else 'FAIL'} - {msg}")
```

**Acceptable differences:**
- Anti-aliasing variations (ImGui vs GPU may differ slightly)
- Sub-pixel rendering differences
- Selection glow intensity (if shader-based)

**Unacceptable differences:**
- Missing provinces
- Wrong colors
- Misaligned geometry
- Z-order issues (features behind provinces)

#### Task 5: Add Runtime Toggle (1 hour)
If not already present, add ability to switch renderers at runtime:
```cpp
// Debug menu option
if (ImGui::MenuItem("Use GPU Renderer", nullptr, use_gpu_renderer_)) {
    use_gpu_renderer_ = !use_gpu_renderer_;
}
```

### Day 1 Deliverables

| Deliverable | Verification |
|-------------|--------------|
| GPU renderer merged | Branch merged cleanly |
| Build succeeds | `make` completes without errors |
| Tests pass | `ctest` all green |
| Manual testing | All checklist items verified |
| Runtime toggle | Can switch ImGui ↔ GPU at runtime |

---

## Day 2: Benchmark & Decision (6-8 hours)

### Objective
Create rigorous benchmarks comparing ImGui vs GPU renderer and make a data-driven decision.

### Task 1: Create Benchmark Framework (2 hours)

**File**: `tests/performance/benchmark_rendering.cpp`

```cpp
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <fstream>

struct BenchmarkResult {
    std::string renderer_name;
    int province_count;
    int lod_level;

    // Timing stats (milliseconds)
    double min_time;
    double max_time;
    double avg_time;
    double median_time;
    double p95_time;
    double p99_time;
    double std_dev;

    // Throughput
    int total_triangles;
    int draw_calls;
    double triangles_per_ms;

    // Memory metrics
    size_t vbo_memory_bytes;      // Vertex buffer size
    size_t ibo_memory_bytes;      // Index buffer size
    size_t texture_memory_bytes;  // Province color/metadata textures
    size_t total_gpu_memory;      // Total GPU allocation
    size_t cpu_geometry_bytes;    // CPU-side cached geometry
};

class RenderBenchmark {
public:
    static constexpr int WARMUP_FRAMES = 100;
    static constexpr int BENCHMARK_FRAMES = 1000;

    BenchmarkResult RunBenchmark(
        MapRenderer& renderer,
        bool use_gpu,
        const std::string& name,
        int province_count,
        int lod_level
    );

    void ExportCSV(const std::vector<BenchmarkResult>& results,
                   const std::string& filename);
};
```

### Task 2: Define Test Scenarios (30 minutes)

| Scenario | Provinces | LOD | Camera Zoom | Purpose |
|----------|-----------|-----|-------------|---------|
| small_lod0 | 100 | 0 | 0.1 | Baseline strategic view |
| small_lod2 | 100 | 2 | 0.5 | Baseline provincial view |
| medium_lod0 | 500 | 0 | 0.1 | Medium strategic |
| medium_lod2 | 500 | 2 | 0.5 | Medium provincial |
| large_lod0 | 2000 | 0 | 0.1 | Target scale strategic |
| large_lod2 | 2000 | 2 | 0.5 | Target scale provincial |
| large_lod4 | 2000 | 4 | 0.9 | Target scale tactical |
| stress_5k | 5000 | 2 | 0.5 | Stress test |
| panning | 2000 | 2 | 0.5 | Camera movement simulation |

### Task 3: Run Benchmarks (2 hours)

```bash
# Build benchmark executable
cmake .. -DCMAKE_BUILD_TYPE=Release
make benchmark_rendering

# Run benchmarks
./benchmark_rendering --output benchmark_results.csv
```

### Task 4: Analyze Results (1 hour)

**Script**: `scripts/analyze_benchmark.py`
```python
#!/usr/bin/env python3
import pandas as pd

def analyze(csv_path):
    df = pd.read_csv(csv_path)

    imgui = df[df['renderer_name'].str.contains('ImGui')]
    gpu = df[df['renderer_name'].str.contains('GPU')]

    # Calculate speedup
    merged = pd.merge(imgui, gpu, on=['province_count', 'lod_level'],
                      suffixes=('_imgui', '_gpu'))
    merged['speedup'] = merged['avg_time_imgui'] / merged['avg_time_gpu']

    print("=== GPU vs ImGui Speedup ===")
    print(merged[['province_count', 'lod_level', 'speedup']])
    print(f"\nAverage speedup: {merged['speedup'].mean():.2f}x")
    print(f"Speedup at 2000 provinces: {merged[merged['province_count']==2000]['speedup'].mean():.2f}x")

    return merged

if __name__ == '__main__':
    import sys
    analyze(sys.argv[1] if len(sys.argv) > 1 else 'benchmark_results.csv')
```

### Task 5: Memory Profiling (1 hour)

#### GPU Memory Measurement

```cpp
// Add to GPUMapRenderer for memory tracking
struct MemoryStats {
    size_t vbo_bytes;
    size_t ibo_bytes[LOD_COUNT];
    size_t color_texture_bytes;
    size_t metadata_texture_bytes;

    size_t GetTotalGPU() const {
        size_t total = vbo_bytes + color_texture_bytes + metadata_texture_bytes;
        for (int i = 0; i < LOD_COUNT; ++i) total += ibo_bytes[i];
        return total;
    }
};

MemoryStats GPUMapRenderer::GetMemoryStats() const {
    MemoryStats stats;
    stats.vbo_bytes = vertex_count_ * sizeof(ProvinceVertex);
    for (int i = 0; i < LOD_COUNT; ++i) {
        stats.ibo_bytes[i] = lod_index_counts_[i] * sizeof(uint32_t);
    }
    // Province textures: 256x256 RGBA8 = 256KB each
    stats.color_texture_bytes = 256 * 256 * 4;
    stats.metadata_texture_bytes = 256 * 256 * 4;
    return stats;
}
```

#### ImGui Memory Estimation

ImGui rebuilds geometry each frame, so measure peak allocation:
```cpp
// After ImGui render, check draw data
ImDrawData* draw_data = ImGui::GetDrawData();
size_t imgui_vertex_bytes = draw_data->TotalVtxCount * sizeof(ImDrawVert);
size_t imgui_index_bytes = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
```

#### Memory Comparison Table

| Metric | ImGui | GPU | Notes |
|--------|-------|-----|-------|
| Vertex buffer | Per-frame | Persistent | GPU uploads once |
| Index buffer | Per-frame | Persistent (3 LODs) | GPU has 3x for LOD |
| Textures | None | 512 KB | Province color/metadata |
| CPU geometry cache | None | ~X MB | Cached for rebuild |
| **Total GPU** | ~Y MB/frame | ~Z MB static | |

---

### Task 6: Make Decision (1 hour)

#### Decision Matrix

| Condition | Decision |
|-----------|----------|
| GPU ≥2x faster at 500+ provinces | **Adopt GPU as default** |
| GPU 20-100% faster at 1000+ provinces | **GPU as optional "performance mode"** |
| GPU <20% faster or inconsistent | **Keep ImGui, archive GPU code** |
| GPU slower than ImGui | **Delete GPU code, document why** |

#### Decision Documentation Template

```markdown
# GPU Renderer Evaluation Results

**Date**: [Date]
**Decision**: [A/B/C/D]

## Benchmark Summary
| Provinces | ImGui (ms) | GPU (ms) | Speedup |
|-----------|------------|----------|---------|
| 100       | X.XX       | X.XX     | X.Xx    |
| 500       | X.XX       | X.XX     | X.Xx    |
| 2000      | X.XX       | X.XX     | X.Xx    |
| 5000      | X.XX       | X.XX     | X.Xx    |

## Recommendation
[Reasoning based on data]

## Next Steps
1. [Action]
2. [Action]
```

### Day 2 Deliverables

| Deliverable | Description |
|-------------|-------------|
| `benchmark_rendering.cpp` | Benchmark framework |
| `benchmark_results.csv` | Raw benchmark data |
| `analyze_benchmark.py` | Analysis script |
| Decision document | Evidence-based recommendation |

---

## Hybrid Approach Considerations

A hybrid approach uses GPU for province polygon fills while keeping ImGui for text, icons, and UI. This may offer the best of both worlds.

### What GPU Does Well

| Task | Why GPU Excels |
|------|----------------|
| Province polygon fills | Thousands of triangles, static geometry, single draw call |
| Province borders | Line rendering with consistent width |
| Selection highlighting | Shader-based glow effects, no CPU cost |
| Color mode switching | Just update uniform, no geometry rebuild |

### What ImGui Does Well

| Task | Why ImGui Excels |
|------|------------------|
| Text rendering | Built-in font atlas, kerning, UTF-8 support |
| Icons/sprites | Simple API, automatic batching |
| UI overlays | Tooltips, menus, windows - ImGui's core strength |
| Rapid iteration | No shader recompilation needed |

### Hybrid Architecture

```
Frame Render Order:
┌─────────────────────────────────────────────┐
│ 1. GPUMapRenderer::Render()                 │
│    └─ Province fills (single glDrawElements)│
│    └─ Province borders (if GPU borders)     │
├─────────────────────────────────────────────┤
│ 2. ImGui Background DrawList                │
│    └─ Feature icons (cities, mountains)     │
│    └─ Army unit sprites                     │
│    └─ Road lines                            │
├─────────────────────────────────────────────┤
│ 3. ImGui Foreground DrawList                │
│    └─ Province names (text)                 │
│    └─ Selection overlay effects             │
│    └─ Tooltips                              │
├─────────────────────────────────────────────┤
│ 4. ImGui UI Windows                         │
│    └─ All game UI (menus, panels, etc.)     │
└─────────────────────────────────────────────┘
```

### Hybrid Implementation

```cpp
void MapRenderer::Render() {
    // GPU: Province geometry (the heavy part)
    if (gpu_renderer_) {
        gpu_renderer_->Render(camera_);
    } else {
        RenderProvincesImGui();  // Fallback
    }

    // ImGui: Everything else (lightweight)
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    RenderFeatures(draw_list);   // Cities, mountains, forests
    RenderRoads(draw_list);      // Road network
    RenderUnits(draw_list);      // Army sprites
    RenderNames(draw_list);      // Province text labels
}
```

### Hybrid Considerations

#### Advantages

| Benefit | Explanation |
|---------|-------------|
| **Best performance** | GPU handles the heavy polygon work |
| **Easy text** | No need to implement GPU font rendering |
| **Incremental adoption** | Can migrate features one at a time |
| **Fallback safety** | ImGui path remains for compatibility |
| **Simpler shaders** | Only need polygon fill, not full rendering |

#### Challenges

| Challenge | Mitigation |
|-----------|------------|
| **Z-ordering** | GPU renders first, ImGui on top - usually correct |
| **Coordinate sync** | Both use same Camera2D, should align |
| **Selection highlight** | GPU does fill highlight, ImGui does outline? Or pick one |
| **Two render paths** | More code to maintain, but clear separation |
| **Depth buffer** | May need to clear between GPU and ImGui passes |

#### Z-Order / Layering Concerns

```
Desired layer order (back to front):
1. Province fills (GPU)
2. Province borders (GPU or ImGui?)
3. Roads (ImGui)
4. Rivers (ImGui)
5. Feature icons (ImGui)
6. Unit sprites (ImGui)
7. Province names (ImGui)
8. Selection glow (GPU shader or ImGui?)
9. Tooltips (ImGui)
```

**Border rendering decision:**
- GPU borders: Consistent with fills, single draw call
- ImGui borders: Easier to style, can be dashed/animated

**Selection rendering decision:**
- GPU selection: Shader-based glow, smooth animation
- ImGui selection: Simpler, can draw outline on top

#### Coordinate System Alignment

Both renderers must use identical world-to-screen transform:
```cpp
// GPU uses this matrix
glm::mat4 vp = glm::ortho(left, right, bottom, top);

// ImGui uses Camera2D::WorldToScreen()
// MUST produce identical results!

// Test: render same point with both, verify pixel-perfect alignment
Vector2 test_world(100.0f, 200.0f);
Vector2 imgui_screen = camera.WorldToScreen(test_world);
// GPU should render to same pixel
```

### Hybrid Decision Matrix

| Scenario | Recommendation |
|----------|----------------|
| GPU 2x+ faster for fills | **Hybrid**: GPU fills + ImGui everything else |
| GPU borders also faster | **Hybrid+**: GPU fills & borders + ImGui text/icons |
| GPU text rendering needed | **Full GPU**: But significant extra work |
| ImGui fast enough | **ImGui only**: Simpler, less maintenance |

### Hybrid Test Checklist

- [ ] Province fills render correctly with GPU
- [ ] ImGui features render on top of GPU provinces
- [ ] No gaps or overlaps at province edges
- [ ] Selection works (decide GPU vs ImGui approach)
- [ ] Text labels align correctly with province centers
- [ ] Camera zoom/pan keeps everything synchronized
- [ ] No z-fighting or flicker between layers
- [ ] Performance better than pure ImGui

---

## Quick Reference

### Branch Information
```bash
# Existing GPU renderer branch
origin/claude/fix-map-rendering-WZB3K

# Key files in that branch
include/map/render/GPUMapRenderer.h
src/rendering/GPUMapRenderer.cpp
shaders/map.vert
shaders/map.frag
shaders/border.vert
shaders/border.frag
```

### GPU Renderer Statistics (Available)
```cpp
gpu_renderer->GetVertexCount();        // Total vertices
gpu_renderer->GetCurrentTriangleCount(); // Triangles this frame
gpu_renderer->GetCurrentLODLevel();    // Current LOD (0-2)
gpu_renderer->GetLastRenderTime();     // Frame time in ms
gpu_renderer->GetProvinceCount();      // Total provinces
```

---

## Success Criteria

1. **Day 1**: GPU renderer merged, builds, runs, and can be toggled at runtime
2. **Day 2**: Benchmark data collected for all scenarios with documented decision

---

*Document Status: Ready for Execution*
*Created: January 30, 2026*
*Updated: January 30, 2026 - Revised to use existing GPU renderer from unmerged branch*
