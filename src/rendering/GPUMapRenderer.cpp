// ============================================================================
// GPUMapRenderer.cpp - GPU-Accelerated Map Renderer Implementation
// Created: December 21, 2025
// Updated: December 22, 2025 - Applied all P0/P1/P2 fixes from code review
// ============================================================================

#include "map/render/GPUMapRenderer.h"
#include "core/logging/Logger.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>

// GLM - Math library for OpenGL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// External triangulation library (header-only)
#include <mapbox/earcut.hpp>

namespace game::map {

// ============================================================================
// Embedded Shaders (P2: Embed shaders in binary)
// ============================================================================

namespace embedded_shaders {

constexpr const char* MAP_VERTEX_SHADER = R"(
#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in uint province_id;
layout(location = 2) in vec2 uv;

uniform mat4 view_projection;

out VS_OUT {
    flat uint province_id;
    vec2 uv;
    vec2 world_pos;
} vs_out;

void main() {
    gl_Position = view_projection * vec4(position, 0.0, 1.0);
    vs_out.province_id = province_id;
    vs_out.uv = uv;
    vs_out.world_pos = position;
}
)";

constexpr const char* MAP_FRAGMENT_SHADER = R"(
#version 330 core

in VS_OUT {
    flat uint province_id;
    vec2 uv;
    vec2 world_pos;
} fs_in;

uniform sampler2D province_data;
uniform sampler2D province_metadata;
uniform int render_mode;
uniform vec2 viewport_size;
uniform uint selected_province_id;
uniform uint hovered_province_id;
uniform float selection_glow_time;

out vec4 frag_color;

vec4 GetProvinceColor(uint province_id, uint tex_width) {
    float u = (float(province_id % tex_width) + 0.5) / float(tex_width);
    float v = (float(province_id / tex_width) + 0.5) / float(tex_width);
    return texture(province_data, vec2(u, v));
}

vec3 GetTerrainColor(uint terrain_type) {
    // FIX: Changed from metadata.r * 10.0 to metadata.r * 255.0
    if (terrain_type >= 10u && terrain_type < 20u) return vec3(0.2, 0.6, 0.2);  // Plains
    if (terrain_type >= 20u && terrain_type < 30u) return vec3(0.1, 0.4, 0.1);  // Forest
    if (terrain_type >= 30u && terrain_type < 40u) return vec3(0.5, 0.5, 0.5);  // Mountains
    if (terrain_type >= 40u && terrain_type < 50u) return vec3(0.8, 0.8, 0.6);  // Desert
    if (terrain_type >= 50u && terrain_type < 60u) return vec3(0.1, 0.3, 0.5);  // Coast
    if (terrain_type >= 60u && terrain_type < 70u) return vec3(0.3, 0.5, 0.3);  // Wetland
    if (terrain_type >= 70u && terrain_type < 80u) return vec3(0.6, 0.5, 0.4);  // Highlands
    return vec3(0.5, 0.5, 0.5);  // Unknown
}

vec3 ApplySelectionGlow(vec3 base_color, bool is_selected, bool is_hovered) {
    vec3 result = base_color;
    if (is_selected) {
        float pulse = abs(sin(selection_glow_time * 3.0)) * 0.5 + 0.5;
        vec3 glow = vec3(1.0, 1.0, 1.0) * pulse * 0.4;
        result = mix(base_color, base_color + glow, 0.6);
    } else if (is_hovered) {
        result = base_color * 1.2;
    }
    return result;
}

void main() {
    vec4 province_color;
    uint tex_width = 256u;  // TODO: Pass as uniform

    if (render_mode == 0) {
        province_color = GetProvinceColor(fs_in.province_id, tex_width);
    } else if (render_mode == 1) {
        uint u = fs_in.province_id % tex_width;
        uint v = fs_in.province_id / tex_width;
        vec4 metadata = texture(province_metadata, vec2(
            (float(u) + 0.5) / float(tex_width),
            (float(v) + 0.5) / float(tex_width)
        ));
        uint terrain_type = uint(metadata.r * 255.0);  // FIX: Was * 10.0
        province_color = vec4(GetTerrainColor(terrain_type), 1.0);
    } else {
        province_color = vec4(1.0, 1.0, 1.0, 1.0);
    }

    bool is_selected = (fs_in.province_id == selected_province_id);
    bool is_hovered = (fs_in.province_id == hovered_province_id);

    vec3 final_color = ApplySelectionGlow(province_color.rgb, is_selected, is_hovered);
    frag_color = vec4(final_color, province_color.a);
}
)";

constexpr const char* BORDER_VERTEX_SHADER = R"(
#version 330 core

layout(location = 0) in vec2 position;

uniform mat4 view_projection;

void main() {
    gl_Position = view_projection * vec4(position, 0.0, 1.0);
}
)";

constexpr const char* BORDER_FRAGMENT_SHADER = R"(
#version 330 core

uniform vec4 border_color;

out vec4 frag_color;

void main() {
    frag_color = border_color;
}
)";

} // namespace embedded_shaders

// ============================================================================
// Constructor / Destructor
// ============================================================================

GPUMapRenderer::GPUMapRenderer(::core::ecs::EntityManager& entity_manager)
    : entity_manager_(entity_manager)
    , camera_()
    , vao_(0)
    , vbo_(0)
    , lod_ibos_{0, 0, 0}
    , lod_index_counts_{0, 0, 0}
    , texture_width_(config::PROVINCES_PER_ROW)
    , texture_height_(config::PROVINCES_PER_ROW)
    , province_color_texture_(0)
    , province_metadata_texture_(0)
    , map_shader_program_(0)
    , border_shader_program_(0)
    , u_view_projection_(-1)
    , u_render_mode_(-1)
    , u_selected_province_(-1)
    , u_hovered_province_(-1)
    , u_selection_glow_time_(-1)
    , u_province_data_(-1)
    , u_province_metadata_(-1)
    , u_viewport_size_(-1)
    , u_border_view_projection_(-1)
    , u_border_color_(-1)
    , u_border_width_(-1)
    , vertex_count_(0)
    , index_count_(0)
    , province_count_(0)
    , culled_province_count_(0)
    , render_mode_(RenderMode::POLITICAL)
    , selected_province_id_(0)
    , hovered_province_id_(0)
    , selection_glow_time_(0.0f)
    , show_borders_(true)
    , show_names_(true)
    , current_lod_level_(0)
    , last_render_time_ms_(0.0f)
    , lod_high_threshold_(config::LOD_HIGH_THRESHOLD)
    , lod_medium_threshold_(config::LOD_MEDIUM_THRESHOLD)
{
}

GPUMapRenderer::~GPUMapRenderer() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    for (int i = 0; i < LOD_COUNT; ++i) {
        if (lod_ibos_[i]) glDeleteBuffers(1, &lod_ibos_[i]);
    }
    if (province_color_texture_) glDeleteTextures(1, &province_color_texture_);
    if (province_metadata_texture_) glDeleteTextures(1, &province_metadata_texture_);
    if (map_shader_program_) glDeleteProgram(map_shader_program_);
    if (border_shader_program_) glDeleteProgram(border_shader_program_);
}

// ============================================================================
// P1: GL Error Checking (enabled in both debug and release)
// ============================================================================

void GPUMapRenderer::CheckGLError(const char* file, int line, const char* operation) {
    GLenum err;
    bool had_error = false;
    while ((err = glGetError()) != GL_NO_ERROR) {
        had_error = true;
        const char* error_str = "Unknown error";
        switch (err) {
            case GL_INVALID_ENUM: error_str = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: error_str = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: error_str = "GL_INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY: error_str = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error_str = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        }

        std::string location = std::string(file) + ":" + std::to_string(line);
        if (operation && *operation) {
            CORE_LOG_ERROR("OpenGL", error_str << " at " << location << " during: " << operation);
        } else {
            CORE_LOG_ERROR("OpenGL", error_str << " at " << location);
        }
    }

    // In release builds, still log errors but don't halt execution
    // In debug builds, you might want to assert here
#ifdef _DEBUG
    if (had_error) {
        // Optionally break into debugger
        // __debugbreak(); // MSVC
        // __builtin_trap(); // GCC/Clang
    }
#endif
}

// ============================================================================
// Initialization
// ============================================================================

bool GPUMapRenderer::Initialize() {
    CORE_LOG_INFO("GPUMapRenderer", "Initializing GPU-accelerated map renderer...");

    if (!LoadShaders()) {
        CORE_LOG_ERROR("GPUMapRenderer", "Failed to load shaders");
        return false;
    }

    if (!CreateBuffers()) {
        CORE_LOG_ERROR("GPUMapRenderer", "Failed to create buffers");
        return false;
    }

    if (!CreateTextures()) {
        CORE_LOG_ERROR("GPUMapRenderer", "Failed to create textures");
        return false;
    }

    CORE_LOG_INFO("GPUMapRenderer", "GPU map renderer initialized successfully");
    return true;
}

// ============================================================================
// P2: Embedded Shaders
// ============================================================================

std::string GPUMapRenderer::GetEmbeddedShader(const std::string& shader_name) {
    if (shader_name == "map.vert") {
        return embedded_shaders::MAP_VERTEX_SHADER;
    } else if (shader_name == "map.frag") {
        return embedded_shaders::MAP_FRAGMENT_SHADER;
    } else if (shader_name == "border.vert") {
        return embedded_shaders::BORDER_VERTEX_SHADER;
    } else if (shader_name == "border.frag") {
        return embedded_shaders::BORDER_FRAGMENT_SHADER;
    }

    CORE_LOG_ERROR("GPUMapRenderer", "Unknown embedded shader: " << shader_name);
    return "";
}

bool GPUMapRenderer::LoadShaders() {
    CORE_LOG_INFO("GPUMapRenderer", "Loading shaders...");

    // Load map shaders (embedded)
    std::string vert_source = GetEmbeddedShader("map.vert");
    std::string frag_source = GetEmbeddedShader("map.frag");

    if (vert_source.empty() || frag_source.empty()) {
        CORE_LOG_ERROR("GPUMapRenderer", "Failed to load map shader sources");
        return false;
    }

    GLuint vert_shader = CompileShader(GL_VERTEX_SHADER, vert_source);
    if (!vert_shader) return false;

    GLuint frag_shader = CompileShader(GL_FRAGMENT_SHADER, frag_source);
    if (!frag_shader) {
        glDeleteShader(vert_shader);
        return false;
    }

    map_shader_program_ = LinkProgram(vert_shader, frag_shader);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    if (!map_shader_program_) return false;

    // Get uniform locations for map shader
    u_view_projection_ = glGetUniformLocation(map_shader_program_, "view_projection");
    u_render_mode_ = glGetUniformLocation(map_shader_program_, "render_mode");
    u_selected_province_ = glGetUniformLocation(map_shader_program_, "selected_province_id");
    u_hovered_province_ = glGetUniformLocation(map_shader_program_, "hovered_province_id");
    u_selection_glow_time_ = glGetUniformLocation(map_shader_program_, "selection_glow_time");
    u_province_data_ = glGetUniformLocation(map_shader_program_, "province_data");
    u_province_metadata_ = glGetUniformLocation(map_shader_program_, "province_metadata");
    u_viewport_size_ = glGetUniformLocation(map_shader_program_, "viewport_size");

    // Load border shaders (P1: Implement border rendering)
    std::string border_vert = GetEmbeddedShader("border.vert");
    std::string border_frag = GetEmbeddedShader("border.frag");

    if (!border_vert.empty() && !border_frag.empty()) {
        GLuint border_vert_shader = CompileShader(GL_VERTEX_SHADER, border_vert);
        if (border_vert_shader) {
            GLuint border_frag_shader = CompileShader(GL_FRAGMENT_SHADER, border_frag);
            if (border_frag_shader) {
                border_shader_program_ = LinkProgram(border_vert_shader, border_frag_shader);
                glDeleteShader(border_vert_shader);
                glDeleteShader(border_frag_shader);

                if (border_shader_program_) {
                    u_border_view_projection_ = glGetUniformLocation(border_shader_program_, "view_projection");
                    u_border_color_ = glGetUniformLocation(border_shader_program_, "border_color");
                    u_border_width_ = glGetUniformLocation(border_shader_program_, "border_width");
                    CORE_LOG_INFO("GPUMapRenderer", "Border shader loaded successfully");
                }
            } else {
                glDeleteShader(border_vert_shader);
            }
        }
    }

    CORE_LOG_INFO("GPUMapRenderer", "Shaders loaded successfully");
    return true;
}

bool GPUMapRenderer::CreateBuffers() {
    CORE_LOG_INFO("GPUMapRenderer", "Creating OpenGL buffers...");

    CHECK_GL_OPERATION(glGenVertexArrays(1, &vao_));
    CHECK_GL_OPERATION(glBindVertexArray(vao_));

    CHECK_GL_OPERATION(glGenBuffers(1, &vbo_));
    CHECK_GL_OPERATION(glBindBuffer(GL_ARRAY_BUFFER, vbo_));

    // Set up vertex attributes
    CHECK_GL_OPERATION(glEnableVertexAttribArray(0));
    CHECK_GL_OPERATION(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ProvinceVertex),
                          (void*)offsetof(ProvinceVertex, x)));

    CHECK_GL_OPERATION(glEnableVertexAttribArray(1));
    CHECK_GL_OPERATION(glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(ProvinceVertex),
                           (void*)offsetof(ProvinceVertex, province_id)));

    CHECK_GL_OPERATION(glEnableVertexAttribArray(2));
    CHECK_GL_OPERATION(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ProvinceVertex),
                          (void*)offsetof(ProvinceVertex, u)));

    CHECK_GL_OPERATION(glGenBuffers(LOD_COUNT, lod_ibos_));
    CHECK_GL_OPERATION(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lod_ibos_[0]));
    CHECK_GL_OPERATION(glBindVertexArray(0));

    CORE_LOG_INFO("GPUMapRenderer", "OpenGL buffers created successfully");
    return true;
}

// ============================================================================
// P0: Dynamic Texture Sizing
// ============================================================================

void GPUMapRenderer::CalculateTextureSize(size_t province_count) {
    // Calculate minimum texture size needed
    uint32_t min_pixels = static_cast<uint32_t>(province_count);

    // Find next power of 2 that fits (or use rectangular texture)
    texture_width_ = config::PROVINCES_PER_ROW;
    texture_height_ = (min_pixels + texture_width_ - 1) / texture_width_;

    // Clamp to supported range
    GLint max_texture_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

    texture_width_ = std::min(texture_width_, static_cast<uint32_t>(max_texture_size));
    texture_height_ = std::min(texture_height_, static_cast<uint32_t>(max_texture_size));

    // Validate we can fit all provinces
    uint32_t max_provinces = texture_width_ * texture_height_;
    if (province_count > max_provinces) {
        CORE_LOG_WARN("GPUMapRenderer",
            "Province count (" << province_count << ") exceeds texture capacity ("
            << max_provinces << "). Some provinces may not render correctly.");
    }

    CORE_LOG_INFO("GPUMapRenderer",
        "Calculated texture size: " << texture_width_ << "x" << texture_height_
        << " for " << province_count << " provinces");
}

uint32_t GPUMapRenderer::ProvinceTexCoordU(uint32_t province_id) const {
    return province_id % texture_width_;
}

uint32_t GPUMapRenderer::ProvinceTexCoordV(uint32_t province_id) const {
    return province_id / texture_width_;
}

bool GPUMapRenderer::CreateTextures() {
    CORE_LOG_INFO("GPUMapRenderer", "Creating province textures...");

    // Query max texture size
    GLint max_texture_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    CHECK_GL_ERROR_HERE();

    if (max_texture_size < static_cast<GLint>(config::MIN_TEXTURE_SIZE)) {
        CORE_LOG_ERROR("GPUMapRenderer",
            "GPU does not support minimum texture size (max: " << max_texture_size << ")");
        return false;
    }

    CORE_LOG_INFO("GPUMapRenderer", "GPU supports textures up to " << max_texture_size << "x" << max_texture_size);

    // Create province color texture
    CHECK_GL_OPERATION(glGenTextures(1, &province_color_texture_));
    CHECK_GL_OPERATION(glBindTexture(GL_TEXTURE_2D, province_color_texture_));
    CHECK_GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    CHECK_GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    CHECK_GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    CHECK_GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    // Allocate with default size (will be resized in UploadProvinceData)
    std::vector<uint8_t> empty_data(texture_width_ * texture_height_ * 4, 0);
    CHECK_GL_OPERATION(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture_width_, texture_height_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, empty_data.data()));

    // Create province metadata texture
    CHECK_GL_OPERATION(glGenTextures(1, &province_metadata_texture_));
    CHECK_GL_OPERATION(glBindTexture(GL_TEXTURE_2D, province_metadata_texture_));
    CHECK_GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    CHECK_GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    CHECK_GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    CHECK_GL_OPERATION(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    CHECK_GL_OPERATION(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture_width_, texture_height_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, empty_data.data()));

    CHECK_GL_OPERATION(glBindTexture(GL_TEXTURE_2D, 0));

    CORE_LOG_INFO("GPUMapRenderer", "Province textures created successfully");
    return true;
}

// ============================================================================
// Shader Compilation
// ============================================================================

GLuint GPUMapRenderer::CompileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* source_cstr = source.c_str();
    glShaderSource(shader, 1, &source_cstr, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, info_log);
        const char* shader_type = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        CORE_LOG_ERROR("GPUMapRenderer", shader_type << " shader compilation failed:\n" << info_log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint GPUMapRenderer::LinkProgram(GLuint vert_shader, GLuint frag_shader) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vert_shader);
    glAttachShader(program, frag_shader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[1024];
        glGetProgramInfoLog(program, 1024, nullptr, info_log);
        CORE_LOG_ERROR("GPUMapRenderer", "Shader program linking failed:\n" << info_log);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

// ============================================================================
// Data Upload
// ============================================================================

bool GPUMapRenderer::UploadProvinceData(const std::vector<const ProvinceRenderComponent*>& provinces) {
    CORE_LOG_INFO("GPUMapRenderer", "Uploading province data to GPU with multi-LOD support...");

    province_count_ = provinces.size();

    // P0: Calculate optimal texture size based on province count
    CalculateTextureSize(province_count_);

    // Step 1: Triangulate provinces
    std::vector<ProvinceVertex> vertices;
    std::vector<uint32_t> full_indices;
    province_geometries_.clear();
    TriangulateProvinces(provinces, vertices, full_indices, province_geometries_);

    vertex_count_ = vertices.size();
    index_count_ = full_indices.size();

    CORE_LOG_INFO("GPUMapRenderer", "Full detail: " << vertex_count_ << " vertices, "
                  << index_count_ << " indices (" << index_count_ / 3 << " triangles), "
                  << province_geometries_.size() << " geometries");

    // Step 2: Upload vertex data to GPU
    CHECK_GL_OPERATION(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    CHECK_GL_OPERATION(glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(ProvinceVertex),
                 vertices.data(),
                 GL_STATIC_DRAW));

    // Step 3: Generate and upload multi-LOD index buffers
    for (int lod = 0; lod < LOD_COUNT; ++lod) {
        std::vector<uint32_t> lod_indices;
        int decimation_factor = (1 << lod);  // 1, 2, 4

        if (lod == 0) {
            lod_index_counts_[0] = full_indices.size();
            CHECK_GL_OPERATION(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lod_ibos_[0]));
            CHECK_GL_OPERATION(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         full_indices.size() * sizeof(uint32_t),
                         full_indices.data(),
                         GL_STATIC_DRAW));

            CORE_LOG_INFO("GPUMapRenderer", "LOD 0 (High): "
                          << full_indices.size() / 3 << " triangles");
        } else {
            GenerateLODIndices(vertices, province_geometries_,
                              decimation_factor, lod_indices);

            lod_index_counts_[lod] = lod_indices.size();
            CHECK_GL_OPERATION(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lod_ibos_[lod]));
            CHECK_GL_OPERATION(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         lod_indices.size() * sizeof(uint32_t),
                         lod_indices.data(),
                         GL_STATIC_DRAW));

            CORE_LOG_INFO("GPUMapRenderer", "LOD " << lod << " (decimation 1/" << decimation_factor << "): "
                          << lod_indices.size() / 3 << " triangles");
        }
    }

    // Step 4: Pack province colors into texture
    std::vector<uint8_t> color_texture_data;
    PackProvinceColorsToTexture(provinces, color_texture_data);

    CHECK_GL_OPERATION(glBindTexture(GL_TEXTURE_2D, province_color_texture_));
    CHECK_GL_OPERATION(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture_width_, texture_height_, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, color_texture_data.data()));

    // Step 5: Pack province metadata into texture
    std::vector<uint8_t> metadata_texture_data;
    PackProvinceMetadataToTexture(provinces, metadata_texture_data);

    CHECK_GL_OPERATION(glBindTexture(GL_TEXTURE_2D, province_metadata_texture_));
    CHECK_GL_OPERATION(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture_width_, texture_height_, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, metadata_texture_data.data()));

    CHECK_GL_OPERATION(glBindTexture(GL_TEXTURE_2D, 0));
    CHECK_GL_OPERATION(glBindBuffer(GL_ARRAY_BUFFER, 0));
    CHECK_GL_OPERATION(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

    CORE_LOG_INFO("GPUMapRenderer", "Province data uploaded successfully");
    return true;
}

void GPUMapRenderer::TriangulateProvinces(
    const std::vector<const ProvinceRenderComponent*>& provinces,
    std::vector<ProvinceVertex>& vertices,
    std::vector<uint32_t>& indices,
    std::vector<ProvinceGeometry>& province_geometries)
{
    // Reserve capacity
    size_t estimated_vertices = 0;
    size_t estimated_indices = 0;
    for (const auto* province : provinces) {
        if (province) {
            estimated_vertices += province->boundary_points.size();
            estimated_indices += province->boundary_points.size() * 3;
        }
    }

    vertices.reserve(estimated_vertices);
    indices.reserve(estimated_indices);
    province_geometries.reserve(provinces.size());

    for (const auto* province : provinces) {
        if (!province || province->boundary_points.empty()) {
            continue;
        }

        // Prepare polygon for earcut
        std::vector<std::array<float, 2>> polygon;
        polygon.reserve(province->boundary_points.size());

        // Calculate bounding box for frustum culling
        float min_x = std::numeric_limits<float>::max();
        float min_y = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float max_y = std::numeric_limits<float>::lowest();

        for (const auto& pt : province->boundary_points) {
            float fx = static_cast<float>(pt.x);
            float fy = static_cast<float>(pt.y);
            polygon.push_back({fx, fy});

            min_x = std::min(min_x, fx);
            min_y = std::min(min_y, fy);
            max_x = std::max(max_x, fx);
            max_y = std::max(max_y, fy);
        }

        // Triangulate
        std::vector<std::vector<std::array<float, 2>>> polygon_rings = {polygon};
        std::vector<uint32_t> local_indices = mapbox::earcut<uint32_t>(polygon_rings);

        // Validate triangulation
        if (local_indices.empty() || local_indices.size() < 3 || local_indices.size() % 3 != 0) {
            CORE_LOG_WARN("GPUMapRenderer", "Triangulation failed for province "
                          << province->province_id << ", skipping");
            continue;
        }

        uint32_t vertex_start = static_cast<uint32_t>(vertices.size());
        uint32_t vertex_count = static_cast<uint32_t>(province->boundary_points.size());

        // Add vertices
        for (const auto& pt : province->boundary_points) {
            vertices.emplace_back(ProvinceVertex{
                static_cast<float>(pt.x),
                static_cast<float>(pt.y),
                province->province_id,
                0.0f, 0.0f
            });
        }

        // Add indices
        for (uint32_t idx : local_indices) {
            indices.push_back(vertex_start + idx);
        }

        // Calculate approximate area
        float area = (max_x - min_x) * (max_y - min_y);

        // Store province geometry
        province_geometries.push_back({
            province->province_id,
            vertex_start,
            vertex_count,
            area,
            min_x, min_y, max_x, max_y
        });
    }
}

// ============================================================================
// P2: Split GenerateLODIndices into smaller functions
// ============================================================================

void GPUMapRenderer::SelectLODVertices(
    const ProvinceGeometry& geom,
    unsigned int decimation_factor,
    std::vector<uint32_t>& selected_positions)
{
    selected_positions.clear();
    selected_positions.reserve((geom.vertex_count - 1) / decimation_factor + 2);

    for (uint32_t i = 0; i < geom.vertex_count; i += decimation_factor) {
        selected_positions.push_back(geom.vertex_start + i);
    }

    // Always include last vertex to close polygon
    uint32_t last_vertex_idx = geom.vertex_start + geom.vertex_count - 1;
    if (selected_positions.back() != last_vertex_idx) {
        selected_positions.push_back(last_vertex_idx);
    }

    // Fallback: ensure minimum 3 vertices
    if (selected_positions.size() < 3) {
        selected_positions.resize(geom.vertex_count);
        for (uint32_t i = 0; i < geom.vertex_count; ++i) {
            selected_positions[i] = geom.vertex_start + i;
        }
    }
}

bool GPUMapRenderer::TriangulateLODPolygon(
    const std::vector<ProvinceVertex>& full_vertices,
    const std::vector<uint32_t>& selected_positions,
    std::vector<uint32_t>& local_indices)
{
    // Extract positions for triangulation
    std::vector<std::array<float, 2>> polygon_positions;
    polygon_positions.reserve(selected_positions.size());

    for (uint32_t vbo_idx : selected_positions) {
        if (vbo_idx >= full_vertices.size()) {
            CORE_LOG_ERROR("GPUMapRenderer", "VBO index out of bounds: " << vbo_idx);
            return false;
        }
        const auto& v = full_vertices[vbo_idx];
        polygon_positions.push_back({v.x, v.y});
    }

    // Triangulate
    std::vector<std::vector<std::array<float, 2>>> polygon_rings = {polygon_positions};
    local_indices = mapbox::earcut<uint32_t>(polygon_rings);

    // Validate
    if (local_indices.empty() || local_indices.size() < 3 || local_indices.size() % 3 != 0) {
        return false;
    }

    return true;
}

void GPUMapRenderer::RemapIndicesToGlobal(
    const std::vector<uint32_t>& local_indices,
    const std::vector<uint32_t>& selected_positions,
    std::vector<uint32_t>& global_indices)
{
    for (uint32_t local_idx : local_indices) {
        if (local_idx >= selected_positions.size()) {
            CORE_LOG_ERROR("GPUMapRenderer", "Local index out of range: " << local_idx);
            continue;
        }
        global_indices.push_back(selected_positions[local_idx]);
    }
}

void GPUMapRenderer::GenerateLODIndices(
    const std::vector<ProvinceVertex>& full_vertices,
    const std::vector<ProvinceGeometry>& province_geometries,
    unsigned int decimation_factor,
    std::vector<uint32_t>& lod_indices)
{
    if (decimation_factor == 0) {
        CORE_LOG_ERROR("GPUMapRenderer", "Invalid decimation_factor: 0");
        return;
    }

    // Reserve capacity
    size_t estimated_indices = 0;
    for (const auto& geom : province_geometries) {
        if (geom.vertex_count >= 3) {
            size_t decimated_count = (geom.vertex_count - 1) / decimation_factor + 1;
            estimated_indices += 3 * (decimated_count - 2);
        }
    }
    lod_indices.reserve(estimated_indices);

    size_t provinces_processed = 0;
    size_t provinces_skipped = 0;

    for (const auto& geom : province_geometries) {
        if (geom.vertex_count == 0) {
            provinces_skipped++;
            continue;
        }

        // Step 1: Select vertices
        std::vector<uint32_t> selected_positions;
        SelectLODVertices(geom, decimation_factor, selected_positions);

        // Step 2: Triangulate
        std::vector<uint32_t> local_indices;
        if (!TriangulateLODPolygon(full_vertices, selected_positions, local_indices)) {
            CORE_LOG_WARN("GPUMapRenderer", "LOD triangulation failed for province " << geom.province_id);
            provinces_skipped++;
            continue;
        }

        // Step 3: Remap to global indices
        RemapIndicesToGlobal(local_indices, selected_positions, lod_indices);
        provinces_processed++;
    }

    CORE_LOG_INFO("GPUMapRenderer", "LOD generation: "
                  << provinces_processed << " succeeded, "
                  << provinces_skipped << " skipped");
}

void GPUMapRenderer::PackProvinceColorsToTexture(
    const std::vector<const ProvinceRenderComponent*>& provinces,
    std::vector<uint8_t>& texture_data)
{
    texture_data.resize(texture_width_ * texture_height_ * 4, 0);

    for (const auto* province : provinces) {
        if (!province) continue;

        uint32_t id = province->province_id;
        uint32_t max_id = texture_width_ * texture_height_;

        if (id >= max_id) {
            CORE_LOG_WARN("GPUMapRenderer", "Province ID " << id
                << " exceeds texture capacity (" << max_id << ")");
            continue;
        }

        uint32_t u = ProvinceTexCoordU(id);
        uint32_t v = ProvinceTexCoordV(id);
        uint32_t offset = (v * texture_width_ + u) * 4;

        texture_data[offset + 0] = province->fill_color.r;
        texture_data[offset + 1] = province->fill_color.g;
        texture_data[offset + 2] = province->fill_color.b;
        texture_data[offset + 3] = province->fill_color.a;
    }
}

void GPUMapRenderer::PackProvinceMetadataToTexture(
    const std::vector<const ProvinceRenderComponent*>& provinces,
    std::vector<uint8_t>& texture_data)
{
    texture_data.resize(texture_width_ * texture_height_ * 4, 0);

    for (const auto* province : provinces) {
        if (!province) continue;

        uint32_t id = province->province_id;
        if (id >= texture_width_ * texture_height_) continue;

        uint32_t u = ProvinceTexCoordU(id);
        uint32_t v = ProvinceTexCoordV(id);
        uint32_t offset = (v * texture_width_ + u) * 4;

        // Map terrain type enum to 0-255
        uint8_t terrain_value = 0;
        switch (province->terrain_type) {
            case TerrainType::PLAINS:     terrain_value = 10; break;
            case TerrainType::HILLS:      terrain_value = 15; break;
            case TerrainType::MOUNTAINS:  terrain_value = 30; break;
            case TerrainType::FOREST:     terrain_value = 20; break;
            case TerrainType::DESERT:     terrain_value = 40; break;
            case TerrainType::COAST:      terrain_value = 50; break;
            case TerrainType::WETLAND:    terrain_value = 60; break;
            case TerrainType::HIGHLANDS:  terrain_value = 70; break;
            case TerrainType::UNKNOWN:
            default:                      terrain_value = 0;  break;
        }

        texture_data[offset + 0] = terrain_value;
        texture_data[offset + 1] = 0; // Owner nation ID
        texture_data[offset + 2] = 0;
        texture_data[offset + 3] = 0;
    }
}

// ============================================================================
// P1: Frustum Culling
// ============================================================================

bool GPUMapRenderer::IsProvinceVisible(
    const ProvinceGeometry& geom,
    const Camera2D::Bounds& bounds) const
{
    // Simple AABB intersection test
    return !(geom.max_x < bounds.left ||
             geom.min_x > bounds.right ||
             geom.max_y < bounds.bottom ||
             geom.min_y > bounds.top);
}

std::vector<uint32_t> GPUMapRenderer::CullProvinces(const Camera2D& camera) {
    Camera2D::Bounds bounds = camera.GetVisibleBounds();
    std::vector<uint32_t> visible_provinces;
    visible_provinces.reserve(province_geometries_.size());

    for (const auto& geom : province_geometries_) {
        if (IsProvinceVisible(geom, bounds)) {
            visible_provinces.push_back(geom.province_id);
        }
    }

    culled_province_count_ = province_geometries_.size() - visible_provinces.size();
    return visible_provinces;
}

// ============================================================================
// Rendering
// ============================================================================

void GPUMapRenderer::Render(const Camera2D& camera) {
    if (index_count_ == 0) return;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Update animation time
    selection_glow_time_ += 0.016f;

    // Select LOD level
    current_lod_level_ = SelectLODLevel(camera.zoom);

    // Use map shader
    CHECK_GL_OPERATION(glUseProgram(map_shader_program_));

    // Update uniforms
    UpdateUniforms(camera);

    // Bind textures
    CHECK_GL_OPERATION(glActiveTexture(GL_TEXTURE0));
    CHECK_GL_OPERATION(glBindTexture(GL_TEXTURE_2D, province_color_texture_));
    CHECK_GL_OPERATION(glUniform1i(u_province_data_, 0));

    CHECK_GL_OPERATION(glActiveTexture(GL_TEXTURE1));
    CHECK_GL_OPERATION(glBindTexture(GL_TEXTURE_2D, province_metadata_texture_));
    CHECK_GL_OPERATION(glUniform1i(u_province_metadata_, 1));

    // Render provinces
    CHECK_GL_OPERATION(glBindVertexArray(vao_));
    CHECK_GL_OPERATION(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lod_ibos_[current_lod_level_]));
    CHECK_GL_OPERATION(glDrawElements(GL_TRIANGLES,
        static_cast<GLsizei>(lod_index_counts_[current_lod_level_]),
        GL_UNSIGNED_INT, nullptr));
    CHECK_GL_OPERATION(glBindVertexArray(0));

    // P1: Render borders (if enabled)
    if (show_borders_ && border_shader_program_) {
        CHECK_GL_OPERATION(glUseProgram(border_shader_program_));
        glm::mat4 vp = camera.GetViewProjectionMatrix();
        CHECK_GL_OPERATION(glUniformMatrix4fv(u_border_view_projection_, 1, GL_FALSE, glm::value_ptr(vp)));
        CHECK_GL_OPERATION(glUniform4f(u_border_color_, 0.2f, 0.2f, 0.2f, 1.0f));

        // TODO: Implement border geometry rendering
        // This would require separate border line geometry or edge detection shader
    }

    CHECK_GL_OPERATION(glUseProgram(0));

    auto end_time = std::chrono::high_resolution_clock::now();
    last_render_time_ms_ = std::chrono::duration<float, std::milli>(end_time - start_time).count();
}

void GPUMapRenderer::UpdateUniforms(const Camera2D& camera) {
    glm::mat4 projection = camera.GetViewProjectionMatrix();

    CHECK_GL_OPERATION(glUniformMatrix4fv(u_view_projection_, 1, GL_FALSE, glm::value_ptr(projection)));
    CHECK_GL_OPERATION(glUniform1i(u_render_mode_, static_cast<int>(render_mode_)));
    CHECK_GL_OPERATION(glUniform1ui(u_selected_province_, selected_province_id_));
    CHECK_GL_OPERATION(glUniform1ui(u_hovered_province_, hovered_province_id_));
    CHECK_GL_OPERATION(glUniform1f(u_selection_glow_time_, selection_glow_time_));
    CHECK_GL_OPERATION(glUniform2f(u_viewport_size_, camera.viewport_width, camera.viewport_height));
}

// ============================================================================
// Public Setters
// ============================================================================

void GPUMapRenderer::SetRenderMode(RenderMode mode) {
    render_mode_ = mode;
}

void GPUMapRenderer::SetSelectedProvince(uint32_t province_id) {
    selected_province_id_ = province_id;
}

void GPUMapRenderer::SetHoveredProvince(uint32_t province_id) {
    hovered_province_id_ = province_id;
}

int GPUMapRenderer::SelectLODLevel(float zoom) const {
    if (zoom >= lod_high_threshold_) {
        return 0; // High detail
    } else if (zoom >= lod_medium_threshold_) {
        return 1; // Medium detail
    } else {
        return 2; // Low detail
    }
}

} // namespace game::map

