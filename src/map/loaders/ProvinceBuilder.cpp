// ============================================================================
// ProvinceBuilder.cpp - Province Builder Implementation
// Mechanica Imperii
// ============================================================================

#include "map/loaders/ProvinceBuilder.h"
#include "map/ProvinceRenderComponent.h"

namespace game::map::loaders {

    ProvinceBuilder::ProvinceBuilder(::core::ecs::ComponentAccessManager& access_manager)
        : m_access_manager(access_manager)
    {
    }

    ProvinceBuilder::~ProvinceBuilder() {}

    ::core::ecs::EntityID ProvinceBuilder::BuildProvince(const ProvinceData& data) {
        // Stub: Create province entity with components
        // This would typically create an entity and add ProvinceRenderComponent
        // and other relevant components
        return ::core::ecs::EntityID{0, 0};
    }

    std::vector<::core::ecs::EntityID> ProvinceBuilder::BuildProvinces(const std::vector<ProvinceData>& provinces) {
        std::vector<::core::ecs::EntityID> entities;
        for (const auto& province : provinces) {
            entities.push_back(BuildProvince(province));
        }
        return entities;
    }

    void ProvinceBuilder::LinkProvinces(const std::vector<ProvinceData>& provinces) {
        // Stub: Link provinces based on adjacency
        // This would analyze province boundaries and create neighbor relationships
    }

} // namespace game::map::loaders
