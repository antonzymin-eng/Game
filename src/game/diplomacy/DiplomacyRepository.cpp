// ============================================================================
// DiplomacyRepository.cpp - Implementation
// Created: 2025-10-28
// ============================================================================

#include "game/diplomacy/DiplomacyRepository.h"
#include "core/logging/Logger.h"

namespace game::diplomacy {

DiplomacyRepository::DiplomacyRepository(::core::ecs::ComponentAccessManager& access_manager)
    : m_access_manager(access_manager) {}

::core::ecs::EntityID DiplomacyRepository::ToEntityHandle(types::EntityID realm_id) const {
    return ::core::ecs::EntityID(static_cast<uint64_t>(realm_id), 1);
}

std::shared_ptr<DiplomacyComponent> DiplomacyRepository::Get(types::EntityID realm_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        ::core::logging::LogError("DiplomacyRepository", "EntityManager not available");
        return nullptr;
    }

    auto handle = ToEntityHandle(realm_id);
    return entity_manager->GetComponent<DiplomacyComponent>(handle);
}

std::shared_ptr<const DiplomacyComponent> DiplomacyRepository::Get(types::EntityID realm_id) const {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return nullptr;
    }

    auto handle = ToEntityHandle(realm_id);
    return entity_manager->GetComponent<DiplomacyComponent>(handle);
}

std::shared_ptr<DiplomacyComponent> DiplomacyRepository::GetOrCreate(types::EntityID realm_id) {
    auto component = Get(realm_id);
    if (component) {
        return component;
    }

    return Create(realm_id);
}

bool DiplomacyRepository::Exists(types::EntityID realm_id) const {
    auto component = Get(realm_id);
    return component != nullptr;
}

std::vector<types::EntityID> DiplomacyRepository::GetAllRealms() const {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return {};
    }

    // Get all entities with DiplomacyComponent
    auto entities = entity_manager->GetEntitiesWithComponent<DiplomacyComponent>();

    std::vector<types::EntityID> realm_ids;
    realm_ids.reserve(entities.size());

    for (const auto& entity : entities) {
        realm_ids.push_back(static_cast<types::EntityID>(entity.id));
    }

    return realm_ids;
}

std::shared_ptr<DiplomacyComponent> DiplomacyRepository::Create(
    types::EntityID realm_id,
    DiplomaticPersonality personality) {

    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        ::core::logging::LogError("DiplomacyRepository",
            "Cannot create component - EntityManager not available");
        return nullptr;
    }

    auto handle = ToEntityHandle(realm_id);

    // Check if already exists
    auto existing = entity_manager->GetComponent<DiplomacyComponent>(handle);
    if (existing) {
        ::core::logging::LogWarning("DiplomacyRepository",
            "Component already exists for realm " + std::to_string(realm_id));
        return existing;
    }

    // Create new component
    auto component = entity_manager->AddComponent<DiplomacyComponent>(handle);
    if (component) {
        component->personality = personality;
        component->prestige = 0.0;
        component->diplomatic_reputation = 1.0;
        component->war_weariness = 0.0;

        ::core::logging::LogInfo("DiplomacyRepository",
            "Created DiplomacyComponent for realm " + std::to_string(realm_id));
    }

    return component;
}

bool DiplomacyRepository::Remove(types::EntityID realm_id) {
    auto* entity_manager = m_access_manager.GetEntityManager();
    if (!entity_manager) {
        return false;
    }

    auto handle = ToEntityHandle(realm_id);
    entity_manager->RemoveComponent<DiplomacyComponent>(handle);

    ::core::logging::LogInfo("DiplomacyRepository",
        "Removed DiplomacyComponent for realm " + std::to_string(realm_id));

    return true;
}

DiplomacyRepository::RealmPair DiplomacyRepository::GetPair(
    types::EntityID realm1,
    types::EntityID realm2) {

    return RealmPair{
        Get(realm1),
        Get(realm2)
    };
}

} // namespace game::diplomacy
