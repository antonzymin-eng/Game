// ============================================================================
// UnitRenderer.cpp - LOD 4 Military and Naval Unit Renderer Implementation
// Created: November 1, 2025
// ============================================================================

#include "utils/PlatformCompat.h"
#include "map/render/UnitRenderer.h"
#include "map/render/TacticalTerrainRenderer.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace game::map {

    // Helper function to convert game::types::EntityID to core::ecs::EntityID
    static inline ::core::ecs::EntityID ToECSEntityID(game::types::EntityID game_id) {
        return ::core::ecs::EntityID{game_id};
    }

    // ========================================================================
    // Constructor
    // ========================================================================
    UnitRenderer::UnitRenderer(::core::ecs::EntityManager& entity_manager)
        : entity_manager_(entity_manager)
    {
    }

    // ========================================================================
    // Initialization
    // ========================================================================
    bool UnitRenderer::Initialize() {
        std::cout << "UnitRenderer: Initializing..." << std::endl;
        std::cout << "UnitRenderer: Initialized successfully" << std::endl;
        return true;
    }

    // ========================================================================
    // Main Render Functions
    // ========================================================================
    void UnitRenderer::RenderAllUnits(
        const Camera2D& camera,
        ImDrawList* draw_list
    ) {
        if (!show_units_ || camera.zoom < min_zoom_for_units_) {
            return;
        }

        rendered_unit_count_ = 0;
        rendered_army_count_ = 0;

        // Get all entities with ArmyComponent
        auto army_entities = entity_manager_.GetEntitiesWithComponent<military::ArmyComponent>();

        for (const auto& entity_id : army_entities) {
            auto army = entity_manager_.GetComponent<military::ArmyComponent>(entity_id);
            if (!army || !army->is_active) continue;

            // Get army position from location
            auto location_render = entity_manager_.GetComponent<ProvinceRenderComponent>(ToECSEntityID(army->current_location));
            if (!location_render) continue;

            // Check if army is visible
            if (!IsUnitVisible(location_render->center_position, camera)) continue;

            RenderArmy(*army, camera, draw_list);
            rendered_army_count_++;
        }

        // Render naval units separately
        RenderNavalUnits(camera, draw_list);
    }

    void UnitRenderer::RenderArmy(
        const military::ArmyComponent& army,
        const Camera2D& camera,
        ImDrawList* draw_list
    ) {
        // Get army location
        auto location_render = entity_manager_.GetComponent<ProvinceRenderComponent>(ToECSEntityID(army.current_location));
        if (!location_render) return;

        // Create formation for army
        FormationData formation = CreateFormation(army, FormationType::LINE);
        formation.center_position = location_render->center_position;

        // Render formation
        RenderFormation(formation, camera, draw_list);
    }

    void UnitRenderer::RenderFormation(
        const FormationData& formation,
        const Camera2D& camera,
        ImDrawList* draw_list
    ) {
        if (!show_formations_) return;

        // Render formation shape (outline)
        RenderFormationShape(formation, camera, draw_list);

        // Render individual units in formation
        RenderFormationUnits(formation, camera, draw_list);
    }

    void UnitRenderer::RenderUnit(
        const UnitVisual& unit,
        const Camera2D& camera,
        ImDrawList* draw_list
    ) {
        if (!IsUnitVisible(unit.world_position, camera)) return;

        Vector2 screen_pos = camera.WorldToScreen(unit.world_position.x, unit.world_position.y);
        float base_size = 12.0f * unit_scale_ * camera.zoom;

        // Render unit based on type
        if (IsInfantry(unit.type)) {
            RenderInfantryUnit(unit, screen_pos, base_size, draw_list);
        }
        else if (IsCavalry(unit.type)) {
            RenderCavalryUnit(unit, screen_pos, base_size, draw_list);
        }
        else if (IsSiege(unit.type)) {
            RenderSiegeUnit(unit, screen_pos, base_size, draw_list);
        }
        else if (IsNaval(unit.type)) {
            RenderNavalUnit(unit, screen_pos, base_size, draw_list);
        }

        // Render indicators
        if (show_strength_indicators_) {
            RenderStrengthIndicator(unit, screen_pos, base_size, draw_list);
            RenderMoraleIndicator(unit, screen_pos, base_size, draw_list);
        }

        if (show_unit_icons_) {
            RenderUnitBadges(unit, screen_pos, base_size, draw_list);
        }

        rendered_unit_count_++;
    }

    void UnitRenderer::RenderNavalUnits(
        const Camera2D& camera,
        ImDrawList* draw_list
    ) {
        // Naval units would be rendered similarly to armies but with naval-specific visuals
        // For now, they're included in RenderAllUnits via ArmyComponent
    }

    // ========================================================================
    // Unit Type Rendering
    // ========================================================================
    void UnitRenderer::RenderInfantryUnit(
        const UnitVisual& unit,
        const Vector2& screen_pos,
        float size,
        ImDrawList* draw_list
    ) {
        // Infantry: Rectangle
        DrawUnitRect(screen_pos, size, unit.rotation, unit.unit_color, draw_list);

        // Add border
        ImU32 border_color = IM_COL32(0, 0, 0, 255);
        float half_size = size * 0.5f;
        draw_list->AddRect(
            ImVec2(screen_pos.x - half_size, screen_pos.y - half_size),
            ImVec2(screen_pos.x + half_size, screen_pos.y + half_size),
            border_color,
            0.0f,
            0,
            2.0f
        );
    }

    void UnitRenderer::RenderCavalryUnit(
        const UnitVisual& unit,
        const Vector2& screen_pos,
        float size,
        ImDrawList* draw_list
    ) {
        // Cavalry: Triangle (pointing in direction of rotation)
        DrawUnitTriangle(screen_pos, size * 1.2f, unit.rotation, unit.unit_color, draw_list);

        // Add border
        ImU32 border_color = IM_COL32(0, 0, 0, 255);
        DrawUnitTriangle(screen_pos, size * 1.2f, unit.rotation, Color(0, 0, 0, 0), draw_list);
    }

    void UnitRenderer::RenderSiegeUnit(
        const UnitVisual& unit,
        const Vector2& screen_pos,
        float size,
        ImDrawList* draw_list
    ) {
        // Siege: Circle with cross
        ImU32 color = IM_COL32(
            static_cast<int>(unit.unit_color.r * 255),
            static_cast<int>(unit.unit_color.g * 255),
            static_cast<int>(unit.unit_color.b * 255),
            static_cast<int>(unit.unit_color.a * 255)
        );

        draw_list->AddCircleFilled(
            ImVec2(screen_pos.x, screen_pos.y),
            size * 0.6f,
            color,
            16
        );

        // Draw cross
        ImU32 cross_color = IM_COL32(0, 0, 0, 255);
        draw_list->AddLine(
            ImVec2(screen_pos.x - size * 0.4f, screen_pos.y),
            ImVec2(screen_pos.x + size * 0.4f, screen_pos.y),
            cross_color,
            2.0f
        );
        draw_list->AddLine(
            ImVec2(screen_pos.x, screen_pos.y - size * 0.4f),
            ImVec2(screen_pos.x, screen_pos.y + size * 0.4f),
            cross_color,
            2.0f
        );

        // Border
        draw_list->AddCircle(
            ImVec2(screen_pos.x, screen_pos.y),
            size * 0.6f,
            IM_COL32(0, 0, 0, 255),
            16,
            2.0f
        );
    }

    void UnitRenderer::RenderNavalUnit(
        const UnitVisual& unit,
        const Vector2& screen_pos,
        float size,
        ImDrawList* draw_list
    ) {
        // Naval: Ship shape
        DrawShip(screen_pos, size, unit.rotation, unit.unit_color, unit.type, draw_list);
    }

    // ========================================================================
    // Formation Rendering
    // ========================================================================
    void UnitRenderer::RenderFormationShape(
        const FormationData& formation,
        const Camera2D& camera,
        ImDrawList* draw_list
    ) {
        // Draw formation outline
        float width = formation.columns * formation.unit_spacing;
        float height = formation.rows * formation.unit_spacing;

        Vector2 screen_center = camera.WorldToScreen(
            formation.center_position.x,
            formation.center_position.y
        );

        float scaled_width = width * camera.zoom;
        float scaled_height = height * camera.zoom;

        ImU32 outline_color = IM_COL32(255, 255, 255, 80);

        // Simple rectangle outline for now
        draw_list->AddRect(
            ImVec2(screen_center.x - scaled_width * 0.5f, screen_center.y - scaled_height * 0.5f),
            ImVec2(screen_center.x + scaled_width * 0.5f, screen_center.y + scaled_height * 0.5f),
            outline_color,
            0.0f,
            0,
            1.0f
        );
    }

    void UnitRenderer::RenderFormationUnits(
        const FormationData& formation,
        const Camera2D& camera,
        ImDrawList* draw_list
    ) {
        // Render each unit in the formation
        for (const auto& unit : formation.units) {
            RenderUnit(unit, camera, draw_list);
        }
    }

    // ========================================================================
    // Unit Indicators
    // ========================================================================
    void UnitRenderer::RenderStrengthIndicator(
        const UnitVisual& unit,
        const Vector2& screen_pos,
        float size,
        ImDrawList* draw_list
    ) {
        // Strength bar below unit
        float bar_width = size * 1.2f;
        float bar_height = 3.0f;
        float bar_y = screen_pos.y + size * 0.7f;

        double strength_ratio = static_cast<double>(unit.current_strength) /
                                static_cast<double>(unit.max_strength);

        // Background
        draw_list->AddRectFilled(
            ImVec2(screen_pos.x - bar_width * 0.5f, bar_y),
            ImVec2(screen_pos.x + bar_width * 0.5f, bar_y + bar_height),
            IM_COL32(50, 50, 50, 200)
        );

        // Strength bar
        Color strength_color = GetStrengthColor(strength_ratio);
        ImU32 color = IM_COL32(
            static_cast<int>(strength_color.r * 255),
            static_cast<int>(strength_color.g * 255),
            static_cast<int>(strength_color.b * 255),
            200
        );

        draw_list->AddRectFilled(
            ImVec2(screen_pos.x - bar_width * 0.5f, bar_y),
            ImVec2(screen_pos.x - bar_width * 0.5f + bar_width * strength_ratio, bar_y + bar_height),
            color
        );

        // Border
        draw_list->AddRect(
            ImVec2(screen_pos.x - bar_width * 0.5f, bar_y),
            ImVec2(screen_pos.x + bar_width * 0.5f, bar_y + bar_height),
            IM_COL32(0, 0, 0, 255),
            0.0f,
            0,
            1.0f
        );
    }

    void UnitRenderer::RenderMoraleIndicator(
        const UnitVisual& unit,
        const Vector2& screen_pos,
        float size,
        ImDrawList* draw_list
    ) {
        // Small colored dot above unit indicating morale
        float dot_y = screen_pos.y - size * 0.7f;
        float dot_radius = 3.0f;

        Color morale_color = GetMoraleColor(unit.morale);
        ImU32 color = IM_COL32(
            static_cast<int>(morale_color.r * 255),
            static_cast<int>(morale_color.g * 255),
            static_cast<int>(morale_color.b * 255),
            255
        );

        draw_list->AddCircleFilled(
            ImVec2(screen_pos.x, dot_y),
            dot_radius,
            color,
            8
        );

        // Border
        draw_list->AddCircle(
            ImVec2(screen_pos.x, dot_y),
            dot_radius,
            IM_COL32(0, 0, 0, 255),
            8,
            1.0f
        );
    }

    void UnitRenderer::RenderUnitBadges(
        const UnitVisual& unit,
        const Vector2& screen_pos,
        float size,
        ImDrawList* draw_list
    ) {
        // Render special badges (engaged, routing, etc.)
        if (unit.is_routing) {
            // Red X for routing
            ImU32 color = IM_COL32(255, 0, 0, 200);
            float badge_size = size * 0.3f;
            draw_list->AddLine(
                ImVec2(screen_pos.x - badge_size, screen_pos.y - badge_size),
                ImVec2(screen_pos.x + badge_size, screen_pos.y + badge_size),
                color,
                2.0f
            );
            draw_list->AddLine(
                ImVec2(screen_pos.x + badge_size, screen_pos.y - badge_size),
                ImVec2(screen_pos.x - badge_size, screen_pos.y + badge_size),
                color,
                2.0f
            );
        }
        else if (unit.is_engaged) {
            // Crossed swords for engaged
            ImU32 color = IM_COL32(255, 255, 0, 200);
            float badge_size = size * 0.3f;
            draw_list->AddLine(
                ImVec2(screen_pos.x - badge_size, screen_pos.y - badge_size),
                ImVec2(screen_pos.x + badge_size, screen_pos.y + badge_size),
                color,
                2.0f
            );
            draw_list->AddLine(
                ImVec2(screen_pos.x + badge_size, screen_pos.y - badge_size),
                ImVec2(screen_pos.x - badge_size, screen_pos.y + badge_size),
                color,
                2.0f
            );
        }
    }

    // ========================================================================
    // Helper Drawing Functions
    // ========================================================================
    void UnitRenderer::DrawUnitRect(
        const Vector2& screen_pos,
        float size,
        float rotation,
        const Color& color,
        ImDrawList* draw_list
    ) {
        ImU32 im_color = IM_COL32(
            static_cast<int>(color.r * 255),
            static_cast<int>(color.g * 255),
            static_cast<int>(color.b * 255),
            static_cast<int>(color.a * 255)
        );

        float half_size = size * 0.5f;

        // For simplicity, no rotation for now (can be added later with matrix transform)
        draw_list->AddRectFilled(
            ImVec2(screen_pos.x - half_size, screen_pos.y - half_size),
            ImVec2(screen_pos.x + half_size, screen_pos.y + half_size),
            im_color
        );
    }

    void UnitRenderer::DrawUnitTriangle(
        const Vector2& screen_pos,
        float size,
        float rotation,
        const Color& color,
        ImDrawList* draw_list
    ) {
        ImU32 im_color = IM_COL32(
            static_cast<int>(color.r * 255),
            static_cast<int>(color.g * 255),
            static_cast<int>(color.b * 255),
            static_cast<int>(color.a * 255)
        );

        // Triangle pointing up (rotation can be applied later)
        float height = size;
        float base = size * 0.866f; // sqrt(3)/2

        ImVec2 p1(screen_pos.x, screen_pos.y - height * 0.5f);
        ImVec2 p2(screen_pos.x - base * 0.5f, screen_pos.y + height * 0.5f);
        ImVec2 p3(screen_pos.x + base * 0.5f, screen_pos.y + height * 0.5f);

        draw_list->AddTriangleFilled(p1, p2, p3, im_color);
    }

    void UnitRenderer::DrawShip(
        const Vector2& screen_pos,
        float size,
        float rotation,
        const Color& color,
        military::UnitType ship_type,
        ImDrawList* draw_list
    ) {
        ImU32 im_color = IM_COL32(
            static_cast<int>(color.r * 255),
            static_cast<int>(color.g * 255),
            static_cast<int>(color.b * 255),
            static_cast<int>(color.a * 255)
        );

        // Simple boat shape - elongated ellipse
        float width = size * 1.5f;
        float height = size * 0.6f;

        // Draw hull
        draw_list->AddEllipseFilled(
            ImVec2(screen_pos.x, screen_pos.y),
            width * 0.5f,
            height * 0.5f,
            im_color,
            0.0f,
            16
        );

        // Draw border
        draw_list->AddEllipse(
            ImVec2(screen_pos.x, screen_pos.y),
            width * 0.5f,
            height * 0.5f,
            IM_COL32(0, 0, 0, 255),
            0.0f,
            16,
            2.0f
        );

        // Draw mast (simple vertical line)
        draw_list->AddLine(
            ImVec2(screen_pos.x, screen_pos.y - height * 0.5f),
            ImVec2(screen_pos.x, screen_pos.y - height * 0.5f - size * 0.5f),
            IM_COL32(100, 60, 30, 255),
            2.0f
        );
    }

    void UnitRenderer::DrawArrow(
        const Vector2& from,
        const Vector2& to,
        const Color& color,
        float thickness,
        ImDrawList* draw_list
    ) {
        ImU32 im_color = IM_COL32(
            static_cast<int>(color.r * 255),
            static_cast<int>(color.g * 255),
            static_cast<int>(color.b * 255),
            static_cast<int>(color.a * 255)
        );

        draw_list->AddLine(
            ImVec2(from.x, from.y),
            ImVec2(to.x, to.y),
            im_color,
            thickness
        );

        // Arrow head
        float dx = to.x - from.x;
        float dy = to.y - from.y;
        float length = std::sqrt(dx * dx + dy * dy);
        if (length > 0) {
            dx /= length;
            dy /= length;

            float arrow_size = 8.0f;
            ImVec2 p1(to.x - dx * arrow_size - dy * arrow_size * 0.5f,
                      to.y - dy * arrow_size + dx * arrow_size * 0.5f);
            ImVec2 p2(to.x - dx * arrow_size + dy * arrow_size * 0.5f,
                      to.y - dy * arrow_size - dx * arrow_size * 0.5f);

            draw_list->AddTriangleFilled(ImVec2(to.x, to.y), p1, p2, im_color);
        }
    }

    // ========================================================================
    // Formation Creation
    // ========================================================================
    FormationData UnitRenderer::CreateFormation(
        const military::ArmyComponent& army,
        FormationType type
    ) {
        FormationData formation;
        formation.type = type;

        // Convert army units to visual units
        for (const auto& unit : army.units) {
            UnitVisual visual;
            visual.type = unit.type;
            visual.unit_class = unit.unit_class;
            visual.current_strength = unit.current_strength;
            visual.max_strength = unit.max_strength;
            visual.morale = static_cast<double>(unit.morale);
            visual.experience = unit.experience;
            visual.unit_color = GetUnitColor(unit.type);

            formation.units.push_back(visual);
        }

        // Calculate formation grid
        int unit_count = static_cast<int>(formation.units.size());
        CalculateFormationGrid(unit_count, type, formation.rows, formation.columns);

        // Update unit positions in formation
        UpdateFormationPositions(formation);

        return formation;
    }

    void UnitRenderer::UpdateFormationPositions(
        FormationData& formation
    ) {
        for (size_t i = 0; i < formation.units.size(); ++i) {
            formation.units[i].world_position = CalculateUnitPosition(formation, static_cast<int>(i));
        }
    }

    Vector2 UnitRenderer::CalculateUnitPosition(
        const FormationData& formation,
        int unit_index
    ) const {
        int row = unit_index / formation.columns;
        int col = unit_index % formation.columns;

        float x_offset = (col - formation.columns * 0.5f + 0.5f) * formation.unit_spacing;
        float y_offset = (row - formation.rows * 0.5f + 0.5f) * formation.unit_spacing;

        return Vector2(
            formation.center_position.x + x_offset,
            formation.center_position.y + y_offset
        );
    }

    void UnitRenderer::CalculateFormationGrid(
        int unit_count,
        FormationType type,
        int& out_rows,
        int& out_columns
    ) const {
        if (unit_count == 0) {
            out_rows = 0;
            out_columns = 0;
            return;
        }

        switch (type) {
            case FormationType::LINE:
                // Wide front, few rows
                out_columns = std::max(1, static_cast<int>(std::sqrt(unit_count * 3)));
                out_rows = (unit_count + out_columns - 1) / out_columns;
                break;

            case FormationType::COLUMN:
                // Narrow front, many rows
                out_columns = std::max(1, static_cast<int>(std::sqrt(unit_count / 3)));
                out_rows = (unit_count + out_columns - 1) / out_columns;
                break;

            case FormationType::SQUARE:
                // Square formation
                out_columns = std::max(1, static_cast<int>(std::sqrt(unit_count)));
                out_rows = (unit_count + out_columns - 1) / out_columns;
                break;

            case FormationType::WEDGE:
                // Triangular wedge
                out_rows = std::max(1, static_cast<int>(std::sqrt(unit_count * 2)));
                out_columns = out_rows;
                break;

            case FormationType::SCATTERED:
                // Random scatter (still use grid for simplicity)
                out_columns = std::max(1, static_cast<int>(std::sqrt(unit_count)));
                out_rows = (unit_count + out_columns - 1) / out_columns;
                break;

            case FormationType::NAVAL_LINE:
                // Single line for naval
                out_columns = unit_count;
                out_rows = 1;
                break;

            case FormationType::NAVAL_CRESCENT:
                // Crescent formation
                out_columns = std::max(1, static_cast<int>(std::sqrt(unit_count * 2)));
                out_rows = (unit_count + out_columns - 1) / out_columns;
                break;

            default:
                out_columns = std::max(1, static_cast<int>(std::sqrt(unit_count)));
                out_rows = (unit_count + out_columns - 1) / out_columns;
                break;
        }
    }

    // ========================================================================
    // Helper Functions
    // ========================================================================
    bool UnitRenderer::IsUnitVisible(
        const Vector2& world_pos,
        const Camera2D& camera
    ) const {
        Vector2 screen_pos = camera.WorldToScreen(world_pos.x, world_pos.y);

        // Check if within viewport with margin
        float margin = 100.0f;
        return (screen_pos.x >= -margin && screen_pos.x <= camera.viewport_width + margin &&
                screen_pos.y >= -margin && screen_pos.y <= camera.viewport_height + margin);
    }

    Color UnitRenderer::GetUnitColor(military::UnitType type) const {
        using military::UnitType;
        using military::UnitClass;

        if (IsInfantry(type)) {
            return Color(0.7f, 0.3f, 0.3f, 1.0f); // Red-brown for infantry
        }
        else if (IsCavalry(type)) {
            return Color(0.3f, 0.5f, 0.8f, 1.0f); // Blue for cavalry
        }
        else if (IsSiege(type)) {
            return Color(0.5f, 0.5f, 0.5f, 1.0f); // Gray for siege
        }
        else if (IsNaval(type)) {
            return Color(0.2f, 0.4f, 0.7f, 1.0f); // Navy blue
        }

        return Color(0.5f, 0.5f, 0.5f, 1.0f); // Default gray
    }

    Color UnitRenderer::GetMoraleColor(double morale) const {
        if (morale >= 0.8) {
            return Color(0.0f, 1.0f, 0.0f, 1.0f); // Green - high morale
        }
        else if (morale >= 0.5) {
            return Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow - medium morale
        }
        else if (morale >= 0.3) {
            return Color(1.0f, 0.5f, 0.0f, 1.0f); // Orange - low morale
        }
        else {
            return Color(1.0f, 0.0f, 0.0f, 1.0f); // Red - very low morale
        }
    }

    Color UnitRenderer::GetStrengthColor(double strength_ratio) const {
        if (strength_ratio >= 0.8) {
            return Color(0.0f, 1.0f, 0.0f, 1.0f); // Green - high strength
        }
        else if (strength_ratio >= 0.5) {
            return Color(1.0f, 1.0f, 0.0f, 1.0f); // Yellow - medium strength
        }
        else if (strength_ratio >= 0.25) {
            return Color(1.0f, 0.5f, 0.0f, 1.0f); // Orange - low strength
        }
        else {
            return Color(1.0f, 0.0f, 0.0f, 1.0f); // Red - very low strength
        }
    }

    bool UnitRenderer::IsInfantry(military::UnitType type) const {
        using military::UnitType;
        return (type == UnitType::LEVIES || type == UnitType::SPEARMEN ||
                type == UnitType::CROSSBOWMEN || type == UnitType::LONGBOWMEN ||
                type == UnitType::MEN_AT_ARMS || type == UnitType::PIKEMEN ||
                type == UnitType::ARQUEBUSIERS || type == UnitType::MUSKETEERS);
    }

    bool UnitRenderer::IsCavalry(military::UnitType type) const {
        using military::UnitType;
        return (type == UnitType::LIGHT_CAVALRY || type == UnitType::HEAVY_CAVALRY ||
                type == UnitType::MOUNTED_ARCHERS || type == UnitType::DRAGOONS);
    }

    bool UnitRenderer::IsSiege(military::UnitType type) const {
        using military::UnitType;
        return (type == UnitType::CATAPULTS || type == UnitType::TREBUCHETS ||
                type == UnitType::CANNONS || type == UnitType::SIEGE_TOWERS);
    }

    bool UnitRenderer::IsNaval(military::UnitType type) const {
        using military::UnitType;
        return (type == UnitType::GALLEYS || type == UnitType::COGS ||
                type == UnitType::CARRACKS || type == UnitType::GALLEONS ||
                type == UnitType::SHIPS_OF_THE_LINE);
    }

} // namespace game::map
