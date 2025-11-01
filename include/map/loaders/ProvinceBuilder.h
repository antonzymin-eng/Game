// ============================================================================
// ProvinceBuilder.h - Province Entity Builder
// Mechanica Imperii - Build province entities in ECS
// ============================================================================

#pragma once

#include "map/MapData.h"
#include "core/ECS/EntityManager.h"
#include "core/ECS/ComponentAccessManager.h"

namespace game::map::loaders {

    // ============================================================================
    // Province Builder
    // ============================================================================

    class ProvinceBuilder {
    public:
        ProvinceBuilder(::core::ecs::ComponentAccessManager& access_manager);
        ~ProvinceBuilder();

        // Build province entities from data
        ::core::ecs::EntityID BuildProvince(const ProvinceData& data);
        std::vector<::core::ecs::EntityID> BuildProvinces(const std::vector<ProvinceData>& provinces);

        // Link provinces (neighbors, etc.)
        void LinkProvinces(const std::vector<ProvinceData>& provinces);

        // Error handling
        std::string GetLastError() const { return m_last_error; }

    private:
        ::core::ecs::ComponentAccessManager& m_access_manager;
        std::string m_last_error;
    };

} // namespace game::map::loaders
