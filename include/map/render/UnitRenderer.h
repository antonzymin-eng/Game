// ============================================================================
// UnitRenderer.h - LOD 4 Military and Naval Unit Renderer
// Created: November 1, 2025
// Description: Renders military and naval units at tactical zoom
//              Handles unit sprites, formations, strength indicators, and
//              naval fleet visualization
// ============================================================================

#pragma once

#include "map/ProvinceRenderComponent.h"
#include "core/ECS/EntityManager.h"
#include "game/military/MilitaryComponents.h"
#include <memory>
#include <unordered_map>
#include <vector>

// Forward declarations
struct ImDrawList;

namespace game::map {

    // Use core::ecs::EntityID
    using ::core::ecs::EntityID;

    // ========================================================================
    // Camera2D - Camera structure (matching MapRenderer)
    // ========================================================================
    struct Camera2D;  // Forward declaration (defined in TacticalTerrainRenderer.h)

    // ========================================================================
    // Formation Types
    // ========================================================================
    enum class FormationType {
        LINE = 0,       // Line formation (wide front)
        COLUMN,         // Column formation (narrow front, deep)
        SQUARE,         // Square formation (defensive)
        WEDGE,          // Wedge formation (offensive)
        SCATTERED,      // Scattered/skirmish formation
        NAVAL_LINE,     // Naval line of battle
        NAVAL_CRESCENT, // Naval crescent formation
        COUNT
    };

    // ========================================================================
    // Unit Visual Data
    // ========================================================================
    struct UnitVisual {
        Vector2 world_position;
        military::UnitType type = military::UnitType::LEVIES;
        military::UnitClass unit_class = military::UnitClass::INFANTRY;

        uint32_t current_strength = 1000;
        uint32_t max_strength = 1000;

        double morale = 0.8;
        double experience = 0.0;

        bool is_engaged = false;
        bool is_routing = false;
        bool is_moving = false;

        // Visual representation
        Color unit_color;
        float rotation = 0.0f;
        float scale = 1.0f;
    };

    // ========================================================================
    // Formation Data
    // ========================================================================
    struct FormationData {
        FormationType type = FormationType::LINE;
        Vector2 center_position;
        float rotation = 0.0f;

        std::vector<UnitVisual> units;

        // Formation dimensions
        int rows = 1;
        int columns = 1;
        float unit_spacing = 8.0f;

        // Formation properties
        double formation_cohesion = 0.8;
        bool is_locked = false;
    };

    // ========================================================================
    // UnitRenderer - Renders military and naval units
    // ========================================================================
    class UnitRenderer {
    public:
        UnitRenderer(::core::ecs::EntityManager& entity_manager);
        ~UnitRenderer() = default;

        // Initialize renderer
        bool Initialize();

        // Render all visible units
        void RenderAllUnits(
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Render a specific army
        void RenderArmy(
            const military::ArmyComponent& army,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Render a specific formation
        void RenderFormation(
            const FormationData& formation,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Render a single unit
        void RenderUnit(
            const UnitVisual& unit,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Render naval units/fleets
        void RenderNavalUnits(
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Settings
        void SetShowUnits(bool show) { show_units_ = show; }
        void SetShowFormations(bool show) { show_formations_ = show; }
        void SetShowStrengthIndicators(bool show) { show_strength_indicators_ = show; }
        void SetShowUnitIcons(bool show) { show_unit_icons_ = show; }
        void SetMinZoomForUnits(float zoom) { min_zoom_for_units_ = zoom; }
        void SetUnitScale(float scale) { unit_scale_ = scale; }

        // Getters
        bool IsShowingUnits() const { return show_units_; }
        bool IsShowingFormations() const { return show_formations_; }
        bool IsShowingStrengthIndicators() const { return show_strength_indicators_; }

        // Statistics
        int GetRenderedUnitCount() const { return rendered_unit_count_; }
        int GetRenderedArmyCount() const { return rendered_army_count_; }

        // Formation helpers
        FormationData CreateFormation(
            const military::ArmyComponent& army,
            FormationType type = FormationType::LINE
        );

        void UpdateFormationPositions(
            FormationData& formation
        );

    private:
        // Core systems
        ::core::ecs::EntityManager& entity_manager_;

        // Rendering settings
        bool show_units_ = true;
        bool show_formations_ = true;
        bool show_strength_indicators_ = true;
        bool show_unit_icons_ = true;
        float min_zoom_for_units_ = 2.0f;  // Only show at tactical zoom
        float unit_scale_ = 1.0f;

        // Statistics
        mutable int rendered_unit_count_ = 0;
        mutable int rendered_army_count_ = 0;

        // Unit rendering methods
        void RenderInfantryUnit(
            const UnitVisual& unit,
            const Vector2& screen_pos,
            float size,
            ImDrawList* draw_list
        );

        void RenderCavalryUnit(
            const UnitVisual& unit,
            const Vector2& screen_pos,
            float size,
            ImDrawList* draw_list
        );

        void RenderSiegeUnit(
            const UnitVisual& unit,
            const Vector2& screen_pos,
            float size,
            ImDrawList* draw_list
        );

        void RenderNavalUnit(
            const UnitVisual& unit,
            const Vector2& screen_pos,
            float size,
            ImDrawList* draw_list
        );

        // Formation rendering
        void RenderFormationShape(
            const FormationData& formation,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        void RenderFormationUnits(
            const FormationData& formation,
            const Camera2D& camera,
            ImDrawList* draw_list
        );

        // Unit indicators
        void RenderStrengthIndicator(
            const UnitVisual& unit,
            const Vector2& screen_pos,
            float size,
            ImDrawList* draw_list
        );

        void RenderMoraleIndicator(
            const UnitVisual& unit,
            const Vector2& screen_pos,
            float size,
            ImDrawList* draw_list
        );

        void RenderUnitBadges(
            const UnitVisual& unit,
            const Vector2& screen_pos,
            float size,
            ImDrawList* draw_list
        );

        // Helper shapes
        void DrawUnitRect(
            const Vector2& screen_pos,
            float size,
            float rotation,
            const Color& color,
            ImDrawList* draw_list
        );

        void DrawUnitTriangle(
            const Vector2& screen_pos,
            float size,
            float rotation,
            const Color& color,
            ImDrawList* draw_list
        );

        void DrawShip(
            const Vector2& screen_pos,
            float size,
            float rotation,
            const Color& color,
            military::UnitType ship_type,
            ImDrawList* draw_list
        );

        void DrawArrow(
            const Vector2& from,
            const Vector2& to,
            const Color& color,
            float thickness,
            ImDrawList* draw_list
        );

        // Viewport culling
        bool IsUnitVisible(
            const Vector2& world_pos,
            const Camera2D& camera
        ) const;

        // Color helpers
        Color GetUnitColor(military::UnitType type) const;
        Color GetMoraleColor(double morale) const;
        Color GetStrengthColor(double strength_ratio) const;

        // Unit type helpers
        bool IsInfantry(military::UnitType type) const;
        bool IsCavalry(military::UnitType type) const;
        bool IsSiege(military::UnitType type) const;
        bool IsNaval(military::UnitType type) const;

        // Formation calculation
        Vector2 CalculateUnitPosition(
            const FormationData& formation,
            int unit_index
        ) const;

        void CalculateFormationGrid(
            int unit_count,
            FormationType type,
            int& out_rows,
            int& out_columns
        ) const;
    };

} // namespace game::map
