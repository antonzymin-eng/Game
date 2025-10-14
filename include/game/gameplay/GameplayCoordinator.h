// ============================================================================
// GameplayCoordinator.h - Minimal stub for TimeManagement system
// Created: October 14, 2025 - Resolving TimeManagement dependency
// Location: include/game/gameplay/GameplayCoordinator.h
// ============================================================================

#pragma once

#include <string>

namespace game::gameplay {

    /**
     * @brief Minimal GameplayCoordinator stub for TimeManagement integration
     * 
     * This is a lightweight stub that provides the interface expected by
     * TimeManagement system without the full gameplay coordination complexity.
     * Can be replaced with a full implementation later.
     */
    class GameplayCoordinator {
    public:
        GameplayCoordinator() = default;
        virtual ~GameplayCoordinator() = default;

        // Placeholder methods that TimeManagement might need
        virtual void Update(double delta_time) {}
        virtual bool ShouldDelegateEvent(const std::string& event_type) { return false; }
        virtual void HandleDelegatedEvent(const std::string& event_data) {}
        
        // Basic state
        virtual bool IsInitialized() const { return true; }
        virtual std::string GetSystemName() const { return "GameplayCoordinator"; }
    };

} // namespace game::gameplay