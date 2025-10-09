// Created: September 25, 2025, 11:30 AM
// Location: include/game/realm/RealmManager.h

#ifndef REALM_MANAGER_H
#define REALM_MANAGER_H

#include "game/realm/RealmComponents.h"
#include "core/ECS/ComponentAccessManager.h"
#include "core/Threading/ThreadSafeMessageBus.h"
#include <memory>
#include <unordered_map>
#include <mutex>

namespace game::realm {

// ============================================================================
// Realm Events
// ============================================================================

namespace events {

struct RealmCreated {
    types::EntityID realmId;
    std::string realmName;
    GovernmentType government;
};

struct SuccessionTriggered {
    types::EntityID realmId;
    types::EntityID previousRuler;
    types::EntityID newRuler;
    SuccessionLaw law;
};

struct WarDeclared {
    types::EntityID aggressor;
    types::EntityID defender;
    CasusBelli justification;
};

struct RealmAnnexed {
    types::EntityID conqueror;
    types::EntityID conquered;
};

struct DiplomaticStatusChanged {
    types::EntityID realm1;
    types::EntityID realm2;
    DiplomaticStatus oldStatus;
    DiplomaticStatus newStatus;
};

struct VassalageChanged {
    types::EntityID vassal;
    types::EntityID liege;
    bool isNowVassal;
};

} // namespace events

// ============================================================================
// Realm Manager - Central realm management system
// ============================================================================

class RealmManager {
private:
    // ECS access
    std::shared_ptr<core::ecs::ComponentAccessManager> m_componentAccess;
    std::shared_ptr<core::threading::ThreadSafeMessageBus> m_messageBus;
    
    // Realm registry
    std::unordered_map<types::EntityID, types::EntityID> m_realmEntities; // RealmID -> EntityID
    std::unordered_map<std::string, types::EntityID> m_realmsByName;
    mutable std::mutex m_registryMutex;
    
    // Dynasty tracking
    std::unordered_map<types::EntityID, types::EntityID> m_dynastyEntities;
    std::unordered_map<std::string, types::EntityID> m_dynastiesByName;
    
    // ID generation
    std::atomic<uint32_t> m_nextRealmId{1};
    std::atomic<uint32_t> m_nextDynastyId{1};
    
    // Statistics
    struct RealmStats {
        uint32_t totalRealms = 0;
        uint32_t activeWars = 0;
        uint32_t totalAlliances = 0;
        uint32_t vassalRelationships = 0;
    } m_stats;
    mutable std::mutex m_statsMutex;
    
public:
    RealmManager(
        std::shared_ptr<core::ecs::ComponentAccessManager> componentAccess,
        std::shared_ptr<core::threading::ThreadSafeMessageBus> messageBus
    );
    ~RealmManager();
    
    // System lifecycle
    void Initialize();
    void Update(float deltaTime);
    void Shutdown();
    
    // Realm creation and management
    types::EntityID CreateRealm(
        const std::string& name,
        GovernmentType government,
        types::EntityID capitalProvince,
        types::EntityID ruler = types::EntityID{0}
    );
    
    bool DestroyRealm(types::EntityID realmId);
    bool MergeRealms(types::EntityID absorber, types::EntityID absorbed);
    
    // Dynasty management
    types::EntityID CreateDynasty(
        const std::string& dynastyName,
        types::EntityID founder
    );
    
    // Territory management
    bool AddProvinceToRealm(types::EntityID realmId, types::EntityID provinceId);
    bool RemoveProvinceFromRealm(types::EntityID realmId, types::EntityID provinceId);
    bool TransferProvince(types::EntityID from, types::EntityID to, types::EntityID provinceId);
    
    // Ruler management
    bool SetRuler(types::EntityID realmId, types::EntityID characterId);
    bool TriggerSuccession(types::EntityID realmId);
    types::EntityID DetermineHeir(types::EntityID realmId) const;
    
    // Diplomatic relations
    bool SetDiplomaticStatus(
        types::EntityID realm1,
        types::EntityID realm2,
        DiplomaticStatus status
    );
    
    bool DeclareWar(
        types::EntityID aggressor,
        types::EntityID defender,
        CasusBelli justification
    );
    
    bool MakePeace(
        types::EntityID realm1,
        types::EntityID realm2,
        float warscore
    );
    
    bool FormAlliance(types::EntityID realm1, types::EntityID realm2);
    bool BreakAlliance(types::EntityID realm1, types::EntityID realm2);
    
    // Vassalage
    bool MakeVassal(types::EntityID liege, types::EntityID vassal);
    bool ReleaseVassal(types::EntityID liege, types::EntityID vassal);
    bool IsVassal(types::EntityID realmId) const;
    types::EntityID GetLiege(types::EntityID vassalId) const;
    std::vector<types::EntityID> GetVassals(types::EntityID liegeId) const;
    
    // Council management
    bool AppointCouncilor(
        types::EntityID realmId,
        CouncilPosition position,
        types::EntityID characterId
    );
    
    bool DismissCouncilor(
        types::EntityID realmId,
        CouncilPosition position
    );
    
    // Law changes
    bool ChangeLaw(types::EntityID realmId, const std::string& lawType, float value);
    bool ChangeSuccessionLaw(types::EntityID realmId, SuccessionLaw newLaw);
    bool ChangeCrownAuthority(types::EntityID realmId, CrownAuthority newLevel);
    
    // Queries
    RealmComponent* GetRealm(types::EntityID realmId);
    const RealmComponent* GetRealm(types::EntityID realmId) const;
    RealmComponent* GetRealmByName(const std::string& name);
    
    DynastyComponent* GetDynasty(types::EntityID dynastyId);
    RulerComponent* GetRuler(types::EntityID characterId);
    DiplomaticRelationsComponent* GetDiplomacy(types::EntityID realmId);
    CouncilComponent* GetCouncil(types::EntityID realmId);
    LawsComponent* GetLaws(types::EntityID realmId);
    
    // Utility queries
    std::vector<types::EntityID> GetAllRealms() const;
    std::vector<types::EntityID> GetRealmsAtWar() const;
    std::vector<types::EntityID> GetIndependentRealms() const;
    float CalculateRealmStrength(types::EntityID realmId) const;
    bool AreAtWar(types::EntityID realm1, types::EntityID realm2) const;
    bool AreAllied(types::EntityID realm1, types::EntityID realm2) const;
    
    // Statistics
    RealmStats GetStatistics() const;
    void UpdateStatistics();
    
private:
    // Internal helpers
    types::EntityID GetEntityForRealm(types::EntityID realmId) const;
    void RegisterRealm(types::EntityID realmId, types::EntityID entityId);
    void UnregisterRealm(types::EntityID realmId);
    
    // Succession helpers
    std::vector<types::EntityID> CalculateSuccessionCandidates(
        const RealmComponent& realm,
        SuccessionLaw law
    ) const;
    
    void ApplySuccessionEffects(
        types::EntityID realmId,
        types::EntityID newRuler
    );
    
    // War helpers
    void UpdateWarscore(types::EntityID aggressor, types::EntityID defender, float change);
    void ApplyWarConsequences(types::EntityID winner, types::EntityID loser, float warscore);
    
    // Diplomatic helpers
    void UpdateOpinion(types::EntityID realm1, types::EntityID realm2, float change);
    void PropagateAllianceEffects(types::EntityID realm1, types::EntityID realm2);
    
    // Event publishing
    void PublishRealmEvent(const events::RealmCreated& event);
    void PublishRealmEvent(const events::SuccessionTriggered& event);
    void PublishRealmEvent(const events::WarDeclared& event);
    void PublishRealmEvent(const events::DiplomaticStatusChanged& event);
};

// ============================================================================
// Realm Factory - Helper for creating realms with proper setup
// ============================================================================

class RealmFactory {
public:
    static types::EntityID CreateFeudalKingdom(
        RealmManager& manager,
        const std::string& name,
        types::EntityID capital,
        types::EntityID ruler
    );
    
    static types::EntityID CreateMerchantRepublic(
        RealmManager& manager,
        const std::string& name,
        types::EntityID capital
    );
    
    static types::EntityID CreateTheocracy(
        RealmManager& manager,
        const std::string& name,
        types::EntityID capital,
        types::EntityID religiousLeader
    );
    
    static types::EntityID CreateTribalChiefdom(
        RealmManager& manager,
        const std::string& name,
        types::EntityID capital,
        types::EntityID chief
    );
    
    static types::EntityID CreateEmpire(
        RealmManager& manager,
        const std::string& name,
        types::EntityID capital,
        types::EntityID emperor,
        const std::vector<types::EntityID>& vassalKingdoms
    );
};

} // namespace game::realm

#endif // REALM_MANAGER_H