// ============================================================================
// LineOfSight.h - Line of Sight Calculation System
// Created: November 18, 2025
// Description: LOS calculations with terrain-based visibility modifiers
//              Handles elevation advantages, forest concealment, etc.
// ============================================================================

#pragma once

#include "utils/PlatformMacros.h"
#include "map/TerrainData.h"
#include "map/FogOfWar.h"
#include "core/ECS/EntityManager.h"
#include <vector>

namespace game::map {

    // ========================================================================
    // LOSModifier - Terrain and environmental modifiers for visibility
    // ========================================================================
    struct LOSModifier {
        float base_range = 50.0f;           // Base vision range in world units
        float elevation_bonus = 0.0f;       // Elevation advantage multiplier
        float terrain_penalty = 0.0f;       // Terrain obstruction penalty
        float weather_modifier = 1.0f;      // Weather visibility modifier
        float forest_concealment = 0.0f;    // Forest reduces detection range

        float GetEffectiveRange() const {
            float range = base_range;
            range += elevation_bonus;
            range -= terrain_penalty;
            range *= weather_modifier;
            range -= forest_concealment;
            return std::max(10.0f, range); // Minimum 10 units
        }
    };

    // ========================================================================
    // TerrainVisibilityModifiers - Static terrain-based LOS modifiers
    // ========================================================================
    struct TerrainVisibilityModifiers {
        // Terrain concealment values (reduces detection range)
        static constexpr float FOREST_CONCEALMENT = 20.0f;      // Dense forest blocks vision
        static constexpr float MARSH_CONCEALMENT = 10.0f;       // Marsh reduces visibility
        static constexpr float HILLS_CONCEALMENT = 5.0f;        // Hills partially block

        // Elevation advantages (per 100m elevation difference)
        static constexpr float ELEVATION_BONUS_PER_100M = 15.0f;

        // Terrain penalties for viewer
        static constexpr float FOREST_VIEWER_PENALTY = 15.0f;   // Hard to see from forest
        static constexpr float MARSH_VIEWER_PENALTY = 10.0f;    // Marsh limits vision

        // Maximum vision ranges by terrain
        static constexpr float PLAINS_MAX_RANGE = 150.0f;
        static constexpr float HILLS_MAX_RANGE = 200.0f;
        static constexpr float MOUNTAIN_MAX_RANGE = 300.0f;
        static constexpr float FOREST_MAX_RANGE = 80.0f;
        static constexpr float DESERT_MAX_RANGE = 200.0f;
    };

    // ========================================================================
    // LineOfSightCalculator - Main LOS calculation engine
    // ========================================================================
    class LineOfSightCalculator {
    public:
        LineOfSightCalculator();
        ~LineOfSightCalculator() = default;

        // Check if target position is visible from source position
        bool HasLineOfSight(
            const Vector2& from,
            const Vector2& to,
            const TerrainGrid* terrain,
            LOSModifier* out_modifier = nullptr
        ) const;

        // Calculate effective vision range from a position
        float CalculateVisionRange(
            const Vector2& position,
            const TerrainGrid* terrain,
            float base_range,
            float weather_modifier = 1.0f
        ) const;

        // Get all visible cells from a position (flood-fill or ray-cast)
        void GetVisibleCells(
            const Vector2& position,
            float vision_range,
            const TerrainGrid* terrain,
            std::vector<std::pair<uint32_t, uint32_t>>& out_visible_cells
        ) const;

        // Calculate LOS modifier for a specific viewer-target pair
        LOSModifier CalculateLOSModifier(
            const Vector2& viewer_pos,
            const Vector2& target_pos,
            const TerrainGrid* terrain,
            float base_range,
            float weather_modifier = 1.0f
        ) const;

        // Settings
        void SetUseElevationBonus(bool use) { use_elevation_bonus_ = use; }
        void SetUseTerrainConcealment(bool use) { use_terrain_concealment_ = use; }
        void SetMaximumRange(float range) { maximum_range_ = range; }

        bool IsUsingElevationBonus() const { return use_elevation_bonus_; }
        bool IsUsingTerrainConcealment() const { return use_terrain_concealment_; }
        float GetMaximumRange() const { return maximum_range_; }

    private:
        // Settings
        bool use_elevation_bonus_ = true;
        bool use_terrain_concealment_ = true;
        float maximum_range_ = 500.0f;  // Absolute maximum vision range

        // Helper methods
        bool RayCast(
            const Vector2& from,
            const Vector2& to,
            const TerrainGrid* terrain,
            float& out_obstruction_factor
        ) const;

        float CalculateElevationBonus(
            float viewer_elevation,
            float target_elevation
        ) const;

        float CalculateTerrainConcealment(
            TerrainCellType terrain_type
        ) const;

        float CalculateTerrainPenalty(
            TerrainCellType viewer_terrain
        ) const;

        // Bresenham line algorithm for ray-casting
        void GetLinePoints(
            const Vector2& from,
            const Vector2& to,
            std::vector<std::pair<int, int>>& out_points
        ) const;

        // Check if terrain blocks line of sight
        bool IsTerrainBlocking(
            const TerrainCell* cell,
            float viewer_elevation,
            float distance_from_viewer
        ) const;
    };

    // ========================================================================
    // UnitVisionRange - Vision ranges for different unit types
    // ========================================================================
    struct UnitVisionRange {
        // Infantry vision ranges
        static constexpr float INFANTRY_BASE = 50.0f;
        static constexpr float ARCHER_BASE = 60.0f;
        static constexpr float PIKEMEN_BASE = 45.0f;

        // Cavalry vision ranges (faster, scouts ahead)
        static constexpr float LIGHT_CAVALRY_BASE = 100.0f;
        static constexpr float HEAVY_CAVALRY_BASE = 75.0f;
        static constexpr float MOUNTED_ARCHERS_BASE = 110.0f;

        // Special units
        static constexpr float SCOUT_BASE = 150.0f;
        static constexpr float SIEGE_BASE = 40.0f;  // Limited vision

        // Fortifications
        static constexpr float WATCHTOWER_BASE = 200.0f;
        static constexpr float FORTRESS_BASE = 250.0f;

        // Naval units (at sea)
        static constexpr float NAVAL_BASE = 120.0f;
    };

    // ========================================================================
    // Detection and Stealth System
    // ========================================================================
    struct DetectionModifier {
        float base_detection_range = 50.0f;
        float stealth_penalty = 0.0f;       // Reduces enemy detection range
        float size_modifier = 0.0f;         // Large armies easier to spot
        float movement_penalty = 0.0f;      // Moving units easier to detect

        float GetDetectionRange() const {
            float range = base_detection_range;
            range -= stealth_penalty;
            range += size_modifier;
            range += movement_penalty;
            return std::max(10.0f, range);
        }
    };

    // ========================================================================
    // Inline implementations
    // ========================================================================

    inline float LineOfSightCalculator::CalculateElevationBonus(
        float viewer_elevation,
        float target_elevation
    ) const {
        if (!use_elevation_bonus_) return 0.0f;

        float elevation_diff = viewer_elevation - target_elevation;
        if (elevation_diff <= 0) return 0.0f; // No bonus if target is higher

        // Bonus per 100m of elevation advantage
        return (elevation_diff / 100.0f) * TerrainVisibilityModifiers::ELEVATION_BONUS_PER_100M;
    }

    inline float LineOfSightCalculator::CalculateTerrainConcealment(
        TerrainCellType terrain_type
    ) const {
        if (!use_terrain_concealment_) return 0.0f;

        switch (terrain_type) {
            case TerrainCellType::FOREST:
                return TerrainVisibilityModifiers::FOREST_CONCEALMENT;
            case TerrainCellType::MARSH:
                return TerrainVisibilityModifiers::MARSH_CONCEALMENT;
            case TerrainCellType::HILLS:
                return TerrainVisibilityModifiers::HILLS_CONCEALMENT;
            default:
                return 0.0f;
        }
    }

    inline float LineOfSightCalculator::CalculateTerrainPenalty(
        TerrainCellType viewer_terrain
    ) const {
        if (!use_terrain_concealment_) return 0.0f;

        switch (viewer_terrain) {
            case TerrainCellType::FOREST:
                return TerrainVisibilityModifiers::FOREST_VIEWER_PENALTY;
            case TerrainCellType::MARSH:
                return TerrainVisibilityModifiers::MARSH_VIEWER_PENALTY;
            default:
                return 0.0f;
        }
    }

} // namespace game::map
