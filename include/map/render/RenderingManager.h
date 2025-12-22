// ============================================================================
// RenderingManager.h - Unified Map Rendering Management
// Created: December 22, 2025
// Description: Manages both CPU (ImGui) and GPU (OpenGL) map renderers
//              with seamless switching between them
// ============================================================================

#pragma once

#include <memory>
#include "map/render/MapRenderer.h"
#include "map/render/GPUMapRenderer.h"
#include "map/render/Camera2D.h"
#include "core/ECS/EntityManager.h"

namespace game::map {

// ============================================================================
// IMapRenderer - Common interface for all renderers
// ============================================================================

class IMapRenderer {
public:
    virtual ~IMapRenderer() = default;

    // Core rendering functions
    virtual bool Initialize() = 0;
    virtual void Render() = 0;
    virtual void HandleInput() = 0;

    // Camera access
    virtual Camera2D& GetCamera() = 0;
    virtual const Camera2D& GetCamera() const = 0;

    // Province selection
    virtual void SetSelectedProvince(uint32_t province_id) = 0;
    virtual void ClearSelection() = 0;

    // Statistics
    virtual float GetLastRenderTime() const = 0;
};

// ============================================================================
// RenderingManager - Manages active renderer and switching
// ============================================================================

class RenderingManager {
public:
    enum class RendererType {
        CPU_IMGUI,    // ImGui immediate-mode rendering (fallback)
        GPU_OPENGL    // OpenGL retained-mode rendering (high performance)
    };

    explicit RenderingManager(::core::ecs::EntityManager& entity_manager);
    ~RenderingManager();

    // Initialization
    bool Initialize();

    // Upload province data to active renderer
    bool UploadProvinceData(const std::vector<const ProvinceRenderComponent*>& provinces);

    // Rendering (call every frame)
    void Render();
    void HandleInput();

    // Renderer selection
    void SetActiveRenderer(RendererType type);
    RendererType GetActiveRenderer() const { return active_renderer_type_; }
    bool IsGPURendererAvailable() const { return gpu_renderer_ != nullptr && gpu_renderer_initialized_; }

    // Camera access (unified across both renderers)
    Camera2D& GetCamera();
    const Camera2D& GetCamera() const;

    // Province selection
    void SetSelectedProvince(uint32_t province_id);
    void ClearSelection();

    // Statistics
    float GetLastRenderTime() const;
    size_t GetVertexCount() const;
    size_t GetTriangleCount() const;
    int GetCurrentLODLevel() const;

    // Rendering options (forwarded to active renderer)
    void SetShowBorders(bool show);
    void SetShowNames(bool show);

    // Map renderer access (for specific features)
    MapRenderer* GetCPURenderer() { return cpu_renderer_.get(); }
    GPUMapRenderer* GetGPURenderer() { return gpu_renderer_.get(); }

private:
    ::core::ecs::EntityManager& entity_manager_;

    // Renderers
    std::unique_ptr<MapRenderer> cpu_renderer_;
    std::unique_ptr<GPUMapRenderer> gpu_renderer_;

    // State
    RendererType active_renderer_type_;
    bool cpu_renderer_initialized_;
    bool gpu_renderer_initialized_;

    // Cached province data (for re-upload when switching renderers)
    std::vector<const ProvinceRenderComponent*> cached_provinces_;
};

} // namespace game::map
