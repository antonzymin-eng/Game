// ============================================================================
// Mechanica Imperii - Realm Repository Implementation
// ============================================================================

#include "game/realm/RealmRepository.h"

namespace game::realm {

    RealmRepository::RealmRepository(
        std::shared_ptr<::core::ecs::ComponentAccessManager> componentAccess)
        : m_componentAccess(componentAccess) {
    }

    std::shared_ptr<RealmComponent> RealmRepository::GetRealm(types::EntityID realmId) {
        types::EntityID entityId = GetEntityForRealm(realmId);
        if (entityId == 0 || !m_componentAccess) return nullptr;

        auto* entityManager = m_componentAccess->GetEntityManager();
        return entityManager->GetComponent<RealmComponent>(::core::ecs::EntityID(entityId));
    }

    std::shared_ptr<const RealmComponent> RealmRepository::GetRealm(types::EntityID realmId) const {
        types::EntityID entityId = GetEntityForRealm(realmId);
        if (entityId == 0 || !m_componentAccess) return nullptr;

        auto* entityManager = m_componentAccess->GetEntityManager();
        return entityManager->GetComponent<RealmComponent>(::core::ecs::EntityID(entityId));
    }

    std::shared_ptr<RealmComponent> RealmRepository::GetRealmByName(const std::string& name) {
        auto it = m_realmsByName.find(name);
        if (it != m_realmsByName.end()) {
            return GetRealm(it->second);
        }
        return nullptr;
    }

    std::shared_ptr<DiplomaticRelationsComponent> RealmRepository::GetDiplomacy(types::EntityID realmId) {
        types::EntityID entityId = GetEntityForRealm(realmId);
        if (entityId == 0 || !m_componentAccess) return nullptr;

        auto* entityManager = m_componentAccess->GetEntityManager();
        return entityManager->GetComponent<DiplomaticRelationsComponent>(::core::ecs::EntityID(entityId));
    }

    std::shared_ptr<const DiplomaticRelationsComponent> RealmRepository::GetDiplomacy(types::EntityID realmId) const {
        types::EntityID entityId = GetEntityForRealm(realmId);
        if (entityId == 0 || !m_componentAccess) return nullptr;

        auto* entityManager = m_componentAccess->GetEntityManager();
        return entityManager->GetComponent<DiplomaticRelationsComponent>(::core::ecs::EntityID(entityId));
    }

    std::shared_ptr<CouncilComponent> RealmRepository::GetCouncil(types::EntityID realmId) {
        types::EntityID entityId = GetEntityForRealm(realmId);
        if (entityId == 0 || !m_componentAccess) return nullptr;

        auto* entityManager = m_componentAccess->GetEntityManager();
        return entityManager->GetComponent<CouncilComponent>(::core::ecs::EntityID(entityId));
    }

    std::shared_ptr<const CouncilComponent> RealmRepository::GetCouncil(types::EntityID realmId) const {
        types::EntityID entityId = GetEntityForRealm(realmId);
        if (entityId == 0 || !m_componentAccess) return nullptr;

        auto* entityManager = m_componentAccess->GetEntityManager();
        return entityManager->GetComponent<CouncilComponent>(::core::ecs::EntityID(entityId));
    }

    std::shared_ptr<LawsComponent> RealmRepository::GetLaws(types::EntityID realmId) {
        types::EntityID entityId = GetEntityForRealm(realmId);
        if (entityId == 0 || !m_componentAccess) return nullptr;

        auto* entityManager = m_componentAccess->GetEntityManager();
        return entityManager->GetComponent<LawsComponent>(::core::ecs::EntityID(entityId));
    }

    std::shared_ptr<const LawsComponent> RealmRepository::GetLaws(types::EntityID realmId) const {
        types::EntityID entityId = GetEntityForRealm(realmId);
        if (entityId == 0 || !m_componentAccess) return nullptr;

        auto* entityManager = m_componentAccess->GetEntityManager();
        return entityManager->GetComponent<LawsComponent>(::core::ecs::EntityID(entityId));
    }

    std::shared_ptr<DynastyComponent> RealmRepository::GetDynasty(types::EntityID dynastyId) {
        if (!m_componentAccess) return nullptr;

        auto it = m_dynastyEntities.find(dynastyId);
        if (it != m_dynastyEntities.end()) {
            auto* entityManager = m_componentAccess->GetEntityManager();
            return entityManager->GetComponent<DynastyComponent>(::core::ecs::EntityID(it->second));
        }
        return nullptr;
    }

    std::shared_ptr<RulerComponent> RealmRepository::GetRuler(types::EntityID characterId) {
        // Simplified - would need character entity lookup
        return nullptr;
    }

    ::core::ecs::EntityManager* RealmRepository::GetEntityManager() {
        return m_componentAccess ? m_componentAccess->GetEntityManager() : nullptr;
    }

    types::EntityID RealmRepository::GetEntityForRealm(types::EntityID realmId) const {
        auto it = m_realmEntities.find(realmId);
        return (it != m_realmEntities.end()) ? it->second : types::EntityID{0};
    }

    void RealmRepository::RegisterRealm(types::EntityID realmId, types::EntityID entityId, const std::string& name) {
        m_realmEntities[realmId] = entityId;
        if (!name.empty()) {
            m_realmsByName[name] = realmId;
        }
    }

    void RealmRepository::UnregisterRealm(types::EntityID realmId) {
        auto it = m_realmEntities.find(realmId);
        if (it != m_realmEntities.end()) {
            auto* entityManager = GetEntityManager();
            if (entityManager) {
                auto realm = entityManager->GetComponent<RealmComponent>(::core::ecs::EntityID(it->second));
                if (realm && !realm->realmName.empty()) {
                    m_realmsByName.erase(realm->realmName);
                }
            }
            m_realmEntities.erase(it);
        }
    }

} // namespace game::realm
