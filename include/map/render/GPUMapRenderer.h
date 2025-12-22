// ============================================================================
// GPUMapRenderer.h - GPU-Accelerated Map Renderer using OpenGL 3.3
// Created: December 21, 2025
// Updated: December 22, 2025 - Fixed critical issues from code review
// Description: Replaces ImGui immediate-mode rendering with retained-mode
//              GPU rendering for massive performance gains
// ============================================================================

#pragma once

#include <glad/glad.h>
#include <vector>
#include <memory>
#include <string>
#include <array>
#include "map/ProvinceRenderComponent.h"
#include "map/render/Camera2D.h"
#include "map/render/ViewportCuller.h"
#include "core/ECS/EntityManager.h"

namespace game::map {

// ============================================================================
// Configuration Constants
// ============================================================================

namespace config {
    // LOD thresholds (zoom levels)
    constexpr float LOD_HIGH_THRESHOLD = 1.5f;    // Zoom >= this uses LOD 0
    constexpr float LOD_MEDIUM_THRESHOLD = 0.75f; // Zoom >= this uses LOD 1

    // Texture configuration
    constexpr uint32_t MIN_TEXTURE_SIZE = 256;     // Minimum texture dimension
    constexpr uint32_t MAX_TEXTURE_SIZE = 4096;    // Maximum texture dimension
    constexpr uint32_t PROVINCES_PER_ROW = 256;    // Default texture width
}

// ============================================================================
// Vertex Structure
// ============================================================================

struct ProvinceVertex {
    float x, y;                     // World coordinates
    uint32_t province_id;           // Province identifier
    float u, v;                     // Texture coordinates (unused currently)
};

// Province geometry metadata for LOD generation
struct ProvinceGeometry {
    uint32_t province_id;    // Province identifier (for debugging)
    uint32_t vertex_start;   // Start index in global vertex buffer
    uint32_t vertex_count;   // Number of vertices for this province
    float area;              // Approximate area (for importance-based LOD)

    // Bounding box (for frustum culling)
    float min_x, min_y, max_x, max_y;
};

// ============================================================================
// GPUMapRenderer - OpenGL-based map rendering with optimizations
// ============================================================================

class GPUMapRenderer {
public:
    GPUMapRenderer(::core::ecs::EntityManager& entity_manager);
    ~GPUMapRenderer();

    // Initialize OpenGL resources (call after OpenGL context is created)
    bool Initialize();

    // Upload province data to GPU (call when provinces are loaded)
    bool UploadProvinceData(const std::vector<const ProvinceRenderComponent*>& provinces);

    // Render the map (call every frame)
    void Render(const Camera2D& camera);

    // Input handling (call every frame before render)
    void HandleInput();

    // Camera management (replaces dependency on MapRenderer)
    Camera2D& GetCamera() { return camera_; }
    const Camera2D& GetCamera() const { return camera_; }
    void SetCamera(const Camera2D& camera) { camera_ = camera; }

    // Render mode control
    enum class RenderMode {
        POLITICAL = 0,    // Province colors by owner
        TERRAIN = 1,      // Colors by terrain type
        TRADE = 2,        // Colors by trade network
        RELIGION = 3,     // Colors by dominant religion
        CULTURE = 4       // Colors by culture group
    };

    void SetRenderMode(RenderMode mode);
    RenderMode GetRenderMode() const { return render_mode_; }

    // Selection control
    void SetSelectedProvince(uint32_t province_id);
    void SetHoveredProvince(uint32_t province_id);
    uint32_t GetSelectedProvince() const { return selected_province_id_; }
    uint32_t GetHoveredProvince() const { return hovered_province_id_; }

    // Rendering options
    void SetShowBorders(bool show) { show_borders_ = show; }
    void SetShowNames(bool show) { show_names_ = show; }
    bool GetShowBorders() const { return show_borders_; }
    bool GetShowNames() const { return show_names_; }

    // LOD configuration
    void SetLODThresholds(float high_threshold, float medium_threshold) {
        lod_high_threshold_ = high_threshold;
        lod_medium_threshold_ = medium_threshold;
    }
    float GetLODHighThreshold() const { return lod_high_threshold_; }
    float GetLODMediumThreshold() const { return lod_medium_threshold_; }

    // Statistics
    size_t GetVertexCount() const { return vertex_count_; }
    size_t GetMaxTriangleCount() const { return index_count_ / 3; }
    size_t GetCurrentTriangleCount() const { return lod_index_counts_[current_lod_level_] / 3; }
    int GetCurrentLODLevel() const { return current_lod_level_; }
    size_t GetProvinceCount() const { return province_count_; }
    size_t GetCulledProvinceCount() const { return culled_province_count_; }
    float GetLastRenderTime() const { return last_render_time_ms_; }

private:
    ::core::ecs::EntityManager& entity_manager_;

    // Camera (owned by renderer)
    Camera2D camera_;

    // OpenGL objects
    GLuint vao_;                       // Vertex Array Object
    GLuint vbo_;                       // Vertex Buffer Object (vertices)

    // Multi-LOD index buffers (3 levels: high, medium, low)
    static constexpr int LOD_COUNT = 3;
    GLuint lod_ibos_[LOD_COUNT];       // Index Buffer Objects per LOD level
    size_t lod_index_counts_[LOD_COUNT]; // Index count per LOD level

    // Dynamic texture sizing
    uint32_t texture_width_;           // Calculated based on province count
    uint32_t texture_height_;          // Calculated based on province count
    GLuint province_color_texture_;    // Province colors (dynamic size RGBA8)
    GLuint province_metadata_texture_; // Terrain type, owner, etc. (dynamic size RGBA8)

    GLuint map_shader_program_;        // Province fill shader
    GLuint border_shader_program_;     // Border shader

    // Shader uniform locations (map shader)
    GLint u_view_projection_;
    GLint u_render_mode_;
    GLint u_selected_province_;
    GLint u_hovered_province_;
    GLint u_selection_glow_time_;
    GLint u_province_data_;
    GLint u_province_metadata_;
    GLint u_viewport_size_;

    // Border shader uniform locations
    GLint u_border_view_projection_;
    GLint u_border_color_;
    GLint u_border_width_;

    // Geometry statistics
    size_t vertex_count_;
    size_t index_count_;
    size_t province_count_;
    size_t culled_province_count_;  // Number of provinces culled by frustum

    // Province geometry mapping (for LOD generation and culling)
    std::vector<ProvinceGeometry> province_geometries_;

    // Rendering state
    RenderMode render_mode_;
    uint32_t selected_province_id_;
    uint32_t hovered_province_id_;
    float selection_glow_time_;
    bool show_borders_;
    bool show_names_;
    int current_lod_level_;  // Last rendered LOD level

    // Performance tracking
    float last_render_time_ms_;

    // LOD configuration
    float lod_high_threshold_;
    float lod_medium_threshold_;

    // Initialization helpers
    bool LoadShaders();
    bool CreateBuffers();
    bool CreateTextures();

    // Shader compilation helpers
    GLuint CompileShader(GLenum type, const std::string& source);
    GLuint LinkProgram(GLuint vert_shader, GLuint frag_shader);
    std::string GetEmbeddedShader(const std::string& shader_name);

    // Data upload helpers
    void TriangulateProvinces(
        const std::vector<const ProvinceRenderComponent*>& provinces,
        std::vector<ProvinceVertex>& vertices,
        std::vector<uint32_t>& indices,
        std::vector<ProvinceGeometry>& province_geometries
    );

    void GenerateLODIndices(
        const std::vector<ProvinceVertex>& full_vertices,
        const std::vector<ProvinceGeometry>& province_geometries,
        unsigned int decimation_factor,
        std::vector<uint32_t>& lod_indices
    );

    // LOD generation sub-functions (split from monolithic function)
    void SelectLODVertices(
        const ProvinceGeometry& geom,
        unsigned int decimation_factor,
        std::vector<uint32_t>& selected_positions
    );

    bool TriangulateLODPolygon(
        const std::vector<ProvinceVertex>& full_vertices,
        const std::vector<uint32_t>& selected_positions,
        std::vector<uint32_t>& local_indices
    );

    void RemapIndicesToGlobal(
        const std::vector<uint32_t>& local_indices,
        const std::vector<uint32_t>& selected_positions,
        std::vector<uint32_t>& global_indices
    );

    void PackProvinceColorsToTexture(
        const std::vector<const ProvinceRenderComponent*>& provinces,
        std::vector<uint8_t>& texture_data
    );

    void PackProvinceMetadataToTexture(
        const std::vector<const ProvinceRenderComponent*>& provinces,
        std::vector<uint8_t>& texture_data
    );

    // Rendering helpers
    void UpdateUniforms(const Camera2D& camera);
    int SelectLODLevel(float zoom) const;

    // Frustum culling
    std::vector<uint32_t> CullProvinces(const Camera2D& camera);
    bool IsProvinceVisible(const ProvinceGeometry& geom, const Camera2D::Bounds& bounds) const;

    // Texture size calculation
    void CalculateTextureSize(size_t province_count);
    uint32_t ProvinceTexCoordU(uint32_t province_id) const;
    uint32_t ProvinceTexCoordV(uint32_t province_id) const;

    // OpenGL error checking (enabled in both debug and release)
    void CheckGLError(const char* file, int line, const char* operation);
};

// GL error checking macro (works in both debug and release)
#define CHECK_GL_ERROR_HERE() CheckGLError(__FILE__, __LINE__, "")
#define CHECK_GL_OPERATION(op) \
    do { \
        op; \
        CheckGLError(__FILE__, __LINE__, #op); \
    } while(0)

} // namespace game::map
