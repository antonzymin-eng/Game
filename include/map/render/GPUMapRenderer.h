// ============================================================================
// GPUMapRenderer.h - GPU-Accelerated Map Renderer using OpenGL 3.3
// Created: December 21, 2025
// Description: Replaces ImGui immediate-mode rendering with retained-mode
//              GPU rendering for massive performance gains
// ============================================================================

#pragma once

#include <glad/glad.h>
#include <vector>
#include <memory>
#include <string>
#include "map/ProvinceRenderComponent.h"
#include "map/render/ViewportCuller.h"
#include "core/ECS/EntityManager.h"

namespace game::map {

// ============================================================================
// Vertex Structure
// ============================================================================

struct ProvinceVertex {
    float x, y;                     // World coordinates
    uint32_t province_id;           // Province identifier
    float u, v;                     // Texture coordinates (unused currently)
};

// ============================================================================
// GPUMapRenderer - OpenGL-based map rendering
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

    // Statistics
    size_t GetVertexCount() const { return vertex_count_; }
    size_t GetTriangleCount() const { return index_count_ / 3; }
    size_t GetProvinceCount() const { return province_count_; }
    float GetLastRenderTime() const { return last_render_time_ms_; }

private:
    ::core::ecs::EntityManager& entity_manager_;

    // OpenGL objects
    GLuint vao_;                       // Vertex Array Object
    GLuint vbo_;                       // Vertex Buffer Object (vertices)
    GLuint ibo_;                       // Index Buffer Object (triangles)
    GLuint province_color_texture_;    // Province colors (256x256 RGBA8)
    GLuint province_metadata_texture_; // Terrain type, owner, etc. (256x256 RGBA8)

    GLuint map_shader_program_;        // Province fill shader
    GLuint border_shader_program_;     // Border shader (future)

    // Shader uniform locations (map shader)
    GLint u_view_projection_;
    GLint u_render_mode_;
    GLint u_selected_province_;
    GLint u_hovered_province_;
    GLint u_selection_glow_time_;
    GLint u_province_data_;
    GLint u_province_metadata_;
    GLint u_viewport_size_;

    // Geometry statistics
    size_t vertex_count_;
    size_t index_count_;
    size_t province_count_;

    // Rendering state
    RenderMode render_mode_;
    uint32_t selected_province_id_;
    uint32_t hovered_province_id_;
    float selection_glow_time_;
    bool show_borders_;
    bool show_names_;

    // Performance tracking
    float last_render_time_ms_;

    // Initialization helpers
    bool LoadShaders();
    bool CreateBuffers();
    bool CreateTextures();

    // Shader compilation helpers
    GLuint CompileShader(GLenum type, const std::string& source);
    GLuint LinkProgram(GLuint vert_shader, GLuint frag_shader);
    std::string ReadShaderFile(const std::string& filepath);

    // Data upload helpers
    void TriangulateProvinces(
        const std::vector<const ProvinceRenderComponent*>& provinces,
        std::vector<ProvinceVertex>& vertices,
        std::vector<uint32_t>& indices
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
    void CalculateViewProjectionMatrix(const Camera2D& camera, float* matrix_out);
};

} // namespace game::map
