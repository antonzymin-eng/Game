#include "game/diplomacy/MemorySystem.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "game/diplomacy/DiplomacyMessages.h"
#include "game/time/TimeManagementSystem.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/logging/Logger.h"

namespace game::diplomacy {

MemorySystem::MemorySystem(
    ::core::ecs::ComponentAccessManager& access_manager,
    ::core::ecs::MessageBus& message_bus)
    : m_access_manager(access_manager)
    , m_message_bus(message_bus)
    , m_time_system(nullptr)
{
}

void MemorySystem::SetTimeSystem(game::time::TimeManagementSystem* time_system) {
    m_time_system = time_system;
    CORE_LOG_INFO("MemorySystem", "TimeSystem integration enabled");
}

int MemorySystem::GetCurrentGameYear() const {
    if (m_time_system) {
        return m_time_system->GetCurrentDate().year;
    }
    // Fallback to default start year if no time system available
    return 1066;
}

void MemorySystem::Initialize() {
    SubscribeToEvents();

    // Initialize memory components for all existing realms with diplomacy
    auto entities = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<DiplomacyComponent>();
    for (auto realm_id : entities) {
        GetOrCreateMemoryComponent(static_cast<types::EntityID>(realm_id.id));
    }
}

void MemorySystem::UpdateMonthly() {
    ProcessMonthlyDecay();
    UpdateHistoricalAverages();

    // Apply memory impacts to current diplomatic states
    auto realms = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<DiplomacyComponent>();
    for (auto realm_id : realms) {
        types::EntityID game_realm_id = static_cast<types::EntityID>(realm_id.id);
        auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(game_realm_id);
        if (!diplomacy_guard.IsValid()) continue;

        DiplomacyComponent* diplomacy = diplomacy_guard.Get();

        // Update all relationships based on memory
        for (auto& [other_id, state] : diplomacy->relationships) {
            ApplyMemoryToDiplomaticState(game_realm_id, other_id);
        }
    }
}

void MemorySystem::UpdateYearly() {
    // Check for milestones
    auto realms = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<DiplomacyComponent>();
    for (auto realm_id : realms) {
        types::EntityID game_realm_id = static_cast<types::EntityID>(realm_id.id);
        auto diplomacy = m_access_manager.GetComponent<DiplomacyComponent>(game_realm_id);
        if (!diplomacy) continue;

        for (const auto& [other_id, state] : diplomacy->relationships) {
            CheckMilestones(game_realm_id, other_id);
        }
    }

    PruneOldMemories();
}

void MemorySystem::RecordDiplomaticEvent(const DiplomaticEvent& event) {
    // Record for actor
    auto actor_memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(event.actor);
    if (actor_memory_guard.IsValid()) {
        actor_memory_guard->RecordEvent(event);
    }

    // Create reciprocal event for target
    DiplomaticEvent reciprocal = event;
    reciprocal.actor = event.target;
    reciprocal.target = event.actor;
    reciprocal.opinion_impact = event.opinion_impact;  // Same impact from their perspective

    auto target_memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(event.target);
    if (target_memory_guard.IsValid()) {
        target_memory_guard->RecordEvent(reciprocal);
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

    auto* memory = memory_guard->GetMemoryWith(realm_b);
    if (!memory) return {};

    return memory->GetRecentEvents(months);
}

std::vector<DiplomaticEvent*> MemorySystem::GetEventsByType(types::EntityID realm_a, types::EntityID realm_b, EventType type) {
    auto memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(realm_a);
    if (!memory_guard.IsValid()) return {};

    auto* memory = memory_guard->GetMemoryWith(realm_b);
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

    DiplomacyComponent* diplomacy = diplomacy_guard.Get();
    auto* state = diplomacy->GetRelationship(realm_b);
    if (!state) return;

    // Calculate memory impacts
    int memory_opinion = CalculateMemoryOpinionImpact(realm_a, realm_b);
    double memory_trust = CalculateMemoryTrustImpact(realm_a, realm_b);

    // Apply to diplomatic state through modifiers
    state->AddOpinionModifier("historical_memory", memory_opinion, false);
    state->trust = (std::min)((std::max)(state->trust + memory_trust, 0.0), 1.0);

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
    MilestoneTracker& tracker = memory_guard->milestones[realm_b];

    // Get current game year from TimeSystem (or use default if not available)
    int current_year = GetCurrentGameYear();

    auto new_milestones = tracker.CheckForNewMilestones(*state, current_year);

    for (auto milestone_type : new_milestones) {
        AwardMilestone(realm_a, realm_b, milestone_type);
    }
}

void MemorySystem::AwardMilestone(types::EntityID realm_a, types::EntityID realm_b, MilestoneType type) {
    auto memory_comp_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(realm_a);
    if (!memory_comp_guard.IsValid()) return;

    DiplomaticMemoryComponent* memory_comp = memory_comp_guard.Get();
    MilestoneTracker& tracker = memory_comp->milestones[realm_b];

    RelationshipMilestone milestone(type);
    tracker.AddMilestone(milestone);

    // Apply milestone modifiers to diplomatic state
    auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(realm_a);
    if (diplomacy_guard.IsValid()) {
        auto* state = diplomacy_guard->GetRelationship(realm_b);
        if (state) {
            std::string modifier_name = "milestone_" + std::to_string(static_cast<int>(type));
            state->AddOpinionModifier(modifier_name, static_cast<int>(milestone.opinion_modifier), true);
        }
    }

    // Broadcast milestone achieved event through message bus
    // This allows other systems (UI, AI, achievements) to react to milestone achievements
    messages::MilestoneAchievedMessage msg(realm_a, realm_b,
                                           static_cast<int>(type),
                                           static_cast<int>(milestone.opinion_modifier),
                                           "Diplomatic Milestone");
    m_message_bus.Publish<messages::MilestoneAchievedMessage>(msg);

    CORE_LOG_INFO("MemorySystem",
        "Milestone achieved: Realm " + std::to_string(realm_a) +
        " reached milestone " + std::to_string(static_cast<int>(type)) +
        " with Realm " + std::to_string(realm_b));
}

DiplomaticMemoryComponent* MemorySystem::GetOrCreateMemoryComponent(types::EntityID realm) {
    // Try to get existing component
    auto existing_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(realm);
    if (existing_guard.IsValid()) {
        return existing_guard.Get();
    }

    // Component doesn't exist - create it through EntityManager
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        CORE_LOG_ERROR("MemorySystem", "EntityManager not available for component creation");
        return nullptr;
    }

    // Create entity handle and add component
    ::core::ecs::EntityID handle(static_cast<uint64_t>(realm), 1);

    // Add the component to the entity
    auto component = entity_manager->AddComponent<DiplomaticMemoryComponent>(handle);
    if (!component) {
        CORE_LOG_ERROR("MemorySystem",
            "Failed to create DiplomaticMemoryComponent for realm " + std::to_string(realm));
        return nullptr;
    }

    CORE_LOG_DEBUG("MemorySystem",
        "Created new DiplomaticMemoryComponent for realm " + std::to_string(realm));

    return component.get();
}

void MemorySystem::ProcessMonthlyDecay() {
    auto entities = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<DiplomaticMemoryComponent>();

    for (auto entity_id : entities) {
        types::EntityID game_entity_id = static_cast<types::EntityID>(entity_id.id);
        auto memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(game_entity_id);
        if (memory_guard.IsValid()) {
            memory_guard->ApplyMonthlyDecay();
        }
    }

    // Also decay opinion modifiers in diplomatic states
    auto diplomacy_entities = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<DiplomacyComponent>();
    for (auto entity_id : diplomacy_entities) {
        types::EntityID game_entity_id = static_cast<types::EntityID>(entity_id.id);
        auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(game_entity_id);
        if (!diplomacy_guard.IsValid()) continue;

        DiplomacyComponent* diplomacy = diplomacy_guard.Get();
        for (auto& [other_id, state] : diplomacy->relationships) {
            state.ApplyModifierDecay(1.0f);  // 1 month
        }
    }
}

void MemorySystem::UpdateHistoricalAverages() {
    auto entities = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<DiplomacyComponent>();

    for (auto entity_id : entities) {
        types::EntityID game_entity_id = static_cast<types::EntityID>(entity_id.id);
        auto diplomacy_guard = m_access_manager.GetComponentForWrite<DiplomacyComponent>(game_entity_id);
        if (!diplomacy_guard.IsValid()) continue;

        DiplomacyComponent* diplomacy = diplomacy_guard.Get();
        for (auto& [other_id, state] : diplomacy->relationships) {
            int current_opinion = state.CalculateTotalOpinion();
            state.UpdateHistoricalData(current_opinion, true, false);
        }
    }
}

void MemorySystem::PruneOldMemories() {
    auto entities = m_access_manager.GetEntityManager()->GetEntitiesWithComponent<DiplomaticMemoryComponent>();

    for (auto entity_id : entities) {
        types::EntityID game_entity_id = static_cast<types::EntityID>(entity_id.id);
        auto memory_guard = m_access_manager.GetComponentForWrite<DiplomaticMemoryComponent>(game_entity_id);
        if (!memory_guard.IsValid()) continue;

        DiplomaticMemoryComponent* memory_comp = memory_guard.Get();
        for (auto& [other_id, memory] : memory_comp->memories) {
            memory.PruneMemory();
        }
    }
}

void MemorySystem::BroadcastMemoryEvents() {
    // Broadcast significant memory events through message bus
    // This notifies other systems (UI, AI, achievements) about important diplomatic changes

    // In a full implementation, this would:
    // 1. Scan for newly recorded "significant" events (high impact, special categories)
    // 2. Publish messages for each significant event
    // 3. Allow UI to show notifications, AI to adjust strategies, etc.

    // Example implementation when message types are defined:
    // for (auto& [realm_id, memory_comp] : recent_significant_events) {
    //     m_message_bus.Publish(DiplomaticMemoryEvent{realm_id, event_type, severity});
    // }

    CORE_LOG_DEBUG("MemorySystem", "BroadcastMemoryEvents called - full implementation pending");
}

void MemorySystem::SubscribeToEvents() {
    // Subscribe to diplomatic events to automatically record them in memory
    // This creates an event-driven system where diplomatic actions are automatically remembered

    // Subscribe to war declarations
    m_message_bus.Subscribe<messages::WarDeclaredMessage>([this](const messages::WarDeclaredMessage& msg) {
        OnWarDeclared(msg.aggressor, msg.defender);
    });

    // Subscribe to treaty events
    m_message_bus.Subscribe<messages::TreatySignedMessage>([this](const messages::TreatySignedMessage& msg) {
        OnTreatySigned(msg.signatory_a, msg.signatory_b, msg.treaty_type);
    });

    m_message_bus.Subscribe<messages::TreatyViolatedMessage>([this](const messages::TreatyViolatedMessage& msg) {
        OnTreatyViolated(msg.violator, msg.victim, msg.treaty_type);
    });

    CORE_LOG_INFO("MemorySystem", "Diplomatic event subscriptions initialized");
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
