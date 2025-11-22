// Created: October 7, 2025, 2:45 PM
// Location: src/game/realm/RealmComponents.cpp

#include "game/realm/RealmComponents.h"
#include <algorithm>
#include <sstream>

namespace game {
namespace realm {

// ============================================================================
// DiplomaticRelationsComponent Implementation
// FIXED: NEW-CRITICAL-001 - Thread-safe GetRelation implementation
// ============================================================================

std::optional<DiplomaticRelation> DiplomaticRelationsComponent::GetRelation(types::EntityID otherRealm) const {
    // FIXED: NEW-CRITICAL-001 - Now returns copy with mutex protection
    std::lock_guard<std::mutex> lock(dataMutex);
    auto it = relations.find(otherRealm);
    if (it != relations.end()) {
        return it->second;  // Return copy (thread-safe)
    }
    return std::nullopt;
}

DiplomaticRelation* DiplomaticRelationsComponent::GetRelationUnsafe(types::EntityID otherRealm) {
    // Unsafe version - caller MUST hold dataMutex lock
    // Used for performance-critical code that manages locking externally
    auto it = relations.find(otherRealm);
    if (it != relations.end()) {
        return &it->second;
    }
    return nullptr;
}

void DiplomaticRelationsComponent::SetRelation(types::EntityID otherRealm, const DiplomaticRelation& relation) {
    // FIXED: HIGH-002 - Add thread safety
    std::lock_guard<std::mutex> lock(dataMutex);
    relations[otherRealm] = relation;
}

bool DiplomaticRelationsComponent::IsAtWarWith(types::EntityID otherRealm) const {
    // FIXED: HIGH-002 - Add thread safety
    std::lock_guard<std::mutex> lock(dataMutex);
    auto it = relations.find(otherRealm);
    if (it != relations.end()) {
        return it->second.atWar;
    }
    return false;
}

bool DiplomaticRelationsComponent::IsAlliedWith(types::EntityID otherRealm) const {
    // FIXED: HIGH-002 - Add thread safety
    std::lock_guard<std::mutex> lock(dataMutex);
    auto it = relations.find(otherRealm);
    if (it != relations.end()) {
        return it->second.hasAlliance;
    }
    return false;
}

// ============================================================================
// CouncilComponent Implementation
// ============================================================================

CouncilMember* CouncilComponent::GetCouncilor(CouncilPosition position) {
    auto it = council.find(position);
    if (it != council.end()) {
        return &it->second;
    }
    return nullptr;
}

void CouncilComponent::AppointCouncilor(CouncilPosition position, types::EntityID characterId) {
    CouncilMember member;
    member.characterId = characterId;
    member.position = position;
    member.competence = 0.5f; // Will be updated based on character stats
    member.loyalty = 0.5f;
    member.yearsInPosition = 0;
    
    council[position] = member;
}

float CouncilComponent::GetCouncilEffectiveness() const {
    if (council.empty()) return 0.0f;
    
    float totalCompetence = 0.0f;
    float totalLoyalty = 0.0f;
    
    for (const auto& [position, member] : council) {
        totalCompetence += member.competence;
        totalLoyalty += member.loyalty;
    }
    
    float avgCompetence = totalCompetence / council.size();
    float avgLoyalty = totalLoyalty / council.size();
    
    // Effectiveness is weighted average of competence and loyalty
    return (avgCompetence * 0.7f) + (avgLoyalty * 0.3f);
}

// ============================================================================
// RealmUtils Implementation
// ============================================================================

namespace RealmUtils {

std::string GovernmentTypeToString(GovernmentType type) {
    switch (type) {
        case GovernmentType::FEUDAL_MONARCHY: return "Feudal Monarchy";
        case GovernmentType::ABSOLUTE_MONARCHY: return "Absolute Monarchy";
        case GovernmentType::ELECTIVE_MONARCHY: return "Elective Monarchy";
        case GovernmentType::REPUBLIC: return "Republic";
        case GovernmentType::MERCHANT_REPUBLIC: return "Merchant Republic";
        case GovernmentType::THEOCRACY: return "Theocracy";
        case GovernmentType::TRIBAL: return "Tribal";
        case GovernmentType::NOMADIC: return "Nomadic";
        case GovernmentType::IMPERIAL: return "Imperial";
        case GovernmentType::CONSTITUTIONAL_MONARCHY: return "Constitutional Monarchy";
        default: return "Unknown";
    }
}

std::string RealmRankToString(RealmRank rank) {
    switch (rank) {
        case RealmRank::BARONY: return "Barony";
        case RealmRank::COUNTY: return "County";
        case RealmRank::DUCHY: return "Duchy";
        case RealmRank::KINGDOM: return "Kingdom";
        case RealmRank::EMPIRE: return "Empire";
        default: return "Unknown";
    }
}

std::string SuccessionLawToString(SuccessionLaw law) {
    switch (law) {
        case SuccessionLaw::PRIMOGENITURE: return "Primogeniture";
        case SuccessionLaw::ULTIMOGENITURE: return "Ultimogeniture";
        case SuccessionLaw::GAVELKIND: return "Gavelkind";
        case SuccessionLaw::ELECTIVE: return "Elective";
        case SuccessionLaw::TANISTRY: return "Tanistry";
        case SuccessionLaw::SENIORITY: return "Seniority";
        case SuccessionLaw::APPOINTMENT: return "Appointment";
        default: return "Unknown";
    }
}

std::string DiplomaticStatusToString(DiplomaticStatus status) {
    switch (status) {
        case DiplomaticStatus::WAR: return "War";
        case DiplomaticStatus::HOSTILE: return "Hostile";
        case DiplomaticStatus::COLD: return "Cold";
        case DiplomaticStatus::NEUTRAL: return "Neutral";
        case DiplomaticStatus::CORDIAL: return "Cordial";
        case DiplomaticStatus::FRIENDLY: return "Friendly";
        case DiplomaticStatus::ALLIED: return "Allied";
        case DiplomaticStatus::VASSAL: return "Vassal";
        case DiplomaticStatus::PERSONAL_UNION: return "Personal Union";
        default: return "Unknown";
    }
}

std::string CasusBelliToString(CasusBelli cb) {
    switch (cb) {
        case CasusBelli::NO_CB: return "No Casus Belli";
        case CasusBelli::CLAIM: return "Claim";
        case CasusBelli::HOLY_WAR: return "Holy War";
        case CasusBelli::CONQUEST: return "Conquest";
        case CasusBelli::LIBERATION: return "Liberation";
        case CasusBelli::RESTORATION: return "Restoration";
        case CasusBelli::SUCCESSION: return "Succession";
        case CasusBelli::TRADE_DISPUTE: return "Trade Dispute";
        case CasusBelli::DEFENSIVE: return "Defensive";
        default: return "Unknown";
    }
}

std::string CouncilPositionToString(CouncilPosition position) {
    switch (position) {
        case CouncilPosition::CHANCELLOR: return "Chancellor";
        case CouncilPosition::MARSHAL: return "Marshal";
        case CouncilPosition::STEWARD: return "Steward";
        case CouncilPosition::SPYMASTER: return "Spymaster";
        case CouncilPosition::COURT_CHAPLAIN: return "Court Chaplain";
        default: return "Unknown";
    }
}

std::string CrownAuthorityToString(CrownAuthority authority) {
    switch (authority) {
        case CrownAuthority::MINIMAL: return "Minimal";
        case CrownAuthority::LOW: return "Low";
        case CrownAuthority::MEDIUM: return "Medium";
        case CrownAuthority::HIGH: return "High";
        case CrownAuthority::ABSOLUTE: return "Absolute";
        default: return "Unknown";
    }
}

float CalculateRealmPower(const RealmComponent& realm) {
    float power = 0.0f;
    
    // Territory power
    power += realm.ownedProvinces.size() * 10.0f;
    
    // Military power
    power += realm.levySize * 0.5f;
    power += realm.standingArmy * 2.0f;
    
    // Economic power
    power += realm.treasury * 0.01f;
    power += realm.monthlyIncome * 5.0f;
    
    // Stability multiplier
    power *= (0.5f + (realm.stability * 0.5f));
    
    // Authority multiplier
    power *= (0.7f + (realm.centralAuthority * 0.3f));
    
    // Legitimacy multiplier
    power *= (0.8f + (realm.legitimacy * 0.2f));
    
    // Vassal contribution (80% of vassal power)
    power += realm.vassalRealms.size() * 50.0f;
    
    return power;
}

bool CanDeclareWar(const RealmComponent& aggressor, const RealmComponent& target) {
    // Cannot declare war on yourself
    if (aggressor.realmId == target.realmId) {
        return false;
    }
    
    // Cannot declare war if you're a vassal (unless allowed by laws)
    if (aggressor.liegeRealm != 0) {
        return false; // Would need to check liege's laws
    }
    
    // Cannot declare war if target is your liege
    if (target.realmId == aggressor.liegeRealm) {
        return false;
    }
    
    // Cannot declare war if target is your vassal
    auto vassalIt = std::find(aggressor.vassalRealms.begin(), 
                              aggressor.vassalRealms.end(), 
                              target.realmId);
    if (vassalIt != aggressor.vassalRealms.end()) {
        return false;
    }
    
    // Must have minimum stability
    if (aggressor.stability < 0.3f) {
        return false;
    }
    
    // Must have minimum treasury (war is expensive)
    if (aggressor.treasury < aggressor.monthlyExpenses * 3.0) {
        return false;
    }
    
    return true;
}

std::vector<types::EntityID> GetValidHeirs(const RealmComponent& realm, SuccessionLaw law) {
    std::vector<types::EntityID> heirs;
    
    // If designated heir exists and succession law allows it
    if (realm.heir != 0) {
        heirs.push_back(realm.heir);
    }
    
    // Add claimants based on succession law
    switch (law) {
        case SuccessionLaw::PRIMOGENITURE:
        case SuccessionLaw::ULTIMOGENITURE:
        case SuccessionLaw::GAVELKIND:
            // Would need to query dynasty component and sort by age/birth order
            // For now, add claimants in order
            heirs.insert(heirs.end(), realm.claimants.begin(), realm.claimants.end());
            break;
            
        case SuccessionLaw::ELECTIVE:
            // All claimants are valid candidates
            heirs.insert(heirs.end(), realm.claimants.begin(), realm.claimants.end());
            break;
            
        case SuccessionLaw::TANISTRY:
            // Dynasty members, would need dynasty component
            heirs.insert(heirs.end(), realm.claimants.begin(), realm.claimants.end());
            break;
            
        case SuccessionLaw::SENIORITY:
            // Oldest dynasty member, would need to sort by age
            heirs.insert(heirs.end(), realm.claimants.begin(), realm.claimants.end());
            break;
            
        case SuccessionLaw::APPOINTMENT:
            // Ruler appoints, so designated heir only
            if (realm.heir != 0) {
                heirs.clear();
                heirs.push_back(realm.heir);
            }
            break;
            
        default:
            break;
    }
    
    return heirs;
}

} // namespace RealmUtils

} // namespace realm
} // namespace game

// ============================================================================
// Explicit Template Instantiations - Fix incomplete type errors
// ============================================================================

template class ::core::ecs::Component<game::realm::RealmComponent>;
template class ::core::ecs::Component<game::realm::DynastyComponent>;
template class ::core::ecs::Component<game::realm::RulerComponent>;
template class ::core::ecs::Component<game::realm::DiplomaticRelationsComponent>;
template class ::core::ecs::Component<game::realm::CouncilComponent>;
template class ::core::ecs::Component<game::realm::LawsComponent>;
