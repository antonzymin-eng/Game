// Created: September 25, 2025, 11:00 AM
// Location: include/game/ai/InformationPropagationSystem_Fixed.h
// FIXES: Added thread safety members and helper methods

#ifndef INFORMATION_PROPAGATION_SYSTEM_H
#define INFORMATION_PROPAGATION_SYSTEM_H

// Include the original header
#include "game/ai/InformationPropagationSystem.h"

namespace AI {

// Extended class with fixes
class InformationPropagationSystem : public InformationPropagationSystem {
private:
    // FIX 2: Additional thread safety
    mutable std::mutex m_propagationQueueMutex;
    
    // FIX 3: Memory management
    void CleanupActivePropagations();
    size_t m_maxActiveProvinces = 1000;  // Limit active provinces tracked
    std::chrono::steady_clock::time_point m_lastCleanup;
    
    // FIX 1: ECS integration helpers  
    uint32_t GetCapitalProvince(uint32_t nationId) const;
    
    // Event structure placeholders (would be defined in actual event headers)
    struct MilitaryEvent {
        uint32_t GetEventId() const { return 0; }
        uint32_t GetSourceProvinceId() const { return 1; }
        float GetSeverity() const { return 0.5f; }
    };
    
    struct DiplomaticEvent {
        uint32_t GetEventId() const { return 0; }
        uint32_t GetNationId() const { return 1; }
    };
    
    struct EconomicEvent {
        uint32_t GetProvinceId() const { return 1; }
        float GetSeverity() const { return 0.5f; }
        float GetImpact() const { return 100.0f; }
    };

public:
    // Constructor matching parent
    InformationPropagationSystem(
        std::shared_ptr<ECS::ComponentAccessManager> componentAccess,
        std::shared_ptr<MessageBus> messageBus,
        std::shared_ptr<TimeManagementSystem> timeSystem)
        : InformationPropagationSystem(componentAccess, messageBus, timeSystem) {
        m_lastCleanup = std::chrono::steady_clock::now();
    }
    
    // Override Update to include cleanup
    void Update(float deltaTime) override;
    
    // Override methods that need thread safety
    void ProcessPropagationQueue() override;
    void StartPropagation(const InformationPacket& packet) override;
    void PropagateToNeighbors(const PropagationNode& node) override;
    
    // Override for proper ECS integration
    void RebuildProvinceCache() override;
    void OnGameEvent(const std::string& eventType, const void* eventData) override;
    void UpdateStatistics(const PropagationNode& node, bool delivered) override;
};

// ProvinceComponent placeholder (would be in actual component header)
class ProvinceComponent : public core::ecs::Component<ProvinceComponent> {
public:
    float GetPositionX() const { return m_x; }
    float GetPositionY() const { return m_y; }
    uint32_t GetOwnerNationId() const { return m_ownerNationId; }
    
    void SetPosition(float x, float y) {
        m_x = x;
        m_y = y;
    }
    
    void SetOwnerNationId(uint32_t nationId) {
        m_ownerNationId = nationId;
    }
    
private:
    float m_x = 0.0f;
    float m_y = 0.0f;
    uint32_t m_ownerNationId = 0;
    
    // Other province data would go here:
    // Population, resources, infrastructure, etc.
};

} // namespace AI

#endif // INFORMATION_PROPAGATION_SYSTEM_H
