# Shared-Vertex LOD Implementation Plan

## Problem Statement

Current multi-LOD implementation is broken:
- Creates decimated vertices per LOD but doesn't upload them to VBO
- LOD indices reference local decimated vertices, but VBO contains full-detail vertices
- Result: Complete index/vertex mismatch causing visual corruption

## Solution Architecture

**Core Principle**: All LOD levels share the SAME vertex buffer, with different index buffers selecting subsets of those vertices.

## Implementation Plan

### Phase 1: Data Structure Changes

**1.1 Modify TriangulateProvinces() to return vertex mapping**
- Current: Returns flat vertex array with indices
- New: Also return per-province vertex mapping:
  ```cpp
  struct ProvinceGeometry {
      uint32_t vertex_start;      // Start index in global vertex buffer
      uint32_t vertex_count;      // Number of vertices for this province
  };
  std::vector<ProvinceGeometry> province_geometries;
  ```
- Note: Indices are NOT stored per-province, they're generated per-LOD

**1.2 Add province geometry tracking to GPUMapRenderer**
- Store `std::vector<ProvinceGeometry> province_geometries_` as member
- Use during LOD generation to know which vertices belong to which province

### Phase 2: LOD Generation Algorithm

**2.1 Full-detail LOD (LOD 0)**
- Upload ALL vertices to VBO (already done correctly)
- Generate indices from all provinces (already done correctly)
- No changes needed

**2.2 Decimated LODs (LOD 1, LOD 2)**

**Critical Understanding:**
- VBO contains ALL full-detail vertices (uploaded once)
- For each LOD, we select a SUBSET of vertices to triangulate
- Triangulation produces indices 0..M-1 (local to the subset)
- We REMAP these local indices to global VBO positions
- Upload only the remapped indices to IBO

**Algorithm (per province)**:
```cpp
for each province in province_geometries:
    vertex_start = province.vertex_start
    vertex_count = province.vertex_count

    // STEP 1: Select which vertices to keep
    selected_vbo_positions = []  // Global VBO indices
    step = max(1, decimation_factor)
    for (i = 0; i < vertex_count; i += step):
        selected_vbo_positions.push_back(vertex_start + i)

    // Always include last vertex to close polygon
    if (selected_vbo_positions.back() != vertex_start + vertex_count - 1):
        selected_vbo_positions.push_back(vertex_start + vertex_count - 1)

    // Fallback: if too few vertices, use all
    if (selected_vbo_positions.size() < 3):
        for (i = 0; i < vertex_count; i++):
            selected_vbo_positions.push_back(vertex_start + i)

    // STEP 2: Extract positions for triangulation
    polygon_positions = []
    for vbo_idx in selected_vbo_positions:
        polygon_positions.push_back(full_vertices[vbo_idx].position)

    // STEP 3: Triangulate the decimated polygon
    local_indices = earcut(polygon_positions)  // Returns 0..M-1

    // STEP 4: Remap local indices to global VBO indices
    for local_idx in local_indices:
        global_vbo_idx = selected_vbo_positions[local_idx]
        lod_indices.push_back(global_vbo_idx)
```

**Example**:
- Province has vertices at VBO positions [100, 101, 102, 103, 104, 105]
- Decimation factor = 2
- Selected positions: [100, 102, 104, 105] (every 2nd + last)
- Extract positions: [(x0,y0), (x2,y2), (x4,y4), (x5,y5)]
- Triangulate → local indices: [0,1,2, 0,2,3] (2 triangles)
- Remap: [100,102,104, 100,104,105]
- These correctly reference the VBO!

### Phase 3: Implementation Steps

**Step 1**: Modify TriangulateProvinces signature
- Add output parameter: `std::vector<ProvinceGeometry>& province_geometries`
- Track vertex_start for each province
- Store vertex_count and indices per province

**Step 2**: Update UploadProvinceData
- Store province_geometries_ as member variable
- Use for LOD generation

**Step 3**: Implement new TriangulateLOD function
```cpp
void GenerateLODIndices(
    const std::vector<ProvinceVertex>& full_vertices,
    const std::vector<ProvinceGeometry>& province_geometries,
    int decimation_factor,
    std::vector<uint32_t>& lod_indices
);
```

**Step 4**: Replace TriangulateProvincesWithDecimation
- Remove entirely (it's fundamentally broken)
- Use GenerateLODIndices instead

**Step 5**: Update UploadProvinceData LOD loop
```cpp
for (int lod = 0; lod < LOD_COUNT; ++lod) {
    std::vector<uint32_t> lod_indices;
    int decimation_factor = (1 << lod);  // 1, 2, 4

    if (lod == 0) {
        // LOD 0: use full detail indices (no copy, use full_indices directly)
        lod_index_counts_[0] = full_indices.size();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lod_ibos_[0]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     full_indices.size() * sizeof(uint32_t),
                     full_indices.data(),
                     GL_STATIC_DRAW);
    } else {
        // LOD 1+: generate decimated indices referencing shared VBO
        GenerateLODIndices(vertices, province_geometries_,
                          decimation_factor, lod_indices);

        lod_index_counts_[lod] = lod_indices.size();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lod_ibos_[lod]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     lod_indices.size() * sizeof(uint32_t),
                     lod_indices.data(),
                     GL_STATIC_DRAW);
    }
    CHECK_GL_ERROR();
}
```

### Phase 4: Edge Cases & Validation

**4.1 Minimum vertices**
- Ensure each province has at least 3 vertices after decimation
- If decimation would result in <3 vertices, use all vertices

**4.2 Closed polygon handling**
- Always include first and last vertex in decimated set
- Ensures polygon remains closed

**4.3 Earcut failure handling**
- If triangulation fails on decimated polygon:
  - Log warning with province ID
  - Fall back to using all vertices for that province
  - Continue processing other provinces

**4.4 Index validation**
- Verify all LOD indices < vertex_count
- Add bounds checking in debug builds
- Validate indices reference correct province vertices

## Data Flow Diagram

```
TriangulateProvinces()
    ↓
[Full Vertex Buffer] → Upload to VBO (shared by all LODs)
[Province Geometries] → Store mapping
    ↓
For LOD 0:
    Use full indices → Upload to IBO[0]
    ↓
For LOD 1,2:
    For each province in geometries:
        Select subset of vertices (every Nth)
        Triangulate subset
        Remap to global vertex indices
    → Upload to IBO[1], IBO[2]
```

## Testing Strategy

1. **Visual verification**: Render each LOD level, verify no corruption
2. **Index bounds check**: All indices < vertex_count
3. **Triangle count check**: LOD 1 ≈ 50% of LOD 0, LOD 2 ≈ 25% of LOD 0
4. **Memory check**: VBO uploaded once, not per-LOD

## Files to Modify

1. `include/map/render/GPUMapRenderer.h`
   - Add ProvinceGeometry struct
   - Add province_geometries_ member
   - Change TriangulateProvinces signature
   - Add GenerateLODIndices declaration
   - Remove TriangulateProvincesWithDecimation

2. `src/rendering/GPUMapRenderer.cpp`
   - Modify TriangulateProvinces implementation
   - Implement GenerateLODIndices
   - Update UploadProvinceData
   - Remove TriangulateProvincesWithDecimation

## Expected Outcome

- Single VBO with all vertices
- 3 IBOs with indices referencing the shared VBO
- LOD 0: All triangles
- LOD 1: ~50% triangles (every 2nd boundary vertex)
- LOD 2: ~25% triangles (every 4th boundary vertex)
- No visual corruption at any LOD level
- Memory efficient (vertices stored once)
