// ============================================================================
// InfluenceCalculator.cpp - Sphere of Influence Calculations Implementation
// Created: November 11, 2025 - Phase 3: Sphere of Influence System
// Location: src/game/diplomacy/InfluenceCalculator.cpp
// ============================================================================

#include "game/diplomacy/InfluenceCalculator.h"
#include <algorithm>
#include <cmath>

namespace game {
namespace diplomacy {

// ============================================================================
// Main Influence Calculation Functions
// ============================================================================

double InfluenceCalculator::CalculateMilitaryInfluence(
    const realm::RealmComponent& realm,
    const DiplomaticState* diplo_state)
{
    double military_strength = CalculateMilitaryStrength(realm);
    double tech_bonus = CalculateMilitaryTechBonus(realm);
    double prestige_bonus = CalculateMilitaryPrestigeBonus(realm);

    double total = military_strength + tech_bonus + prestige_bonus;

    // Relationship affects military influence projection
    if (diplo_state) {
        total = ApplyRelationshipModifier(total, diplo_state->opinion);
    }

    return NormalizeInfluence(total, 150.0);  // Max ~150 raw, normalized to 100
}

double InfluenceCalculator::CalculateEconomicInfluence(
    const realm::RealmComponent& realm,
    const DiplomaticState* diplo_state)
{
    double wealth_score = CalculateWealthScore(realm);
    double trade_dominance = CalculateTradeDominance(realm, diplo_state);
    double trade_hub_bonus = CalculateTradeHubBonus(realm);

    double total = wealth_score + trade_dominance + trade_hub_bonus;

    // Economic influence less affected by opinion
    if (diplo_state) {
        // Only apply 50% of relationship modifier for economic influence
        double modifier = ApplyRelationshipModifier(1.0, diplo_state->opinion);
        total *= (0.5 + 0.5 * modifier);
    }

    return NormalizeInfluence(total, 100.0);
}

double InfluenceCalculator::CalculateDynasticInfluence(
    const realm::RealmComponent& source_realm,
    const realm::RealmComponent& target_realm,
    const realm::DynastyComponent* source_dynasty,
    const realm::DynastyComponent* target_dynasty)
{
    double marriage_strength = CalculateMarriageTieStrength(source_realm, target_realm);
    double dynasty_prestige = CalculateDynastyPrestige(source_dynasty);
    double family_bonus = CalculateFamilyConnectionBonus(source_dynasty, target_dynasty);

    double total = marriage_strength + dynasty_prestige + family_bonus;

    return NormalizeInfluence(total, 100.0);
}

double InfluenceCalculator::CalculatePersonalInfluence(
    const realm::RealmComponent& source_realm,
    const realm::RealmComponent& target_realm,
    const DiplomaticState* diplo_state)
{
    double ruler_friendship = CalculateRulerFriendship(source_realm, target_realm, diplo_state);
    double trust_bonus = CalculateTrustBonus(diplo_state);
    double personal_bond = CalculatePersonalBondStrength(source_realm, target_realm);

    double total = ruler_friendship + trust_bonus + personal_bond;

    return NormalizeInfluence(total, 100.0);
}

double InfluenceCalculator::CalculateReligiousInfluence(
    const realm::RealmComponent& source_realm,
    const realm::RealmComponent& target_realm)
{
    double religious_authority = CalculateReligiousAuthority(source_realm);
    double same_faith_bonus = CalculateSameFaithBonus(source_realm, target_realm);

    double total = religious_authority + same_faith_bonus;

    return NormalizeInfluence(total, 100.0);
}

double InfluenceCalculator::CalculateCulturalInfluence(
    const realm::RealmComponent& source_realm,
    const realm::RealmComponent& target_realm)
{
    double cultural_similarity = CalculateCulturalSimilarity(source_realm, target_realm);
    double cultural_attraction = CalculateCulturalAttraction(source_realm);

    double total = cultural_similarity + cultural_attraction;

    return NormalizeInfluence(total, 100.0);
}

double InfluenceCalculator::CalculatePrestigeInfluence(
    const realm::RealmComponent& realm,
    const realm::DynastyComponent* dynasty)
{
    double diplomatic_reputation = CalculateDiplomaticReputation(realm);
    double glory_score = CalculateGloryScore(realm, dynasty);
    double victory_bonus = CalculateRecentVictoryBonus(realm);

    double total = diplomatic_reputation + glory_score + victory_bonus;

    return NormalizeInfluence(total, 100.0);
}

// ============================================================================
// Component Calculations for Military Influence
// ============================================================================

double InfluenceCalculator::CalculateMilitaryStrength(const realm::RealmComponent& realm) {
    // Total military forces
    double standing_army = static_cast<double>(realm.standingArmy);
    double levies = static_cast<double>(realm.levySize);

    // Standing army worth more than levies (professional vs militia)
    double total_strength = standing_army * 2.0 + levies * 1.0;

    // Scale to 0-100 range
    // Assume 10,000 effective troops = 100 strength
    double strength = (total_strength / 10000.0) * 100.0;

    // Realm rank multiplier
    double rank_multiplier = 1.0;
    switch(realm.rank) {
        case realm::RealmRank::BARONY:  rank_multiplier = 0.5; break;
        case realm::RealmRank::COUNTY:  rank_multiplier = 0.7; break;
        case realm::RealmRank::DUCHY:   rank_multiplier = 1.0; break;
        case realm::RealmRank::KINGDOM: rank_multiplier = 1.3; break;
        case realm::RealmRank::EMPIRE:  rank_multiplier = 1.5; break;
        default: rank_multiplier = 1.0; break;
    }

    strength *= rank_multiplier;

    return Clamp(strength, 0.0, 100.0);
}

double InfluenceCalculator::CalculateMilitaryTechBonus(const realm::RealmComponent& realm) {
    // Placeholder for future military technology system
    // For now, base on government type and realm development

    double tech_bonus = 0.0;

    // More advanced government types have better military tech
    switch(realm.governmentType) {
        case realm::GovernmentType::TRIBAL:
        case realm::GovernmentType::NOMADIC:
            tech_bonus = 0.0;
            break;
        case realm::GovernmentType::FEUDAL_MONARCHY:
        case realm::GovernmentType::THEOCRACY:
            tech_bonus = 5.0;
            break;
        case realm::GovernmentType::ABSOLUTE_MONARCHY:
        case realm::GovernmentType::REPUBLIC:
            tech_bonus = 10.0;
            break;
        case realm::GovernmentType::IMPERIAL:
        case realm::GovernmentType::CONSTITUTIONAL_MONARCHY:
            tech_bonus = 15.0;
            break;
        default:
            tech_bonus = 5.0;
            break;
    }

    // Stability bonus (stable realms maintain better armies)
    tech_bonus += realm.stability * 5.0;

    return Clamp(tech_bonus, 0.0, 20.0);
}

double InfluenceCalculator::CalculateMilitaryPrestigeBonus(const realm::RealmComponent& realm) {
    // Based on realm rank and legitimacy
    double prestige = 0.0;

    // Rank contribution
    switch(realm.rank) {
        case realm::RealmRank::BARONY:  prestige = 2.0; break;
        case realm::RealmRank::COUNTY:  prestige = 5.0; break;
        case realm::RealmRank::DUCHY:   prestige = 10.0; break;
        case realm::RealmRank::KINGDOM: prestige = 20.0; break;
        case realm::RealmRank::EMPIRE:  prestige = 30.0; break;
        default: prestige = 5.0; break;
    }

    // Legitimacy affects military prestige
    prestige *= realm.legitimacy;

    return Clamp(prestige, 0.0, 30.0);
}

// ============================================================================
// Component Calculations for Economic Influence
// ============================================================================

double InfluenceCalculator::CalculateWealthScore(const realm::RealmComponent& realm) {
    // Based on treasury and income
    double treasury_score = std::log10(realm.treasury + 1.0) * 10.0;  // Log scale
    double income_score = (realm.monthlyIncome / 100.0) * 20.0;

    double wealth = treasury_score + income_score;

    return Clamp(wealth, 0.0, 60.0);
}

double InfluenceCalculator::CalculateTradeDominance(
    const realm::RealmComponent& realm,
    const DiplomaticState* diplo_state)
{
    double trade_dominance = 0.0;

    if (diplo_state) {
        // Trade volume contribution
        trade_dominance += (diplo_state->trade_volume / 1000.0) * 20.0;

        // Economic dependency gives influence
        trade_dominance += diplo_state->economic_dependency * 10.0;
    }

    // Merchant republics get bonus
    if (realm.governmentType == realm::GovernmentType::MERCHANT_REPUBLIC) {
        trade_dominance *= 1.5;
    }

    return Clamp(trade_dominance, 0.0, 30.0);
}

double InfluenceCalculator::CalculateTradeHubBonus(const realm::RealmComponent& realm) {
    // Bonus based on number of provinces (more provinces = more trade hubs)
    double hub_bonus = std::min(10.0, realm.ownedProvinces.size() / 5.0);

    // Capital bonus
    if (realm.capitalProvince != 0) {
        hub_bonus += 5.0;
    }

    return Clamp(hub_bonus, 0.0, 10.0);
}

// ============================================================================
// Component Calculations for Dynastic Influence
// ============================================================================

double InfluenceCalculator::CalculateMarriageTieStrength(
    const realm::RealmComponent& source_realm,
    const realm::RealmComponent& target_realm)
{
    // INTEGRATION NOTE: This is now enhanced with CharacterRelationshipsComponent
    // See InfluenceSystemIntegration.cpp for full implementation using
    // CalculateMarriageTieStrengthWithCharacters()

    // For backward compatibility, keep simple placeholder logic
    // Full implementation should use InfluenceSystemIntegrationHelper
    double marriage_strength = 0.0;

    if (source_realm.heir != 0 && target_realm.heir != 0) {
        marriage_strength = 10.0;  // Base value for potential marriage ties
    }

    // If source ruler has heir from target realm, increase strength
    if (source_realm.currentRuler != 0 && target_realm.currentRuler != 0) {
        marriage_strength += 5.0;
    }

    return Clamp(marriage_strength, 0.0, 50.0);
}

double InfluenceCalculator::CalculateDynastyPrestige(const realm::DynastyComponent* dynasty) {
    if (!dynasty) return 0.0;

    double prestige = dynasty->dynasticPrestige / 10.0;  // Scale down from 100-1000 range

    // Bonus for number of realms ruled
    prestige += dynasty->realmsRuled * 2.0;

    // Bonus for longevity (generation count)
    prestige += std::min(10.0, dynasty->generationCount / 2.0);

    return Clamp(prestige, 0.0, 30.0);
}

double InfluenceCalculator::CalculateFamilyConnectionBonus(
    const realm::DynastyComponent* source_dynasty,
    const realm::DynastyComponent* target_dynasty)
{
    if (!source_dynasty || !target_dynasty) return 0.0;

    // Same dynasty = very high bonus
    if (source_dynasty->dynastyId == target_dynasty->dynastyId) {
        return 20.0;
    }

    // Cadet branches
    for (const auto& cadet : source_dynasty->cadetBranches) {
        if (cadet == target_dynasty->dynastyId) {
            return 15.0;
        }
    }

    // TODO: Check for shared ancestors or marriage connections

    return 0.0;
}

// ============================================================================
// Component Calculations for Personal Influence
// ============================================================================

double InfluenceCalculator::CalculateRulerFriendship(
    const realm::RealmComponent& source_realm,
    const realm::RealmComponent& target_realm,
    const DiplomaticState* diplo_state)
{
    if (!diplo_state) return 0.0;

    // Opinion is primary factor
    // Opinion ranges from -100 to +100
    // Scale to 0-60 range
    double friendship = ((diplo_state->opinion + 100.0) / 200.0) * 60.0;

    return Clamp(friendship, 0.0, 60.0);
}

double InfluenceCalculator::CalculateTrustBonus(const DiplomaticState* diplo_state) {
    if (!diplo_state) return 0.0;

    // Trust ranges from 0.0 to 1.0
    double trust_bonus = diplo_state->trust * 20.0;

    return Clamp(trust_bonus, 0.0, 20.0);
}

double InfluenceCalculator::CalculatePersonalBondStrength(
    const realm::RealmComponent& source_realm,
    const realm::RealmComponent& target_realm)
{
    // Placeholder for character trait compatibility
    // Would check if rulers have compatible personalities

    // For now, base on government type similarity
    double bond = 0.0;

    if (source_realm.governmentType == target_realm.governmentType) {
        bond = 10.0;
    }

    // Similar rank = easier to relate
    if (source_realm.rank == target_realm.rank) {
        bond += 5.0;
    }

    return Clamp(bond, 0.0, 20.0);
}

// ============================================================================
// Component Calculations for Religious Influence
// ============================================================================

double InfluenceCalculator::CalculateReligiousAuthority(const realm::RealmComponent& realm) {
    double authority = 0.0;

    // Theocracies have highest religious authority
    if (realm.governmentType == realm::GovernmentType::THEOCRACY) {
        authority = 40.0;
    } else {
        authority = 10.0;  // Base religious influence
    }

    // Higher rank = more religious authority
    switch(realm.rank) {
        case realm::RealmRank::EMPIRE:  authority += 20.0; break;
        case realm::RealmRank::KINGDOM: authority += 15.0; break;
        case realm::RealmRank::DUCHY:   authority += 10.0; break;
        case realm::RealmRank::COUNTY:  authority += 5.0; break;
        default: break;
    }

    // Stability affects religious authority
    authority *= realm.stability;

    return Clamp(authority, 0.0, 60.0);
}

double InfluenceCalculator::CalculateSameFaithBonus(
    const realm::RealmComponent& source_realm,
    const realm::RealmComponent& target_realm)
{
    // INTEGRATION NOTE: This is now enhanced with ReligionComponents
    // See InfluenceSystemIntegration.cpp for full implementation using
    // CalculateReligiousInfluenceWithFaith()

    // For backward compatibility, keep placeholder logic
    // Full implementation should use InfluenceSystemIntegrationHelper
    // and ReligionSystemData to check actual faith compatibility

    // Return moderate bonus as placeholder
    // Real implementation would check:
    // - Same faith = 40.0 bonus
    // - Same denomination = 25.0 bonus
    // - Same religion group = 10.0 bonus
    return 20.0;
}

// ============================================================================
// Component Calculations for Cultural Influence
// ============================================================================

double InfluenceCalculator::CalculateCulturalSimilarity(
    const realm::RealmComponent& source_realm,
    const realm::RealmComponent& target_realm)
{
    // Placeholder: Would check cultural groups and similarity

    // For now, use geographic proximity as proxy
    // (neighboring realms often share culture)
    double similarity = 20.0;  // Base value

    if (AreNeighbors(source_realm, target_realm)) {
        similarity = 50.0;  // Higher for neighbors
    }

    return Clamp(similarity, 0.0, 70.0);
}

double InfluenceCalculator::CalculateCulturalAttraction(const realm::RealmComponent& realm) {
    // Advanced/prestigious realms have more cultural attraction
    double attraction = 0.0;

    // Rank contribution
    switch(realm.rank) {
        case realm::RealmRank::EMPIRE:  attraction = 25.0; break;
        case realm::RealmRank::KINGDOM: attraction = 20.0; break;
        case realm::RealmRank::DUCHY:   attraction = 15.0; break;
        case realm::RealmRank::COUNTY:  attraction = 10.0; break;
        default: attraction = 5.0; break;
    }

    // Wealthy realms are culturally attractive
    attraction += std::min(5.0, std::log10(realm.treasury + 1.0));

    return Clamp(attraction, 0.0, 30.0);
}

// ============================================================================
// Component Calculations for Prestige Influence
// ============================================================================

double InfluenceCalculator::CalculateDiplomaticReputation(const realm::RealmComponent& realm) {
    double reputation = 0.0;

    // Rank gives reputation
    switch(realm.rank) {
        case realm::RealmRank::EMPIRE:  reputation = 40.0; break;
        case realm::RealmRank::KINGDOM: reputation = 30.0; break;
        case realm::RealmRank::DUCHY:   reputation = 20.0; break;
        case realm::RealmRank::COUNTY:  reputation = 10.0; break;
        default: reputation = 5.0; break;
    }

    // Stability and legitimacy boost reputation
    reputation *= (realm.stability + realm.legitimacy) / 2.0;

    return Clamp(reputation, 0.0, 50.0);
}

double InfluenceCalculator::CalculateGloryScore(
    const realm::RealmComponent& realm,
    const realm::DynastyComponent* dynasty)
{
    double glory = 0.0;

    // Dynasty prestige contributes to glory
    if (dynasty) {
        glory = std::min(20.0, dynasty->dynasticPrestige / 10.0);
    }

    // Number of provinces = expansion success
    glory += std::min(10.0, realm.ownedProvinces.size() / 3.0);

    return Clamp(glory, 0.0, 30.0);
}

double InfluenceCalculator::CalculateRecentVictoryBonus(const realm::RealmComponent& realm) {
    // Placeholder: Would track recent military victories
    // For now, base on military strength vs maintenance

    double victory_bonus = 0.0;

    // If spending on military, might have recent victories
    if (realm.militaryMaintenance > 0.5) {
        victory_bonus = 10.0;
    }

    // TODO: Implement actual victory tracking

    return Clamp(victory_bonus, 0.0, 20.0);
}

// ============================================================================
// Utility Functions
// ============================================================================

double InfluenceCalculator::NormalizeInfluence(double raw_value, double max_value) {
    return Clamp((raw_value / max_value) * 100.0, 0.0, 100.0);
}

double InfluenceCalculator::ApplyRelationshipModifier(double base_influence, int opinion) {
    // Opinion from -100 to +100 affects effectiveness
    // -100 opinion = 0.5x effectiveness, +100 = 1.5x effectiveness
    double modifier = 1.0 + (opinion / 200.0);
    modifier = Clamp(modifier, 0.5, 1.5);

    return base_influence * modifier;
}

double InfluenceCalculator::CalculateGeographicDecay(int hops, InfluenceType type) {
    // Type-specific decay rates (same as in InfluenceSource)
    double decay_rate = 0.0;
    switch(type) {
        case InfluenceType::MILITARY:    decay_rate = 0.40; break;  // High decay, short range
        case InfluenceType::ECONOMIC:    decay_rate = 0.15; break;  // Low decay, long range
        case InfluenceType::DYNASTIC:    decay_rate = 0.05; break;  // Very low, family ties transcend distance
        case InfluenceType::PERSONAL:    decay_rate = 0.25; break;  // Medium decay
        case InfluenceType::RELIGIOUS:   decay_rate = 0.00; break;  // No decay within same faith
        case InfluenceType::CULTURAL:    decay_rate = 0.20; break;  // Medium decay
        case InfluenceType::PRESTIGE:    decay_rate = 0.10; break;  // Low decay, reputation travels far
        default: decay_rate = 0.30; break;
    }

    // Calculate modifier: modifier = (1 - decay_rate)^hops
    double modifier = std::pow(1.0 - decay_rate, static_cast<double>(hops));
    return Clamp(modifier, 0.0, 1.0);
}

bool InfluenceCalculator::AreNeighbors(
    const realm::RealmComponent& realm1,
    const realm::RealmComponent& realm2)
{
    // INTEGRATION NOTE: This is now enhanced with ProvinceAdjacencyManager
    // See InfluenceSystemIntegration.cpp for full implementation using
    // AreRealmsNeighborsWithProvinces()

    // For backward compatibility, keep simple logic
    // Full implementation should use ProvinceAdjacencyManager::RealmsShareBorder()

    if (realm1.ownedProvinces.empty() || realm2.ownedProvinces.empty()) {
        return false;
    }

    // Placeholder logic: assume realms with many provinces are more likely to neighbor
    return (realm1.ownedProvinces.size() > 5 && realm2.ownedProvinces.size() > 5);
}

double InfluenceCalculator::Clamp(double value, double min_val, double max_val) {
    return std::max(min_val, std::min(value, max_val));
}

} // namespace diplomacy
} // namespace game
