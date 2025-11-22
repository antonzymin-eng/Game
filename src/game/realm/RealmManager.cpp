// Created: October 7, 2025, 2:50 PM
// Location: src/game/realm/RealmManager.cpp
// Part 1 of 3 - Lines 1-500

#include "game/realm/RealmManager.h"
#include "game/realm/RealmComponents.h"
#include <algorithm>
#include <iostream>
#include "core/logging/Logger.h"

namespace game {
namespace realm {

// ============================================================================
// RealmManager Implementation
// ============================================================================

RealmManager::RealmManager(
    std::shared_ptr<::core::ecs::ComponentAccessManager> componentAccess,
    std::shared_ptr<::core::threading::ThreadSafeMessageBus> messageBus)  // FIXED: CRITICAL-001
    : m_componentAccess(componentAccess)
    , m_messageBus(messageBus) {
}

RealmManager::~RealmManager() {
    Shutdown();
}

void RealmManager::Initialize() {
    CORE_STREAM_INFO("RealmManager") << "Initializing realm management system...";

    // Register component types if needed
    if (m_componentAccess) {
        // Components should already be registered by ECS system
        CORE_STREAM_INFO("RealmManager") << "Component access ready";
    }

    // Reset statistics - FIXED: HIGH-005 - Stats are now atomic, no mutex needed
    m_stats.totalRealms = 0;
    m_stats.activeWars = 0;
    m_stats.totalAlliances = 0;
    m_stats.vassalRelationships = 0;

    CORE_STREAM_INFO("RealmManager") << "Initialization complete";
}

void RealmManager::Update(float deltaTime) {
    // Periodic updates for realm management
    // Most operations are event-driven, so this is lightweight

    // Update statistics periodically
    static float timeSinceStatsUpdate = 0.0f;
    timeSinceStatsUpdate += deltaTime;

    if (timeSinceStatsUpdate > RealmConstants::STATS_UPDATE_INTERVAL_SEC) {
        UpdateStatistics();
        timeSinceStatsUpdate = 0.0f;
    }
}

void RealmManager::Shutdown() {
    CORE_STREAM_INFO("RealmManager") << "Shutting down realm management system...";

    // Clear realm registries - FIXED: CRITICAL-002
    {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        m_realmEntities.clear();
        m_realmsByName.clear();
    }

    // Clear dynasty registries - FIXED: CRITICAL-002 - Using separate mutex
    {
        std::lock_guard<std::mutex> lock(m_dynastyMutex);
        m_dynastyEntities.clear();
        m_dynastiesByName.clear();
    }

    CORE_STREAM_INFO("RealmManager") << "Shutdown complete";
}

// ============================================================================
// Realm Creation and Management
// ============================================================================

game::types::EntityID RealmManager::CreateRealm(
    const std::string& name,
    game::realm::GovernmentType government,
    game::types::EntityID capitalProvince,
    game::types::EntityID ruler) {

    // IMPROVED: Comprehensive input validation
    if (name.empty()) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot create realm - name is empty";
        return types::EntityID{0};
    }

    if (name.length() > 100) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot create realm - name too long (max 100 chars)";
        return types::EntityID{0};
    }

    if (government >= GovernmentType::COUNT) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot create realm - invalid government type";
        return types::EntityID{0};
    }

    if (capitalProvince == 0) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot create realm - invalid capital province";
        return types::EntityID{0};
    }

    // Check for duplicate realm name
    {
        std::lock_guard<std::mutex> lock(m_registryMutex);
        if (m_realmsByName.find(name) != m_realmsByName.end()) {
            CORE_STREAM_ERROR("RealmManager") << "Cannot create realm - name already exists: " << name;
            return types::EntityID{0};
        }
    }

    if (!m_componentAccess) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot create realm - no component access";
        return types::EntityID{0};
    }

    // Create entity
    auto* entityManager = m_componentAccess->GetEntityManager();
    if (!entityManager) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot create realm - no entity manager";
        return types::EntityID{0};
    }
    
    auto entityHandle = entityManager->CreateEntity();
    types::EntityID entityId = entityHandle.id;
    
    // Generate realm ID
    types::EntityID realmId = m_nextRealmId.fetch_add(1);
    
    // Create and attach realm component
    RealmComponent realm(realmId);
    realm.realmName = name;
    realm.governmentType = government;
    realm.capitalProvince = capitalProvince;
    realm.currentRuler = ruler;
    realm.rank = RealmRank::COUNTY; // Default, can be changed
    // FIXED: HIGH-001 - Use GameDate instead of system_clock
    realm.foundedDate = game::time::GameDate(1066, 10, 14);  // TODO: Get current game date from TimeSystem
    realm.lastSuccession = realm.foundedDate;
    
    // Set initial stats based on government type
    switch (government) {
        case GovernmentType::FEUDAL_MONARCHY:
            realm.centralAuthority = 0.4f;
            realm.legitimacy = 0.8f;
            break;
        case GovernmentType::ABSOLUTE_MONARCHY:
            realm.centralAuthority = 0.9f;
            realm.legitimacy = 0.7f;
            break;
        case GovernmentType::REPUBLIC:
            realm.centralAuthority = 0.6f;
            realm.legitimacy = 0.9f;
            break;
        case GovernmentType::THEOCRACY:
            realm.centralAuthority = 0.7f;
            realm.legitimacy = 1.0f;
            break;
        case GovernmentType::TRIBAL:
            realm.centralAuthority = 0.3f;
            realm.legitimacy = 0.6f;
            break;
        default:
            realm.centralAuthority = 0.5f;
            realm.legitimacy = 0.8f;
            break;
    }
    
    entityManager->AddComponent<RealmComponent>(entityHandle, std::move(realm));
    
    // Create diplomatic relations component
    DiplomaticRelationsComponent diplomacy(realmId);
    entityManager->AddComponent<DiplomaticRelationsComponent>(entityHandle, std::move(diplomacy));
    
    // Create council component
    CouncilComponent council(realmId);
    entityManager->AddComponent<CouncilComponent>(entityHandle, std::move(council));
    
    // Create laws component
    LawsComponent laws(realmId);
    entityManager->AddComponent<LawsComponent>(entityHandle, std::move(laws));
    
    // Register realm
    RegisterRealm(realmId, entityId);
    
    // Create ruler component if ruler specified
    if (ruler != 0) {
        SetRuler(realmId, ruler);
    }
    
    // Publish creation event
    events::RealmCreated event;
    event.realmId = realmId;
    event.realmName = name;
    event.government = government;
    PublishRealmEvent(event);
    
    CORE_STREAM_INFO("RealmManager") << "Created realm: " << name 
              << " (ID: " << realmId << ")";
    
    return realmId;
}

bool RealmManager::DestroyRealm(types::EntityID realmId) {
    auto realm = GetRealm(realmId);
    if (!realm) {
        return false;
    }
    
    // Release all vassals
    for (auto vassalId : realm->vassalRealms) {
        ReleaseVassal(realmId, vassalId);
    }
    
    // Remove from liege's vassals if applicable
    if (realm->liegeRealm != 0) {
        ReleaseVassal(realm->liegeRealm, realmId);
    }
    
    // Break all alliances
    auto diplomacy = GetDiplomacy(realmId);
    if (diplomacy) {
        for (auto allianceId : diplomacy->alliances) {
            BreakAlliance(realmId, allianceId);
        }
    }
    
    // Get entity ID
    types::EntityID entityId = GetEntityForRealm(realmId);
    if (entityId != 0) {
        // Remove entity
        auto entityManager = m_componentAccess->GetEntityManager();
        if (entityManager) {
            entityManager->DestroyEntity(::core::ecs::EntityID(entityId));
        }
    }
    
    // Unregister
    UnregisterRealm(realmId);
    
    CORE_STREAM_INFO("RealmManager") << "Destroyed realm: " << realm->realmName;
    
    return true;
}

bool RealmManager::MergeRealms(types::EntityID absorber, types::EntityID absorbed) {
    auto absorberRealm = GetRealm(absorber);
    auto absorbedRealm = GetRealm(absorbed);

    if (!absorberRealm || !absorbedRealm) {
        return false;
    }

    // FIXED: ADDITIONAL-001 - Add validation
    // Cannot merge same realm
    if (absorber == absorbed) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot merge realm with itself";
        return false;
    }

    // Cannot merge if realms are at war
    auto diplomacy = GetDiplomacy(absorber);
    if (diplomacy && diplomacy->IsAtWarWith(absorbed)) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot merge realms that are at war";
        return false;
    }

    // Check for circular vassalage
    if (absorberRealm->liegeRealm == absorbed) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot merge: would create circular vassalage";
        return false;
    }

    // FIXED: HIGH-001 - Make explicit copy to prevent iterator invalidation
    std::vector<types::EntityID> provincesToTransfer;
    {
        std::lock_guard<std::mutex> lock(absorbedRealm->dataMutex);
        provincesToTransfer = absorbedRealm->ownedProvinces;
    }

    // Transfer all provinces
    for (auto provinceId : provincesToTransfer) {
        AddProvinceToRealm(absorber, provinceId);
    }

    // Transfer treasury (protected by shared_ptr access)
    absorberRealm->treasury += absorbedRealm->treasury;

    // Transfer vassals - FIXED: HIGH-001 - Make copy first
    std::vector<types::EntityID> vassalsToTransfer;
    {
        std::lock_guard<std::mutex> lock(absorbedRealm->dataMutex);
        vassalsToTransfer = absorbedRealm->vassalRealms;
    }

    for (auto vassalId : vassalsToTransfer) {
        // Make them vassals of absorber instead
        auto vassal = GetRealm(vassalId);
        if (vassal) {
            std::lock_guard<std::mutex> vassalLock(vassal->dataMutex);
            std::lock_guard<std::mutex> absorberLock(absorberRealm->dataMutex);
            vassal->liegeRealm = absorber;
            absorberRealm->vassalRealms.push_back(vassalId);
        }
    }

    // Publish annexation event
    events::RealmAnnexed event;
    event.conqueror = absorber;
    event.conquered = absorbed;
    if (m_messageBus) {
        m_messageBus->Publish<events::RealmAnnexed>(event);
    }

    // Destroy absorbed realm
    DestroyRealm(absorbed);

    CORE_STREAM_INFO("RealmManager") << "" << absorberRealm->realmName
              << " annexed " << absorbedRealm->realmName;

    return true;
}

// ============================================================================
// Dynasty Management
// ============================================================================

types::EntityID RealmManager::CreateDynasty(
    const std::string& dynastyName,
    types::EntityID founder) {

    // IMPROVED: Input validation
    if (dynastyName.empty()) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot create dynasty - name is empty";
        return types::EntityID{0};
    }

    if (dynastyName.length() > 100) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot create dynasty - name too long (max 100 chars)";
        return types::EntityID{0};
    }

    if (founder == 0) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot create dynasty - invalid founder";
        return types::EntityID{0};
    }

    // Check for duplicate dynasty name
    {
        std::lock_guard<std::mutex> lock(m_dynastyMutex);
        if (m_dynastiesByName.find(dynastyName) != m_dynastiesByName.end()) {
            CORE_STREAM_ERROR("RealmManager") << "Cannot create dynasty - name already exists: " << dynastyName;
            return types::EntityID{0};
        }
    }

    if (!m_componentAccess) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot create dynasty - no component access";
        return types::EntityID{0};
    }

    auto entityManager = m_componentAccess->GetEntityManager();
    if (!entityManager) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot create dynasty - no entity manager";
        return types::EntityID{0};
    }
    
    types::EntityID entityId = entityManager->CreateEntity().id;
    types::EntityID dynastyId = m_nextDynastyId.fetch_add(1);
    
    // Create dynasty component
    DynastyComponent dynasty(dynastyId);
    dynasty.dynastyName = dynastyName;
    dynasty.founder = founder;
    dynasty.currentHead = founder;
    dynasty.livingMembers.push_back(founder);
    
    entityManager->AddComponent<DynastyComponent>(::core::ecs::EntityID(entityId), dynasty);

    // Register dynasty - FIXED: ADDITIONAL-005 - Use dynastyMutex
    {
        std::lock_guard<std::mutex> lock(m_dynastyMutex);  // FIXED: ADDITIONAL-005
        m_dynastyEntities[dynastyId] = entityId;
        m_dynastiesByName[dynastyName] = dynastyId;
    }

    CORE_STREAM_INFO("RealmManager") << "Created dynasty: " << dynastyName
              << " (ID: " << dynastyId << ")";
    
    return dynastyId;
}

// ============================================================================
// Territory Management
// ============================================================================

bool RealmManager::AddProvinceToRealm(types::EntityID realmId, types::EntityID provinceId) {
    // IMPROVED: Validation
    if (provinceId == 0) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot add province - invalid province ID";
        return false;
    }

    auto realm = GetRealm(realmId);
    if (!realm) {
        CORE_STREAM_ERROR("RealmManager") << "Cannot add province - realm not found: " << realmId;
        return false;
    }

    // FIXED: NEW-CRITICAL-003 - Add mutex protection for vector operations
    std::lock_guard<std::mutex> lock(realm->dataMutex);

    // Check if province already owned
    auto it = std::find(realm->ownedProvinces.begin(),
                       realm->ownedProvinces.end(),
                       provinceId);
    if (it != realm->ownedProvinces.end()) {
        CORE_STREAM_WARN("RealmManager") << "Province already owned by realm: " << provinceId;
        return false;
    }

    // Add province
    realm->ownedProvinces.push_back(provinceId);

    // Update realm rank based on province count - IMPROVED: Use named constants
    size_t provinceCount = realm->ownedProvinces.size();
    if (provinceCount >= RealmConstants::EMPIRE_MIN_PROVINCES) {
        realm->rank = RealmRank::EMPIRE;
    } else if (provinceCount >= RealmConstants::KINGDOM_MIN_PROVINCES) {
        realm->rank = RealmRank::KINGDOM;
    } else if (provinceCount >= RealmConstants::DUCHY_MIN_PROVINCES) {
        realm->rank = RealmRank::DUCHY;
    } else if (provinceCount >= RealmConstants::COUNTY_MIN_PROVINCES) {
        realm->rank = RealmRank::COUNTY;
    } else {
        realm->rank = RealmRank::BARONY;
    }

    CORE_STREAM_INFO("RealmManager") << "Added province " << provinceId
                                     << " to realm " << realm->realmName
                                     << " (now " << realm->ownedProvinces.size() << " provinces)";

    return true;
}

bool RealmManager::RemoveProvinceFromRealm(types::EntityID realmId, types::EntityID provinceId) {
    auto realm = GetRealm(realmId);
    if (!realm) {
        return false;
    }

    // FIXED: NEW-CRITICAL-003 - Add mutex protection for vector operations
    std::lock_guard<std::mutex> lock(realm->dataMutex);

    auto it = std::find(realm->ownedProvinces.begin(),
                       realm->ownedProvinces.end(),
                       provinceId);
    if (it == realm->ownedProvinces.end()) {
        return false; // Not owned
    }

    realm->ownedProvinces.erase(it);

    return true;
}

bool RealmManager::TransferProvince(
    types::EntityID from, 
    types::EntityID to, 
    types::EntityID provinceId) {
    
    if (!RemoveProvinceFromRealm(from, provinceId)) {
        return false;
    }
    
    return AddProvinceToRealm(to, provinceId);
}

// ============================================================================
// Ruler Management
// ============================================================================

bool RealmManager::SetRuler(types::EntityID realmId, types::EntityID characterId) {
    auto realm = GetRealm(realmId);
    if (!realm) {
        return false;
    }
    
    types::EntityID oldRuler = realm->currentRuler;
    realm->currentRuler = characterId;
    
    // Create or update ruler component
    if (m_componentAccess) {
        auto entityManager = m_componentAccess->GetEntityManager();
        if (entityManager) {
            // Check if character entity exists
            // For now, create ruler component on realm entity
            RulerComponent ruler(characterId);
            ruler.ruledRealm = realmId;
            // FIXED: HIGH-001 - Use GameDate
            ruler.reignStart = game::time::GameDate(1066, 10, 14);  // TODO: Get current game date
            ruler.reignYears = 0;
            
            // This would ideally be on the character entity
            // For now, storing reference
        }
    }
    
    return true;
}

bool RealmManager::TriggerSuccession(types::EntityID realmId) {
    auto realm = GetRealm(realmId);
    if (!realm) {
        return false;
    }
    
    // Determine heir
    types::EntityID heir = DetermineHeir(realmId);
    if (heir == 0) {
        CORE_STREAM_ERROR("RealmManager") << "No valid heir for realm " << realmId;
        return false;
    }
    
    types::EntityID oldRuler = realm->currentRuler;
    
    // Apply succession
    ApplySuccessionEffects(realmId, heir);
    
    // Publish succession event
    events::SuccessionTriggered event;
    event.realmId = realmId;
    event.previousRuler = oldRuler;
    event.newRuler = heir;
    event.law = realm->successionLaw;
    PublishRealmEvent(event);
    
    CORE_STREAM_INFO("RealmManager") << "Succession in " << realm->realmName 
              << ": " << oldRuler << " -> " << heir;
    
    return true;
}

types::EntityID RealmManager::DetermineHeir(types::EntityID realmId) const {
    auto realm = GetRealm(realmId);
    if (!realm) {
        return types::EntityID{0};
    }
    
    // If designated heir exists
    if (realm->heir != 0) {
        return realm->heir;
    }
    
    // Use succession law
    auto heirs = RealmUtils::GetValidHeirs(*realm, realm->successionLaw);
    
    if (heirs.empty()) {
        return types::EntityID{0};
    }
    
    // Return first valid heir
    return heirs[0];
}


// ============================================================================
// Diplomatic Relations
// ============================================================================

bool RealmManager::SetDiplomaticStatus(
    types::EntityID realm1,
    types::EntityID realm2,
    DiplomaticStatus status) {

    auto diplomacy1 = GetDiplomacy(realm1);
    auto diplomacy2 = GetDiplomacy(realm2);

    if (!diplomacy1 || !diplomacy2) {
        return false;
    }

    // FIXED: Update to use thread-safe GetRelation API
    // Get or create relation for realm1
    auto relation1Opt = diplomacy1->GetRelation(realm2);
    if (!relation1Opt) {
        DiplomaticRelation newRelation;
        newRelation.otherRealm = realm2;
        newRelation.relationshipStart = game::time::GameDate(1066, 10, 14);  // TODO: Get current game date
        diplomacy1->SetRelation(realm2, newRelation);
        relation1Opt = diplomacy1->GetRelation(realm2);
    }

    // Get or create relation for realm2
    auto relation2Opt = diplomacy2->GetRelation(realm1);
    if (!relation2Opt) {
        DiplomaticRelation newRelation;
        newRelation.otherRealm = realm1;
        newRelation.relationshipStart = game::time::GameDate(1066, 10, 14);  // TODO: Get current game date
        diplomacy2->SetRelation(realm1, newRelation);
        relation2Opt = diplomacy2->GetRelation(realm1);
    }

    DiplomaticStatus oldStatus = relation1Opt->status;

    // Update status using WithRelation for thread safety
    diplomacy1->WithRelation(realm2, [status](DiplomaticRelation& rel) {
        rel.status = status;
    });

    diplomacy2->WithRelation(realm1, [status](DiplomaticRelation& rel) {
        rel.status = status;
    });

    // Publish event
    events::DiplomaticStatusChanged event;
    event.realm1 = realm1;
    event.realm2 = realm2;
    event.oldStatus = oldStatus;
    event.newStatus = status;
    if (m_messageBus) {
        m_messageBus->Publish<events::DiplomaticStatusChanged>(event);
    }

    return true;
}

bool RealmManager::DeclareWar(
    types::EntityID aggressor,
    types::EntityID defender,
    CasusBelli justification) {
    
    auto aggressorRealm = GetRealm(aggressor);
    auto defenderRealm = GetRealm(defender);
    
    if (!aggressorRealm || !defenderRealm) {
        return false;
    }
    
    // Validate war declaration
    if (!RealmUtils::CanDeclareWar(*aggressorRealm, *defenderRealm)) {
        CORE_STREAM_ERROR("RealmManager") << "Invalid war declaration";
        return false;
    }
    
    auto diplomacy1 = GetDiplomacy(aggressor);
    auto diplomacy2 = GetDiplomacy(defender);
    
    if (!diplomacy1 || !diplomacy2) {
        return false;
    }
    
    // FIXED: Use WithRelation for thread-safe updates
    diplomacy1->WithRelation(defender, [justification](DiplomaticRelation& rel) {
        rel.atWar = true;
        rel.warJustification = justification;
        rel.warscore = 0.0f;
        rel.warsCount++;
        rel.status = DiplomaticStatus::WAR;
    });

    diplomacy2->WithRelation(aggressor, [](DiplomaticRelation& rel) {
        rel.atWar = true;
        rel.warJustification = CasusBelli::DEFENSIVE;
        rel.warscore = 0.0f;
        rel.warsCount++;
        rel.status = DiplomaticStatus::WAR;
    });

    // Increase aggressive expansion - IMPROVED: Use named constant
    diplomacy1->aggressiveExpansion += RealmConstants::AGGRESSIVE_EXPANSION_PER_WAR;

    // Decrease stability - IMPROVED: Use named constants
    aggressorRealm->stability = std::max(RealmConstants::MIN_STABILITY,
                                         std::min(RealmConstants::MAX_STABILITY,
                                                  aggressorRealm->stability * RealmConstants::WAR_AGGRESSOR_STABILITY_MULT));
    defenderRealm->stability = std::max(RealmConstants::MIN_STABILITY,
                                        std::min(RealmConstants::MAX_STABILITY,
                                                 defenderRealm->stability * RealmConstants::WAR_DEFENDER_STABILITY_MULT));

    // Publish war event
    events::WarDeclared event;
    event.aggressor = aggressor;
    event.defender = defender;
    event.justification = justification;
    PublishRealmEvent(event);
    
    CORE_STREAM_INFO("RealmManager") << "" << aggressorRealm->realmName 
              << " declares war on " << defenderRealm->realmName;
    
    return true;
}

bool RealmManager::MakePeace(
    types::EntityID realm1,
    types::EntityID realm2,
    float warscore) {
    
    auto diplomacy1 = GetDiplomacy(realm1);
    auto diplomacy2 = GetDiplomacy(realm2);
    
    if (!diplomacy1 || !diplomacy2) {
        return false;
    }
    
    auto* relation1 = diplomacy1->GetRelation(realm2);
    auto* relation2 = diplomacy2->GetRelation(realm1);
    
    if (!relation1 || !relation2 || !relation1->atWar) {
        return false;
    }
    
    // Apply war consequences
    types::EntityID winner = (warscore > 0) ? realm1 : realm2;
    types::EntityID loser = (warscore > 0) ? realm2 : realm1;
    ApplyWarConsequences(winner, loser, std::abs(warscore));
    
    // End war
    relation1->atWar = false;
    relation2->atWar = false;
    
    // Set appropriate post-war status
    if (warscore > 50.0f) {
        relation1->status = DiplomaticStatus::HOSTILE;
        relation2->status = DiplomaticStatus::HOSTILE;
    } else {
        relation1->status = DiplomaticStatus::COLD;
        relation2->status = DiplomaticStatus::COLD;
    }
    
    CORE_STREAM_INFO("RealmManager") << "Peace made between realms " 
              << realm1 << " and " << realm2 
              << " (warscore: " << warscore << ")";
    
    return true;
}

bool RealmManager::FormAlliance(types::EntityID realm1, types::EntityID realm2) {
    auto diplomacy1 = GetDiplomacy(realm1);
    auto diplomacy2 = GetDiplomacy(realm2);
    
    if (!diplomacy1 || !diplomacy2) {
        return false;
    }
    
    auto* relation1 = diplomacy1->GetRelation(realm2);
    auto* relation2 = diplomacy2->GetRelation(realm1);
    
    if (!relation1 || !relation2) {
        return false;
    }
    
    // Cannot ally if at war
    if (relation1->atWar) {
        return false;
    }
    
    // Set alliance
    relation1->hasAlliance = true;
    relation2->hasAlliance = true;
    relation1->alliancesCount++;
    relation2->alliancesCount++;
    
    // Update status
    relation1->status = DiplomaticStatus::ALLIED;
    relation2->status = DiplomaticStatus::ALLIED;
    
    // Add to alliance lists
    diplomacy1->alliances.push_back(realm2);
    diplomacy2->alliances.push_back(realm1);

    // Improve opinions - IMPROVED: Use named constants
    relation1->opinion = std::min(RealmConstants::MAX_OPINION,
                                  relation1->opinion + RealmConstants::ALLIANCE_OPINION_BONUS);
    relation2->opinion = std::min(RealmConstants::MAX_OPINION,
                                  relation2->opinion + RealmConstants::ALLIANCE_OPINION_BONUS);
    
    // Propagate alliance effects
    PropagateAllianceEffects(realm1, realm2);
    
    CORE_STREAM_INFO("RealmManager") << "Alliance formed between realms " 
              << realm1 << " and " << realm2;
    
    return true;
}

bool RealmManager::BreakAlliance(types::EntityID realm1, types::EntityID realm2) {
    auto diplomacy1 = GetDiplomacy(realm1);
    auto diplomacy2 = GetDiplomacy(realm2);
    
    if (!diplomacy1 || !diplomacy2) {
        return false;
    }
    
    auto* relation1 = diplomacy1->GetRelation(realm2);
    auto* relation2 = diplomacy2->GetRelation(realm1);
    
    if (!relation1 || !relation2 || !relation1->hasAlliance) {
        return false;
    }
    
    // Break alliance
    relation1->hasAlliance = false;
    relation2->hasAlliance = false;
    
    // Update status
    relation1->status = DiplomaticStatus::NEUTRAL;
    relation2->status = DiplomaticStatus::NEUTRAL;
    
    // Remove from alliance lists
    auto it1 = std::find(diplomacy1->alliances.begin(), 
                         diplomacy1->alliances.end(), realm2);
    if (it1 != diplomacy1->alliances.end()) {
        diplomacy1->alliances.erase(it1);
    }
    
    auto it2 = std::find(diplomacy2->alliances.begin(), 
                         diplomacy2->alliances.end(), realm1);
    if (it2 != diplomacy2->alliances.end()) {
        diplomacy2->alliances.erase(it2);
    }
    
    // Decrease opinions - IMPROVED: Use named constants
    relation1->opinion = std::max(RealmConstants::MIN_OPINION,
                                  relation1->opinion + RealmConstants::ALLIANCE_BREAK_OPINION_PENALTY);
    relation2->opinion = std::max(RealmConstants::MIN_OPINION,
                                  relation2->opinion + RealmConstants::ALLIANCE_BREAK_OPINION_PENALTY);

    // Decrease trustworthiness - IMPROVED: Use named constant
    diplomacy1->trustworthiness *= RealmConstants::ALLIANCE_BREAK_TRUST_MULT;
    diplomacy1->trustworthiness = std::max(0.0f, diplomacy1->trustworthiness);
    
    CORE_STREAM_INFO("RealmManager") << "Alliance broken between realms " 
              << realm1 << " and " << realm2;
    
    return true;
}

// ============================================================================
// Vassalage
// ============================================================================

bool RealmManager::MakeVassal(types::EntityID liege, types::EntityID vassal) {
    auto liegeRealm = GetRealm(liege);
    auto vassalRealm = GetRealm(vassal);

    if (!liegeRealm || !vassalRealm) {
        return false;
    }

    // Cannot vassal yourself
    if (liege == vassal) {
        return false;
    }

    // FIXED: NEW-CRITICAL-003 - Lock both realms to prevent data races
    // Lock in consistent order to prevent deadlock (lower ID first)
    std::unique_lock<std::mutex> lock1, lock2;
    if (liege < vassal) {
        lock1 = std::unique_lock<std::mutex>(liegeRealm->dataMutex);
        lock2 = std::unique_lock<std::mutex>(vassalRealm->dataMutex);
    } else {
        lock2 = std::unique_lock<std::mutex>(vassalRealm->dataMutex);
        lock1 = std::unique_lock<std::mutex>(liegeRealm->dataMutex);
    }

    // Check if already a vassal
    if (vassalRealm->liegeRealm != 0) {
        return false;
    }

    // Set vassalage
    vassalRealm->liegeRealm = liege;
    liegeRealm->vassalRealms.push_back(vassal);

    // Update diplomatic status
    SetDiplomaticStatus(liege, vassal, DiplomaticStatus::VASSAL);

    // Publish event
    events::VassalageChanged event;
    event.vassal = vassal;
    event.liege = liege;
    event.isNowVassal = true;
    if (m_messageBus) {
        m_messageBus->Publish<events::VassalageChanged>(event);
    }

    CORE_STREAM_INFO("RealmManager") << "" << vassalRealm->realmName
              << " is now vassal of " << liegeRealm->realmName;

    return true;
}

bool RealmManager::ReleaseVassal(types::EntityID liege, types::EntityID vassal) {
    auto liegeRealm = GetRealm(liege);
    auto vassalRealm = GetRealm(vassal);

    if (!liegeRealm || !vassalRealm) {
        return false;
    }

    // FIXED: NEW-CRITICAL-003 - Lock both realms to prevent data races
    // Lock in consistent order to prevent deadlock (lower ID first)
    std::unique_lock<std::mutex> lock1, lock2;
    if (liege < vassal) {
        lock1 = std::unique_lock<std::mutex>(liegeRealm->dataMutex);
        lock2 = std::unique_lock<std::mutex>(vassalRealm->dataMutex);
    } else {
        lock2 = std::unique_lock<std::mutex>(vassalRealm->dataMutex);
        lock1 = std::unique_lock<std::mutex>(liegeRealm->dataMutex);
    }

    // Check if actually a vassal
    if (vassalRealm->liegeRealm != liege) {
        return false;
    }

    // Release vassal
    vassalRealm->liegeRealm = 0;

    auto it = std::find(liegeRealm->vassalRealms.begin(),
                       liegeRealm->vassalRealms.end(), vassal);
    if (it != liegeRealm->vassalRealms.end()) {
        liegeRealm->vassalRealms.erase(it);
    }

    // Update diplomatic status
    SetDiplomaticStatus(liege, vassal, DiplomaticStatus::NEUTRAL);

    // Publish event
    events::VassalageChanged event;
    event.vassal = vassal;
    event.liege = liege;
    event.isNowVassal = false;
    if (m_messageBus) {
        m_messageBus->Publish<events::VassalageChanged>(event);
    }

    CORE_STREAM_INFO("RealmManager") << "" << vassalRealm->realmName
              << " released from vassalage";

    return true;
}

bool RealmManager::IsVassal(types::EntityID realmId) const {
    auto realm = GetRealm(realmId);
    if (!realm) {
        return false;
    }
    
    return realm->liegeRealm != 0;
}

types::EntityID RealmManager::GetLiege(types::EntityID vassalId) const {
    auto realm = GetRealm(vassalId);
    if (!realm) {
        return types::EntityID{0};
    }
    
    return realm->liegeRealm;
}

std::vector<types::EntityID> RealmManager::GetVassals(types::EntityID liegeId) const {
    auto realm = GetRealm(liegeId);
    if (!realm) {
        return std::vector<types::EntityID>();
    }
    
    return realm->vassalRealms;
}

// ============================================================================
// Council Management
// ============================================================================

bool RealmManager::AppointCouncilor(
    types::EntityID realmId,
    CouncilPosition position,
    types::EntityID characterId) {
    
    auto council = GetCouncil(realmId);
    if (!council) {
        return false;
    }
    
    council->AppointCouncilor(position, characterId);
    
    CORE_STREAM_INFO("RealmManager") << "Appointed councilor to position " 
              << static_cast<int>(position) << " in realm " << realmId;
    
    return true;
}

bool RealmManager::DismissCouncilor(
    types::EntityID realmId,
    CouncilPosition position) {
    
    auto council = GetCouncil(realmId);
    if (!council) {
        return false;
    }
    
    auto it = council->council.find(position);
    if (it != council->council.end()) {
        council->council.erase(it);
        return true;
    }
    
    return false;
}

// ============================================================================
// Law Changes
// ============================================================================

bool RealmManager::ChangeLaw(types::EntityID realmId, const std::string& lawType, float value) {
    auto laws = GetLaws(realmId);
    if (!laws) {
        return false;
    }
    
    // Update specific law type
    if (lawType == "base_tax") {
        laws->baseTaxRate = value;
    } else if (lawType == "noble_tax") {
        laws->nobleTaxRate = value;
    } else if (lawType == "merchant_tax") {
        laws->merchantTaxRate = value;
    } else if (lawType == "peasant_tax") {
        laws->peasantTaxRate = value;
    } else if (lawType == "levy_obligation") {
        laws->levyObligation = value;
    } else {
        return false;
    }
    
    CORE_STREAM_INFO("RealmManager") << "Changed law " << lawType 
              << " to " << value << " in realm " << realmId;
    
    return true;
}

bool RealmManager::ChangeSuccessionLaw(types::EntityID realmId, SuccessionLaw newLaw) {
    auto realm = GetRealm(realmId);
    if (!realm) {
        return false;
    }
    
    realm->successionLaw = newLaw;
    
    CORE_STREAM_INFO("RealmManager") << "Changed succession law to " 
              << RealmUtils::SuccessionLawToString(newLaw) 
              << " in realm " << realmId;
    
    return true;
}

bool RealmManager::ChangeCrownAuthority(types::EntityID realmId, CrownAuthority newLevel) {
    auto laws = GetLaws(realmId);
    if (!laws) {
        return false;
    }
    
    laws->crownAuthority = newLevel;
    
    // Update realm central authority based on crown authority
    auto realm = GetRealm(realmId);
    if (realm) {
        switch (newLevel) {
            case CrownAuthority::MINIMAL:
                realm->centralAuthority = 0.2f;
                break;
            case CrownAuthority::LOW:
                realm->centralAuthority = 0.4f;
                break;
            case CrownAuthority::MEDIUM:
                realm->centralAuthority = 0.6f;
                break;
            case CrownAuthority::HIGH:
                realm->centralAuthority = 0.8f;
                break;
            case CrownAuthority::ABSOLUTE:
                realm->centralAuthority = 1.0f;
                break;
            default:
                break;
        }
    }
    
    CORE_STREAM_INFO("RealmManager") << "Changed crown authority to " 
              << RealmUtils::CrownAuthorityToString(newLevel) 
              << " in realm " << realmId;
    
    return true;
}

// ============================================================================
// Queries
// ============================================================================

std::shared_ptr<RealmComponent> RealmManager::GetRealm(types::EntityID realmId) {
    if (!m_componentAccess) return nullptr;
    
    types::EntityID entityId = GetEntityForRealm(realmId);
    if (entityId == 0) return nullptr;
    
    // Convert to EntityID handle and use EntityManager directly
    auto* entityManager = m_componentAccess->GetEntityManager();
    ::core::ecs::EntityID entityHandle(entityId);
    return entityManager->GetComponent<RealmComponent>(entityHandle);
}

std::shared_ptr<const RealmComponent> RealmManager::GetRealm(types::EntityID realmId) const {
    if (!m_componentAccess) return nullptr;
    
    types::EntityID entityId = GetEntityForRealm(realmId);
    if (entityId == 0) return nullptr;
    
    // Convert to EntityID handle and use EntityManager directly
    auto* entityManager = m_componentAccess->GetEntityManager();
    ::core::ecs::EntityID entityHandle(entityId);
    return entityManager->GetComponent<RealmComponent>(entityHandle);
}

std::shared_ptr<RealmComponent> RealmManager::GetRealmByName(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    auto it = m_realmsByName.find(name);
    if (it != m_realmsByName.end()) {
        return GetRealm(it->second);
    }
    
    return nullptr;
}

std::shared_ptr<DynastyComponent> RealmManager::GetDynasty(types::EntityID dynastyId) {
    if (!m_componentAccess) return nullptr;

    // FIXED: NEW-CRITICAL-002 - Use dynastyMutex instead of registryMutex
    std::lock_guard<std::mutex> lock(m_dynastyMutex);

    auto it = m_dynastyEntities.find(dynastyId);
    if (it != m_dynastyEntities.end()) {
        auto* entityManager = m_componentAccess->GetEntityManager();
        return entityManager->GetComponent<DynastyComponent>(::core::ecs::EntityID(it->second));
    }

    return nullptr;
}

std::shared_ptr<RulerComponent> RealmManager::GetRuler(types::EntityID characterId) {
    if (!m_componentAccess) return nullptr;
    
    // Would need to search for character entity
    // For now, simplified
    return nullptr;
}

std::shared_ptr<DiplomaticRelationsComponent> RealmManager::GetDiplomacy(types::EntityID realmId) {
    if (!m_componentAccess) return nullptr;
    
    types::EntityID entityId = GetEntityForRealm(realmId);
    if (entityId == 0) return nullptr;
    
    // Convert to EntityID handle and use EntityManager directly
    auto* entityManager = m_componentAccess->GetEntityManager();
    ::core::ecs::EntityID entityHandle(entityId);
    return entityManager->GetComponent<DiplomaticRelationsComponent>(entityHandle);
}

std::shared_ptr<CouncilComponent> RealmManager::GetCouncil(types::EntityID realmId) {
    if (!m_componentAccess) return nullptr;
    
    types::EntityID entityId = GetEntityForRealm(realmId);
    if (entityId == 0) return nullptr;
    
    // Convert to EntityID handle and use EntityManager directly
    auto* entityManager = m_componentAccess->GetEntityManager();
    ::core::ecs::EntityID entityHandle(entityId);
    return entityManager->GetComponent<CouncilComponent>(entityHandle);
}

std::shared_ptr<LawsComponent> RealmManager::GetLaws(types::EntityID realmId) {
    if (!m_componentAccess) return nullptr;
    
    types::EntityID entityId = GetEntityForRealm(realmId);
    if (entityId == 0) return nullptr;
    
    // Convert to EntityID handle and use EntityManager directly
    auto* entityManager = m_componentAccess->GetEntityManager();
    ::core::ecs::EntityID entityHandle(entityId);
    return entityManager->GetComponent<LawsComponent>(entityHandle);
}

// ============================================================================
// Utility Queries
// ============================================================================

std::vector<types::EntityID> RealmManager::GetAllRealms() const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    std::vector<types::EntityID> realms;
    realms.reserve(m_realmEntities.size());
    
    for (const auto& [realmId, entityId] : m_realmEntities) {
        realms.push_back(realmId);
    }
    
    return realms;
}

std::vector<types::EntityID> RealmManager::GetRealmsAtWar() const {
    std::vector<types::EntityID> warringRealms;
    
    auto allRealms = GetAllRealms();
    for (auto realmId : allRealms) {
        auto diplomacy = const_cast<RealmManager*>(this)->GetDiplomacy(realmId);
        if (diplomacy) {
            for (const auto& [otherId, relation] : diplomacy->relations) {
                if (relation.atWar) {
                    warringRealms.push_back(realmId);
                    break;
                }
            }
        }
    }
    
    return warringRealms;
}

std::vector<types::EntityID> RealmManager::GetIndependentRealms() const {
    std::vector<types::EntityID> independent;
    
    auto allRealms = GetAllRealms();
    for (auto realmId : allRealms) {
        if (!IsVassal(realmId)) {
            independent.push_back(realmId);
        }
    }
    
    return independent;
}

float RealmManager::CalculateRealmStrength(types::EntityID realmId) const {
    auto realm = GetRealm(realmId);
    if (!realm) {
        return 0.0f;
    }
    
    return RealmUtils::CalculateRealmPower(*realm);
}

bool RealmManager::AreAtWar(types::EntityID realm1, types::EntityID realm2) const {
    auto diplomacy = const_cast<RealmManager*>(this)->GetDiplomacy(realm1);
    if (!diplomacy) {
        return false;
    }
    
    return diplomacy->IsAtWarWith(realm2);
}

bool RealmManager::AreAllied(types::EntityID realm1, types::EntityID realm2) const {
    auto diplomacy = const_cast<RealmManager*>(this)->GetDiplomacy(realm1);
    if (!diplomacy) {
        return false;
    }
    
    return diplomacy->IsAlliedWith(realm2);
}

// ============================================================================
// Statistics
// ============================================================================

RealmStats RealmManager::GetStatistics() const {
    // FIXED: HIGH-005 - Stats are atomic, create non-atomic copy to return
    return RealmStats{
        m_stats.totalRealms.load(),
        m_stats.activeWars.load(),
        m_stats.totalAlliances.load(),
        m_stats.vassalRelationships.load()
    };
}

void RealmManager::UpdateStatistics() {
    // FIXED: HIGH-005 - Stats are atomic, no mutex needed

    m_stats.totalRealms = m_realmEntities.size();

    // Count wars
    uint32_t activeWarsCount = 0;
    auto allRealms = GetAllRealms();
    for (auto realmId : allRealms) {
        auto diplomacy = const_cast<RealmManager*>(this)->GetDiplomacy(realmId);
        if (diplomacy) {
            for (const auto& [otherId, relation] : diplomacy->relations) {
                if (relation.atWar && realmId < otherId) { // Count each war once
                    activeWarsCount++;
                }
            }
        }
    }
    m_stats.activeWars = activeWarsCount;

    // Count alliances
    uint32_t totalAlliancesCount = 0;
    for (auto realmId : allRealms) {
        auto diplomacy = const_cast<RealmManager*>(this)->GetDiplomacy(realmId);
        if (diplomacy) {
            totalAlliancesCount += diplomacy->alliances.size();
        }
    }
    m_stats.totalAlliances = totalAlliancesCount / 2; // Each alliance counted twice

    // Count vassal relationships
    uint32_t vassalRelationshipsCount = 0;
    for (auto realmId : allRealms) {
        auto realm = GetRealm(realmId);
        if (realm) {
            vassalRelationshipsCount += realm->vassalRealms.size();
        }
    }
    m_stats.vassalRelationships = vassalRelationshipsCount;
}
// ============================================================================
// Internal Helpers
// ============================================================================

types::EntityID RealmManager::GetEntityForRealm(types::EntityID realmId) const {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    auto it = m_realmEntities.find(realmId);
    if (it != m_realmEntities.end()) {
        return it->second;
    }
    
    return types::EntityID{0};
}

void RealmManager::RegisterRealm(types::EntityID realmId, types::EntityID entityId) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    m_realmEntities[realmId] = entityId;
    
    // Also register by name if available
    auto* entityManager = m_componentAccess->GetEntityManager();
    auto realm = entityManager->GetComponent<RealmComponent>(::core::ecs::EntityID(entityId));
    if (realm && !realm->realmName.empty()) {
        m_realmsByName[realm->realmName] = realmId;
    }
}

void RealmManager::UnregisterRealm(types::EntityID realmId) {
    std::lock_guard<std::mutex> lock(m_registryMutex);
    
    // Get realm name before removing
    auto it = m_realmEntities.find(realmId);
    if (it != m_realmEntities.end()) {
        auto* entityManager = m_componentAccess->GetEntityManager();
        auto realm = entityManager->GetComponent<RealmComponent>(::core::ecs::EntityID(it->second));
        if (realm && !realm->realmName.empty()) {
            m_realmsByName.erase(realm->realmName);
        }
        
        m_realmEntities.erase(it);
    }
}

// ============================================================================
// Succession Helpers
// ============================================================================

std::vector<types::EntityID> RealmManager::CalculateSuccessionCandidates(
    const RealmComponent& realm,
    SuccessionLaw law) const {
    
    return RealmUtils::GetValidHeirs(realm, law);
}

void RealmManager::ApplySuccessionEffects(
    types::EntityID realmId,
    types::EntityID newRuler) {
    
    auto realm = GetRealm(realmId);
    if (!realm) return;
    
    types::EntityID oldRuler = realm->currentRuler;

    // Set new ruler
    realm->currentRuler = newRuler;
    // FIXED: HIGH-001 - Use GameDate
    realm->lastSuccession = game::time::GameDate(1066, 10, 14);  // TODO: Get current game date
    
    // Handle succession type effects - IMPROVED: Use named constants
    switch (realm->successionLaw) {
        case SuccessionLaw::GAVELKIND:
            // Territory might be divided (simplified)
            realm->stability *= RealmConstants::SUCCESSION_STABILITY_GAVELKIND;
            realm->legitimacy *= RealmConstants::SUCCESSION_LEGITIMACY_GAVELKIND;
            break;

        case SuccessionLaw::ELECTIVE:
            // Elected rulers have high legitimacy
            realm->legitimacy = RealmConstants::SUCCESSION_LEGITIMACY_ELECTIVE;
            realm->stability *= RealmConstants::SUCCESSION_STABILITY_ELECTIVE;
            break;

        case SuccessionLaw::PRIMOGENITURE:
        case SuccessionLaw::ULTIMOGENITURE:
            // Clear succession, minimal instability
            realm->stability *= RealmConstants::SUCCESSION_STABILITY_HEREDITARY;
            realm->legitimacy *= RealmConstants::SUCCESSION_LEGITIMACY_HEREDITARY;
            break;

        default:
            realm->stability *= RealmConstants::SUCCESSION_STABILITY_DEFAULT;
            realm->legitimacy *= RealmConstants::SUCCESSION_LEGITIMACY_DEFAULT;
            break;
    }

    // FIXED: ADDITIONAL-003 - Add bounds checking with named constants
    realm->stability = std::max(RealmConstants::MIN_STABILITY,
                                std::min(RealmConstants::MAX_STABILITY, realm->stability));
    realm->legitimacy = std::max(RealmConstants::MIN_LEGITIMACY,
                                 std::min(RealmConstants::MAX_LEGITIMACY, realm->legitimacy));

    // Clear heir designation
    realm->heir = types::EntityID{0};
}

// ============================================================================
// War Helpers
// ============================================================================

void RealmManager::UpdateWarscore(
    types::EntityID aggressor,
    types::EntityID defender,
    float change) {
    
    auto diplomacy = GetDiplomacy(aggressor);
    if (!diplomacy) return;
    
    auto* relation = diplomacy->GetRelation(defender);
    if (!relation) return;
    
    relation->warscore += change;
    relation->warscore = std::clamp(relation->warscore, -100.0f, 100.0f);
    
    // Mirror for defender
    auto defenderDiplomacy = GetDiplomacy(defender);
    if (defenderDiplomacy) {
        auto* defenderRelation = defenderDiplomacy->GetRelation(aggressor);
        if (defenderRelation) {
            defenderRelation->warscore = -relation->warscore;
        }
    }
}

void RealmManager::ApplyWarConsequences(
    types::EntityID winner,
    types::EntityID loser,
    float warscore) {
    
    auto winnerRealm = GetRealm(winner);
    auto loserRealm = GetRealm(loser);
    
    if (!winnerRealm || !loserRealm) return;
    
    // Calculate territory transfer based on warscore - IMPROVED: Use named constants
    if (warscore > RealmConstants::WARSCORE_TOTAL_CONQUEST) {
        // Total conquest - annex entire realm
        MergeRealms(winner, loser);
    } else if (warscore > RealmConstants::WARSCORE_MAJOR_VICTORY) {
        // Major victory - transfer significant territory
        size_t provincesToTransfer = loserRealm->ownedProvinces.size() / 3;
        for (size_t i = 0; i < provincesToTransfer && i < loserRealm->ownedProvinces.size(); ++i) {
            TransferProvince(loser, winner, loserRealm->ownedProvinces[i]);
        }
    } else if (warscore > RealmConstants::WARSCORE_MINOR_VICTORY) {
        // Minor victory - transfer some territory
        size_t provincesToTransfer = loserRealm->ownedProvinces.size() / 10;
        for (size_t i = 0; i < provincesToTransfer && i < loserRealm->ownedProvinces.size(); ++i) {
            TransferProvince(loser, winner, loserRealm->ownedProvinces[i]);
        }
    }

    // War reparations - IMPROVED: Use named constants
    double reparations = loserRealm->treasury * (warscore / 100.0) * RealmConstants::WAR_REPARATIONS_MULT;
    loserRealm->treasury = std::max(0.0, loserRealm->treasury - reparations);
    winnerRealm->treasury += reparations;

    // Prestige and stability changes - IMPROVED: Use named constants
    winnerRealm->stability = std::max(RealmConstants::MIN_STABILITY,
                                      std::min(RealmConstants::MAX_STABILITY,
                                               winnerRealm->stability + RealmConstants::WAR_WINNER_STABILITY_BONUS));
    loserRealm->stability = std::max(RealmConstants::MIN_STABILITY,
                                     std::min(RealmConstants::MAX_STABILITY,
                                              loserRealm->stability * RealmConstants::WAR_LOSER_STABILITY_MULT));
    loserRealm->legitimacy = std::max(RealmConstants::MIN_LEGITIMACY,
                                      std::min(RealmConstants::MAX_LEGITIMACY,
                                               loserRealm->legitimacy * RealmConstants::WAR_LOSER_LEGITIMACY_MULT));
}

// ============================================================================
// Diplomatic Helpers
// ============================================================================

void RealmManager::UpdateOpinion(
    types::EntityID realm1,
    types::EntityID realm2,
    float change) {
    
    auto diplomacy1 = GetDiplomacy(realm1);
    auto diplomacy2 = GetDiplomacy(realm2);
    
    if (!diplomacy1 || !diplomacy2) return;
    
    auto* relation1 = diplomacy1->GetRelation(realm2);
    if (relation1) {
        relation1->opinion += change;
        relation1->opinion = std::clamp(relation1->opinion, -100.0f, 100.0f);
    }
    
    auto* relation2 = diplomacy2->GetRelation(realm1);
    if (relation2) {
        relation2->opinion += change;
        relation2->opinion = std::clamp(relation2->opinion, -100.0f, 100.0f);
    }
}

void RealmManager::PropagateAllianceEffects(
    types::EntityID realm1,
    types::EntityID realm2) {
    
    // Improve relations with each other's allies
    auto diplomacy1 = GetDiplomacy(realm1);
    auto diplomacy2 = GetDiplomacy(realm2);
    
    if (!diplomacy1 || !diplomacy2) return;
    
    // Realm1's allies improve opinion of Realm2
    for (auto allyId : diplomacy1->alliances) {
        if (allyId != realm2) {
            UpdateOpinion(allyId, realm2, 10.0f);
        }
    }
    
    // Realm2's allies improve opinion of Realm1
    for (auto allyId : diplomacy2->alliances) {
        if (allyId != realm1) {
            UpdateOpinion(allyId, realm1, 10.0f);
        }
    }
}

// ============================================================================
// Event Publishing
// ============================================================================

void RealmManager::PublishRealmEvent(const events::RealmCreated& event) {
    if (m_messageBus) {
        m_messageBus->Publish<events::RealmCreated>(event);
    }
}

void RealmManager::PublishRealmEvent(const events::SuccessionTriggered& event) {
    if (m_messageBus) {
        m_messageBus->Publish<events::SuccessionTriggered>(event);
    }
}

void RealmManager::PublishRealmEvent(const events::WarDeclared& event) {
    if (m_messageBus) {
        m_messageBus->Publish<events::WarDeclared>(event);
    }
}

void RealmManager::PublishRealmEvent(const events::DiplomaticStatusChanged& event) {
    if (m_messageBus) {
        m_messageBus->Publish<events::DiplomaticStatusChanged>(event);
    }
}

// ============================================================================
// RealmFactory Implementation
// ============================================================================

types::EntityID RealmFactory::CreateFeudalKingdom(
    RealmManager& manager,
    const std::string& name,
    types::EntityID capital,
    types::EntityID ruler) {
    
    types::EntityID realmId = manager.CreateRealm(
        name,
        GovernmentType::FEUDAL_MONARCHY,
        capital,
        ruler
    );
    
    if (realmId == 0) return realmId;
    
    // Set feudal-specific settings
    auto realm = manager.GetRealm(realmId);
    if (realm) {
        realm->rank = RealmRank::KINGDOM;
        realm->successionLaw = SuccessionLaw::PRIMOGENITURE;
        realm->centralAuthority = 0.4f;
        realm->legitimacy = 0.8f;
    }
    
    auto laws = manager.GetLaws(realmId);
    if (laws) {
        laws->crownAuthority = CrownAuthority::MEDIUM;
        laws->vassalWarDeclaration = true;
        laws->vassalInheritance = true;
        laws->levyObligation = 0.4f;
        laws->serfdom = true;
    }
    
    return realmId;
}

types::EntityID RealmFactory::CreateMerchantRepublic(
    RealmManager& manager,
    const std::string& name,
    types::EntityID capital) {
    
    types::EntityID realmId = manager.CreateRealm(
        name,
        GovernmentType::MERCHANT_REPUBLIC,
        capital,
        types::EntityID{0} // No hereditary ruler
    );
    
    if (realmId == 0) return realmId;
    
    auto realm = manager.GetRealm(realmId);
    if (realm) {
        realm->rank = RealmRank::DUCHY;
        realm->successionLaw = SuccessionLaw::ELECTIVE;
        realm->centralAuthority = 0.6f;
        realm->legitimacy = 0.9f;
        realm->monthlyIncome *= 1.5; // Merchants are wealthy
    }
    
    auto laws = manager.GetLaws(realmId);
    if (laws) {
        laws->crownAuthority = CrownAuthority::LOW;
        laws->merchantTaxRate = 0.1f; // Lower merchant taxes
        laws->serfdom = false;
        laws->guildRights = true;
    }
    
    return realmId;
}

types::EntityID RealmFactory::CreateTheocracy(
    RealmManager& manager,
    const std::string& name,
    types::EntityID capital,
    types::EntityID religiousLeader) {
    
    types::EntityID realmId = manager.CreateRealm(
        name,
        GovernmentType::THEOCRACY,
        capital,
        religiousLeader
    );
    
    if (realmId == 0) return realmId;
    
    auto realm = manager.GetRealm(realmId);
    if (realm) {
        realm->rank = RealmRank::DUCHY;
        realm->successionLaw = SuccessionLaw::APPOINTMENT;
        realm->centralAuthority = 0.7f;
        realm->legitimacy = 1.0f; // Divine legitimacy
    }
    
    auto laws = manager.GetLaws(realmId);
    if (laws) {
        laws->crownAuthority = CrownAuthority::HIGH;
        laws->religiousTolerance = false;
    }
    
    return realmId;
}

types::EntityID RealmFactory::CreateTribalChiefdom(
    RealmManager& manager,
    const std::string& name,
    types::EntityID capital,
    types::EntityID chief) {
    
    types::EntityID realmId = manager.CreateRealm(
        name,
        GovernmentType::TRIBAL,
        capital,
        chief
    );
    
    if (realmId == 0) return realmId;
    
    auto realm = manager.GetRealm(realmId);
    if (realm) {
        realm->rank = RealmRank::COUNTY;
        realm->successionLaw = SuccessionLaw::TANISTRY;
        realm->centralAuthority = 0.3f;
        realm->legitimacy = 0.6f;
        realm->levySize *= 1.5; // Tribal levies are larger
        realm->treasury *= 0.5; // But poorer
    }
    
    auto laws = manager.GetLaws(realmId);
    if (laws) {
        laws->crownAuthority = CrownAuthority::MINIMAL;
        laws->levyObligation = 0.6f; // High levy rates
        laws->standingArmyAllowed = false;
    }
    
    return realmId;
}

types::EntityID RealmFactory::CreateEmpire(
    RealmManager& manager,
    const std::string& name,
    types::EntityID capital,
    types::EntityID emperor,
    const std::vector<types::EntityID>& vassalKingdoms) {
    
    types::EntityID realmId = manager.CreateRealm(
        name,
        GovernmentType::IMPERIAL,
        capital,
        emperor
    );
    
    if (realmId == 0) return realmId;
    
    auto realm = manager.GetRealm(realmId);
    if (realm) {
        realm->rank = RealmRank::EMPIRE;
        realm->successionLaw = SuccessionLaw::PRIMOGENITURE;
        realm->centralAuthority = 0.5f;
        realm->legitimacy = 0.9f;
        realm->treasury *= 5.0; // Empires are wealthy
        realm->monthlyIncome *= 3.0;
    }
    
    auto laws = manager.GetLaws(realmId);
    if (laws) {
        laws->crownAuthority = CrownAuthority::MEDIUM;
        laws->standingArmyAllowed = true;
    }
    
    // Make kingdoms vassals
    for (auto kingdomId : vassalKingdoms) {
        manager.MakeVassal(realmId, kingdomId);
    }
    
    return realmId;
}

} // namespace realm
} // namespace game
