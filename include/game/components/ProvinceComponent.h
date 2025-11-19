#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <memory>

// ============================================================================
// DEPRECATION NOTICE
// ============================================================================
// This component is being phased out in favor of game::province::ProvinceDataComponent
//
// For new code, use: game::province::ProvinceDataComponent (in game/province/ProvinceSystem.h)
// This minimal component should only be used by legacy AI systems.
//
// Migration plan: AI systems should transition to using the full ProvinceDataComponent
// or create a lightweight view/wrapper around it.
// ============================================================================

namespace AI {

// Province component for AI systems
// Represents a province entity in the game world
// NOTE: This is a LEGACY component - see deprecation notice above
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
