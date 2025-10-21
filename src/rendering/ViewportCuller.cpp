 // ============================================================================
// ViewportCuller.cpp - Viewport Culling Implementation
// Created: October 21, 2025
// ============================================================================

#include "map/render/ViewportCuller.h"
#include <iostream>

namespace game::map {

    ViewportCuller::ViewportCuller() {
        // Initialize with default viewport
        viewport_bounds_ = Rect(0, 0, 1920, 1080);
        expanded_viewport_bounds_ = viewport_bounds_;
    }

    void ViewportCuller::UpdateViewport(const Camera2D& camera) {
        viewport_bounds_ = camera.GetViewportBounds();
        expanded_viewport_bounds_ = ExpandViewport(viewport_bounds_, 1.2f);
    }

    bool ViewportCuller::IsProvinceVisible(const ProvinceRenderComponent& province) const {
        // Simple bounding box test
        return viewport_bounds_.Intersects(province.bounding_box);
    }

    bool ViewportCuller::IsFeatureVisible(const FeatureRenderData& feature, int current_lod) const {
        // Check LOD range
        if (current_lod < feature.lod_min || current_lod > feature.lod_max) {
            return false;
        }
        
        // Check if feature is within viewport
        return IsPointVisible(feature.position.x, feature.position.y);
    }

    bool ViewportCuller::IsPointVisible(float x, float y) const {
        return viewport_bounds_.Contains(x, y);
    }

    std::vector<EntityID> ViewportCuller::GetVisibleProvinces(
        ::core::ecs::EntityManager& entity_manager
    ) const {
        std::vector<EntityID> visible;
        
        auto all_provinces = entity_manager.GetEntitiesWithComponent<ProvinceRenderComponent>();
        
        for (const auto& entity_id : all_provinces) {
            auto render = entity_manager.GetComponent<ProvinceRenderComponent>(entity_id);
            if (render && IsProvinceVisible(*render)) {
                visible.push_back(entity_id);
            }
        }
        
        return visible;
    }

    std::vector<EntityID> ViewportCuller::GetVisibleProvincesExpanded(
        ::core::ecs::EntityManager& entity_manager,
        float expansion_factor
    ) const {
        std::vector<EntityID> visible;
        
        Rect expanded_viewport = ExpandViewport(viewport_bounds_, expansion_factor);
        
        auto all_provinces = entity_manager.GetEntitiesWithComponent<ProvinceRenderComponent>();
        
        for (const auto& entity_id : all_provinces) {
            auto render = entity_manager.GetComponent<ProvinceRenderComponent>(entity_id);
            if (render && expanded_viewport.Intersects(render->bounding_box)) {
                visible.push_back(entity_id);
            }
        }
        
        return visible;
    }

    void ViewportCuller::UpdateProvinceVisibility(::core::ecs::EntityManager& entity_manager) {
        auto all_provinces = entity_manager.GetEntitiesWithComponent<ProvinceRenderComponent>();
        
        visible_province_count_ = 0;
        total_province_count_ = all_provinces.size();
        
        for (const auto& entity_id : all_provinces) {
            auto render = entity_manager.GetComponent<ProvinceRenderComponent>(entity_id);
            if (render) {
                render->is_visible = IsProvinceVisible(*render);
                if (render->is_visible) {
                    visible_province_count_++;
                }
            }
        }
        
        // Debug output (can be removed later)
        if (total_province_count_ > 0) {
            std::cout << "Viewport Culling: " << visible_province_count_ 
                      << " / " << total_province_count_ << " provinces visible ("
                      << (int)(GetCullingEfficiency() * 100.0f) << "% culled)" << std::endl;
        }
    }

} // namespace game::map
