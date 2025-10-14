// ============================================================================
// EconomicComponents.cpp - ECS Component Method Implementations
// Created: October 13, 2025 - Following Architecture Database Patterns
// Location: src/game/economy/EconomicComponents.cpp
// ============================================================================

#include "game/economy/EconomicComponents.h"
#include "core/logging/Logger.h"

namespace game::economy {

    // ============================================================================
    // EconomicComponent Methods
    // ============================================================================

    std::string EconomicComponent::GetComponentTypeName() const {
        return "EconomicComponent";
    }

    // ============================================================================
    // TradeComponent Methods
    // ============================================================================

    std::string TradeComponent::GetComponentTypeName() const {
        return "TradeComponent";
    }

    // ============================================================================
    // EconomicEventsComponent Methods
    // ============================================================================

    std::string EconomicEventsComponent::GetComponentTypeName() const {
        return "EconomicEventsComponent";
    }

    // ============================================================================
    // MarketComponent Methods
    // ============================================================================

    std::string MarketComponent::GetComponentTypeName() const {
        return "MarketComponent";
    }

    // ============================================================================
    // TreasuryComponent Methods
    // ============================================================================

    std::string TreasuryComponent::GetComponentTypeName() const {
        return "TreasuryComponent";
    }

} // namespace game::economy