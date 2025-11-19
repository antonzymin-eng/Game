// Created: September 25, 2025, 11:30 AM
// Location: include/game/realm/RealmComponents.h

#ifndef REALM_COMPONENTS_H
#define REALM_COMPONENTS_H

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include "game/time/TimeComponents.h"  // FIXED: HIGH-001 - Added for GameDate
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>  // FIXED: HIGH-002 - Added for synchronization

namespace game::realm {

// ============================================================================
// Realm Component - Core nation/state entity
// ============================================================================

enum class GovernmentType : uint8_t {
    FEUDAL_MONARCHY,
    ABSOLUTE_MONARCHY,
    ELECTIVE_MONARCHY,
    REPUBLIC,
    MERCHANT_REPUBLIC,
    THEOCRACY,
    TRIBAL,
    NOMADIC,
    IMPERIAL,
    CONSTITUTIONAL_MONARCHY,
    COUNT
};

enum class RealmRank : uint8_t {
    BARONY,
    COUNTY,
    DUCHY,
    KINGDOM,
    EMPIRE,
    COUNT
};

enum class SuccessionLaw : uint8_t {
    PRIMOGENITURE,      // Eldest child inherits
    ULTIMOGENITURE,     // Youngest child inherits
    GAVELKIND,         // Split between children
    ELECTIVE,          // Vassals vote
    TANISTRY,          // Family elders choose
    SENIORITY,         // Oldest dynasty member
    APPOINTMENT,       // Ruler appoints
    COUNT
};

class RealmComponent : public ::core::ecs::Component<RealmComponent> {
public:
    // Core identity
    types::EntityID realmId{0};
    std::string realmName;
    std::string adjective; // "Roman" for Rome
    RealmRank rank = RealmRank::COUNTY;
    
    // Government
    GovernmentType governmentType = GovernmentType::FEUDAL_MONARCHY;
    SuccessionLaw successionLaw = SuccessionLaw::PRIMOGENITURE;
    
    // Territory
    types::EntityID capitalProvince{0};
    std::vector<types::EntityID> ownedProvinces;
    std::vector<types::EntityID> claimedProvinces;
    
    // Ruler
    types::EntityID currentRuler{0};
    types::EntityID heir{0};
    std::vector<types::EntityID> claimants;
    
    // Hierarchy
    types::EntityID liegeRealm{0};  // Who this realm owes allegiance to
    std::vector<types::EntityID> vassalRealms;
    
    // Stats
    float legitimacy = 1.0f;  // 0-1, affects stability
    float centralAuthority = 0.5f;  // 0-1, control vs autonomy
    float stability = 1.0f;  // 0-1, internal order
    
    // Economics
    double treasury = 1000.0;
    double monthlyIncome = 100.0;
    double monthlyExpenses = 80.0;
    
    // Military
    uint32_t levySize = 1000;
    uint32_t standingArmy = 100;
    float militaryMaintenance = 0.5f; // % of income

    // Dates - FIXED: HIGH-001 - Changed from system_clock to GameDate
    game::time::GameDate foundedDate;      // FIXED: HIGH-001
    game::time::GameDate lastSuccession;   // FIXED: HIGH-001

    // Thread safety - FIXED: HIGH-002
    mutable std::mutex dataMutex;  // FIXED: HIGH-002 - Protects vectors and mutable state

    RealmComponent() = default;
    explicit RealmComponent(types::EntityID id) : realmId(id) {}

    // FIXED: HIGH-002 - Custom copy constructor (don't copy mutex)
    RealmComponent(const RealmComponent& other) : realmId(other.realmId), realmName(other.realmName),
        adjective(other.adjective), rank(other.rank), governmentType(other.governmentType),
        successionLaw(other.successionLaw), capitalProvince(other.capitalProvince),
        ownedProvinces(other.ownedProvinces), claimedProvinces(other.claimedProvinces),
        currentRuler(other.currentRuler), heir(other.heir), claimants(other.claimants),
        liegeRealm(other.liegeRealm), vassalRealms(other.vassalRealms),
        legitimacy(other.legitimacy), centralAuthority(other.centralAuthority),
        stability(other.stability), treasury(other.treasury), monthlyIncome(other.monthlyIncome),
        monthlyExpenses(other.monthlyExpenses), levySize(other.levySize),
        standingArmy(other.standingArmy), militaryMaintenance(other.militaryMaintenance),
        foundedDate(other.foundedDate), lastSuccession(other.lastSuccession) {}

    // FIXED: HIGH-002 - Custom copy assignment (don't copy mutex)
    RealmComponent& operator=(const RealmComponent& other) {
        if (this != &other) {
            realmId = other.realmId;
            realmName = other.realmName;
            adjective = other.adjective;
            rank = other.rank;
            governmentType = other.governmentType;
            successionLaw = other.successionLaw;
            capitalProvince = other.capitalProvince;
            ownedProvinces = other.ownedProvinces;
            claimedProvinces = other.claimedProvinces;
            currentRuler = other.currentRuler;
            heir = other.heir;
            claimants = other.claimants;
            liegeRealm = other.liegeRealm;
            vassalRealms = other.vassalRealms;
            legitimacy = other.legitimacy;
            centralAuthority = other.centralAuthority;
            stability = other.stability;
            treasury = other.treasury;
            monthlyIncome = other.monthlyIncome;
            monthlyExpenses = other.monthlyExpenses;
            levySize = other.levySize;
            standingArmy = other.standingArmy;
            militaryMaintenance = other.militaryMaintenance;
            foundedDate = other.foundedDate;
            lastSuccession = other.lastSuccession;
        }
        return *this;
    }
};

// ============================================================================
// Dynasty Component - Ruling families
// ============================================================================

    class DynastyComponent : public ::core::ecs::Component<DynastyComponent> {
public:
    types::EntityID dynastyId{0};
    std::string dynastyName;
    std::string motto;
    
    // Members
    types::EntityID founder{0};
    types::EntityID currentHead{0};
    std::vector<types::EntityID> livingMembers;
    std::vector<types::EntityID> cadetBranches;
    
    // Statistics
    uint32_t generationCount = 1;
    uint32_t totalMembersEver = 1;
    uint32_t realmsRuled = 0;
    
    // Prestige and legacy
    float dynasticPrestige = 100.0f;
    std::vector<std::string> dynasticTitles;
    std::vector<std::string> achievements;
    
    // Claims
    std::vector<types::EntityID> dynasticClaims; // Realms this dynasty has claims on
    
    DynastyComponent() = default;
    explicit DynastyComponent(types::EntityID id) : dynastyId(id) {}
};

// ============================================================================
// Ruler Component - Links character to realm
// ============================================================================

    class RulerComponent : public ::core::ecs::Component<RulerComponent> {
public:
    types::EntityID characterId{0};
    types::EntityID ruledRealm{0};
    types::EntityID dynasty{0};

    // Rule details - FIXED: HIGH-001
    game::time::GameDate reignStart;  // FIXED: HIGH-001 - Changed from system_clock
    uint32_t reignYears = 0;
    
    // Authority
    float rulerAuthority = 0.5f;  // Personal power vs council/vassals
    float popularity = 0.5f;  // With subjects
    float vassalOpinion = 0.0f;  // Average opinion
    
    // Titles
    std::vector<types::EntityID> heldTitles;  // All realm titles held
    std::string primaryTitle;  // "King of France"
    
    // Succession
    bool hasDesignatedHeir = false;
    types::EntityID designatedHeir{0};
    std::vector<types::EntityID> potentialHeirs;
    
    RulerComponent() = default;
    explicit RulerComponent(types::EntityID charId) : characterId(charId) {}
};

// ============================================================================
// Diplomatic Relations Component
// ============================================================================

enum class DiplomaticStatus : uint8_t {
    WAR,
    HOSTILE,
    COLD,
    NEUTRAL,
    CORDIAL,
    FRIENDLY,
    ALLIED,
    VASSAL,
    PERSONAL_UNION,
    COUNT
};

enum class CasusBelli : uint8_t {
    NO_CB,
    CLAIM,
    HOLY_WAR,
    CONQUEST,
    LIBERATION,
    RESTORATION,
    SUCCESSION,
    TRADE_DISPUTE,
    DEFENSIVE,
    COUNT
};

struct DiplomaticRelation {
    types::EntityID otherRealm{0};
    DiplomaticStatus status = DiplomaticStatus::NEUTRAL;
    float opinion = 0.0f;  // -100 to +100
    
    // Treaties
    bool hasAlliance = false;
    bool hasTradeAgreement = false;
    bool hasNonAggression = false;
    bool hasMilitaryAccess = false;
    
    // War
    bool atWar = false;
    CasusBelli warJustification = CasusBelli::NO_CB;
    float warscore = 0.0f;  // -100 to +100

    // History - FIXED: HIGH-001
    game::time::GameDate relationshipStart;  // FIXED: HIGH-001 - Changed from system_clock
    uint32_t warsCount = 0;
    uint32_t alliancesCount = 0;
};

class DiplomaticRelationsComponent : public ::core::ecs::Component<DiplomaticRelationsComponent> {
public:
    types::EntityID realmId{0};

    // Relations with other realms
    std::unordered_map<types::EntityID, DiplomaticRelation> relations;

    // Diplomatic reputation
    float diplomaticReputation = 0.0f;
    float aggressiveExpansion = 0.0f;  // Bad boy score
    float trustworthiness = 1.0f;

    // Active agreements
    std::vector<types::EntityID> alliances;
    std::vector<types::EntityID> guarantees;  // Realms we guarantee
    std::vector<types::EntityID> tributaries;  // Realms paying tribute

    // Thread safety - FIXED: HIGH-002
    mutable std::mutex dataMutex;  // FIXED: HIGH-002 - Protects relations and vectors

    DiplomaticRelationsComponent() = default;
    explicit DiplomaticRelationsComponent(types::EntityID id) : realmId(id) {}

    // FIXED: HIGH-002 - Custom copy constructor (don't copy mutex)
    DiplomaticRelationsComponent(const DiplomaticRelationsComponent& other)
        : realmId(other.realmId), relations(other.relations),
          diplomaticReputation(other.diplomaticReputation),
          aggressiveExpansion(other.aggressiveExpansion),
          trustworthiness(other.trustworthiness),
          alliances(other.alliances), guarantees(other.guarantees),
          tributaries(other.tributaries) {}

    // FIXED: HIGH-002 - Custom copy assignment (don't copy mutex)
    DiplomaticRelationsComponent& operator=(const DiplomaticRelationsComponent& other) {
        if (this != &other) {
            realmId = other.realmId;
            relations = other.relations;
            diplomaticReputation = other.diplomaticReputation;
            aggressiveExpansion = other.aggressiveExpansion;
            trustworthiness = other.trustworthiness;
            alliances = other.alliances;
            guarantees = other.guarantees;
            tributaries = other.tributaries;
        }
        return *this;
    }

    // Utility methods
    DiplomaticRelation* GetRelation(types::EntityID otherRealm);
    void SetRelation(types::EntityID otherRealm, const DiplomaticRelation& relation);
    bool IsAtWarWith(types::EntityID otherRealm) const;
    bool IsAlliedWith(types::EntityID otherRealm) const;
};

// ============================================================================
// Council Component - Realm advisors
// ============================================================================

enum class CouncilPosition : uint8_t {
    CHANCELLOR,      // Diplomacy
    MARSHAL,        // Military
    STEWARD,        // Economics
    SPYMASTER,      // Intrigue
    COURT_CHAPLAIN, // Religion/Learning
    COUNT
};

struct CouncilMember {
    types::EntityID characterId{0};
    CouncilPosition position;
    float competence = 0.5f;  // 0-1
    float loyalty = 0.5f;     // 0-1
    uint32_t yearsInPosition = 0;
};

class CouncilComponent : public ::core::ecs::Component<CouncilComponent> {
public:
    types::EntityID realmId{0};
    
    std::unordered_map<CouncilPosition, CouncilMember> council;
    
    // Council power
    float councilAuthority = 0.3f;  // How much power council has vs ruler
    bool councilCanVeto = false;
    
    // Voting record
    uint32_t proposalsApproved = 0;
    uint32_t proposalsRejected = 0;
    
    CouncilComponent() = default;
    explicit CouncilComponent(types::EntityID id) : realmId(id) {}
    
    CouncilMember* GetCouncilor(CouncilPosition position);
    void AppointCouncilor(CouncilPosition position, types::EntityID characterId);
    float GetCouncilEffectiveness() const;
};

// ============================================================================
// Laws Component - Realm legislation
// ============================================================================

enum class CrownAuthority : uint8_t {
    MINIMAL,
    LOW,
    MEDIUM,
    HIGH,
    ABSOLUTE,
    COUNT
};

class LawsComponent : public ::core::ecs::Component<LawsComponent> {
public:
    types::EntityID realmId{0};
    
    // Authority laws
    CrownAuthority crownAuthority = CrownAuthority::MEDIUM;
    bool vassalWarDeclaration = true;  // Can vassals declare war
    bool vassalInheritance = true;     // Can vassals inherit freely
    
    // Economic laws
    float baseTaxRate = 0.10f;
    float nobleTaxRate = 0.05f;
    float merchantTaxRate = 0.15f;
    float peasantTaxRate = 0.20f;
    
    // Military laws
    float levyObligation = 0.40f;  // % of population in levy
    bool mercenariesAllowed = true;
    bool standingArmyAllowed = false;
    
    // Social laws
    bool serfdom = true;
    bool religiousTolerance = false;
    bool guildRights = false;
    
    LawsComponent() = default;
    explicit LawsComponent(types::EntityID id) : realmId(id) {}
};

// ============================================================================
// Utility Functions
// ============================================================================

namespace RealmUtils {
    std::string GovernmentTypeToString(GovernmentType type);
    std::string RealmRankToString(RealmRank rank);
    std::string SuccessionLawToString(SuccessionLaw law);
    std::string DiplomaticStatusToString(DiplomaticStatus status);
    std::string CasusBelliToString(CasusBelli cb);
    std::string CouncilPositionToString(CouncilPosition position);
    std::string CrownAuthorityToString(CrownAuthority authority);
    
    float CalculateRealmPower(const RealmComponent& realm);
    bool CanDeclareWar(const RealmComponent& aggressor, const RealmComponent& target);
    std::vector<types::EntityID> GetValidHeirs(const RealmComponent& realm, SuccessionLaw law);
}

} // namespace game::realm

#endif // REALM_COMPONENTS_H