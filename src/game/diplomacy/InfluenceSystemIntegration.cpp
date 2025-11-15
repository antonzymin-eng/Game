// ============================================================================
// InfluenceSystemIntegration.cpp - Integration with Character/Religion/Province
// Created: November 15, 2025 - Phase 3: Influence System Integration
// Location: src/game/diplomacy/InfluenceSystemIntegration.cpp
// ============================================================================

#include "game/diplomacy/InfluenceCalculator.h"
#include "game/diplomacy/InfluenceSystem.h"
#include "game/character/CharacterRelationships.h"
#include "game/religion/ReligionComponents.h"
#include "game/province/ProvinceAdjacency.h"
#include <algorithm>

namespace game {
namespace diplomacy {

// ============================================================================
// Character/Marriage Integration for Dynastic Influence
// ============================================================================

/**
 * Enhanced implementation of marriage tie strength calculation
 * Uses CharacterRelationshipsComponent to detect actual marriages
 */
double CalculateMarriageTieStrengthWithCharacters(
    types::EntityID source_ruler,
    types::EntityID target_ruler,
    const character::CharacterRelationshipsComponent* source_relationships,
    const character::CharacterRelationshipsComponent* target_relationships)
{
    if (!source_relationships || !target_relationships) {
        return 0.0;
    }

    double marriage_strength = 0.0;

    // Check for direct marriage between rulers
    if (source_relationships->IsMarriedTo(target_ruler)) {
        marriage_strength += 30.0;  // Direct marriage = strong tie
        return std::min(50.0, marriage_strength);
    }

    // Check if source ruler is married to someone from target's realm
    for (const auto& marriage : source_relationships->marriages) {
        // If spouse is from target's realm, add marriage tie strength
        if (marriage.realm_of_spouse != 0) {
            marriage_strength += 15.0;  // Marriage to realm member
            if (marriage.is_alliance) {
                marriage_strength += 10.0;  // Alliance marriage bonus
            }
        }
    }

    // Check for family connections (siblings, children)
    if (source_relationships->IsSiblingOf(target_ruler)) {
        marriage_strength += 20.0;
    }

    if (source_relationships->IsChildOf(target_ruler) ||
        target_relationships->IsChildOf(source_ruler)) {
        marriage_strength += 25.0;  // Parent-child bond very strong
    }

    return std::min(50.0, marriage_strength);
}

/**
 * Enhanced implementation of personal influence calculation
 * Uses CharacterRelationshipsComponent for actual friendship data
 */
double CalculatePersonalInfluenceWithCharacters(
    types::EntityID source_ruler,
    types::EntityID target_ruler,
    const character::CharacterRelationshipsComponent* source_relationships,
    const DiplomaticState* diplo_state)
{
    if (!source_relationships) return 0.0;

    double total = 0.0;

    // Check for friendship
    if (source_relationships->IsFriendsWith(target_ruler)) {
        double bond_strength = source_relationships->GetFriendshipBondStrength(target_ruler);
        total += (bond_strength / 100.0) * 40.0;  // Scale 0-100 bond to 0-40 influence
    }

    // Get relationship details
    auto relationship = source_relationships->GetRelationship(target_ruler);
    if (relationship) {
        // Opinion modifier
        double opinion_influence = ((relationship->opinion + 100.0) / 200.0) * 30.0;
        total += opinion_influence;

        // Special relationship types
        switch (relationship->type) {
            case character::RelationshipType::FRIEND:
                total += 10.0;
                break;
            case character::RelationshipType::BLOOD_BROTHER:
                total += 20.0;  // Very strong bond
                break;
            case character::RelationshipType::RIVAL:
                total -= 15.0;  // Negative influence
                break;
            default:
                break;
        }
    }

    // Diplomatic state trust bonus
    if (diplo_state) {
        total += diplo_state->trust * 20.0;
    }

    return std::max(0.0, std::min(100.0, total));
}

// ============================================================================
// Religion Integration for Religious Influence
// ============================================================================

/**
 * Enhanced implementation of religious influence calculation
 * Uses ReligionComponents for actual faith checking
 */
double CalculateReligiousInfluenceWithFaith(
    const religion::CharacterReligionComponent* source_ruler_religion,
    const religion::RealmReligionComponent* source_realm_religion,
    const religion::CharacterReligionComponent* target_ruler_religion,
    const religion::RealmReligionComponent* target_realm_religion,
    const religion::ReligionSystemData* religion_data)
{
    if (!source_ruler_religion || !target_realm_religion || !religion_data) {
        return 0.0;
    }

    double total = 0.0;

    // Religious authority of source ruler
    double authority = source_ruler_religion->GetReligiousAuthority();
    total += (authority / 100.0) * 40.0;  // Scale to 0-40

    // Same faith bonus
    types::EntityID source_faith = source_realm_religion ? source_realm_religion->state_faith : source_ruler_religion->faith_id;
    types::EntityID target_faith = target_realm_religion->state_faith;

    if (religion_data->AreSameFaith(source_faith, target_faith)) {
        total += 40.0;  // Very strong bonus for same faith
    } else if (religion_data->AreSameDenomination(source_faith, target_faith)) {
        total += 25.0;  // Medium bonus for same denomination
    } else if (religion_data->AreSameReligionGroup(source_faith, target_faith)) {
        total += 10.0;  // Small bonus for same religion group
    }

    // Bonus for controlling holy sites
    if (source_realm_religion && !source_realm_religion->owned_holy_sites.empty()) {
        total += source_realm_religion->owned_holy_sites.size() * 3.0;
    }

    // Bonus for high clergy loyalty
    if (source_realm_religion && source_realm_religion->clergy_loyalty > 70.0) {
        total += 10.0;
    }

    // Penalty for religious diversity in source realm
    if (source_realm_religion && source_realm_religion->HasReligiousDiversity()) {
        total *= 0.8;  // 20% penalty
    }

    return std::min(100.0, total);
}

// ============================================================================
// Province Integration for Geographic Neighbor Detection
// ============================================================================

/**
 * Enhanced implementation of neighbor checking
 * Uses ProvinceAdjacencyManager for actual border detection
 */
bool AreRealmsNeighborsWithProvinces(
    const realm::RealmComponent& realm1,
    const realm::RealmComponent& realm2,
    const province::ProvinceAdjacencyManager* adjacency_manager)
{
    if (!adjacency_manager) {
        return false;  // Can't check without adjacency data
    }

    // Use adjacency manager to check if realms share a border
    return adjacency_manager->RealmsShareBorder(realm1.realmId, realm2.realmId);
}

/**
 * Get all realms that border a specific realm
 */
std::vector<types::EntityID> GetNeighboringRealmsWithProvinces(
    types::EntityID realm_id,
    const province::ProvinceAdjacencyManager* adjacency_manager)
{
    if (!adjacency_manager) {
        return {};
    }

    return adjacency_manager->GetNeighboringRealms(realm_id);
}

/**
 * Calculate border strength (how much border is shared)
 * Useful for influence calculations
 */
double CalculateBorderStrength(
    const realm::RealmComponent& realm1,
    const realm::RealmComponent& realm2,
    const province::ProvinceAdjacencyManager* adjacency_manager)
{
    if (!adjacency_manager) {
        return 0.0;
    }

    double border_strength = 0.0;
    int shared_border_count = 0;

    // Count how many province-to-province borders exist
    for (types::EntityID prov1 : realm1.ownedProvinces) {
        auto adj = adjacency_manager->GetAdjacency(prov1);
        if (!adj) continue;

        for (types::EntityID adj_prov : adj->GetAdjacentProvinces()) {
            // Check if adjacent province is owned by realm2
            auto it = std::find(realm2.ownedProvinces.begin(),
                              realm2.ownedProvinces.end(), adj_prov);
            if (it != realm2.ownedProvinces.end()) {
                shared_border_count++;
            }
        }
    }

    // Normalize border strength (more shared borders = stronger connection)
    border_strength = std::min(100.0, shared_border_count * 10.0);
    return border_strength;
}

// ============================================================================
// Integration Helper Class for InfluenceSystem
// ============================================================================

/**
 * Helper class that provides integrated influence calculations
 * This can be used by InfluenceSystem to access new components
 */
class InfluenceSystemIntegrationHelper {
private:
    // Component caches (would be populated by InfluenceSystem)
    std::unordered_map<types::EntityID, character::CharacterRelationshipsComponent*> m_character_relationships;
    std::unordered_map<types::EntityID, religion::CharacterReligionComponent*> m_character_religions;
    std::unordered_map<types::EntityID, religion::RealmReligionComponent*> m_realm_religions;

    province::ProvinceAdjacencyManager* m_adjacency_manager = nullptr;
    religion::ReligionSystemData* m_religion_data = nullptr;

public:
    // ========================================================================
    // Setters for component access
    // ========================================================================

    void SetAdjacencyManager(province::ProvinceAdjacencyManager* manager) {
        m_adjacency_manager = manager;
    }

    void SetReligionData(religion::ReligionSystemData* data) {
        m_religion_data = data;
    }

    void RegisterCharacterRelationships(types::EntityID char_id,
                                       character::CharacterRelationshipsComponent* component) {
        m_character_relationships[char_id] = component;
    }

    void RegisterCharacterReligion(types::EntityID char_id,
                                   religion::CharacterReligionComponent* component) {
        m_character_religions[char_id] = component;
    }

    void RegisterRealmReligion(types::EntityID realm_id,
                              religion::RealmReligionComponent* component) {
        m_realm_religions[realm_id] = component;
    }

    // ========================================================================
    // Integrated calculations
    // ========================================================================

    double CalculateDynasticInfluenceIntegrated(
        types::EntityID source_ruler,
        types::EntityID target_ruler,
        const realm::DynastyComponent* source_dynasty,
        const realm::DynastyComponent* target_dynasty)
    {
        auto source_rel = GetCharacterRelationships(source_ruler);
        auto target_rel = GetCharacterRelationships(target_ruler);

        double marriage_strength = CalculateMarriageTieStrengthWithCharacters(
            source_ruler, target_ruler, source_rel, target_rel);

        // Dynasty prestige (existing logic)
        double dynasty_prestige = 0.0;
        if (source_dynasty) {
            dynasty_prestige = source_dynasty->dynasticPrestige / 10.0;
            dynasty_prestige += source_dynasty->realmsRuled * 2.0;
        }

        // Family bonus (same dynasty or cadet branch)
        double family_bonus = 0.0;
        if (source_dynasty && target_dynasty) {
            if (source_dynasty->dynastyId == target_dynasty->dynastyId) {
                family_bonus = 20.0;
            }
        }

        return std::min(100.0, marriage_strength + dynasty_prestige + family_bonus);
    }

    double CalculatePersonalInfluenceIntegrated(
        types::EntityID source_ruler,
        types::EntityID target_ruler,
        const DiplomaticState* diplo_state)
    {
        auto source_rel = GetCharacterRelationships(source_ruler);
        return CalculatePersonalInfluenceWithCharacters(
            source_ruler, target_ruler, source_rel, diplo_state);
    }

    double CalculateReligiousInfluenceIntegrated(
        types::EntityID source_ruler,
        types::EntityID source_realm,
        types::EntityID target_ruler,
        types::EntityID target_realm)
    {
        auto source_ruler_religion = GetCharacterReligion(source_ruler);
        auto source_realm_religion = GetRealmReligion(source_realm);
        auto target_ruler_religion = GetCharacterReligion(target_ruler);
        auto target_realm_religion = GetRealmReligion(target_realm);

        return CalculateReligiousInfluenceWithFaith(
            source_ruler_religion, source_realm_religion,
            target_ruler_religion, target_realm_religion,
            m_religion_data);
    }

    bool AreRealmsNeighborsIntegrated(const realm::RealmComponent& realm1,
                                     const realm::RealmComponent& realm2)
    {
        return AreRealmsNeighborsWithProvinces(realm1, realm2, m_adjacency_manager);
    }

    std::vector<types::EntityID> GetNeighboringRealmsIntegrated(types::EntityID realm_id) {
        return GetNeighboringRealmsWithProvinces(realm_id, m_adjacency_manager);
    }

private:
    character::CharacterRelationshipsComponent* GetCharacterRelationships(types::EntityID char_id) {
        auto it = m_character_relationships.find(char_id);
        return (it != m_character_relationships.end()) ? it->second : nullptr;
    }

    religion::CharacterReligionComponent* GetCharacterReligion(types::EntityID char_id) {
        auto it = m_character_religions.find(char_id);
        return (it != m_character_religions.end()) ? it->second : nullptr;
    }

    religion::RealmReligionComponent* GetRealmReligion(types::EntityID realm_id) {
        auto it = m_realm_religions.find(realm_id);
        return (it != m_realm_religions.end()) ? it->second : nullptr;
    }
};

} // namespace diplomacy
} // namespace game
