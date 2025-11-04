 // ============================================================================
// MapRenderer.cpp - Map Rendering Implementation
// Created: October 21, 2025
// ============================================================================

#include "utils/PlatformCompat.h"
#include "map/render/MapRenderer.h"
#include "map/render/TacticalTerrainRenderer.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath>

namespace game::map {

    // ========================================================================
    // Constructor / Destructor
    // ========================================================================
    MapRenderer::MapRenderer(
        ::core::ecs::EntityManager& entity_manager
    )
        : entity_manager_(entity_manager)
        , current_lod_(LODLevel::PROVINCIAL)
        , selected_province_()
        , hovered_province_()
    {
        // Initialize camera to center of test map
        camera_.position = Vector2(260.0f, 130.0f);  // Center of British Isles test data
        camera_.zoom = 1.5f;
        camera_.viewport_width = 1920.0f;
        camera_.viewport_height = 1080.0f;
    }

    // ========================================================================
    // Initialization
    // ========================================================================
    bool MapRenderer::Initialize() {
        std::cout << "MapRenderer: Initializing..." << std::endl;

        // Initialize LOD 4 Tactical Terrain Renderer
        tactical_terrain_renderer_ = std::make_unique<TacticalTerrainRenderer>(entity_manager_);
        if (!tactical_terrain_renderer_->Initialize()) {
            std::cerr << "MapRenderer: Failed to initialize TacticalTerrainRenderer" << std::endl;
            return false;
        }

        // Load fonts if available (optional)
        ImGuiIO& io = ImGui::GetIO();
        if (io.Fonts->Fonts.size() > 0) {
            font_medium_ = io.Fonts->Fonts[0];
            font_large_ = font_medium_;
            font_small_ = font_medium_;
        }

        std::cout << "MapRenderer: Initialized successfully" << std::endl;
        return true;
    }

    // ========================================================================
    // Main Render Function
    // ========================================================================
    void MapRenderer::Render() {
        auto start_time = std::chrono::high_resolution_clock::now();

        // Update LOD based on zoom
        UpdateLOD();

        // Update viewport culling
        culler_.UpdateViewport(camera_);

        // Reset statistics
        rendered_province_count_ = 0;
        rendered_feature_count_ = 0;

        // At LOD 4 (TACTICAL), render heightmap terrain instead of province polygons
        if (current_lod_ == LODLevel::TACTICAL) {
            ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

            // Render terrain heightmap for all visible provinces
            auto visible_provinces = culler_.GetVisibleProvinces(entity_manager_);
            for (const auto& entity_id : visible_provinces) {
                auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(entity_id);
                if (!render) continue;

                tactical_terrain_renderer_->RenderProvinceTerrain(*render, camera_, draw_list);
                rendered_province_count_++;
            }

            // Still render province borders for context
            if (render_borders_) {
                for (const auto& entity_id : visible_provinces) {
                    auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(entity_id);
                    if (!render) continue;
                    RenderProvinceBorder(*render, draw_list);
                }
            }

            // Render features if enabled
            if (render_features_) {
                for (const auto& entity_id : visible_provinces) {
                    auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(entity_id);
                    if (!render) continue;
                    RenderFeatures(*render, draw_list);
                }
            }

            // Render military and naval units at LOD 4
            if (tactical_terrain_renderer_ && tactical_terrain_renderer_->GetUnitRenderer()) {
                tactical_terrain_renderer_->GetUnitRenderer()->RenderAllUnits(camera_, draw_list);
            }

            // Update and render weather and environmental effects at LOD 4
            if (tactical_terrain_renderer_ && tactical_terrain_renderer_->GetEnvironmentalEffectRenderer()) {
                // Update weather state (particle positions, lightning, etc.)
                tactical_terrain_renderer_->GetEnvironmentalEffectRenderer()->Update(1.0f / 60.0f);  // Assume 60 FPS

                // Render all weather effects
                tactical_terrain_renderer_->GetEnvironmentalEffectRenderer()->RenderAllEffects(camera_, draw_list);
            }
        }
        else {
            // For LOD 0-3, use standard province rendering
            RenderProvinces();
        }

        // Render selection highlight
        if (selected_province_.id != 0) {
            RenderSelection();
        }

        // Render debug info
        if (show_debug_info_) {
            RenderDebugInfo();
        }

        // Calculate render time
        auto end_time = std::chrono::high_resolution_clock::now();
        last_render_time_ms_ = std::chrono::duration<float, std::milli>(end_time - start_time).count();
    }

    // ========================================================================
    // LOD Management
    // ========================================================================
    void MapRenderer::UpdateLOD() {
        float zoom = camera_.zoom;
        
        if (zoom < 0.3f) {
            current_lod_ = LODLevel::STRATEGIC;
        } else if (zoom < 0.6f) {
            current_lod_ = LODLevel::REGIONAL;
        } else if (zoom < 1.2f) {
            current_lod_ = LODLevel::PROVINCIAL;
        } else if (zoom < 2.5f) {
            current_lod_ = LODLevel::LOCAL;
        } else {
            current_lod_ = LODLevel::TACTICAL;
        }
    }

    // ========================================================================
    // Province Rendering
    // ========================================================================
    void MapRenderer::RenderProvinces() {
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        
        // Get visible provinces
        auto visible_provinces = culler_.GetVisibleProvinces(entity_manager_);
        
        for (const auto& entity_id : visible_provinces) {
            auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(entity_id);
            if (!render) continue;
            
            // Render province
            RenderProvince(*render, draw_list);
            rendered_province_count_++;
            
            // Render features if enabled
            if (render_features_ && current_lod_ >= LODLevel::PROVINCIAL) {
                RenderFeatures(*render, draw_list);
            }
        }
    }

    void MapRenderer::RenderProvince(const ProvinceRenderComponent& province, ImDrawList* draw_list) {
        // Get appropriate boundary for current LOD
        const auto& boundary = GetBoundaryForLOD(province);
        if (boundary.size() < 3) return;
        
        // Convert world coordinates to screen coordinates
        std::vector<ImVec2> screen_points;
        screen_points.reserve(boundary.size());
        
        for (const auto& world_pt : boundary) {
            auto screen = camera_.WorldToScreen(world_pt.x, world_pt.y);
            screen_points.push_back(ImVec2(screen.x, screen.y));
        }
        
        // Render filled province polygon
        uint32_t fill_color = ColorToImU32(province.fill_color);
        if (screen_points.size() >= 3) {
            draw_list->AddConvexPolyFilled(
                screen_points.data(),
                static_cast<int>(screen_points.size()),
                fill_color
            );
        }
        
        // Render border if enabled
        if (render_borders_) {
            RenderProvinceBorder(province, draw_list);
        }
        
        // Render province name if enabled and LOD appropriate
        if (render_names_ && current_lod_ >= LODLevel::REGIONAL) {
            RenderProvinceName(province, draw_list);
        }
    }

    void MapRenderer::RenderProvinceBorder(const ProvinceRenderComponent& province, ImDrawList* draw_list) {
        const auto& boundary = GetBoundaryForLOD(province);
        if (boundary.size() < 2) return;
        
        std::vector<ImVec2> screen_points;
        screen_points.reserve(boundary.size() + 1);
        
        for (const auto& world_pt : boundary) {
            auto screen = camera_.WorldToScreen(world_pt.x, world_pt.y);
            screen_points.push_back(ImVec2(screen.x, screen.y));
        }
        
        // Close the polygon
        screen_points.push_back(screen_points[0]);
        
        // Determine border color and thickness
        uint32_t border_color = ColorToImU32(province.border_color);
        float thickness = 1.5f;
        
        // Thicker border for selected province
        if (province.is_selected) {
            border_color = IM_COL32(255, 255, 255, 255);  // White
            thickness = 3.0f;
        }
        // Highlighted border for hovered province
        else if (province.is_hovered) {
            border_color = IM_COL32(200, 200, 200, 255);  // Light grey
            thickness = 2.5f;
        }
        
        draw_list->AddPolyline(
            screen_points.data(),
            static_cast<int>(screen_points.size()),
            border_color,
            ImDrawFlags_Closed,
            thickness
        );
    }

    void MapRenderer::RenderProvinceName(const ProvinceRenderComponent& province, ImDrawList* draw_list) {
        auto screen_center = camera_.WorldToScreen(province.center_position.x, province.center_position.y);
        
        // Choose font size based on LOD
        float font_size = 16.0f;
        if (current_lod_ == LODLevel::STRATEGIC) {
            font_size = 20.0f;
        } else if (current_lod_ == LODLevel::REGIONAL) {
            font_size = 18.0f;
        }
        
        // Calculate text size for centering
        ImVec2 text_size = ImGui::CalcTextSize(province.name.c_str());
        ImVec2 text_pos(screen_center.x - text_size.x / 2.0f, screen_center.y - text_size.y / 2.0f);
        
        // Draw text with black outline for readability
        uint32_t outline_color = IM_COL32(0, 0, 0, 255);
        uint32_t text_color = IM_COL32(255, 255, 255, 255);
        
        // Draw outline (4 directions)
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) continue;
                draw_list->AddText(
                    ImVec2(text_pos.x + dx, text_pos.y + dy),
                    outline_color,
                    province.name.c_str()
                );
            }
        }
        
        // Draw main text
        draw_list->AddText(text_pos, text_color, province.name.c_str());
    }

    // ========================================================================
    // Feature Rendering
    // ========================================================================
    void MapRenderer::RenderFeatures(const ProvinceRenderComponent& province, ImDrawList* draw_list) {
        int lod_level = static_cast<int>(current_lod_);
        
        for (const auto& feature : province.features) {
            // Check if feature should be visible at current LOD
            if (lod_level >= feature.lod_min && lod_level <= feature.lod_max) {
                RenderFeature(feature, draw_list);
                rendered_feature_count_++;
            }
        }
    }

    void MapRenderer::RenderFeature(const FeatureRenderData& feature, ImDrawList* draw_list) {
        auto screen_pos = camera_.WorldToScreen(feature.position.x, feature.position.y);
        
        // Render based on feature type
        switch (feature.type) {
            case FeatureType::CITY:
            case FeatureType::TOWN:
            case FeatureType::VILLAGE: {
                // Render city as a circle
                float radius = 4.0f * feature.size;
                if (current_lod_ >= LODLevel::LOCAL) {
                    radius = 6.0f * feature.size;
                }
                
                uint32_t city_color = IM_COL32(50, 50, 50, 255);  // Dark grey
                uint32_t outline_color = IM_COL32(255, 255, 255, 255);  // White
                
                draw_list->AddCircleFilled(ImVec2(screen_pos.x, screen_pos.y), radius + 1.0f, outline_color);
                draw_list->AddCircleFilled(ImVec2(screen_pos.x, screen_pos.y), radius, city_color);
                
                // Render city name at higher LOD
                if (current_lod_ >= LODLevel::LOCAL && !feature.name.empty()) {
                    ImVec2 text_size = ImGui::CalcTextSize(feature.name.c_str());
                    ImVec2 text_pos(screen_pos.x - text_size.x / 2.0f, screen_pos.y + radius + 2.0f);
                    draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), feature.name.c_str());
                }
                break;
            }
            
            case FeatureType::MOUNTAIN: {
                // Render mountain as a triangle
                float size = 8.0f;
                ImVec2 p1(screen_pos.x, screen_pos.y - size);
                ImVec2 p2(screen_pos.x - size * 0.866f, screen_pos.y + size * 0.5f);
                ImVec2 p3(screen_pos.x + size * 0.866f, screen_pos.y + size * 0.5f);
                
                uint32_t mountain_color = IM_COL32(139, 90, 43, 255);  // Brown
                draw_list->AddTriangleFilled(p1, p2, p3, mountain_color);
                draw_list->AddTriangle(p1, p2, p3, IM_COL32(0, 0, 0, 255), 1.5f);
                break;
            }
            
            case FeatureType::FOREST: {
                // Render forest as a green circle
                float radius = 5.0f;
                uint32_t forest_color = IM_COL32(34, 139, 34, 200);  // Forest green
                draw_list->AddCircleFilled(ImVec2(screen_pos.x, screen_pos.y), radius, forest_color);
                break;
            }
            
            case FeatureType::FORTRESS: {
                // Render fortress as a square
                float size = 6.0f;
                ImVec2 p_min(screen_pos.x - size, screen_pos.y - size);
                ImVec2 p_max(screen_pos.x + size, screen_pos.y + size);
                
                uint32_t fortress_color = IM_COL32(128, 128, 128, 255);  // Grey
                draw_list->AddRectFilled(p_min, p_max, fortress_color);
                draw_list->AddRect(p_min, p_max, IM_COL32(0, 0, 0, 255), 0.0f, 0, 1.5f);
                break;
            }
            
            default:
                // Generic feature - small dot
                draw_list->AddCircleFilled(ImVec2(screen_pos.x, screen_pos.y), 3.0f, IM_COL32(100, 100, 100, 255));
                break;
        }
    }

    // ========================================================================
    // Selection Rendering
    // ========================================================================
    void MapRenderer::RenderSelection() {
        auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(selected_province_);
        if (!render) return;
        
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        
        // Highlight with animated glow effect
        static float anim_time = 0.0f;
        anim_time += 0.05f;
        
        float pulse = 0.5f + 0.5f * std::sin(anim_time);
        uint8_t alpha = static_cast<uint8_t>(100 + 100 * pulse);
        
        const auto& boundary = GetBoundaryForLOD(*render);
        if (boundary.size() < 3) return;
        
        std::vector<ImVec2> screen_points;
        for (const auto& world_pt : boundary) {
            auto screen = camera_.WorldToScreen(world_pt.x, world_pt.y);
            screen_points.push_back(ImVec2(screen.x, screen.y));
        }
        
        // Draw highlight overlay
        uint32_t highlight_color = IM_COL32(255, 255, 255, alpha);
        draw_list->AddConvexPolyFilled(
            screen_points.data(),
            static_cast<int>(screen_points.size()),
            highlight_color
        );
    }

    // ========================================================================
    // Debug Info Rendering
    // ========================================================================
    void MapRenderer::RenderDebugInfo() {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Once);
        
        if (ImGui::Begin("Map Renderer Debug", &show_debug_info_)) {
            ImGui::Text("Camera Position: (%.1f, %.1f)", camera_.position.x, camera_.position.y);
            ImGui::Text("Camera Zoom: %.2f", camera_.zoom);
            
            const char* lod_names[] = {"Strategic", "Regional", "Provincial", "Local", "Tactical"};
            ImGui::Text("Current LOD: %s", lod_names[static_cast<int>(current_lod_)]);
            
            ImGui::Separator();
            ImGui::Text("Rendered Provinces: %d / %d", 
                culler_.GetVisibleProvinceCount(), 
                culler_.GetTotalProvinceCount());
            ImGui::Text("Rendered Features: %d", rendered_feature_count_);
            ImGui::Text("Culling Efficiency: %.1f%%", culler_.GetCullingEfficiency() * 100.0f);
            
            ImGui::Separator();
            ImGui::Text("Render Time: %.2f ms", last_render_time_ms_);
            ImGui::Text("FPS: %.1f", 1000.0f / last_render_time_ms_);
            
            ImGui::Separator();
            ImGui::Checkbox("Render Borders", &render_borders_);
            ImGui::Checkbox("Render Names", &render_names_);
            ImGui::Checkbox("Render Features", &render_features_);
        }
        ImGui::End();
    }

    // ========================================================================
    // Camera Controls
    // ========================================================================
    void MapRenderer::HandleInput() {
        ImGuiIO& io = ImGui::GetIO();
        
        // WASD camera panning
        float pan_speed = 300.0f / camera_.zoom;
        if (ImGuiCompat::IsKeyDown(ImGuiKey_W)) camera_.Pan(0, -pan_speed * io.DeltaTime);
        if (ImGuiCompat::IsKeyDown(ImGuiKey_S)) camera_.Pan(0, pan_speed * io.DeltaTime);
        if (ImGuiCompat::IsKeyDown(ImGuiKey_A)) camera_.Pan(-pan_speed * io.DeltaTime, 0);
        if (ImGuiCompat::IsKeyDown(ImGuiKey_D)) camera_.Pan(pan_speed * io.DeltaTime, 0);
        
        // Mouse wheel zooming
        if (io.MouseWheel != 0.0f && !io.WantCaptureMouse) {
            float zoom_factor = 1.0f + io.MouseWheel * 0.1f;
            auto mouse_world = camera_.ScreenToWorld(io.MousePos.x, io.MousePos.y);
            ZoomCameraAt(mouse_world.x, mouse_world.y, zoom_factor);
        }
        
        // Middle mouse button dragging
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle) && !io.WantCaptureMouse) {
            if (!mouse_dragging_) {
                mouse_dragging_ = true;
                last_mouse_pos_ = Vector2(io.MousePos.x, io.MousePos.y);
            } else {
                float dx = io.MousePos.x - last_mouse_pos_.x;
                float dy = io.MousePos.y - last_mouse_pos_.y;
                PanCamera(-dx, -dy);
                last_mouse_pos_ = Vector2(io.MousePos.x, io.MousePos.y);
            }
        } else {
            mouse_dragging_ = false;
        }
        
        // Left click for selection
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse) {
            HandleMouseClick(io.MousePos.x, io.MousePos.y);
        }
        
        // Mouse hover for highlighting
        if (!io.WantCaptureMouse) {
            HandleMouseMove(io.MousePos.x, io.MousePos.y);
        }
    }

    void MapRenderer::PanCamera(float dx, float dy) {
        camera_.Pan(dx, dy);
    }

    void MapRenderer::ZoomCamera(float delta) {
        camera_.zoom *= delta;
        camera_.zoom = std::max(0.1f, std::min(10.0f, camera_.zoom));
    }

    void MapRenderer::ZoomCameraAt(float world_x, float world_y, float delta) {
        camera_.ZoomAt(world_x, world_y, delta);
    }

    void MapRenderer::SetCameraPosition(float x, float y) {
        camera_.position.x = x;
        camera_.position.y = y;
    }

    void MapRenderer::SetCameraZoom(float zoom) {
        camera_.zoom = std::max(0.1f, std::min(10.0f, zoom));
    }

    void MapRenderer::SetViewportSize(float width, float height) {
        camera_.viewport_width = width;
        camera_.viewport_height = height;
    }

    // ========================================================================
    // Province Selection
    // ========================================================================
    void MapRenderer::HandleMouseClick(float screen_x, float screen_y) {
        auto world_pos = camera_.ScreenToWorld(screen_x, screen_y);
        EntityID province_id = GetProvinceAtPoint(world_pos.x, world_pos.y);
        
        if (province_id.id != 0) {
            SelectProvince(province_id);
            std::cout << "Selected province: " << province_id.id << std::endl;
        } else {
            ClearSelection();
        }
    }

    void MapRenderer::HandleMouseMove(float screen_x, float screen_y) {
        auto world_pos = camera_.ScreenToWorld(screen_x, screen_y);
        EntityID province_id = GetProvinceAtPoint(world_pos.x, world_pos.y);
        
        // Clear previous hover
        if (hovered_province_.id != 0 && !(hovered_province_ == province_id)) {
            auto prev_render = entity_manager_.GetComponent<ProvinceRenderComponent>(hovered_province_);
            if (prev_render) {
                prev_render->is_hovered = false;
            }
        }
        
        // Set new hover
        hovered_province_ = province_id;
        if (hovered_province_.id != 0) {
            auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(hovered_province_);
            if (render) {
                render->is_hovered = true;
            }
        }
    }

    EntityID MapRenderer::GetProvinceAtPoint(float world_x, float world_y) {
        auto visible_provinces = culler_.GetVisibleProvincesExpanded(entity_manager_, 1.0f);
        
        // Check provinces from front to back (smaller provinces first for overlap handling)
        for (const auto& entity_id : visible_provinces) {
            auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(entity_id);
            if (!render) continue;
            
            // Quick bounding box test
            if (!render->ContainsPoint(world_x, world_y)) continue;
            
            // Precise point-in-polygon test
            const auto& boundary = GetBoundaryForLOD(*render);
            if (IsPointInPolygon(boundary, world_x, world_y)) {
                return entity_id;
            }
        }
        
        return EntityID();  // Return empty EntityID
    }

    void MapRenderer::SelectProvince(EntityID province_id) {
        // Clear previous selection
        if (selected_province_.id != 0) {
            auto prev_render = entity_manager_.GetComponent<ProvinceRenderComponent>(selected_province_);
            if (prev_render) {
                prev_render->is_selected = false;
            }
        }
        
        // Set new selection
        selected_province_ = province_id;
        if (selected_province_.id != 0) {
            auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(selected_province_);
            if (render) {
                render->is_selected = true;
                std::cout << "Selected province: " << render->name << " (ID: " << render->province_id << ")" << std::endl;
            }
        }
    }

    void MapRenderer::ClearSelection() {
        if (selected_province_.id != 0) {
            auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(selected_province_);
            if (render) {
                render->is_selected = false;
            }
            selected_province_ = EntityID();
        }
    }

    // ========================================================================
    // Helper Methods
    // ========================================================================
    const std::vector<Vector2>& MapRenderer::GetBoundaryForLOD(const ProvinceRenderComponent& province) const {
        switch (current_lod_) {
            case LODLevel::STRATEGIC:
                return !province.boundary_lod0.empty() ? province.boundary_lod0 : province.boundary_points;
            case LODLevel::REGIONAL:
                return !province.boundary_lod1.empty() ? province.boundary_lod1 : province.boundary_points;
            case LODLevel::PROVINCIAL:
                return !province.boundary_lod2.empty() ? province.boundary_lod2 : province.boundary_points;
            case LODLevel::LOCAL:
            case LODLevel::TACTICAL:
            default:
                return province.boundary_points;
        }
    }

    bool MapRenderer::IsPointInPolygon(const std::vector<Vector2>& polygon, float x, float y) const {
        if (polygon.size() < 3) return false;
        
        // Ray casting algorithm
        int crossings = 0;
        size_t n = polygon.size();
        
        for (size_t i = 0; i < n; ++i) {
            const Vector2& p1 = polygon[i];
            const Vector2& p2 = polygon[(i + 1) % n];
            
            if ((p1.y <= y && p2.y > y) || (p1.y > y && p2.y <= y)) {
                float x_intersection = p1.x + (y - p1.y) / (p2.y - p1.y) * (p2.x - p1.x);
                if (x < x_intersection) {
                    crossings++;
                }
            }
        }
        
        return (crossings % 2) == 1;
    }

    uint32_t MapRenderer::ColorToImU32(const Color& color) const {
        return IM_COL32(color.r, color.g, color.b, color.a);
    }

    uint32_t MapRenderer::ColorToImU32(const Color& color, uint8_t alpha) const {
        return IM_COL32(color.r, color.g, color.b, alpha);
    }

    // ========================================================================
    // Layer Visibility Control
    // ========================================================================
    void MapRenderer::SetLayerVisible(MapLayer layer, bool visible) {
        switch (layer) {
            case MapLayer::POLITICAL_BORDERS:
                render_settings_.layer_political_borders = visible;
                render_settings_.show_borders = visible;
                break;
            case MapLayer::TERRAIN_BASE:
                render_settings_.layer_terrain_base = visible;
                break;
            case MapLayer::TRADE_ROUTES:
                render_settings_.layer_trade_routes = visible;
                break;
            case MapLayer::MILITARY_UNITS:
                render_settings_.layer_military_units = visible;
                break;
        }
    }

    bool MapRenderer::IsLayerVisible(MapLayer layer) const {
        switch (layer) {
            case MapLayer::POLITICAL_BORDERS:
                return render_settings_.layer_political_borders;
            case MapLayer::TERRAIN_BASE:
                return render_settings_.layer_terrain_base;
            case MapLayer::TRADE_ROUTES:
                return render_settings_.layer_trade_routes;
            case MapLayer::MILITARY_UNITS:
                return render_settings_.layer_military_units;
            default:
                return false;
        }
    }

} // namespace game::map
