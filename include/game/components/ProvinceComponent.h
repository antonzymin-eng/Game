#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <memory>

// ============================================================================
// ⚠️ DEPRECATED - DO NOT USE ⚠️
// ============================================================================
// This entire component is DEPRECATED and NO LONGER USED.
// It has been fully replaced by the modern ECS province system.
//
// Migration:
//   - AI systems MUST use game::province::ProvinceDataComponent
//   - Access provinces via game::province::ProvinceSystem
//   - Use spatial queries: FindProvincesInRadius(), etc.
//
// Modern ECS system (in game/province/ProvinceSystem.h):
//   - ProvinceSystem::FindProvincesInRadius()    (spatial queries)
//   - ProvinceSystem::GetProvinceData()          (full province data)
//   - ProvinceSpatialIndex for O(1) lookups      (1000+ provinces optimized)
//
// This component is NO LONGER CREATED by MapDataLoader.
// Any AI code still using this will NOT work correctly.
//
// File scheduled for removal in next cleanup phase.
// ============================================================================

#warning "AI::ProvinceComponent is DEPRECATED and NO LONGER USED - Use game::province::ProvinceSystem"

namespace AI {

// DEPRECATED: Use game::province::ProvinceDataComponent instead
[[deprecated("Use game::province::ProvinceDataComponent and ProvinceSystem")]]
class ProvinceComponent : public game::core::Component<ProvinceComponent> {
public:
    ProvinceComponent() : m_x(0.0f), m_y(0.0f), m_ownerNationId(0) {}
    ~ProvinceComponent() = default;
    
    // Position accessors
    float GetPositionX() const { return m_x; }
    float GetPositionY() const { return m_y; }
    void SetPosition(float x, float y) { m_x = x; m_y = y; }
    
    // Ownership
    uint32_t GetOwnerNationId() const { return m_ownerNationId; }
    void SetOwnerNationId(uint32_t nationId) { m_ownerNationId = nationId; }
    
    // Component interface
    std::unique_ptr<game::core::IComponent> Clone() const override {
        auto clone = std::make_unique<ProvinceComponent>();
        clone->m_x = m_x;
        clone->m_y = m_y;
        clone->m_ownerNationId = m_ownerNationId;
        return clone;
    }
    
private:
    float m_x;
    float m_y;
    uint32_t m_ownerNationId;
    
    // Additional province data can be added here in the future:
    // Population, resources, infrastructure, culture, religion, etc.
};

} // namespace AI
