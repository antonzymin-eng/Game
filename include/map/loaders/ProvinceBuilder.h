// ============================================================================
// ProvinceBuilder.h - Province Entity Builder
// Mechanica Imperii - Build province entities in ECS
// ============================================================================

#pragma once

#include "map/MapData.h"
#include "core/ECS/EntityManager.h"

namespace game::map::loaders {

    // ============================================================================
    // Province Builder - Creates Province Entities and Computes Adjacency
    // ============================================================================
    //
    // ProvinceBuilder is responsible for:
    // 1. Creating province entities from ProvinceData structures (BuildProvince)
    // 2. Batch creating multiple provinces (BuildProvinces)
    // 3. Computing province adjacency/neighbors using geometry (LinkProvinces)
    //
    // Usage Example:
    //   ProvinceBuilder builder;
    //
    //   // Compute adjacency first
    //   builder.LinkProvinces(province_data_list, 1.0);
    //
    //   // Then build entities
    //   auto entities = builder.BuildProvinces(province_data_list, entity_manager);
    //
    // Note: BuildProvince creates entities with default grey colors.
    //       MapDataLoader can override colors and add LOD boundaries afterward.
    //
    class ProvinceBuilder {
    public:
        ProvinceBuilder() = default;
        ~ProvinceBuilder();

        // Build province entities from data
        // Creates an entity with ProvinceRenderComponent populated from ProvinceData
        // Note: Uses default colors and doesn't generate LOD boundaries
        ::core::ecs::EntityID BuildProvince(const ProvinceData& data, ::core::ecs::EntityManager& entity_manager);
        std::vector<::core::ecs::EntityID> BuildProvinces(
            const std::vector<ProvinceData>& provinces,
            ::core::ecs::EntityManager& entity_manager);

        // Link provinces (neighbors, etc.) - modifies province neighbor data
        // tolerance: distance threshold for considering provinces as neighbors (map coordinates)
        //            If tolerance <= 0.0, adaptive tolerance is calculated based on average province size
        void LinkProvinces(std::vector<ProvinceData>& provinces, double tolerance = 1.0);

        // Error handling
        std::string GetLastError() const { return m_last_error; }

    private:
        std::string m_last_error;
    };

} // namespace game::map::loaders
