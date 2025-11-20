// ============================================================================
// ProvinceAdapter.h - Adapter for Legacy Province Code Migration
// Created: November 19, 2025
// Purpose: Bridge between deprecated game::Province and new ECS system
// ============================================================================

#pragma once

#include "game/province/ProvinceSystem.h"
#include "game/gameplay/Province.h"
#include "core/types/game_types.h"
#include <memory>

namespace game::province {

/**
 * Adapter class to convert between legacy Province struct and ECS components
 * Use this during migration - DO NOT use for new code!
 */
class ProvinceAdapter {
public:
    /**
     * Create a legacy Province struct from ECS components
     * @param province_system The province system
     * @param province_id The province entity ID
     * @return Legacy Province struct (for UI compatibility)
     */
    static game::Province CreateLegacyProvince(
        const ProvinceSystem& province_system,
        types::EntityID province_id
    ) {
        game::Province legacy;

        auto* data = const_cast<ProvinceSystem&>(province_system).GetProvinceData(province_id);
        if (!data) {
            return legacy;  // Return empty
        }

        // Map ECS components to legacy struct
        legacy.id = static_cast<int>(province_id);
        legacy.name = data->name;
        legacy.owner_nation_id = static_cast<int>(data->owner_nation);

        // Convert coordinates
        legacy.x_coordinate = data->x_coordinate;
        legacy.y_coordinate = data->y_coordinate;

        // Map administrative data
        legacy.admin_efficiency = 0.5f;  // Default
        legacy.autonomy = static_cast<float>(data->autonomy);
        legacy.stability = static_cast<float>(data->stability);
        legacy.war_exhaustion = static_cast<float>(data->war_exhaustion);

        // Map development
        legacy.development_level = data->development_level;

        // Population data (defaults - would need PopulationComponent for real values)
        legacy.base_population = 1000;
        legacy.current_population = 1000;
        legacy.base_tax_capacity = 100;

        return legacy;
    }

    /**
     * Update ECS components from legacy Province struct
     * @param province_system The province system
     * @param legacy The legacy province data
     * @return true if successful
     */
    static bool UpdateFromLegacy(
        ProvinceSystem& province_system,
        const game::Province& legacy
    ) {
        types::EntityID province_id = static_cast<types::EntityID>(legacy.id);

        auto* data = province_system.GetProvinceData(province_id);
        if (!data) {
            return false;
        }

        // Update from legacy data
        data->name = legacy.name;
        data->owner_nation = static_cast<types::EntityID>(legacy.owner_nation_id);
        data->x_coordinate = legacy.x_coordinate;
        data->y_coordinate = legacy.y_coordinate;
        data->autonomy = static_cast<double>(legacy.autonomy);
        data->stability = static_cast<double>(legacy.stability);
        data->war_exhaustion = static_cast<double>(legacy.war_exhaustion);
        data->development_level = legacy.development_level;

        // Mark dirty for update
        province_system.MarkDirty(province_id);

        return true;
    }
};

} // namespace game::province
