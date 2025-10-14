// ============================================================================
// AdministrativeComponents.cpp - ECS Component Method Implementations
// Created: October 13, 2025 - Following Architecture Database Patterns
// Location: src/game/administration/AdministrativeComponents.cpp
// ============================================================================

#include "game/administration/AdministrativeComponents.h"
#include "core/logging/Logger.h"

namespace game::administration {

    // ============================================================================
    // GovernanceComponent Methods
    // ============================================================================

    std::string GovernanceComponent::GetComponentTypeName() const {
        return "GovernanceComponent";
    }

    // ============================================================================
    // BureaucracyComponent Methods
    // ============================================================================

    std::string BureaucracyComponent::GetComponentTypeName() const {
        return "BureaucracyComponent";
    }

    // ============================================================================
    // LawComponent Methods
    // ============================================================================

    std::string LawComponent::GetComponentTypeName() const {
        return "LawComponent";
    }

    // ============================================================================
    // AdministrativeEventsComponent Methods
    // ============================================================================

    std::string AdministrativeEventsComponent::GetComponentTypeName() const {
        return "AdministrativeEventsComponent";
    }

} // namespace game::administration