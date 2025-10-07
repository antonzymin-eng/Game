// ============================================================================
// MapRenderer.h - Map Graphics and Visual Rendering System
// Mechanica Imperii - Multi-scale Map Rendering with Performance Optimization
// ============================================================================

#pragma once

#include "MapSystem.h"
#include <GL/gl.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

namespace game {
    namespace map {

        // ============================================================================
        // Rendering Configuration and Constants
        // ============================================================================

        struct RenderSettings {
            // Performance settings
            bool enable_vsync = true;
            bool enable_msaa = true;
            int msaa_samples = 4;
            bool enable_frustum_culling = true;
            bool enable_occlusion_culling = false;

            // Visual quality
            float line_width = 1.0f;
            float border_width = 2.0f;
            bool show_province_names = true;
            bool show_settlement_names = true;
            bool show_terrain_colors = true;
            bool show_political_colors = true;

            // Level of detail thresholds
            float strategic_zoom_threshold = 0.5f;    // Show only major features
            float operational_zoom_threshold = 2.0f;  // Show settlements and roads
            float tactical_zoom_threshold = 10.0f;    // Show buildings and details

            // Color schemes
            bool use_satellite_mode = false;
            bool use_political_mode = true;
            bool use_terrain_mode = false;
            bool use_trade_mode = false;

            RenderSettings() = default;
        };

        // ============================================================================
        // Shader Management
        // ============================================================================

        class ShaderProgram {
        public:
            ShaderProgram();
            ~ShaderProgram();

            bool LoadFromFiles(const std::string& vertex_path, const std::string& fragment_path);
            bool LoadFromSource(const std::string& vertex_source, const std::string& fragment_source);

            void Use() const;
            void Unuse() const;

            // Uniform setters
            void SetUniform(const std::string& name, float value) const;
            void SetUniform(const std::string& name, int value) const;
            void SetUniform(const std::string& name, const float* matrix4x4) const;
            void SetUniform(const std::string& name, float x, float y) const;
            void SetUniform(const std::string& name, float x, float y, float z) const;
            void SetUniform(const std::string& name, float x, float y, float z, float w) const;

            GLuint GetProgramID() const { return m_program_id; }
            bool IsValid() const { return m_is_valid; }

        private:
            GLuint m_program_id = 0;
            bool m_is_valid = false;
            mutable std::unordered_map<std::string, GLint> m_uniform_cache;

            GLuint CompileShader(GLenum shader_type, const std::string& source) const;
            GLint GetUniformLocation(const std::string& name) const;
        };

        // ============================================================================
        // Render Batching for Performance
        // ============================================================================

        struct RenderBatch {
            std::vector<float> vertices;
            std::vector<uint32_t> indices;
            std::vector<uint32_t> colors;

            GLuint vertex_buffer = 0;
            GLuint index_buffer = 0;
            GLuint color_buffer = 0;
            GLuint vertex_array = 0;

            bool dirty = true;
            size_t max_vertices = 10000;
            size_t max_indices = 30000;

            void Clear();
            void Reserve(size_t vertex_count, size_t index_count);
            void AddQuad(float x1, float y1, float x2, float y2, uint32_t color);
            void AddTriangle(float x1, float y1, float x2, float y2, float x3, float y3, uint32_t color);
            void AddPolygon(const std::vector<Coordinate>& points, uint32_t color);
            void Upload();
            void Render() const;

            ~RenderBatch();
        };

        // ============================================================================
        // Map Layer System
        // ============================================================================

        enum class MapLayer {
            TERRAIN_BASE = 0,    // Base terrain colors
            WATER_BODIES,        // Rivers, lakes, coastlines
            POLITICAL_BORDERS,   // Province and realm boundaries
            SETTLEMENTS,         // Cities, towns, villages
            TRADE_ROUTES,        // Commercial and diplomatic routes
            MILITARY_UNITS,      // Armies and navies
            OVERLAY_INFO,        // Tooltips and information overlays
            COUNT
        };

        class MapLayerRenderer {
        public:
            MapLayerRenderer(MapLayer layer_type);
            virtual ~MapLayerRenderer() = default;

            virtual void Initialize() = 0;
            virtual void Update(float delta_time) = 0;
            virtual void Render(const Camera& camera, const RenderSettings& settings) = 0;
            virtual void Shutdown() = 0;

            // Layer management
            void SetVisible(bool visible) { m_visible = visible; }
            bool IsVisible() const { return m_visible; }
            void SetRenderOrder(int order) { m_render_order = order; }
            int GetRenderOrder() const { return m_render_order; }

            MapLayer GetLayerType() const { return m_layer_type; }

        protected:
            MapLayer m_layer_type;
            bool m_visible = true;
            int m_render_order = 0;
            bool m_needs_update = true;
        };

        // ============================================================================
        // Specific Layer Implementations
        // ============================================================================

        class TerrainRenderer : public MapLayerRenderer {
        public:
            TerrainRenderer(const MapSystem& map_system);

            void Initialize() override;
            void Update(float delta_time) override;
            void Render(const Camera& camera, const RenderSettings& settings) override;
            void Shutdown() override;

            void UpdateTerrainColors();
            void SetColorMode(bool use_political, bool use_terrain);

        private:
            const MapSystem& m_map_system;
            std::unique_ptr<RenderBatch> m_terrain_batch;
            std::unique_ptr<ShaderProgram> m_terrain_shader;

            void BuildTerrainMesh();
            void UpdateProvinceColors();
            uint32_t GetProvinceColor(types::EntityID province_id, const RenderSettings& settings) const;
        };

        class BorderRenderer : public MapLayerRenderer {
        public:
            BorderRenderer(const MapSystem& map_system);

            void Initialize() override;
            void Update(float delta_time) override;
            void Render(const Camera& camera, const RenderSettings& settings) override;
            void Shutdown() override;

        private:
            const MapSystem& m_map_system;
            std::unique_ptr<RenderBatch> m_border_batch;
            std::unique_ptr<ShaderProgram> m_line_shader;

            void BuildBorderLines();
            void UpdateBorderVisibility(const Camera& camera);
        };

        class SettlementRenderer : public MapLayerRenderer {
        public:
            SettlementRenderer(const MapSystem& map_system);

            void Initialize() override;
            void Update(float delta_time) override;
            void Render(const Camera& camera, const RenderSettings& settings) override;
            void Shutdown() override;

        private:
            const MapSystem& m_map_system;
            std::unique_ptr<RenderBatch> m_settlement_batch;
            std::unique_ptr<ShaderProgram> m_point_shader;

            void BuildSettlementPoints();
            void UpdateSettlementSizes(const Camera& camera);
            float GetSettlementDisplaySize(types::EntityID settlement_id, double zoom_level) const;
        };

        class TradeRouteRenderer : public MapLayerRenderer {
        public:
            TradeRouteRenderer(const MapSystem& map_system);

            void Initialize() override;
            void Update(float delta_time) override;
            void Render(const Camera& camera, const RenderSettings& settings) override;
            void Shutdown() override;

        private:
            const MapSystem& m_map_system;
            std::unique_ptr<RenderBatch> m_route_batch;
            std::unique_ptr<ShaderProgram> m_route_shader;

            void BuildTradeRouteLines();
            void UpdateRouteActivity();
        };

        // ============================================================================
        // Main Map Renderer
        // ============================================================================

        class MapRenderer {
        public:
            explicit MapRenderer(const MapSystem& map_system);
            ~MapRenderer();

            // Initialization and cleanup
            bool Initialize();
            void Shutdown();

            // Main rendering interface
            void Render(const Camera& camera);
            void SetViewport(int width, int height);

            // Settings and configuration
            RenderSettings& GetSettings() { return m_settings; }
            const RenderSettings& GetSettings() const { return m_settings; }
            void UpdateSettings(const RenderSettings& settings);

            // Layer management
            void SetLayerVisible(MapLayer layer, bool visible);
            bool IsLayerVisible(MapLayer layer) const;
            void SetLayerRenderOrder(MapLayer layer, int order);

            // Performance and debugging
            void EnableWireframeMode(bool enable);
            void EnableDebugOverlay(bool enable);
            void SetPerformanceMode(bool enable);

            // Statistics
            int GetDrawCalls() const { return m_draw_calls; }
            int GetVerticesRendered() const { return m_vertices_rendered; }
            int GetTrianglesRendered() const { return m_triangles_rendered; }
            float GetFrameTime() const { return m_frame_time; }

            // Utility
            void ClearBuffers();
            void SwapBuffers();

        private:
            const MapSystem& m_map_system;
            RenderSettings m_settings;

            // Layer renderers
            std::array<std::unique_ptr<MapLayerRenderer>, static_cast<size_t>(MapLayer::COUNT)> m_layers;
            std::vector<MapLayer> m_render_order;

            // OpenGL state
            int m_viewport_width = 1280;
            int m_viewport_height = 720;
            bool m_initialized = false;

            // Projection and view matrices
            float m_projection_matrix[16];
            float m_view_matrix[16];

            // Performance tracking
            mutable int m_draw_calls = 0;
            mutable int m_vertices_rendered = 0;
            mutable int m_triangles_rendered = 0;
            mutable float m_frame_time = 0.0f;

            // Debug features
            bool m_wireframe_mode = false;
            bool m_debug_overlay = false;
            bool m_performance_mode = false;

            // Internal methods
            void InitializeLayers();
            void UpdateProjectionMatrix(const Camera& camera);
            void UpdateViewMatrix(const Camera& camera);
            void SortLayersByRenderOrder();
            void ResetStatistics();
            void RenderDebugOverlay();

            // Matrix utilities
            void CalculateOrthographicMatrix(float left, float right, float bottom, float top,
                float near_plane, float far_plane, float* matrix);
            void CalculateViewMatrix(const Camera& camera, float* matrix);
            void MatrixMultiply(const float* a, const float* b, float* result);
            void MatrixIdentity(float* matrix);
        };

        // ============================================================================
        // Utility Classes
        // ============================================================================

        class TextRenderer {
        public:
            TextRenderer();
            ~TextRenderer();

            bool Initialize();
            void Shutdown();

            void RenderText(const std::string& text, float x, float y, float scale, uint32_t color);
            void SetFont(const std::string& font_path, int font_size);

            float GetTextWidth(const std::string& text, float scale) const;
            float GetTextHeight(float scale) const;

        private:
            struct FontData;
            std::unique_ptr<FontData> m_font_data;
            bool m_initialized = false;
        };

        class IconRenderer {
        public:
            IconRenderer();
            ~IconRenderer();

            bool Initialize();
            void Shutdown();

            void RenderIcon(const std::string& icon_name, float x, float y, float size, uint32_t color);
            bool LoadIconAtlas(const std::string& atlas_path);

        private:
            struct IconAtlas;
            std::unique_ptr<IconAtlas> m_atlas;
            bool m_initialized = false;
        };

        // ============================================================================
        // Map Event System
        // ============================================================================

        struct MapRenderEvent {
            enum Type {
                PROVINCE_SELECTED,
                PROVINCE_HOVERED,
                ZOOM_CHANGED,
                LAYER_TOGGLED,
                RENDER_MODE_CHANGED
            } type;

            types::EntityID entity_id = { 0 };
            float zoom_level = 1.0f;
            MapLayer layer = MapLayer::TERRAIN_BASE;

            MapRenderEvent(Type t) : type(t) {}
        };

        // ============================================================================
        // Performance Optimization Utilities
        // ============================================================================

        namespace RenderUtils {
            // Viewport culling
            bool IsPointVisible(const Coordinate& point, const Camera& camera);
            bool IsBoundsVisible(const BoundingBox& bounds, const Camera& camera);

            // Level of detail calculation
            RenderLevel CalculateLOD(const BoundingBox& bounds, const Camera& camera);
            float CalculateScreenSize(const BoundingBox& bounds, const Camera& camera);

            // Color utilities
            uint32_t PackColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
            void UnpackColor(uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a);
            uint32_t LerpColor(uint32_t color1, uint32_t color2, float t);

            // Mesh utilities
            void TriangulatePolygon(const std::vector<Coordinate>& polygon,
                std::vector<uint32_t>& indices);
            void SimplifyPolygon(const std::vector<Coordinate>& input,
                std::vector<Coordinate>& output, float tolerance);

            // Performance monitoring
            void BeginGPUTimer(const std::string& name);
            void EndGPUTimer(const std::string& name);
            float GetGPUTime(const std::string& name);
        }

    } // namespace map
} // namespace game