#include "game/diplomacy/MemorySystem.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "core/ECS/ComponentAccessManager.h"

namespace game::diplomacy {

MemorySystem::MemorySystem(
    ::core::ecs::ComponentAccessManager& access_manager,
    ::core::ecs::MessageBus& message_bus)
    : m_access_manager(access_manager)
    , m_message_bus(message_bus)
{
}

void MemorySystem::Initialize() {
    SubscribeToEvents();

    // Initialize memory components for all existing realms with diplomacy
    auto entities = m_access_manager.GetEntitiesWithComponent<DiplomacyComponent>();
    for (auto realm_id : entities) {
        GetOrCreateMemoryComponent(realm_id);
    }
}

void MemorySystem::UpdateMonthly() {
    ProcessMonthlyDecay();
    UpdateHistoricalAverages();

    // Apply memory impacts to current diplomatic states
    auto realms = m_access_manager.GetEntitiesWithComponent<DiplomacyComponent>();
    for (auto realm_id : realms) {
        auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(realm_id);
        if (!diplomacy_guard.IsValid()) continue;

        auto& diplomacy = diplomacy_guard.Get();

        // Update all relationships based on memory
        for (auto& [other_id, state] : diplomacy.relationships) {
            ApplyMemoryToDiplomaticState(realm_id, other_id);
        }
    }
}

void MemorySystem::UpdateYearly() {
    // Check for milestones
    auto realms = m_access_manager.GetEntitiesWithComponent<DiplomacyComponent>();
    for (auto realm_id : realms) {
        auto diplomacy = m_access_manager.GetComponent<DiplomacyComponent>(realm_id);
        if (!diplomacy) continue;

        for (const auto& [other_id, state] : diplomacy->relationships) {
            CheckMilestones(realm_id, other_id);
        }
    }

    PruneOldMemories();
}

void MemorySystem::RecordDiplomaticEvent(const DiplomaticEvent& event) {
    // Record for actor
    auto actor_memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(event.actor);
    if (actor_memory_guard.IsValid()) {
        actor_memory_guard.Get().RecordEvent(event);
    }

    // Create reciprocal event for target
    DiplomaticEvent reciprocal = event;
    reciprocal.actor = event.target;
    reciprocal.target = event.actor;
    reciprocal.opinion_impact = event.opinion_impact;  // Same impact from their perspective

    auto target_memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(event.target);
    if (target_memory_guard.IsValid()) {
        target_memory_guard.Get().RecordEvent(reciprocal);
    }

    // Apply immediate effects to diplomatic state
    ApplyMemoryToDiplomaticState(event.actor, event.target);

    // Broadcast event
    BroadcastMemoryEvents();
}

void MemorySystem::RecordEventBatch(const std::vector<DiplomaticEvent>& events) {
    for (const auto& event : events) {
        RecordDiplomaticEvent(event);
    }
}

DiplomaticEvent MemorySystem::CreateEvent(
    EventType type,
    types::EntityID actor,
    types::EntityID target,
    const std::string& description)
{
    DiplomaticEvent event(type, actor, target);
    event.description = description;
    return event;
}

const EventMemory* MemorySystem::GetMemory(types::EntityID realm_a, types::EntityID realm_b) const {
    auto memory_comp = m_access_manager.GetComponent<DiplomaticMemoryComponent>(realm_a);
    if (memory_comp) {
        return memory_comp->GetMemoryWith(realm_b);
    }
    return nullptr;
}

std::vector<DiplomaticEvent*> MemorySystem::GetRecentEvents(types::EntityID realm_a, types::EntityID realm_b, int months) {
    auto memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(realm_a);
    if (!memory_guard.IsValid()) return {};

    auto* memory = memory_guard.Get().GetMemoryWith(realm_b);
    if (!memory) return {};

    return memory->GetRecentEvents(months);
}

std::vector<DiplomaticEvent*> MemorySystem::GetEventsByType(types::EntityID realm_a, types::EntityID realm_b, EventType type) {
    auto memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(realm_a);
    if (!memory_guard.IsValid()) return {};

    auto* memory = memory_guard.Get().GetMemoryWith(realm_b);
    if (!memory) return {};

    return memory->GetEventsByType(type);
}

bool MemorySystem::HasGrudge(types::EntityID realm_a, types::EntityID realm_b) const {
    auto memory = GetMemory(realm_a, realm_b);
    return memory ? memory->HasGrudge() : false;
}

bool MemorySystem::HasFriendship(types::EntityID realm_a, types::EntityID realm_b) const {
    auto memory = GetMemory(realm_a, realm_b);
    return memory ? memory->HasDeepFriendship() : false;
}

bool MemorySystem::AreHistoricalRivals(types::EntityID realm_a, types::EntityID realm_b) const {
    auto memory = GetMemory(realm_a, realm_b);
    return memory ? memory->IsHistoricalRival() : false;
}

bool MemorySystem::AreHistoricalAllies(types::EntityID realm_a, types::EntityID realm_b) const {
    auto memory = GetMemory(realm_a, realm_b);
    return memory ? memory->IsHistoricalAlly() : false;
}

int MemorySystem::CalculateMemoryOpinionImpact(types::EntityID realm_a, types::EntityID realm_b) const {
    auto memory = GetMemory(realm_a, realm_b);
    if (!memory) return 0;

    auto now = std::chrono::system_clock::now();
    return memory->CalculateTotalOpinionImpact(now);
}

double MemorySystem::CalculateMemoryTrustImpact(types::EntityID realm_a, types::EntityID realm_b) const {
    auto memory = GetMemory(realm_a, realm_b);
    if (!memory) return 0.0;

    auto now = std::chrono::system_clock::now();
    return memory->CalculateTotalTrustImpact(now);
}

void MemorySystem::ApplyMemoryToDiplomaticState(types::EntityID realm_a, types::EntityID realm_b) {
    auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(realm_a);
    if (!diplomacy_guard.IsValid()) return;

    auto& diplomacy = diplomacy_guard.Get();
    auto* state = diplomacy.GetRelationship(realm_b);
    if (!state) return;

    // Calculate memory impacts
    int memory_opinion = CalculateMemoryOpinionImpact(realm_a, realm_b);
    double memory_trust = CalculateMemoryTrustImpact(realm_a, realm_b);

    // Apply to diplomatic state through modifiers
    state->AddOpinionModifier("historical_memory", memory_opinion, false);
    state->trust = std::clamp(state->trust + memory_trust, 0.0, 1.0);

    // Check for special memory patterns
    if (HasGrudge(realm_a, realm_b)) {
        state->AddOpinionModifier("grudge", -20, true);
    }

    if (HasFriendship(realm_a, realm_b)) {
        state->AddOpinionModifier("deep_friendship", 15, true);
    }

    // Update historical tracking
    int total_opinion = state->CalculateTotalOpinion();
    state->UpdateHistoricalData(total_opinion, true, false);
}

void MemorySystem::CheckMilestones(types::EntityID realm_a, types::EntityID realm_b) {
    auto memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(realm_a);
    if (!memory_guard.IsValid()) return;

    auto diplomacy = m_access_manager.GetComponent<DiplomacyComponent>(realm_a);
    if (!diplomacy) return;

    auto* state = diplomacy->GetRelationship(realm_b);
    if (!state) return;

    // Get milestone tracker
    auto& tracker = memory_guard.Get().milestones[realm_b];

    // TODO: Get current game year from time system
    int current_year = 1066;  // Placeholder

    auto new_milestones = tracker.CheckForNewMilestones(*state, current_year);

    for (auto milestone_type : new_milestones) {
        AwardMilestone(realm_a, realm_b, milestone_type);
    }
}

void MemorySystem::AwardMilestone(types::EntityID realm_a, types::EntityID realm_b, MilestoneType type) {
    auto memory_comp_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(realm_a);
    if (!memory_comp_guard.IsValid()) return;

    auto& memory_comp = memory_comp_guard.Get();
    auto& tracker = memory_comp.milestones[realm_b];

    RelationshipMilestone milestone(type);
    tracker.AddMilestone(milestone);

    // Apply milestone modifiers to diplomatic state
    auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(realm_a);
    if (diplomacy_guard.IsValid()) {
        auto* state = diplomacy_guard.Get().GetRelationship(realm_b);
        if (state) {
            std::string modifier_name = "milestone_" + std::to_string(static_cast<int>(type));
            state->AddOpinionModifier(modifier_name, static_cast<int>(milestone.opinion_modifier), true);
        }
    }

    // TODO: Broadcast milestone achieved event
}

DiplomaticMemoryComponent* MemorySystem::GetOrCreateMemoryComponent(types::EntityID realm) {
    // Try to get existing component
    auto existing_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(realm);
    if (existing_guard.IsValid()) {
        return &existing_guard.Get();
    }

    // Component doesn't exist - would need to be created through proper ECS API
    // This is a placeholder - actual implementation depends on your ECS system
    // In a real implementation, you'd use something like:
    // m_access_manager.AddComponent<DiplomaticMemoryComponent>(realm);
    return nullptr;
}

void MemorySystem::ProcessMonthlyDecay() {
    auto entities = m_access_manager.GetEntitiesWithComponent<DiplomaticMemoryComponent>();

    for (auto entity_id : entities) {
        auto memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(entity_id);
        if (memory_guard.IsValid()) {
            memory_guard.Get().ApplyMonthlyDecay();
        }
    }

    // Also decay opinion modifiers in diplomatic states
    auto diplomacy_entities = m_access_manager.GetEntitiesWithComponent<DiplomacyComponent>();
    for (auto entity_id : diplomacy_entities) {
        auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(entity_id);
        if (!diplomacy_guard.IsValid()) continue;

        auto& diplomacy = diplomacy_guard.Get();
        for (auto& [other_id, state] : diplomacy.relationships) {
            state.ApplyModifierDecay(1.0f);  // 1 month
        }
    }
}

void MemorySystem::UpdateHistoricalAverages() {
    auto entities = m_access_manager.GetEntitiesWithComponent<DiplomacyComponent>();

    for (auto entity_id : entities) {
        auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(entity_id);
        if (!diplomacy_guard.IsValid()) continue;

        auto& diplomacy = diplomacy_guard.Get();
        for (auto& [other_id, state] : diplomacy.relationships) {
            int current_opinion = state.CalculateTotalOpinion();
            state.UpdateHistoricalData(current_opinion, true, false);
        }
    }
}

void MemorySystem::PruneOldMemories() {
    auto entities = m_access_manager.GetEntitiesWithComponent<DiplomaticMemoryComponent>();

    for (auto entity_id : entities) {
        auto memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(entity_id);
        if (!memory_guard.IsValid()) continue;

        auto& memory_comp = memory_guard.Get();
        for (auto& [other_id, memory] : memory_comp.memories) {
            memory.PruneMemory();
        }
    }
}

void MemorySystem::BroadcastMemoryEvents() {
    // TODO: Implement event broadcasting through message bus
    // This would notify other systems about significant memory events
}

void MemorySystem::SubscribeToEvents() {
    // TODO: Subscribe to diplomatic events to automatically record them
    // This depends on your MessageBus implementation
    // Example:
    // m_message_bus.Subscribe<WarDeclaredEvent>([this](const WarDeclaredEvent& evt) {
    //     OnWarDeclared(evt.aggressor, evt.target);
    // });
}

void MemorySystem::OnWarDeclared(types::EntityID aggressor, types::EntityID target) {
    auto event = CreateEvent(EventType::WAR_DECLARED, aggressor, target, "War declared");
    RecordDiplomaticEvent(event);
}

void MemorySystem::OnTreatySigned(types::EntityID realm_a, types::EntityID realm_b, TreatyType type) {
    EventType event_type = EventType::TREATY_SIGNED;
    if (type == TreatyType::ALLIANCE) {
        event_type = EventType::ALLIANCE_FORMED;
    } else if (type == TreatyType::TRADE_AGREEMENT) {
        event_type = EventType::TRADE_AGREEMENT_SIGNED;
    }

    auto event = CreateEvent(event_type, realm_a, realm_b, "Treaty signed");
    RecordDiplomaticEvent(event);
}

void MemorySystem::OnTreatyViolated(types::EntityID violator, types::EntityID victim, TreatyType type) {
    EventType event_type = EventType::TREATY_VIOLATED;
    if (type == TreatyType::ALLIANCE) {
        event_type = EventType::ALLIANCE_BROKEN;
    } else if (type == TreatyType::TRADE_AGREEMENT) {
        event_type = EventType::TRADE_AGREEMENT_BROKEN;
    }

    auto event = CreateEvent(event_type, violator, victim, "Treaty violated");
    RecordDiplomaticEvent(event);
}

} // namespace game::diplomacy
