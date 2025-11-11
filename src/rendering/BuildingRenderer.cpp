// ============================================================================
// BuildingRenderer.cpp - Building and Structure Renderer Implementation
// Created: November 1, 2025
// ============================================================================

#include "map/render/BuildingRenderer.h"
#include "map/render/TacticalTerrainRenderer.h"  // For Camera2D
#include "utils/PlatformCompat.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>
#include "core/logging/Logger.h"

namespace game::map {

    // ========================================================================
    // Random number generation utilities
    // ========================================================================
    namespace {
        std::mt19937 rng(std::random_device{}());

        float RandomFloat(float min, float max) {
            std::uniform_real_distribution<float> dist(min, max);
            return dist(rng);
        }

        int RandomInt(int min, int max) {
            std::uniform_int_distribution<int> dist(min, max);
            return dist(rng);
        }
    }

    // ========================================================================
    // Constructor / Destructor
    // ========================================================================
    BuildingRenderer::BuildingRenderer(::core::ecs::EntityManager& entity_manager)
        : entity_manager_(entity_manager)
    {
    }

    // ========================================================================
    // Initialization
    // ========================================================================
    bool BuildingRenderer::Initialize() {
        CORE_STREAM_INFO("BuildingRenderer") << "BuildingRenderer: Initializing..." << std::endl;
        building_data_.clear();
        CORE_STREAM_INFO("BuildingRenderer") << "BuildingRenderer: Initialized successfully" << std::endl;
        return true;
    }

    // ========================================================================
    // Main Rendering
    // ========================================================================
    void BuildingRenderer::RenderProvinceBuildings(
        const ProvinceRenderComponent& province,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        // Only render buildings at high zoom levels
        if (camera.zoom < min_zoom_for_buildings_) {
            return;
        }

        // Get or generate building data for this province
        auto it = building_data_.find(province.province_id);
        if (it == building_data_.end()) {
            building_data_[province.province_id] = GenerateDefaultBuildings(province);
            it = building_data_.find(province.province_id);
        }

        const ProvinceBuildingData& buildings = it->second;
        if (!buildings.has_buildings) {
            return;
        }

        // Render cities
        if (show_cities_) {
            for (const auto& city : buildings.cities) {
                RenderCity(city, camera, draw_list);
            }
        }

        // Render rural buildings
        if (show_buildings_) {
            for (const auto& building : buildings.rural_buildings) {
                if (IsBuildingVisible(building.position, camera)) {
                    RenderBuilding(building, camera, draw_list);
                    rendered_building_count_++;
                }
            }
        }

        // Render military buildings
        if (show_buildings_) {
            for (const auto& building : buildings.military_buildings) {
                if (IsBuildingVisible(building.position, camera)) {
                    RenderBuilding(building, camera, draw_list);
                    rendered_building_count_++;
                }
            }
        }
    }

    void BuildingRenderer::RenderAllBuildings(
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        rendered_building_count_ = 0;
        rendered_city_count_ = 0;

        // Get all provinces with render components
        auto entities = entity_manager_.GetEntitiesWithComponent<ProvinceRenderComponent>();

        for (const auto& entity_id : entities) {
            auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(entity_id);
            if (!render) continue;

            RenderProvinceBuildings(*render, camera, draw_list);
        }
    }

    // ========================================================================
    // Building Rendering
    // ========================================================================
    void BuildingRenderer::RenderBuilding(
        const Building& building,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        Vector2 screen_pos = camera.WorldToScreen(building.position.x, building.position.y);
        float size = building.GetDisplaySize() * camera.zoom;

        // Minimum size for visibility
        if (size < 2.0f) size = 2.0f;

        Color color = building.GetColor();

        // Render based on building type
        switch (building.type) {
            case BuildingType::CASTLE:
            case BuildingType::FORTRESS:
                DrawCastle(screen_pos, size, color, draw_list);
                break;

            case BuildingType::CHURCH:
            case BuildingType::CATHEDRAL:
            case BuildingType::TEMPLE:
                DrawChurch(screen_pos, size, color, draw_list);
                break;

            case BuildingType::TOWER:
            case BuildingType::WATCHTOWER:
                DrawTower(screen_pos, size, color, draw_list);
                break;

            case BuildingType::WALL_SECTION:
                // Walls are drawn as thick lines
                // For simplicity, draw a small rectangle
                DrawBuildingRect(screen_pos, size * 0.5f, building.rotation, color, draw_list);
                break;

            default:
                // Generic building - draw as rectangle
                DrawBuildingRect(screen_pos, size, building.rotation, color, draw_list);
                break;
        }

        // Add health indicator for damaged buildings
        if (building.is_damaged && camera.zoom > 3.0f) {
            float health_ratio = building.health / 100.0f;
            Color health_color = health_ratio > 0.5f ? Color(255, 200, 0) : Color(255, 0, 0);

            // Draw health bar above building
            float bar_width = size * 2.0f;
            float bar_height = 2.0f;
            Vector2 bar_pos(screen_pos.x - bar_width / 2.0f, screen_pos.y - size - 5.0f);

            draw_list->AddRectFilled(
                ImVec2(bar_pos.x, bar_pos.y),
                ImVec2(bar_pos.x + bar_width * health_ratio, bar_pos.y + bar_height),
                IM_COL32(health_color.r, health_color.g, health_color.b, 200)
            );
        }
    }

    void BuildingRenderer::RenderCity(
        const CityLayout& city,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        if (!IsBuildingVisible(city.center, camera)) {
            return;
        }

        rendered_city_count_++;

        // Render city districts
        for (const auto& district : city.districts) {
            RenderDistrict(district, camera, draw_list);
        }

        // Render city fortifications
        if (show_fortifications_) {
            for (const auto& fort : city.fortifications) {
                RenderFortification(fort, camera, draw_list);
            }
        }

        // Render city name at high zoom
        if (camera.zoom > 3.0f) {
            Vector2 screen_pos = camera.WorldToScreen(city.center.x, city.center.y);
            std::string label = city.name + " (" + std::to_string(city.population) + ")";

            ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
            ImVec2 text_pos(screen_pos.x - text_size.x / 2.0f, screen_pos.y - 30.0f);

            // Draw text with background
            draw_list->AddRectFilled(
                ImVec2(text_pos.x - 2, text_pos.y - 2),
                ImVec2(text_pos.x + text_size.x + 2, text_pos.y + text_size.y + 2),
                IM_COL32(0, 0, 0, 180)
            );
            draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 255), label.c_str());
        }
    }

    void BuildingRenderer::RenderDistrict(
        const UrbanDistrict& district,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        // Render all buildings in the district
        for (const auto& building : district.buildings) {
            if (IsBuildingVisible(building.position, camera)) {
                RenderBuilding(building, camera, draw_list);
                rendered_building_count_++;
            }
        }

        // At medium zoom, draw district outline
        if (camera.zoom >= 2.5f && camera.zoom < 4.0f) {
            Vector2 screen_center = camera.WorldToScreen(district.center.x, district.center.y);
            float screen_radius = district.radius * camera.zoom;

            // Get district color
            Color district_color;
            switch (district.primary_category) {
                case BuildingCategory::RESIDENTIAL:
                    district_color = Color(150, 100, 50, 50);
                    break;
                case BuildingCategory::COMMERCIAL:
                    district_color = Color(100, 100, 150, 50);
                    break;
                case BuildingCategory::ECONOMIC:
                    district_color = Color(100, 100, 100, 50);
                    break;
                default:
                    district_color = Color(128, 128, 128, 50);
            }

            draw_list->AddCircle(
                ImVec2(screen_center.x, screen_center.y),
                screen_radius,
                IM_COL32(district_color.r, district_color.g, district_color.b, district_color.a),
                32,
                1.0f
            );
        }
    }

    void BuildingRenderer::RenderFortification(
        const Building& fortification,
        const Camera2D& camera,
        ImDrawList* draw_list)
    {
        if (IsBuildingVisible(fortification.position, camera)) {
            RenderBuilding(fortification, camera, draw_list);
            rendered_building_count_++;
        }
    }

    // ========================================================================
    // Building Shape Drawing
    // ========================================================================
    void BuildingRenderer::DrawBuildingRect(
        const Vector2& screen_pos,
        float size,
        float rotation,
        const Color& color,
        ImDrawList* draw_list)
    {
        uint32_t fill_color = IM_COL32(color.r, color.g, color.b, 255);
        uint32_t outline_color = IM_COL32(0, 0, 0, 255);

        if (rotation == 0.0f) {
            // Axis-aligned rectangle (faster)
            draw_list->AddRectFilled(
                ImVec2(screen_pos.x - size, screen_pos.y - size),
                ImVec2(screen_pos.x + size, screen_pos.y + size),
                fill_color
            );
            draw_list->AddRect(
                ImVec2(screen_pos.x - size, screen_pos.y - size),
                ImVec2(screen_pos.x + size, screen_pos.y + size),
                outline_color,
                0.0f,
                0,
                1.0f
            );
        }
        else {
            // Rotated rectangle
            float cos_r = std::cos(rotation);
            float sin_r = std::sin(rotation);

            ImVec2 corners[4] = {
                ImVec2(screen_pos.x + (-size * cos_r - -size * sin_r), screen_pos.y + (-size * sin_r + -size * cos_r)),
                ImVec2(screen_pos.x + (size * cos_r - -size * sin_r), screen_pos.y + (size * sin_r + -size * cos_r)),
                ImVec2(screen_pos.x + (size * cos_r - size * sin_r), screen_pos.y + (size * sin_r + size * cos_r)),
                ImVec2(screen_pos.x + (-size * cos_r - size * sin_r), screen_pos.y + (-size * sin_r + size * cos_r))
            };

            draw_list->AddConvexPolyFilled(corners, 4, fill_color);
            draw_list->AddPolyline(corners, 4, outline_color, ImDrawFlags_Closed, 1.0f);
        }
    }

    void BuildingRenderer::DrawBuildingCircle(
        const Vector2& screen_pos,
        float radius,
        const Color& color,
        ImDrawList* draw_list)
    {
        uint32_t fill_color = IM_COL32(color.r, color.g, color.b, 255);
        uint32_t outline_color = IM_COL32(0, 0, 0, 255);

        draw_list->AddCircleFilled(
            ImVec2(screen_pos.x, screen_pos.y),
            radius,
            fill_color
        );
        draw_list->AddCircle(
            ImVec2(screen_pos.x, screen_pos.y),
            radius,
            outline_color,
            16,
            1.0f
        );
    }

    void BuildingRenderer::DrawCastle(
        const Vector2& screen_pos,
        float size,
        const Color& color,
        ImDrawList* draw_list)
    {
        uint32_t fill_color = IM_COL32(color.r, color.g, color.b, 255);
        uint32_t outline_color = IM_COL32(0, 0, 0, 255);

        // Main keep (central rectangle)
        draw_list->AddRectFilled(
            ImVec2(screen_pos.x - size * 0.6f, screen_pos.y - size * 0.6f),
            ImVec2(screen_pos.x + size * 0.6f, screen_pos.y + size * 0.6f),
            fill_color
        );

        // Corner towers (circles)
        float tower_radius = size * 0.3f;
        for (int i = 0; i < 4; ++i) {
            float x_offset = (i % 2 == 0) ? -size * 0.7f : size * 0.7f;
            float y_offset = (i < 2) ? -size * 0.7f : size * 0.7f;

            draw_list->AddCircleFilled(
                ImVec2(screen_pos.x + x_offset, screen_pos.y + y_offset),
                tower_radius,
                fill_color
            );
            draw_list->AddCircle(
                ImVec2(screen_pos.x + x_offset, screen_pos.y + y_offset),
                tower_radius,
                outline_color,
                12,
                1.0f
            );
        }

        // Outline
        draw_list->AddRect(
            ImVec2(screen_pos.x - size * 0.6f, screen_pos.y - size * 0.6f),
            ImVec2(screen_pos.x + size * 0.6f, screen_pos.y + size * 0.6f),
            outline_color,
            0.0f,
            0,
            1.5f
        );
    }

    void BuildingRenderer::DrawChurch(
        const Vector2& screen_pos,
        float size,
        const Color& color,
        ImDrawList* draw_list)
    {
        uint32_t fill_color = IM_COL32(color.r, color.g, color.b, 255);
        uint32_t outline_color = IM_COL32(0, 0, 0, 255);

        // Main building (rectangle)
        draw_list->AddRectFilled(
            ImVec2(screen_pos.x - size * 0.8f, screen_pos.y - size * 0.5f),
            ImVec2(screen_pos.x + size * 0.8f, screen_pos.y + size * 0.8f),
            fill_color
        );

        // Tower/spire (small rectangle on top)
        draw_list->AddRectFilled(
            ImVec2(screen_pos.x - size * 0.3f, screen_pos.y - size * 1.2f),
            ImVec2(screen_pos.x + size * 0.3f, screen_pos.y - size * 0.5f),
            fill_color
        );

        // Cross on top (if zoom is high enough)
        if (size > 5.0f) {
            // Vertical
            draw_list->AddLine(
                ImVec2(screen_pos.x, screen_pos.y - size * 1.5f),
                ImVec2(screen_pos.x, screen_pos.y - size * 1.2f),
                outline_color,
                1.5f
            );
            // Horizontal
            draw_list->AddLine(
                ImVec2(screen_pos.x - size * 0.15f, screen_pos.y - size * 1.35f),
                ImVec2(screen_pos.x + size * 0.15f, screen_pos.y - size * 1.35f),
                outline_color,
                1.5f
            );
        }

        // Outlines
        draw_list->AddRect(
            ImVec2(screen_pos.x - size * 0.8f, screen_pos.y - size * 0.5f),
            ImVec2(screen_pos.x + size * 0.8f, screen_pos.y + size * 0.8f),
            outline_color,
            0.0f,
            0,
            1.0f
        );
        draw_list->AddRect(
            ImVec2(screen_pos.x - size * 0.3f, screen_pos.y - size * 1.2f),
            ImVec2(screen_pos.x + size * 0.3f, screen_pos.y - size * 0.5f),
            outline_color,
            0.0f,
            0,
            1.0f
        );
    }

    void BuildingRenderer::DrawTower(
        const Vector2& screen_pos,
        float size,
        const Color& color,
        ImDrawList* draw_list)
    {
        uint32_t fill_color = IM_COL32(color.r, color.g, color.b, 255);
        uint32_t outline_color = IM_COL32(0, 0, 0, 255);

        // Tower body (tall rectangle)
        draw_list->AddRectFilled(
            ImVec2(screen_pos.x - size * 0.4f, screen_pos.y - size * 1.5f),
            ImVec2(screen_pos.x + size * 0.4f, screen_pos.y + size * 0.5f),
            fill_color
        );

        // Crenellations on top (if zoom is high enough)
        if (size > 4.0f) {
            float crenel_width = size * 0.2f;
            for (int i = 0; i < 3; ++i) {
                float x_offset = -size * 0.4f + i * crenel_width * 1.5f;
                draw_list->AddRectFilled(
                    ImVec2(screen_pos.x + x_offset, screen_pos.y - size * 1.7f),
                    ImVec2(screen_pos.x + x_offset + crenel_width, screen_pos.y - size * 1.5f),
                    fill_color
                );
            }
        }

        // Outline
        draw_list->AddRect(
            ImVec2(screen_pos.x - size * 0.4f, screen_pos.y - size * 1.5f),
            ImVec2(screen_pos.x + size * 0.4f, screen_pos.y + size * 0.5f),
            outline_color,
            0.0f,
            0,
            1.5f
        );
    }

    // ========================================================================
    // Viewport Culling
    // ========================================================================
    bool BuildingRenderer::IsBuildingVisible(
        const Vector2& world_pos,
        const Camera2D& camera) const
    {
        // Simple bounding box check
        Vector2 top_left_world = camera.ScreenToWorld(0, 0);
        Vector2 bottom_right_world = camera.ScreenToWorld(camera.viewport_width, camera.viewport_height);

        // Add margin for building size
        float margin = 20.0f / camera.zoom;

        return world_pos.x >= top_left_world.x - margin &&
               world_pos.x <= bottom_right_world.x + margin &&
               world_pos.y >= top_left_world.y - margin &&
               world_pos.y <= bottom_right_world.y + margin;
    }

    // ========================================================================
    // Building Generation
    // ========================================================================
    void BuildingRenderer::GenerateBuildingsForProvince(EntityID province_id) {
        auto render = entity_manager_.GetComponent<ProvinceRenderComponent>(province_id);
        if (!render) return;

        building_data_[render->province_id] = GenerateDefaultBuildings(*render);
    }

    ProvinceBuildingData BuildingRenderer::GenerateDefaultBuildings(
        const ProvinceRenderComponent& province)
    {
        ProvinceBuildingData data(province.province_id);

        // Generate cities based on existing features
        for (const auto& feature : province.features) {
            if (feature.type == FeatureType::CITY || feature.type == FeatureType::TOWN) {
                CityLayout city(
                    static_cast<uint32_t>(data.cities.size()),
                    feature.name,
                    feature.position,
                    feature.population
                );

                GenerateCityLayout(city, province);
                data.cities.push_back(city);
            }
        }

        // Generate rural buildings
        GenerateRuralBuildings(data.rural_buildings, province);

        data.has_buildings = true;

        CORE_STREAM_INFO("BuildingRenderer") << "Generated buildings for province " << province.province_id
                  << " (" << province.name << "): "
                  << data.cities.size() << " cities, "
                  << data.rural_buildings.size() << " rural buildings" << std::endl;

        return data;
    }

    void BuildingRenderer::GenerateCityLayout(
        CityLayout& city,
        const ProvinceRenderComponent& province)
    {
        // Determine city size based on population
        float city_radius = 5.0f + (city.population / 10000.0f);
        city_radius = std::min(city_radius, 30.0f);

        // Determine if city has walls (larger cities)
        city.has_walls = city.population > 10000;

        // Generate districts based on population
        int district_count = 1 + (city.population / 20000);
        district_count = std::min(district_count, 6);

        for (int i = 0; i < district_count; ++i) {
            BuildingCategory category;

            // First district is always residential
            if (i == 0) {
                category = BuildingCategory::RESIDENTIAL;
            }
            // Second district is commercial
            else if (i == 1) {
                category = BuildingCategory::COMMERCIAL;
            }
            // Others vary
            else {
                int rand_cat = RandomInt(0, 4);
                switch (rand_cat) {
                    case 0: category = BuildingCategory::RESIDENTIAL; break;
                    case 1: category = BuildingCategory::COMMERCIAL; break;
                    case 2: category = BuildingCategory::MILITARY; break;
                    case 3: category = BuildingCategory::RELIGIOUS; break;
                    default: category = BuildingCategory::ADMINISTRATIVE; break;
                }
            }

            // Generate district
            UrbanDistrict district;
            district.primary_category = category;
            district.radius = city_radius / (district_count * 0.8f);

            // Position districts in a circle around city center
            float angle = (i / static_cast<float>(district_count)) * 2.0f * 3.14159f;
            float dist = city_radius * 0.5f;
            district.center = Vector2(
                city.center.x + dist * std::cos(angle),
                city.center.y + dist * std::sin(angle)
            );

            // Generate buildings in district
            int building_count = 5 + RandomInt(0, 15);
            GenerateUrbanDistrict(district, category, building_count);

            city.districts.push_back(district);
        }

        // Generate fortifications if the city has walls
        if (city.has_walls) {
            GenerateFortifications(city.fortifications, city.center, city_radius, true);
        }
    }

    void BuildingRenderer::GenerateUrbanDistrict(
        UrbanDistrict& district,
        BuildingCategory category,
        uint32_t building_count)
    {
        district.buildings.reserve(building_count);

        // Determine building types based on category
        std::vector<BuildingType> possible_types;

        switch (category) {
            case BuildingCategory::RESIDENTIAL:
                possible_types = { BuildingType::HOUSE, BuildingType::APARTMENT, BuildingType::MANSION };
                break;
            case BuildingCategory::COMMERCIAL:
                possible_types = { BuildingType::SHOP, BuildingType::MARKET, BuildingType::WAREHOUSE, BuildingType::WORKSHOP };
                break;
            case BuildingCategory::MILITARY:
                possible_types = { BuildingType::BARRACKS, BuildingType::ARMORY, BuildingType::TRAINING_GROUND };
                break;
            case BuildingCategory::RELIGIOUS:
                possible_types = { BuildingType::CHURCH, BuildingType::SHRINE };
                break;
            case BuildingCategory::ADMINISTRATIVE:
                possible_types = { BuildingType::TOWN_HALL, BuildingType::COURTHOUSE };
                break;
            default:
                possible_types = { BuildingType::HOUSE };
        }

        // Generate buildings
        for (uint32_t i = 0; i < building_count; ++i) {
            BuildingType type = possible_types[RandomInt(0, possible_types.size() - 1)];
            Vector2 pos = GetRandomPositionInCircle(district.center, district.radius);

            Building building(type, pos);
            building.rotation = RandomFloat(0.0f, 3.14159f * 2.0f);
            building.size = RandomFloat(0.8f, 1.5f);

            district.buildings.push_back(building);
        }
    }

    void BuildingRenderer::GenerateRuralBuildings(
        std::vector<Building>& buildings,
        const ProvinceRenderComponent& province)
    {
        // Generate farms and rural buildings scattered across the province
        Rect bounds = province.bounding_box;

        // Number of rural buildings based on province size
        float area = bounds.GetWidth() * bounds.GetHeight();
        int building_count = static_cast<int>(area / 50.0f);  // One building per 50 square units
        building_count = std::min(building_count, 100);  // Cap at 100

        std::vector<BuildingType> rural_types = {
            BuildingType::FARM,
            BuildingType::MILL,
            BuildingType::BARN,
            BuildingType::FISHING_HUT
        };

        for (int i = 0; i < building_count; ++i) {
            BuildingType type = rural_types[RandomInt(0, rural_types.size() - 1)];
            Vector2 pos = GetRandomPositionInRect(bounds);

            Building building(type, pos);
            building.size = RandomFloat(0.8f, 1.2f);

            buildings.push_back(building);
        }
    }

    void BuildingRenderer::GenerateFortifications(
        std::vector<Building>& fortifications,
        const Vector2& city_center,
        float city_radius,
        bool has_walls)
    {
        if (!has_walls) return;

        // Generate wall sections around the city
        int wall_segments = 16;
        float wall_radius = city_radius * 1.2f;

        for (int i = 0; i < wall_segments; ++i) {
            float angle = (i / static_cast<float>(wall_segments)) * 2.0f * 3.14159f;
            Vector2 pos(
                city_center.x + wall_radius * std::cos(angle),
                city_center.y + wall_radius * std::sin(angle)
            );

            Building wall(BuildingType::WALL_SECTION, pos);
            wall.rotation = angle + 3.14159f / 2.0f;  // Perpendicular to radius
            wall.size = 1.0f;

            fortifications.push_back(wall);
        }

        // Generate towers at cardinal directions
        for (int i = 0; i < 4; ++i) {
            float angle = (i / 4.0f) * 2.0f * 3.14159f;
            Vector2 pos(
                city_center.x + wall_radius * std::cos(angle),
                city_center.y + wall_radius * std::sin(angle)
            );

            Building tower(BuildingType::TOWER, pos);
            tower.size = 1.5f;

            fortifications.push_back(tower);
        }

        // Generate gates
        for (int i = 0; i < 4; ++i) {
            float angle = ((i + 0.5f) / 4.0f) * 2.0f * 3.14159f;  // Between towers
            Vector2 pos(
                city_center.x + wall_radius * std::cos(angle),
                city_center.y + wall_radius * std::sin(angle)
            );

            Building gate(BuildingType::GATE, pos);
            gate.size = 1.2f;

            fortifications.push_back(gate);
        }
    }

    // ========================================================================
    // Helper Methods
    // ========================================================================
    Vector2 BuildingRenderer::GetRandomPositionInCircle(const Vector2& center, float radius) {
        float angle = RandomFloat(0.0f, 2.0f * 3.14159f);
        float r = std::sqrt(RandomFloat(0.0f, 1.0f)) * radius;

        return Vector2(
            center.x + r * std::cos(angle),
            center.y + r * std::sin(angle)
        );
    }

    Vector2 BuildingRenderer::GetRandomPositionInRect(const Rect& bounds) {
        return Vector2(
            RandomFloat(bounds.min_x, bounds.max_x),
            RandomFloat(bounds.min_y, bounds.max_y)
        );
    }

    float BuildingRenderer::RandomFloat(float min, float max) {
        return ::game::map::RandomFloat(min, max);
    }

} // namespace game::map
