// ============================================================================
// GPUMapRenderer.cpp - GPU-Accelerated Map Renderer Implementation
// Created: December 21, 2025
// ============================================================================

#include "map/render/GPUMapRenderer.h"
#include "core/logging/Logger.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>

// GLM - Math library for OpenGL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// External triangulation library (header-only)
// Install via: FetchContent or manually add to include path
// GitHub: https://github.com/mapbox/earcut.hpp
#include <mapbox/earcut.hpp>

// OpenGL error checking macro (debug builds only)
#ifdef _DEBUG
#define CHECK_GL_ERROR() \
    { \
        GLenum err; \
        while ((err = glGetError()) != GL_NO_ERROR) { \
            CORE_LOG_ERROR("OpenGL", "Error 0x" << std::hex << err << std::dec \
                           << " at " << __FILE__ << ":" << __LINE__); \
        } \
    }
#else
#define CHECK_GL_ERROR() // No-op in release builds
#endif

namespace game::map {

// ============================================================================
// Constructor / Destructor
// ============================================================================

GPUMapRenderer::GPUMapRenderer(::core::ecs::EntityManager& entity_manager)
    : entity_manager_(entity_manager)
    , vao_(0)
    , vbo_(0)
    , lod_ibos_{0, 0, 0}
    , lod_index_counts_{0, 0, 0}
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
    , current_lod_level_(0)
    , last_render_time_ms_(0.0f)
    , lod_high_threshold_(1.5f)
    , lod_medium_threshold_(0.75f)
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
    // Clean up all LOD index buffers
    for (int i = 0; i < LOD_COUNT; ++i) {
        if (lod_ibos_[i]) {
            glDeleteBuffers(1, &lod_ibos_[i]);
        }
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
    CHECK_GL_ERROR();
    glBindVertexArray(vao_);
    CHECK_GL_ERROR();

    // Generate VBO (vertices)
    glGenBuffers(1, &vbo_);
    CHECK_GL_ERROR();
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    CHECK_GL_ERROR();

    // Set up vertex attributes
    // Layout: vec2 position, uint province_id, vec2 uv
    glEnableVertexAttribArray(0);
    CHECK_GL_ERROR();
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ProvinceVertex),
                          (void*)offsetof(ProvinceVertex, x));
    CHECK_GL_ERROR();

    glEnableVertexAttribArray(1);
    CHECK_GL_ERROR();
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, sizeof(ProvinceVertex),
                           (void*)offsetof(ProvinceVertex, province_id));
    CHECK_GL_ERROR();

    glEnableVertexAttribArray(2);
    CHECK_GL_ERROR();
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ProvinceVertex),
                          (void*)offsetof(ProvinceVertex, u));
    CHECK_GL_ERROR();

    // Generate LOD IBOs (indices for each level of detail)
    glGenBuffers(LOD_COUNT, lod_ibos_);
    CHECK_GL_ERROR();
    // Bind the first LOD IBO to the VAO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lod_ibos_[0]);
    CHECK_GL_ERROR();

    // Unbind VAO
    glBindVertexArray(0);
    CHECK_GL_ERROR();

    CORE_LOG_INFO("GPUMapRenderer", "OpenGL buffers created successfully");
    return true;
}

bool GPUMapRenderer::CreateTextures() {
    CORE_LOG_INFO("GPUMapRenderer", "Creating province textures...");

    // Query max texture size supported by GPU
    GLint max_texture_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    CHECK_GL_ERROR();

    if (max_texture_size < 256) {
        CORE_LOG_ERROR("GPUMapRenderer", "GPU does not support 256x256 textures (max: "
                       << max_texture_size << ")");
        return false;
    }

    // Create province color texture (256x256 RGBA8)
    glGenTextures(1, &province_color_texture_);
    CHECK_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, province_color_texture_);
    CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR();

    // Allocate empty texture (will be filled during UploadProvinceData)
    std::vector<uint8_t> empty_data(256 * 256 * 4, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, empty_data.data());
    CHECK_GL_ERROR();

    // Create province metadata texture (256x256 RGBA8)
    glGenTextures(1, &province_metadata_texture_);
    CHECK_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, province_metadata_texture_);
    CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, empty_data.data());
    CHECK_GL_ERROR();

    glBindTexture(GL_TEXTURE_2D, 0);
    CHECK_GL_ERROR();

    CORE_LOG_INFO("GPUMapRenderer", "Province textures created successfully");
    return true;
}

// ============================================================================
// Shader Compilation Helpers
// ============================================================================

std::string GPUMapRenderer::ReadShaderFile(const std::string& filepath) {
    // Get current working directory
    std::filesystem::path cwd = std::filesystem::current_path();

    // Try multiple paths relative to executable location
    std::vector<std::filesystem::path> search_paths = {
        cwd / filepath,                      // ./shaders/map.vert
        cwd / "bin" / filepath,             // ./bin/shaders/map.vert
        cwd / ".." / filepath,              // ../shaders/map.vert
        cwd / ".." / "bin" / filepath,      // ../bin/shaders/map.vert
        cwd / ".." / ".." / filepath,       // ../../shaders/map.vert
        cwd / ".." / ".." / "bin" / filepath // ../../bin/shaders/map.vert
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

    // Failed to find shader - log all attempted paths
    CORE_LOG_ERROR("GPUMapRenderer", "Failed to find shader: " << filepath);
    CORE_LOG_ERROR("GPUMapRenderer", "Searched paths:");
    for (const auto& path : search_paths) {
        CORE_LOG_ERROR("GPUMapRenderer", "  - " << path.string()
                       << (std::filesystem::exists(path) ? " (exists but failed to open)" : " (not found)"));
    }

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
    CORE_LOG_INFO("GPUMapRenderer", "Uploading province data to GPU with multi-LOD support...");

    province_count_ = provinces.size();

    // Step 1: Triangulate provinces at full detail (shared vertices + geometry mapping)
    std::vector<ProvinceVertex> vertices;
    std::vector<uint32_t> full_indices;
    province_geometries_.clear();
    TriangulateProvinces(provinces, vertices, full_indices, province_geometries_);

    vertex_count_ = vertices.size();
    index_count_ = full_indices.size();

    CORE_LOG_INFO("GPUMapRenderer", "Full detail: " << vertex_count_ << " vertices, "
                  << index_count_ << " indices (" << index_count_ / 3 << " triangles), "
                  << province_geometries_.size() << " geometries");

    // Step 2: Upload vertex data to GPU (shared across all LOD levels)
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(ProvinceVertex),
                 vertices.data(),
                 GL_STATIC_DRAW);
    CHECK_GL_ERROR();

    // Step 3: Generate and upload multi-LOD index buffers
    // LOD 0 (high detail): All triangles
    // LOD 1 (medium detail): Decimated (every 2nd vertex)
    // LOD 2 (low detail): Heavily decimated (every 4th vertex)

    for (int lod = 0; lod < LOD_COUNT; ++lod) {
        std::vector<uint32_t> lod_indices;
        int decimation_factor = (1 << lod);  // 1, 2, 4

        if (lod == 0) {
            // LOD 0: use full detail indices directly (no decimation)
            lod_index_counts_[0] = full_indices.size();
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lod_ibos_[0]);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         full_indices.size() * sizeof(uint32_t),
                         full_indices.data(),
                         GL_STATIC_DRAW);
            CHECK_GL_ERROR();

            CORE_LOG_INFO("GPUMapRenderer", "LOD 0 (High): "
                          << full_indices.size() / 3 << " triangles");
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
            CHECK_GL_ERROR();

            CORE_LOG_INFO("GPUMapRenderer", "LOD " << lod << " (decimation 1/" << decimation_factor << "): "
                          << lod_indices.size() / 3 << " triangles");
        }
    }

    // Step 4: Pack province colors into texture
    std::vector<uint8_t> color_texture_data;
    PackProvinceColorsToTexture(provinces, color_texture_data);

    glBindTexture(GL_TEXTURE_2D, province_color_texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256,
                    GL_RGBA, GL_UNSIGNED_BYTE, color_texture_data.data());
    CHECK_GL_ERROR();

    // Step 5: Pack province metadata into texture
    std::vector<uint8_t> metadata_texture_data;
    PackProvinceMetadataToTexture(provinces, metadata_texture_data);

    glBindTexture(GL_TEXTURE_2D, province_metadata_texture_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256,
                    GL_RGBA, GL_UNSIGNED_BYTE, metadata_texture_data.data());
    CHECK_GL_ERROR();

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    CORE_LOG_INFO("GPUMapRenderer", "Province data uploaded successfully");
    return true;
}

void GPUMapRenderer::TriangulateProvinces(
    const std::vector<const ProvinceRenderComponent*>& provinces,
    std::vector<ProvinceVertex>& vertices,
    std::vector<uint32_t>& indices,
    std::vector<ProvinceGeometry>& province_geometries)
{
    // Reserve capacity upfront to avoid reallocations
    size_t estimated_vertices = 0;
    size_t estimated_indices = 0;
    for (const auto* province : provinces) {
        if (province) {
            estimated_vertices += province->boundary_points.size();
            // Rough estimate: assume average 3x vertices for triangles
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

        // Prepare polygon for earcut (requires std::array<float, 2>)
        std::vector<std::array<float, 2>> polygon;
        polygon.reserve(province->boundary_points.size());

        for (const auto& pt : province->boundary_points) {
            polygon.push_back({static_cast<float>(pt.x), static_cast<float>(pt.y)});
        }

        // Triangulate using earcut
        std::vector<std::vector<std::array<float, 2>>> polygon_rings = {polygon};
        std::vector<uint32_t> local_indices = mapbox::earcut<uint32_t>(polygon_rings);

        // Validate triangulation result
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

        // Validate all indices are within bounds of the polygon vertex count
        uint32_t vertex_count = static_cast<uint32_t>(province->boundary_points.size());
        bool indices_valid = true;
        for (uint32_t idx : local_indices) {
            if (idx >= vertex_count) {
                CORE_LOG_ERROR("GPUMapRenderer", "Index out of bounds for province "
                               << province->province_id << ": index=" << idx
                               << ", vertex_count=" << vertex_count);
                indices_valid = false;
                break;
            }
        }
        if (!indices_valid) {
            continue; // Skip this province
        }

        // Track province geometry mapping for LOD generation
        uint32_t vertex_start = static_cast<uint32_t>(vertices.size());
        uint32_t vertex_count = static_cast<uint32_t>(province->boundary_points.size());

        // Add vertices (use emplace_back for efficiency)
        for (const auto& pt : province->boundary_points) {
            vertices.emplace_back(ProvinceVertex{
                static_cast<float>(pt.x),
                static_cast<float>(pt.y),
                province->province_id,
                0.0f, 0.0f
            });
        }

        // Add indices (offset by vertex_start)
        for (uint32_t idx : local_indices) {
            indices.push_back(vertex_start + idx);
        }

        // Store province geometry metadata
        province_geometries.push_back({province->province_id, vertex_start, vertex_count});
    }
}

void GPUMapRenderer::GenerateLODIndices(
    const std::vector<ProvinceVertex>& full_vertices,
    const std::vector<ProvinceGeometry>& province_geometries,
    unsigned int decimation_factor,
    std::vector<uint32_t>& lod_indices)
{
    // Validate decimation factor
    if (decimation_factor == 0) {
        CORE_LOG_ERROR("GPUMapRenderer", "Invalid decimation_factor: 0 (must be > 0)");
        return;
    }

    // Reserve capacity
    size_t estimated_indices = 0;
    for (const auto& geom : province_geometries) {
        // Avoid overflow: use safer calculation
        size_t decimated_count = (geom.vertex_count == 0) ? 0 : ((geom.vertex_count - 1) / decimation_factor + 1);
        estimated_indices += decimated_count * 3;
    }
    lod_indices.reserve(estimated_indices);

    size_t vbo_size = full_vertices.size();

    // Statistics
    size_t provinces_processed = 0;
    size_t provinces_skipped = 0;
    size_t provinces_fallback = 0;

    // Process each province
    for (const auto& geom : province_geometries) {
        uint32_t province_id = geom.province_id;
        uint32_t vertex_start = geom.vertex_start;
        uint32_t vertex_count = geom.vertex_count;

        // Validate geometry bounds
        if (vertex_count == 0) {
            CORE_LOG_WARN("GPUMapRenderer", "Province " << province_id << " has zero vertices, skipping");
            provinces_skipped++;
            continue;
        }

        if (vertex_start + vertex_count > vbo_size) {
            CORE_LOG_ERROR("GPUMapRenderer", "Province " << province_id
                           << " geometry out of bounds: start=" << vertex_start
                           << ", count=" << vertex_count << ", vbo_size=" << vbo_size);
            provinces_skipped++;
            continue;
        }

        // STEP 1: Select which vertices to keep (every Nth vertex)
        std::vector<uint32_t> selected_vbo_positions;
        size_t estimated_size = (vertex_count - 1) / decimation_factor + 2;
        selected_vbo_positions.reserve(std::min(vertex_count, static_cast<uint32_t>(estimated_size)));

        for (uint32_t i = 0; i < vertex_count; i += decimation_factor) {
            selected_vbo_positions.push_back(vertex_start + i);
        }

        // Always include last vertex to close polygon
        uint32_t last_vertex_idx = vertex_start + vertex_count - 1;
        if (selected_vbo_positions.back() != last_vertex_idx) {
            selected_vbo_positions.push_back(last_vertex_idx);
        }

        // Fallback: if too few vertices after decimation, use all vertices
        bool use_full_detail = false;
        bool used_fallback = false;
        if (selected_vbo_positions.size() < 3) {
            use_full_detail = true;
            used_fallback = true;
            selected_vbo_positions.resize(vertex_count);
            for (uint32_t i = 0; i < vertex_count; ++i) {
                selected_vbo_positions[i] = vertex_start + i;
            }
        }

        // STEP 2: Extract positions for triangulation (with bounds checking)
        std::vector<std::array<float, 2>> polygon_positions;
        polygon_positions.reserve(selected_vbo_positions.size());
        for (uint32_t vbo_idx : selected_vbo_positions) {
            if (vbo_idx >= vbo_size) {
                CORE_LOG_ERROR("GPUMapRenderer", "Province " << province_id
                               << " VBO index out of bounds: " << vbo_idx << " >= " << vbo_size);
                polygon_positions.clear();
                break;
            }
            const auto& v = full_vertices[vbo_idx];
            polygon_positions.push_back({v.x, v.y});
        }

        if (polygon_positions.empty()) {
            provinces_skipped++;
            continue; // Skip if position extraction failed
        }

        // STEP 3: Triangulate the decimated polygon
        std::vector<std::vector<std::array<float, 2>>> polygon_rings = {polygon_positions};
        std::vector<uint32_t> local_indices = mapbox::earcut<uint32_t>(polygon_rings);

        // Validate triangulation
        if (local_indices.empty() || local_indices.size() < 3 || local_indices.size() % 3 != 0) {
            if (!use_full_detail) {
                // Triangulation failed - try fallback to full detail
                CORE_LOG_WARN("GPUMapRenderer", "LOD triangulation failed for province "
                              << province_id << ", falling back to full detail");

                use_full_detail = true;
                used_fallback = true;

                // Optimized: directly build polygon_positions from validated geometry
                polygon_positions.resize(vertex_count);
                for (uint32_t i = 0; i < vertex_count; ++i) {
                    uint32_t vbo_idx = vertex_start + i;
                    // Bounds already validated at line 605
                    const auto& v = full_vertices[vbo_idx];
                    polygon_positions[i] = {v.x, v.y};
                }

                // Update selected positions for later remapping
                selected_vbo_positions.resize(vertex_count);
                for (uint32_t i = 0; i < vertex_count; ++i) {
                    selected_vbo_positions[i] = vertex_start + i;
                }

                polygon_rings = {polygon_positions};
                local_indices = mapbox::earcut<uint32_t>(polygon_rings);

                // Re-validate after fallback
                if (local_indices.empty() || local_indices.size() < 3 || local_indices.size() % 3 != 0) {
                    CORE_LOG_ERROR("GPUMapRenderer", "Province " << province_id
                                   << " triangulation failed completely (fallback also failed), skipping");
                    provinces_skipped++;
                    continue;
                }
            } else {
                // Already using full detail and still failed
                CORE_LOG_ERROR("GPUMapRenderer", "Province " << province_id
                               << " triangulation failed completely, skipping");
                provinces_skipped++;
                continue;
            }
        }

        // STEP 4: Remap local indices to global VBO indices (with bounds check)
        size_t indices_before = lod_indices.size();
        bool valid = true;
        for (uint32_t local_idx : local_indices) {
            // Defensive: verify local_idx is within selected_vbo_positions
            if (local_idx >= selected_vbo_positions.size()) {
                CORE_LOG_ERROR("GPUMapRenderer", "Province " << province_id
                               << " local index out of range during remapping: " << local_idx);
                valid = false;
                break;
            }
            uint32_t global_vbo_idx = selected_vbo_positions[local_idx];
            lod_indices.push_back(global_vbo_idx);
        }

        if (!valid) {
            // Remapping failed, remove actually added indices
            lod_indices.resize(indices_before);
            provinces_skipped++;
            continue;
        }

        provinces_processed++;
        if (used_fallback) {
            provinces_fallback++;
        }
    }

    // Log statistics
    CORE_LOG_INFO("GPUMapRenderer", "LOD generation complete: "
                  << provinces_processed << " succeeded, "
                  << provinces_fallback << " used fallback, "
                  << provinces_skipped << " skipped");
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
        // Actual TerrainType values from MapData.h:
        // PLAINS=0, HILLS, MOUNTAINS, FOREST, DESERT, COAST, WETLAND, HIGHLANDS, UNKNOWN
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

    // Select appropriate LOD level based on zoom
    current_lod_level_ = SelectLODLevel(camera.zoom);

    // Use map shader
    glUseProgram(map_shader_program_);
    CHECK_GL_ERROR();

    // Update uniforms
    UpdateUniforms(camera);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    CHECK_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, province_color_texture_);
    CHECK_GL_ERROR();
    glUniform1i(u_province_data_, 0);
    CHECK_GL_ERROR();

    glActiveTexture(GL_TEXTURE1);
    CHECK_GL_ERROR();
    glBindTexture(GL_TEXTURE_2D, province_metadata_texture_);
    CHECK_GL_ERROR();
    glUniform1i(u_province_metadata_, 1);
    CHECK_GL_ERROR();

    // Bind VAO and appropriate LOD index buffer
    glBindVertexArray(vao_);
    CHECK_GL_ERROR();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lod_ibos_[current_lod_level_]);
    CHECK_GL_ERROR();
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(lod_index_counts_[current_lod_level_]), GL_UNSIGNED_INT, nullptr);
    CHECK_GL_ERROR();
    glBindVertexArray(0);
    CHECK_GL_ERROR();

    // Reset state
    glUseProgram(0);
    CHECK_GL_ERROR();

    auto end_time = std::chrono::high_resolution_clock::now();
    last_render_time_ms_ = std::chrono::duration<float, std::milli>(end_time - start_time).count();
}

void GPUMapRenderer::UpdateUniforms(const Camera2D& camera) {
    // Calculate view-projection matrix
    glm::mat4 projection = CalculateViewProjectionMatrix(camera);

    // Upload uniforms (pass matrix pointer directly - no copy needed)
    glUniformMatrix4fv(u_view_projection_, 1, GL_FALSE, glm::value_ptr(projection));
    CHECK_GL_ERROR();
    glUniform1i(u_render_mode_, static_cast<int>(render_mode_));
    CHECK_GL_ERROR();
    glUniform1ui(u_selected_province_, selected_province_id_);
    CHECK_GL_ERROR();
    glUniform1ui(u_hovered_province_, hovered_province_id_);
    CHECK_GL_ERROR();
    glUniform1f(u_selection_glow_time_, selection_glow_time_);
    CHECK_GL_ERROR();
    glUniform2f(u_viewport_size_, camera.viewport_width, camera.viewport_height);
    CHECK_GL_ERROR();
}

glm::mat4 GPUMapRenderer::CalculateViewProjectionMatrix(const Camera2D& camera) {
    // Calculate orthographic projection matrix using GLM
    // Maps world coordinates to normalized device coordinates [-1, 1]

    float half_width = (camera.viewport_width / camera.zoom) / 2.0f;
    float half_height = (camera.viewport_height / camera.zoom) / 2.0f;

    float left = camera.position.x - half_width;
    float right = camera.position.x + half_width;
    float bottom = camera.position.y - half_height;
    float top = camera.position.y + half_height;

    // Create and return orthographic projection matrix (GLM matrices are column-major, matching OpenGL)
    return glm::ortho(left, right, bottom, top);
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
    // Select LOD based on zoom level
    // Higher zoom = closer view = need higher detail
    // Lower zoom = farther view = can use lower detail
    //
    // LOD 0 (high detail): zoom >= lod_high_threshold_
    // LOD 1 (medium detail): lod_medium_threshold_ <= zoom < lod_high_threshold_
    // LOD 2 (low detail): zoom < lod_medium_threshold_

    if (zoom >= lod_high_threshold_) {
        return 0; // High detail
    } else if (zoom >= lod_medium_threshold_) {
        return 1; // Medium detail
    } else {
        return 2; // Low detail
    }
}

} // namespace game::map
