// ============================================================================
// GPUMapRenderer.cpp - GPU-Accelerated Map Renderer Implementation
// Created: December 21, 2025
// ============================================================================

#include "map/render/GPUMapRenderer.h"
#include "core/logging/Logger.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

// External triangulation library (header-only)
// Install via: FetchContent or manually add to include path
// GitHub: https://github.com/mapbox/earcut.hpp
#include <mapbox/earcut.hpp>

namespace game::map {

// ============================================================================
// Constructor / Destructor
// ============================================================================

GPUMapRenderer::GPUMapRenderer(::core::ecs::EntityManager& entity_manager)
    : entity_manager_(entity_manager)
    , vao_(0)
    , vbo_(0)
    , ibo_(0)
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
    , vertex_count_(0)
    , index_count_(0)
    , province_count_(0)
    , render_mode_(RenderMode::POLITICAL)
    , selected_province_id_(0)
    , hovered_province_id_(0)
    , selection_glow_time_(0.0f)
    , show_borders_(true)
    , show_names_(true)
    , last_render_time_ms_(0.0f)
{
}

GPUMapRenderer::~GPUMapRenderer() {
    // Cleanup OpenGL resources
    if (vao_) {
        glDeleteVertexArrays(1, &vao_);
    }
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
    }
    if (ibo_) {
        glDeleteBuffers(1, &ibo_);
    }
    if (province_color_texture_) {
        glDeleteTextures(1, &province_color_texture_);
    }
    if (province_metadata_texture_) {
        glDeleteTextures(1, &province_metadata_texture_);
    }
    if (map_shader_program_) {
        glDeleteProgram(map_shader_program_);
    }
    if (border_shader_program_) {
        glDeleteProgram(border_shader_program_);
    }
}

// ============================================================================
// Initialization
// ============================================================================

bool GPUMapRenderer::Initialize() {
    CORE_LOG_INFO("GPUMapRenderer", "Initializing GPU-accelerated map renderer...");

    // Load and compile shaders
    if (!LoadShaders()) {
        CORE_LOG_ERROR("GPUMapRenderer", "Failed to load shaders");
        return false;
    }

    // Create OpenGL buffers
    if (!CreateBuffers()) {
        CORE_LOG_ERROR("GPUMapRenderer", "Failed to create buffers");
        return false;
    }

    // Create textures
    if (!CreateTextures()) {
        CORE_LOG_ERROR("GPUMapRenderer", "Failed to create textures");
        return false;
    }

    CORE_LOG_INFO("GPUMapRenderer", "GPU map renderer initialized successfully");
    return true;
}

bool GPUMapRenderer::LoadShaders() {
    CORE_LOG_INFO("GPUMapRenderer", "Loading shaders...");

    // Read shader source files
    std::string vert_source = ReadShaderFile("shaders/map.vert");
    std::string frag_source = ReadShaderFile("shaders/map.frag");

    if (vert_source.empty() || frag_source.empty()) {
        CORE_LOG_ERROR("GPUMapRenderer", "Failed to read shader files");
        return false;
    }

    // Compile shaders
    GLuint vert_shader = CompileShader(GL_VERTEX_SHADER, vert_source);
    if (!vert_shader) {
        return false;
    }

    GLuint frag_shader = CompileShader(GL_FRAGMENT_SHADER, frag_source);
    if (!frag_shader) {
        glDeleteShader(vert_shader);
        return false;
    }

    // Link program
    map_shader_program_ = LinkProgram(vert_shader, frag_shader);
    if (!map_shader_program_) {
        glDeleteShader(vert_shader);
        glDeleteShader(frag_shader);
        return false;
    }

    // Cleanup intermediate shaders
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);

    // Get uniform locations
    u_view_projection_ = glGetUniformLocation(map_shader_program_, "view_projection");
    u_render_mode_ = glGetUniformLocation(map_shader_program_, "render_mode");
    u_selected_province_ = glGetUniformLocation(map_shader_program_, "selected_province_id");
    u_hovered_province_ = glGetUniformLocation(map_shader_program_, "hovered_province_id");
    u_selection_glow_time_ = glGetUniformLocation(map_shader_program_, "selection_glow_time");
    u_province_data_ = glGetUniformLocation(map_shader_program_, "province_data");
    u_province_metadata_ = glGetUniformLocation(map_shader_program_, "province_metadata");
    u_viewport_size_ = glGetUniformLocation(map_shader_program_, "viewport_size");

    CORE_LOG_INFO("GPUMapRenderer", "Shaders loaded and linked successfully");
    return true;
}

bool GPUMapRenderer::CreateBuffers() {
    CORE_LOG_INFO("GPUMapRenderer", "Creating OpenGL buffers...");

    // Generate VAO
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    // Generate VBO (vertices)
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    // Set up vertex attributes
    // Layout: vec2 position, uint province_id, vec2 uv
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ProvinceVertex),
                          (void*)offsetof(ProvinceVertex, x));

    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(ProvinceVertex),
                           (void*)offsetof(ProvinceVertex, province_id));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ProvinceVertex),
                          (void*)offsetof(ProvinceVertex, u));

    // Generate IBO (indices)
    glGenBuffers(1, &ibo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);

    // Unbind VAO
    glBindVertexArray(0);

    CORE_LOG_INFO("GPUMapRenderer", "OpenGL buffers created successfully");
    return true;
}

bool GPUMapRenderer::CreateTextures() {
    CORE_LOG_INFO("GPUMapRenderer", "Creating province textures...");

    // Create province color texture (256x256 RGBA8)
    glGenTextures(1, &province_color_texture_);
    glBindTexture(GL_TEXTURE_2D, province_color_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Allocate empty texture (will be filled during UploadProvinceData)
    std::vector<uint8_t> empty_data(256 * 256 * 4, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, empty_data.data());

    // Create province metadata texture (256x256 RGBA8)
    glGenTextures(1, &province_metadata_texture_);
    glBindTexture(GL_TEXTURE_2D, province_metadata_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, empty_data.data());

    glBindTexture(GL_TEXTURE_2D, 0);

    CORE_LOG_INFO("GPUMapRenderer", "Province textures created successfully");
    return true;
}

// ============================================================================
// Shader Compilation Helpers
// ============================================================================

std::string GPUMapRenderer::ReadShaderFile(const std::string& filepath) {
    // Try multiple paths (bin directory vs source directory)
    std::vector<std::string> paths = {
        filepath,
        "bin/" + filepath,
        "../" + filepath,
        "../../" + filepath
    };

    for (const auto& path : paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            CORE_LOG_INFO("GPUMapRenderer", "Loaded shader: " << path);
            return buffer.str();
        }
    }

    CORE_LOG_ERROR("GPUMapRenderer", "Failed to open shader file: " << filepath);
    return "";
}

GLuint GPUMapRenderer::CompileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* source_cstr = source.c_str();
    glShaderSource(shader, 1, &source_cstr, nullptr);
    glCompileShader(shader);

    // Check compilation status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        const char* shader_type = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        CORE_LOG_ERROR("GPUMapRenderer", shader_type << " shader compilation failed: " << info_log);
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

    // Check linking status
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(program, 512, nullptr, info_log);
        CORE_LOG_ERROR("GPUMapRenderer", "Shader program linking failed: " << info_log);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

// ============================================================================
// Data Upload
// ============================================================================

bool GPUMapRenderer::UploadProvinceData(const std::vector<const ProvinceRenderComponent*>& provinces) {
    CORE_LOG_INFO("GPUMapRenderer", "Uploading province data to GPU...");

    province_count_ = provinces.size();

    // Step 1: Triangulate all provinces
    std::vector<ProvinceVertex> vertices;
    std::vector<uint32_t> indices;
    TriangulateProvinces(provinces, vertices, indices);

    vertex_count_ = vertices.size();
    index_count_ = indices.size();

    CORE_LOG_INFO("GPUMapRenderer", "Triangulated: " << vertex_count_ << " vertices, "
                  << index_count_ << " indices (" << index_count_ / 3 << " triangles)");

    // Step 2: Upload vertex data to GPU
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(ProvinceVertex),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // Step 3: Upload index data to GPU
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(uint32_t),
                 indices.data(),
                 GL_STATIC_DRAW);

    // Step 4: Pack province colors into texture
    std::vector<uint8_t> color_texture_data;
    PackProvinceColorsToTexture(provinces, color_texture_data);

    glBindTexture(GL_TEXTURE_2D, province_color_texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256,
                    GL_RGBA, GL_UNSIGNED_BYTE, color_texture_data.data());

    // Step 5: Pack province metadata into texture
    std::vector<uint8_t> metadata_texture_data;
    PackProvinceMetadataToTexture(provinces, metadata_texture_data);

    glBindTexture(GL_TEXTURE_2D, province_metadata_texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256,
                    GL_RGBA, GL_UNSIGNED_BYTE, metadata_texture_data.data());

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    CORE_LOG_INFO("GPUMapRenderer", "Province data uploaded successfully");
    return true;
}

void GPUMapRenderer::TriangulateProvinces(
    const std::vector<const ProvinceRenderComponent*>& provinces,
    std::vector<ProvinceVertex>& vertices,
    std::vector<uint32_t>& indices)
{
    for (const auto* province : provinces) {
        if (!province || province->boundary_points.empty()) {
            continue;
        }

        // Prepare polygon for earcut (requires std::array<float, 2>)
        std::vector<std::array<float, 2>> polygon;
        polygon.reserve(province->boundary_points.size());

        for (const auto& pt : province->boundary_points) {
            polygon.push_back({static_cast<float>(pt.x), static_cast<float>(pt.y)});
        }

        // Triangulate using earcut
        std::vector<std::vector<std::array<float, 2>>> polygon_rings = {polygon};
        std::vector<uint32_t> local_indices = mapbox::earcut<uint32_t>(polygon_rings);

        // Add vertices
        uint32_t base_vertex = static_cast<uint32_t>(vertices.size());
        for (const auto& pt : province->boundary_points) {
            ProvinceVertex vert;
            vert.x = static_cast<float>(pt.x);
            vert.y = static_cast<float>(pt.y);
            vert.province_id = province->province_id;
            vert.u = 0.0f;
            vert.v = 0.0f;
            vertices.push_back(vert);
        }

        // Add indices (offset by base_vertex)
        for (uint32_t idx : local_indices) {
            indices.push_back(base_vertex + idx);
        }
    }
}

void GPUMapRenderer::PackProvinceColorsToTexture(
    const std::vector<const ProvinceRenderComponent*>& provinces,
    std::vector<uint8_t>& texture_data)
{
    // Create 256x256 RGBA texture
    texture_data.resize(256 * 256 * 4, 0);

    for (const auto* province : provinces) {
        if (!province) continue;

        uint32_t id = province->province_id;
        if (id >= 256 * 256) {
            CORE_LOG_WARN("GPUMapRenderer", "Province ID " << id << " exceeds texture size (65536)");
            continue;
        }

        uint32_t u = id % 256;
        uint32_t v = id / 256;
        uint32_t offset = (v * 256 + u) * 4;

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
    // Create 256x256 RGBA texture
    // R = terrain type (0-255)
    // G = owner nation ID (0-255)
    // B = unused
    // A = unused
    texture_data.resize(256 * 256 * 4, 0);

    for (const auto* province : provinces) {
        if (!province) continue;

        uint32_t id = province->province_id;
        if (id >= 256 * 256) continue;

        uint32_t u = id % 256;
        uint32_t v = id / 256;
        uint32_t offset = (v * 256 + u) * 4;

        // Map terrain type enum to 0-255
        uint8_t terrain_value = 0;
        if (province->terrain_type == TerrainType::PLAINS) terrain_value = 10;
        else if (province->terrain_type == TerrainType::FOREST) terrain_value = 20;
        else if (province->terrain_type == TerrainType::MOUNTAINS) terrain_value = 30;
        else if (province->terrain_type == TerrainType::DESERT) terrain_value = 40;
        else if (province->terrain_type == TerrainType::SNOW) terrain_value = 50;
        else if (province->terrain_type == TerrainType::WATER) terrain_value = 60;

        texture_data[offset + 0] = terrain_value;
        texture_data[offset + 1] = 0; // Owner nation ID (TODO: link to RealmComponent)
        texture_data[offset + 2] = 0;
        texture_data[offset + 3] = 0;
    }
}

// ============================================================================
// Rendering
// ============================================================================

void GPUMapRenderer::Render(const Camera2D& camera) {
    if (index_count_ == 0) {
        // No geometry uploaded yet
        return;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Update selection glow animation
    selection_glow_time_ += 0.016f; // Assume ~60 FPS (will be corrected by delta time)

    // Use map shader
    glUseProgram(map_shader_program_);

    // Update uniforms
    UpdateUniforms(camera);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, province_color_texture_);
    glUniform1i(u_province_data_, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, province_metadata_texture_);
    glUniform1i(u_province_metadata_, 1);

    // Bind VAO and draw
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(index_count_), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Reset state
    glUseProgram(0);

    auto end_time = std::chrono::high_resolution_clock::now();
    last_render_time_ms_ = std::chrono::duration<float, std::milli>(end_time - start_time).count();
}

void GPUMapRenderer::UpdateUniforms(const Camera2D& camera) {
    // Calculate view-projection matrix
    float view_projection[16];
    CalculateViewProjectionMatrix(camera, view_projection);

    // Upload uniforms
    glUniformMatrix4fv(u_view_projection_, 1, GL_FALSE, view_projection);
    glUniform1i(u_render_mode_, static_cast<int>(render_mode_));
    glUniform1ui(u_selected_province_, selected_province_id_);
    glUniform1ui(u_hovered_province_, hovered_province_id_);
    glUniform1f(u_selection_glow_time_, selection_glow_time_);
    glUniform2f(u_viewport_size_, camera.viewport_width, camera.viewport_height);
}

void GPUMapRenderer::CalculateViewProjectionMatrix(const Camera2D& camera, float* matrix_out) {
    // Calculate orthographic projection matrix
    // Maps world coordinates to normalized device coordinates [-1, 1]

    float half_width = (camera.viewport_width / camera.zoom) / 2.0f;
    float half_height = (camera.viewport_height / camera.zoom) / 2.0f;

    float left = camera.position.x - half_width;
    float right = camera.position.x + half_width;
    float bottom = camera.position.y - half_height;
    float top = camera.position.y + half_height;

    // Orthographic projection matrix (column-major)
    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        -(right + left) / (right - left), -(top + bottom) / (top - bottom), 0.0f, 1.0f
    };

    // Copy to output
    for (int i = 0; i < 16; ++i) {
        matrix_out[i] = projection[i];
    }
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

} // namespace game::map
