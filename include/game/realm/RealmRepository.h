// ============================================================================
// Mechanica Imperii - Realm Repository Header
// Component Access Layer for Realm System
// ============================================================================

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "game/realm/RealmComponents.h"
#include <memory>
#include <vector>

namespace game::realm {

    /**
     * @brief Repository pattern for accessing realm-related ECS components
     * Eliminates 40+ instances of boilerplate component access code
     */
    class RealmRepository {
    public:
        explicit RealmRepository(
            std::shared_ptr<::core::ecs::ComponentAccessManager> componentAccess);

        // Realm component access
        std::shared_ptr<RealmComponent> GetRealm(types::EntityID realmId);
        std::shared_ptr<const RealmComponent> GetRealm(types::EntityID realmId) const;
        std::shared_ptr<RealmComponent> GetRealmByName(const std::string& name);

        // Diplomatic relations access
        std::shared_ptr<DiplomaticRelationsComponent> GetDiplomacy(types::EntityID realmId);
        std::shared_ptr<const DiplomaticRelationsComponent> GetDiplomacy(types::EntityID realmId) const;

        // Council access
        std::shared_ptr<CouncilComponent> GetCouncil(types::EntityID realmId);
        std::shared_ptr<const CouncilComponent> GetCouncil(types::EntityID realmId) const;

        // Laws access
        std::shared_ptr<LawsComponent> GetLaws(types::EntityID realmId);
        std::shared_ptr<const LawsComponent> GetLaws(types::EntityID realmId) const;

        // Dynasty access
        std::shared_ptr<DynastyComponent> GetDynasty(types::EntityID dynastyId);

        // Ruler access
        std::shared_ptr<RulerComponent> GetRuler(types::EntityID characterId);

        // Entity manager access
        ::core::ecs::EntityManager* GetEntityManager();

        // Realm-Entity ID mapping
        types::EntityID GetEntityForRealm(types::EntityID realmId) const;
        void RegisterRealm(types::EntityID realmId, types::EntityID entityId, const std::string& name);
        void UnregisterRealm(types::EntityID realmId);

        // Registry access
        std::unordered_map<types::EntityID, types::EntityID>& GetRealmEntities() { return m_realmEntities; }
        std::unordered_map<std::string, types::EntityID>& GetRealmsByName() { return m_realmsByName; }
        std::unordered_map<types::EntityID, types::EntityID>& GetDynastyEntities() { return m_dynastyEntities; }

    private:
        std::shared_ptr<::core::ecs::ComponentAccessManager> m_componentAccess;

        // Realm registry (shared with RealmManager)
        std::unordered_map<types::EntityID, types::EntityID> m_realmEntities;
        std::unordered_map<std::string, types::EntityID> m_realmsByName;
        std::unordered_map<types::EntityID, types::EntityID> m_dynastyEntities;
    };

} // namespace game::realm
