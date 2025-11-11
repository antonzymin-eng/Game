#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
#include "game/diplomacy/DiplomaticMemory.h"
#include "game/diplomacy/DiplomacyComponents.h"

namespace game::diplomacy {

class MemorySystem {
public:
    explicit MemorySystem(
        ::core::ecs::ComponentAccessManager& access_manager,
        ::core::ecs::MessageBus& message_bus
    );

    ~MemorySystem() = default;

    // Initialize memory system
    void Initialize();

    // Update (called monthly)
    void UpdateMonthly();

    // Update (called yearly)
    void UpdateYearly();

    // Record events
    void RecordDiplomaticEvent(const DiplomaticEvent& event);
    void RecordEventBatch(const std::vector<DiplomaticEvent>& events);

    // Event creation helpers
    DiplomaticEvent CreateEvent(
        EventType type,
        types::EntityID actor,
        types::EntityID target,
        const std::string& description = ""
    );

    // Memory queries
    const EventMemory* GetMemory(types::EntityID realm_a, types::EntityID realm_b) const;
    std::vector<DiplomaticEvent*> GetRecentEvents(types::EntityID realm_a, types::EntityID realm_b, int months = 12);
    std::vector<DiplomaticEvent*> GetEventsByType(types::EntityID realm_a, types::EntityID realm_b, EventType type);

    // Relationship pattern detection
    bool HasGrudge(types::EntityID realm_a, types::EntityID realm_b) const;
    bool HasFriendship(types::EntityID realm_a, types::EntityID realm_b) const;
    bool AreHistoricalRivals(types::EntityID realm_a, types::EntityID realm_b) const;
    bool AreHistoricalAllies(types::EntityID realm_a, types::EntityID realm_b) const;

    // Memory impact calculations
    int CalculateMemoryOpinionImpact(types::EntityID realm_a, types::EntityID realm_b) const;
    double CalculateMemoryTrustImpact(types::EntityID realm_a, types::EntityID realm_b) const;

    // Milestone management
    void CheckMilestones(types::EntityID realm_a, types::EntityID realm_b);
    void AwardMilestone(types::EntityID realm_a, types::EntityID realm_b, MilestoneType type);

    // Integration with diplomacy system
    void ApplyMemoryToDiplomaticState(types::EntityID realm_a, types::EntityID realm_b);

private:
    ::core::ecs::ComponentAccessManager& m_access_manager;
    ::core::ecs::MessageBus& m_message_bus;

    // Helper methods
    DiplomaticMemoryComponent* GetOrCreateMemoryComponent(types::EntityID realm);
    void ProcessMonthlyDecay();
    void UpdateHistoricalAverages();
    void PruneOldMemories();
    void BroadcastMemoryEvents();

    // Event subscription
    void SubscribeToEvents();
    void OnWarDeclared(types::EntityID aggressor, types::EntityID target);
    void OnTreatySigned(types::EntityID realm_a, types::EntityID realm_b, TreatyType type);
    void OnTreatyViolated(types::EntityID violator, types::EntityID victim, TreatyType type);
};

} // namespace game::diplomacy
