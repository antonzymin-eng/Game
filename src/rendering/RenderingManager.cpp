// ============================================================================
// RenderingManager.cpp - Unified Map Rendering Management Implementation
// Created: December 22, 2025
// ============================================================================

#include "map/render/RenderingManager.h"
#include "core/logging/Logger.h"

namespace game::map {

RenderingManager::RenderingManager(::core::ecs::EntityManager& entity_manager)
    : entity_manager_(entity_manager)
    , cpu_renderer_(nullptr)
    , gpu_renderer_(nullptr)
    , active_renderer_type_(RendererType::CPU_IMGUI)  // Default to CPU
    , cpu_renderer_initialized_(false)
    , gpu_renderer_initialized_(false)
{
}

RenderingManager::~RenderingManager() {
    // Cleanup handled by unique_ptr
}

bool RenderingManager::Initialize() {
    CORE_LOG_INFO("RenderingManager", "Initializing rendering system...");

    // Always create CPU renderer (fallback)
    cpu_renderer_ = std::make_unique<MapRenderer>(entity_manager_);
    if (!cpu_renderer_->Initialize()) {
        CORE_LOG_ERROR("RenderingManager", "Failed to initialize CPU renderer");
        return false;
    }
    cpu_renderer_initialized_ = true;
    CORE_LOG_INFO("RenderingManager", "CPU renderer (ImGui) initialized successfully");

    // Try to create GPU renderer (optional)
    try {
        gpu_renderer_ = std::make_unique<GPUMapRenderer>(entity_manager_);
        if (gpu_renderer_->Initialize()) {
            gpu_renderer_initialized_ = true;
            CORE_LOG_INFO("RenderingManager", "GPU renderer (OpenGL) initialized successfully");

            // Default to GPU if available
            active_renderer_type_ = RendererType::GPU_OPENGL;
            CORE_LOG_INFO("RenderingManager", "Using GPU renderer by default");
        } else {
            CORE_LOG_WARN("RenderingManager", "GPU renderer initialization failed - using CPU fallback");
            gpu_renderer_.reset();
        }
    } catch (const std::exception& e) {
        CORE_LOG_ERROR("RenderingManager", "Exception during GPU renderer initialization: " << e.what());
        gpu_renderer_.reset();
    }

    return true;
}

bool RenderingManager::UploadProvinceData(const std::vector<const ProvinceRenderComponent*>& provinces) {
    // Cache province data for renderer switching
    cached_provinces_ = provinces;

    bool success = true;

    // Upload to CPU renderer if initialized
    if (cpu_renderer_initialized_ && cpu_renderer_) {
        // CPU renderer doesn't need explicit upload, it reads from ECS every frame
        CORE_LOG_INFO("RenderingManager", "CPU renderer will use province data from ECS");
    }

    // Upload to GPU renderer if initialized
    if (gpu_renderer_initialized_ && gpu_renderer_) {
        if (!gpu_renderer_->UploadProvinceData(provinces)) {
            CORE_LOG_ERROR("RenderingManager", "Failed to upload province data to GPU renderer");
            success = false;
        } else {
            CORE_LOG_INFO("RenderingManager", "Province data uploaded to GPU renderer successfully");
        }
    }

    return success;
}

void RenderingManager::Render() {
    if (active_renderer_type_ == RendererType::GPU_OPENGL && gpu_renderer_initialized_ && gpu_renderer_) {
        // Use GPU renderer
        Camera2D& camera = gpu_renderer_->GetCamera();
        gpu_renderer_->Render(camera);
    } else if (cpu_renderer_initialized_ && cpu_renderer_) {
        // Use CPU renderer (fallback)
        cpu_renderer_->Render();
    } else {
        CORE_LOG_WARN("RenderingManager", "No renderer available for rendering");
    }
}

void RenderingManager::HandleInput() {
    if (active_renderer_type_ == RendererType::GPU_OPENGL && gpu_renderer_initialized_ && gpu_renderer_) {
        // GPU renderer handles input via its own camera
        // Input handling would need to be implemented in GPUMapRenderer
        // For now, delegate to CPU renderer's input handling
        if (cpu_renderer_) {
            cpu_renderer_->HandleInput();
        }
    } else if (cpu_renderer_initialized_ && cpu_renderer_) {
        cpu_renderer_->HandleInput();
    }
}

void RenderingManager::SetActiveRenderer(RendererType type) {
    if (type == RendererType::GPU_OPENGL && !gpu_renderer_initialized_) {
        CORE_LOG_WARN("RenderingManager", "GPU renderer not available, staying on CPU renderer");
        return;
    }

    if (active_renderer_type_ != type) {
        CORE_LOG_INFO("RenderingManager", "Switching renderer from "
            << (active_renderer_type_ == RendererType::CPU_IMGUI ? "CPU" : "GPU")
            << " to "
            << (type == RendererType::CPU_IMGUI ? "CPU" : "GPU"));

        // Sync camera state when switching
        if (type == RendererType::GPU_OPENGL && gpu_renderer_ && cpu_renderer_) {
            // Copy camera from CPU to GPU
            gpu_renderer_->SetCamera(cpu_renderer_->GetCamera());
        } else if (type == RendererType::CPU_IMGUI && cpu_renderer_ && gpu_renderer_) {
            // Copy camera from GPU to CPU
            // Note: MapRenderer needs to expose SetCamera for this to work
            // For now, cameras will be independent
        }

        active_renderer_type_ = type;
    }
}

Camera2D& RenderingManager::GetCamera() {
    if (active_renderer_type_ == RendererType::GPU_OPENGL && gpu_renderer_) {
        return gpu_renderer_->GetCamera();
    } else if (cpu_renderer_) {
        return cpu_renderer_->GetCamera();
    }

    // Fallback: return CPU renderer camera
    static Camera2D fallback_camera;
    return fallback_camera;
}

const Camera2D& RenderingManager::GetCamera() const {
    if (active_renderer_type_ == RendererType::GPU_OPENGL && gpu_renderer_) {
        return gpu_renderer_->GetCamera();
    } else if (cpu_renderer_) {
        return cpu_renderer_->GetCamera();
    }

    static Camera2D fallback_camera;
    return fallback_camera;
}

void RenderingManager::SetSelectedProvince(uint32_t province_id) {
    if (gpu_renderer_) {
        gpu_renderer_->SetSelectedProvince(province_id);
    }
    if (cpu_renderer_) {
        cpu_renderer_->SetSelectedProvince(province_id);
    }
}

void RenderingManager::ClearSelection() {
    if (gpu_renderer_) {
        gpu_renderer_->SetSelectedProvince(0);
    }
    if (cpu_renderer_) {
        cpu_renderer_->ClearSelection();
    }
}

float RenderingManager::GetLastRenderTime() const {
    if (active_renderer_type_ == RendererType::GPU_OPENGL && gpu_renderer_) {
        return gpu_renderer_->GetLastRenderTime();
    } else if (cpu_renderer_) {
        // CPU renderer doesn't track render time yet
        return 0.0f;
    }
    return 0.0f;
}

size_t RenderingManager::GetVertexCount() const {
    if (gpu_renderer_) {
        return gpu_renderer_->GetVertexCount();
    }
    return 0;
}

size_t RenderingManager::GetTriangleCount() const {
    if (gpu_renderer_) {
        if (active_renderer_type_ == RendererType::GPU_OPENGL) {
            return gpu_renderer_->GetCurrentTriangleCount();
        }
        return gpu_renderer_->GetMaxTriangleCount();
    }
    return 0;
}

int RenderingManager::GetCurrentLODLevel() const {
    if (gpu_renderer_) {
        return gpu_renderer_->GetCurrentLODLevel();
    }
    return 0;
}

void RenderingManager::SetShowBorders(bool show) {
    if (gpu_renderer_) {
        gpu_renderer_->SetShowBorders(show);
    }
    if (cpu_renderer_) {
        // CPU renderer would need this method implemented
    }
}

void RenderingManager::SetShowNames(bool show) {
    if (gpu_renderer_) {
        gpu_renderer_->SetShowNames(show);
    }
    if (cpu_renderer_) {
        // CPU renderer would need this method implemented
    }
}

} // namespace game::map
