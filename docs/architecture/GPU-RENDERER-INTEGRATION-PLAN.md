# GPU Renderer Integration Plan

**Date**: January 30, 2026
**Status**: Planning
**Estimated Time**: 2 days implementation + decision day
**Branch**: `claude/gpu-renderer-integration-plan-71TI2`

---

## Executive Summary

This plan outlines a data-driven approach to evaluate GPU-accelerated map rendering. The current ImGui DrawList-based renderer works but may have performance limitations at scale. Before investing significant engineering effort, we will:

1. **Day 1**: Create a minimal GPUMapRenderer prototype
2. **Day 2**: Benchmark both renderers with scientific methodology
3. **Day 3**: Make evidence-based decision on whether to proceed

---

## Current State Analysis

### Existing Architecture

| Component | Technology | Status |
|-----------|------------|--------|
| MapRenderer | ImGui DrawList API | ✅ Production |
| Province Rendering | `AddConvexPolyFilled()` | ✅ Working |
| Border Rendering | `AddPolyline()` | ✅ Working |
| Feature Icons | `AddCircleFilled()`, `AddText()` | ✅ Working |
| OpenGL Context | OpenGL 3.2+ (via SDL2) | ✅ Available |
| FBO Support | glGenFramebuffers, etc. | ✅ Loaded |

### Current Performance Instrumentation

```cpp
// Already tracked in MapRenderer:
int rendered_province_count_;      // Provinces rendered per frame
int rendered_feature_count_;       // Features (cities, etc.)
float last_render_time_ms_;        // Frame time in ms

// Already tracked in ViewportCuller:
int visible_province_count_;       // After culling
float GetCullingEfficiency();      // % culled
```

### Performance Targets (from architecture doc)

| LOD Level | Target Render Time | Typical Polygons |
|-----------|-------------------|------------------|
| LOD 0-1   | < 1ms             | 10-200          |
| LOD 2-3   | < 5ms             | 200-2000        |
| LOD 4     | < 16ms (60 FPS)   | 2000-5000+      |

---

## Day 1: GPUMapRenderer Integration (8 hours)

### Objective
Create a minimal GPU-accelerated renderer that can render province polygons using OpenGL shaders, running parallel to the existing ImGui renderer for A/B comparison.

### Hour-by-Hour Breakdown

#### Hours 1-2: Shader Infrastructure

**Files to create:**
- `include/map/render/GPUMapRenderer.h`
- `src/rendering/GPUMapRenderer.cpp`
- `assets/shaders/province.vert`
- `assets/shaders/province.frag`

**Vertex Shader** (`province.vert`):
```glsl
#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uViewProjection;

out vec4 vColor;

void main() {
    gl_Position = uViewProjection * vec4(aPosition, 0.0, 1.0);
    vColor = aColor;
}
```

**Fragment Shader** (`province.frag`):
```glsl
#version 330 core

in vec4 vColor;
out vec4 FragColor;

void main() {
    FragColor = vColor;
}
```

#### Hours 3-4: GPUMapRenderer Class

```cpp
// include/map/render/GPUMapRenderer.h
class GPUMapRenderer {
public:
    GPUMapRenderer(core::ecs::EntityManager& entity_manager);
    ~GPUMapRenderer();

    bool Initialize();
    void Render(const Camera2D& camera);

    // Performance metrics
    float GetLastRenderTime() const { return last_render_time_ms_; }
    int GetDrawCallCount() const { return draw_call_count_; }
    int GetVertexCount() const { return vertex_count_; }
    int GetTriangleCount() const { return triangle_count_; }

private:
    // Shader management
    GLuint shader_program_;
    GLuint vertex_shader_;
    GLuint fragment_shader_;

    // Buffer objects
    GLuint vao_;           // Vertex Array Object
    GLuint vbo_;           // Vertex Buffer Object
    GLuint ebo_;           // Element Buffer Object (indices)

    // Cached province mesh data
    struct ProvinceMesh {
        std::vector<float> vertices;    // x, y, r, g, b, a per vertex
        std::vector<uint32_t> indices;  // Triangle indices
    };
    std::unordered_map<EntityID, ProvinceMesh> province_meshes_;

    // Combined batch buffer
    std::vector<float> batch_vertices_;
    std::vector<uint32_t> batch_indices_;
    bool needs_rebuild_ = true;

    // Statistics
    float last_render_time_ms_ = 0.0f;
    int draw_call_count_ = 0;
    int vertex_count_ = 0;
    int triangle_count_ = 0;

    // Helpers
    bool CompileShader(GLuint shader, const char* source);
    void RebuildBatchBuffers();
    void TriangulatePolygon(const std::vector<Vector2>& boundary,
                            ProvinceMesh& mesh, const Color& color);
};
```

#### Hours 5-6: Polygon Triangulation

Implement ear-clipping triangulation for province polygons:

```cpp
void GPUMapRenderer::TriangulatePolygon(
    const std::vector<Vector2>& boundary,
    ProvinceMesh& mesh,
    const Color& color
) {
    // For convex polygons (most provinces), use fan triangulation
    // For complex polygons, use ear-clipping algorithm

    if (boundary.size() < 3) return;

    // Assume convex for MVP (provinces are mostly convex)
    // Fan triangulation: vertex 0 connects to all other edges
    uint32_t base_index = mesh.vertices.size() / 6; // 6 floats per vertex

    // Add all vertices
    for (const auto& point : boundary) {
        mesh.vertices.push_back(point.x);
        mesh.vertices.push_back(point.y);
        mesh.vertices.push_back(color.r / 255.0f);
        mesh.vertices.push_back(color.g / 255.0f);
        mesh.vertices.push_back(color.b / 255.0f);
        mesh.vertices.push_back(color.a / 255.0f);
    }

    // Fan triangulation
    for (size_t i = 1; i < boundary.size() - 1; ++i) {
        mesh.indices.push_back(base_index);
        mesh.indices.push_back(base_index + i);
        mesh.indices.push_back(base_index + i + 1);
    }
}
```

#### Hours 7-8: Integration & Render Toggle

Add to `MapRenderer`:

```cpp
// In MapRenderer.h
class MapRenderer {
    // ...existing code...

    // GPU renderer (optional)
    std::unique_ptr<GPUMapRenderer> gpu_renderer_;
    bool use_gpu_renderer_ = false;

public:
    void SetUseGPURenderer(bool use) { use_gpu_renderer_ = use; }
    bool IsUsingGPURenderer() const { return use_gpu_renderer_; }
    GPUMapRenderer* GetGPURenderer() { return gpu_renderer_.get(); }
};

// In MapRenderer::Render()
void MapRenderer::Render() {
    if (use_gpu_renderer_ && gpu_renderer_) {
        gpu_renderer_->Render(camera_);
        // Still render features/names with ImGui (text rendering)
        RenderFeatures();
        RenderNames();
    } else {
        // Existing ImGui-based rendering
        RenderProvinces();
        RenderFeatures();
        RenderNames();
    }
}
```

### Day 1 Deliverables

| Deliverable | Description | Verification |
|-------------|-------------|--------------|
| `GPUMapRenderer.h` | Header with class definition | Compiles |
| `GPUMapRenderer.cpp` | Implementation (~300 lines) | Compiles |
| `province.vert` | Vertex shader | Loads without errors |
| `province.frag` | Fragment shader | Loads without errors |
| Debug toggle | `SetUseGPURenderer(bool)` | Can switch at runtime |
| Metrics output | Render time, draw calls, vertices | Visible in debug UI |

---

## Day 2: Benchmark Actual Performance (8 hours)

### Objective
Create a rigorous benchmarking framework that produces reproducible, statistically valid performance comparisons.

### Hour-by-Hour Breakdown

#### Hours 1-2: Benchmark Framework

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
    LODLevel lod_level;

    // Timing stats (in milliseconds)
    double min_time;
    double max_time;
    double avg_time;
    double median_time;
    double p95_time;      // 95th percentile
    double p99_time;      // 99th percentile
    double std_dev;

    // Throughput metrics
    int total_vertices;
    int total_triangles;
    int draw_calls;
    double triangles_per_ms;

    // Memory metrics
    size_t gpu_memory_bytes;
    size_t cpu_memory_bytes;
};

class RenderBenchmark {
public:
    static constexpr int WARMUP_FRAMES = 100;
    static constexpr int BENCHMARK_FRAMES = 1000;

    BenchmarkResult RunBenchmark(
        MapRenderer& renderer,
        const std::string& name,
        int province_count,
        LODLevel lod
    );

    void ExportResults(const std::vector<BenchmarkResult>& results,
                       const std::string& filename);
};
```

#### Hours 3-4: Test Scenarios

Create standardized test scenarios:

```cpp
// Scenario definitions
struct BenchmarkScenario {
    std::string name;
    int province_count;
    LODLevel lod_level;
    Vector2 camera_position;
    float camera_zoom;
};

std::vector<BenchmarkScenario> GetStandardScenarios() {
    return {
        // Small scale tests
        {"small_lod0", 100, LODLevel::STRATEGIC, {0,0}, 0.1f},
        {"small_lod2", 100, LODLevel::PROVINCIAL, {0,0}, 0.5f},
        {"small_lod4", 100, LODLevel::TACTICAL, {0,0}, 0.9f},

        // Medium scale tests
        {"medium_lod0", 500, LODLevel::STRATEGIC, {0,0}, 0.1f},
        {"medium_lod2", 500, LODLevel::PROVINCIAL, {0,0}, 0.5f},
        {"medium_lod4", 500, LODLevel::TACTICAL, {0,0}, 0.9f},

        // Large scale tests (target: 2000+ provinces)
        {"large_lod0", 2000, LODLevel::STRATEGIC, {0,0}, 0.1f},
        {"large_lod2", 2000, LODLevel::PROVINCIAL, {0,0}, 0.5f},
        {"large_lod4", 2000, LODLevel::TACTICAL, {0,0}, 0.9f},

        // Stress tests
        {"stress_lod2", 5000, LODLevel::PROVINCIAL, {0,0}, 0.5f},
        {"stress_lod4", 5000, LODLevel::TACTICAL, {0,0}, 0.9f},

        // Camera movement test (simulates gameplay)
        {"panning_stress", 2000, LODLevel::PROVINCIAL, {0,0}, 0.5f},
    };
}
```

#### Hours 5-6: Automated Benchmark Runner

```cpp
int main(int argc, char* argv[]) {
    // Initialize SDL + OpenGL
    if (!InitializeGraphicsContext()) {
        std::cerr << "Failed to initialize graphics" << std::endl;
        return 1;
    }

    RenderBenchmark benchmark;
    std::vector<BenchmarkResult> all_results;

    auto scenarios = GetStandardScenarios();

    for (const auto& scenario : scenarios) {
        std::cout << "Running scenario: " << scenario.name << std::endl;

        // Create entities
        auto entity_manager = CreateTestProvinces(scenario.province_count);

        // Test ImGui renderer
        {
            MapRenderer imgui_renderer(entity_manager);
            imgui_renderer.Initialize();
            imgui_renderer.SetUseGPURenderer(false);

            auto result = benchmark.RunBenchmark(
                imgui_renderer,
                "ImGui_" + scenario.name,
                scenario.province_count,
                scenario.lod_level
            );
            all_results.push_back(result);
        }

        // Test GPU renderer
        {
            MapRenderer gpu_renderer(entity_manager);
            gpu_renderer.Initialize();
            gpu_renderer.SetUseGPURenderer(true);

            auto result = benchmark.RunBenchmark(
                gpu_renderer,
                "GPU_" + scenario.name,
                scenario.province_count,
                scenario.lod_level
            );
            all_results.push_back(result);
        }
    }

    // Export results
    benchmark.ExportResults(all_results, "benchmark_results.csv");
    benchmark.ExportResults(all_results, "benchmark_results.json");

    return 0;
}
```

#### Hours 7-8: Results Analysis & Visualization

**Output format** (`benchmark_results.csv`):
```csv
renderer,scenario,provinces,lod,min_ms,max_ms,avg_ms,median_ms,p95_ms,p99_ms,stddev,vertices,triangles,draw_calls,tris_per_ms,gpu_mem_kb,cpu_mem_kb
ImGui,small_lod2,100,2,0.12,0.45,0.18,0.16,0.32,0.41,0.05,1200,400,100,2222,0,48
GPU,small_lod2,100,2,0.08,0.21,0.11,0.10,0.18,0.20,0.02,1200,400,1,3636,256,12
...
```

**Analysis script** (`scripts/analyze_benchmark.py`):
```python
#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt

def analyze_benchmarks(csv_path):
    df = pd.read_csv(csv_path)

    # Calculate speedup
    imgui = df[df['renderer'].str.startswith('ImGui')]
    gpu = df[df['renderer'].str.startswith('GPU')]

    comparison = pd.merge(
        imgui, gpu,
        on=['scenario', 'provinces', 'lod'],
        suffixes=('_imgui', '_gpu')
    )
    comparison['speedup'] = comparison['avg_ms_imgui'] / comparison['avg_ms_gpu']

    # Print summary
    print("=== Benchmark Results ===")
    print(f"Average GPU speedup: {comparison['speedup'].mean():.2f}x")
    print(f"Max GPU speedup: {comparison['speedup'].max():.2f}x")
    print(f"Min GPU speedup: {comparison['speedup'].min():.2f}x")

    # Identify crossover point
    crossover = comparison[comparison['speedup'] < 1.0]
    if len(crossover) > 0:
        print(f"\nWARNING: GPU slower in {len(crossover)} scenarios")
        print(crossover[['scenario', 'provinces', 'speedup']])

    # Generate charts
    generate_charts(comparison)

    return comparison

def generate_charts(df):
    # Chart 1: Render time by province count
    fig, ax = plt.subplots(figsize=(10, 6))
    for renderer in ['imgui', 'gpu']:
        data = df[['provinces', f'avg_ms_{renderer}']].groupby('provinces').mean()
        ax.plot(data.index, data[f'avg_ms_{renderer}'], label=renderer.upper(), marker='o')
    ax.set_xlabel('Province Count')
    ax.set_ylabel('Average Render Time (ms)')
    ax.set_title('Render Time vs Province Count')
    ax.legend()
    ax.axhline(y=16.67, color='r', linestyle='--', label='60 FPS target')
    plt.savefig('benchmark_render_time.png')

    # Chart 2: Speedup by scenario
    fig, ax = plt.subplots(figsize=(12, 6))
    ax.bar(df['scenario'], df['speedup'])
    ax.axhline(y=1.0, color='r', linestyle='--')
    ax.set_ylabel('GPU Speedup (x times faster)')
    ax.set_title('GPU vs ImGui Speedup by Scenario')
    plt.xticks(rotation=45, ha='right')
    plt.tight_layout()
    plt.savefig('benchmark_speedup.png')

if __name__ == '__main__':
    import sys
    csv_path = sys.argv[1] if len(sys.argv) > 1 else 'benchmark_results.csv'
    analyze_benchmarks(csv_path)
```

### Day 2 Deliverables

| Deliverable | Description | Verification |
|-------------|-------------|--------------|
| `benchmark_rendering.cpp` | Benchmark framework | Compiles and runs |
| `benchmark_results.csv` | Raw performance data | Contains all scenarios |
| `benchmark_results.json` | Structured data | Valid JSON |
| `analyze_benchmark.py` | Analysis script | Generates charts |
| `benchmark_render_time.png` | Time vs province count | Readable chart |
| `benchmark_speedup.png` | Speedup comparison | Readable chart |

---

## Day 3: Data-Driven Decision (4 hours)

### Decision Framework

Based on benchmark results, use this decision matrix:

| Metric | GPU Wins If | ImGui Wins If |
|--------|-------------|---------------|
| Average render time | GPU < ImGui by >20% | GPU >= ImGui |
| 95th percentile | GPU more consistent | ImGui acceptable |
| Memory usage | GPU < 2x ImGui | GPU > 4x ImGui |
| Implementation complexity | Worth the maintenance | Not worth it |
| Crossover point | >500 provinces | <200 provinces |

### Decision Outcomes

#### Outcome A: GPU Clearly Superior
**Criteria**:
- GPU 2x+ faster at 500+ provinces
- GPU 95th percentile < ImGui average
- Memory overhead < 100MB

**Action**:
1. Complete GPUMapRenderer implementation
2. Add shader hot-reloading for development
3. Implement GPU-based border rendering
4. Deprecate ImGui polygon rendering (keep for fallback)
5. Timeline: 3-5 additional days

#### Outcome B: GPU Marginally Better
**Criteria**:
- GPU 20-100% faster at 1000+ provinces
- Crossover point at 300-500 provinces
- Current game has <500 visible provinces

**Action**:
1. Keep GPUMapRenderer as optional "performance mode"
2. Default to ImGui renderer
3. Add user setting to enable GPU mode
4. Revisit when game scope increases
5. Timeline: 1 additional day (polish toggle)

#### Outcome C: No Significant Difference
**Criteria**:
- GPU < 20% faster
- High variance in GPU performance
- ImGui already meets 60 FPS target

**Action**:
1. Archive GPUMapRenderer code (don't delete)
2. Document findings for future reference
3. Focus optimization on other bottlenecks (culling, data loading)
4. Revisit if ImGui becomes bottleneck
5. Timeline: 0 additional days

#### Outcome D: ImGui Actually Better
**Criteria**:
- GPU slower or equal
- GPU has higher variance
- ImGui batching is highly optimized

**Action**:
1. Delete GPUMapRenderer code
2. Document why GPU wasn't beneficial
3. Optimize ImGui usage (reduce state changes, batch by color)
4. Focus on CPU-side optimizations
5. Timeline: 0 additional days

### Documentation Requirements

Regardless of outcome, document:

```markdown
# GPU Renderer Evaluation Results

**Date**: [Date]
**Evaluator**: [Name]

## Summary
[1-2 sentence conclusion]

## Key Metrics
| Metric | ImGui | GPU | Winner |
|--------|-------|-----|--------|
| Avg render time (500 provinces) | X ms | Y ms | ? |
| P95 render time | X ms | Y ms | ? |
| Memory usage | X MB | Y MB | ? |
| Draw calls | X | Y | ? |

## Charts
[Embed benchmark_render_time.png]
[Embed benchmark_speedup.png]

## Decision
[Outcome A/B/C/D]: [Reasoning]

## Next Steps
1. [Action item]
2. [Action item]
3. [Action item]

## Raw Data
See: benchmark_results.csv
```

---

## Risk Mitigation

### Technical Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| OpenGL context issues | Medium | High | Keep ImGui fallback, test on multiple GPUs |
| Shader compilation failures | Low | Medium | Validate shaders at startup, graceful fallback |
| Memory leaks | Medium | Medium | Use RAII, test with Valgrind |
| Platform differences | Medium | Medium | Test on Windows and Linux |

### Schedule Risks

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Triangulation more complex | Medium | Medium | Use simple fan for MVP, earcut library if needed |
| Benchmark setup takes longer | Low | Low | Use existing test infrastructure |
| Unexpected ImGui limitations | Low | High | Research ImGui GL backend integration |

---

## File Checklist

### Day 1 Files
- [ ] `include/map/render/GPUMapRenderer.h`
- [ ] `src/rendering/GPUMapRenderer.cpp`
- [ ] `assets/shaders/province.vert`
- [ ] `assets/shaders/province.frag`
- [ ] Updated `include/map/render/MapRenderer.h` (toggle)
- [ ] Updated `src/rendering/MapRenderer.cpp` (integration)
- [ ] Updated `CMakeLists.txt` (new sources)

### Day 2 Files
- [ ] `tests/performance/benchmark_rendering.cpp`
- [ ] `scripts/analyze_benchmark.py`
- [ ] `benchmark_results.csv` (generated)
- [ ] `benchmark_results.json` (generated)
- [ ] `benchmark_render_time.png` (generated)
- [ ] `benchmark_speedup.png` (generated)

### Day 3 Files
- [ ] `docs/architecture/GPU-RENDERER-EVALUATION.md`

---

## Success Criteria

The integration is successful if:

1. **Day 1**: Can toggle between ImGui and GPU rendering at runtime without crashes
2. **Day 2**: Have quantitative data for at least 10 scenarios with statistical validity
3. **Day 3**: Clear decision made with documented reasoning

---

## Appendix A: Quick Reference Commands

```bash
# Build with GPU renderer
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run benchmarks
./benchmark_rendering

# Analyze results
python3 scripts/analyze_benchmark.py benchmark_results.csv

# Toggle GPU renderer in-game
# Press F9 or use Debug menu > Rendering > Use GPU Renderer
```

---

## Appendix B: Shader Development Tips

```bash
# Validate shaders offline (requires glslangValidator)
glslangValidator -V assets/shaders/province.vert
glslangValidator -V assets/shaders/province.frag

# Check for OpenGL errors
export MESA_DEBUG=1  # Linux
# Or check glGetError() after each GL call in debug mode
```

---

*Document Status: Ready for Implementation*
*Created: January 30, 2026*
