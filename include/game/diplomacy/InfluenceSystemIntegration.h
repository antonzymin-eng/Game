// ============================================================================
// InfluenceSystemIntegration.h - Integration Helper Header
// Created: November 15, 2025 - Phase 3: Influence System Integration
// Location: include/game/diplomacy/InfluenceSystemIntegration.h
// ============================================================================

#pragma once

#include "game/diplomacy/InfluenceCalculator.h"
#include "game/diplomacy/DiplomacyComponents.h"
#include "game/character/CharacterRelationships.h"
#include "game/religion/ReligionComponents.h"
#include "game/province/ProvinceAdjacency.h"
#include "game/realm/RealmComponents.h"
#include "core/types/game_types.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace game {
namespace diplomacy {

// Forward declarations
class InfluenceSystem;

// ============================================================================
// Integration Helper Class for InfluenceSystem
// ============================================================================

/**
 * @brief Helper class that provides integrated influence calculations
 *
 * This class manages access to Character, Religion, and Province components
 * and provides enhanced influence calculation methods that use real game data
 * instead of placeholders.
 *
 * Usage:
 *   InfluenceSystemIntegrationHelper helper;
 *   helper.SetAdjacencyManager(&adjacency_manager);
 *   helper.SetReligionData(&religion_data);
 *   // ... register components ...
 *   double influence = helper.CalculateDynasticInfluenceIntegrated(...);
 */
class InfluenceSystemIntegrationHelper {
public:
    InfluenceSystemIntegrationHelper() = default;
    ~InfluenceSystemIntegrationHelper() = default;

    // ========================================================================
    // Component Registration
    // ========================================================================

    /**
     * Set the province adjacency manager
     */
    void SetAdjacencyManager(province::ProvinceAdjacencyManager* manager);

    /**
     * Set the religion system data
     */
    void SetReligionData(religion::ReligionSystemData* data);

    /**
     * Register a character's relationship component
     */
    void RegisterCharacterRelationships(types::EntityID char_id,
                                       character::CharacterRelationshipsComponent* component);

    /**
     * Register a character's religion component
     */
    void RegisterCharacterReligion(types::EntityID char_id,
                                   religion::CharacterReligionComponent* component);

    /**
     * Register a realm's religion component
     */
    void RegisterRealmReligion(types::EntityID realm_id,
                              religion::RealmReligionComponent* component);

    /**
     * Unregister components (when entities are destroyed)
     */
    void UnregisterCharacterRelationships(types::EntityID char_id);
    void UnregisterCharacterReligion(types::EntityID char_id);
    void UnregisterRealmReligion(types::EntityID realm_id);

    // ========================================================================
    // Integrated Calculations
    // ========================================================================

    /**
     * Calculate dynastic influence using actual marriage ties
     * @return Influence strength (0-100)
     */
    double CalculateDynasticInfluenceIntegrated(
        types::EntityID source_ruler,
        types::EntityID target_ruler,
        const realm::DynastyComponent* source_dynasty,
        const realm::DynastyComponent* target_dynasty);

    /**
     * Calculate personal influence using actual friendships
     * @return Influence strength (0-100)
     */
    double CalculatePersonalInfluenceIntegrated(
        types::EntityID source_ruler,
        types::EntityID target_ruler,
        const DiplomaticState* diplo_state);

    /**
     * Calculate religious influence using actual faith data
     * @return Influence strength (0-100)
     */
    double CalculateReligiousInfluenceIntegrated(
        types::EntityID source_ruler,
        types::EntityID source_realm,
        types::EntityID target_ruler,
        types::EntityID target_realm);

    /**
     * Check if two realms are neighbors using province adjacency
     * @return true if realms share a border
     */
    bool AreRealmsNeighborsIntegrated(const realm::RealmComponent& realm1,
                                     const realm::RealmComponent& realm2);

    /**
     * Get all realms that border a specific realm
     * @return Vector of neighboring realm IDs
     */
    std::vector<types::EntityID> GetNeighboringRealmsIntegrated(types::EntityID realm_id);

    /**
     * Check if integration is fully enabled
     * @return true if all managers are set and integration is active
     */
    bool IsIntegrationEnabled() const;

private:
    // ========================================================================
    // Component Access Helpers
    // ========================================================================

    character::CharacterRelationshipsComponent* GetCharacterRelationships(types::EntityID char_id);
    const character::CharacterRelationshipsComponent* GetCharacterRelationships(types::EntityID char_id) const;

    religion::CharacterReligionComponent* GetCharacterReligion(types::EntityID char_id);
    const religion::CharacterReligionComponent* GetCharacterReligion(types::EntityID char_id) const;

    religion::RealmReligionComponent* GetRealmReligion(types::EntityID realm_id);
    const religion::RealmReligionComponent* GetRealmReligion(types::EntityID realm_id) const;

    // ========================================================================
    // Member Variables
    // ========================================================================

    // Component caches
    std::unordered_map<types::EntityID, character::CharacterRelationshipsComponent*> m_character_relationships;
    std::unordered_map<types::EntityID, religion::CharacterReligionComponent*> m_character_religions;
    std::unordered_map<types::EntityID, religion::RealmReligionComponent*> m_realm_religions;

    // System references
    province::ProvinceAdjacencyManager* m_adjacency_manager = nullptr;
    religion::ReligionSystemData* m_religion_data = nullptr;
};

// ============================================================================
// Standalone Integration Functions
// ============================================================================

/**
 * Calculate marriage tie strength using character relationships
 * Used by CalculateDynasticInfluenceIntegrated
 */
double CalculateMarriageTieStrengthWithCharacters(
    types::EntityID source_ruler,
    types::EntityID target_ruler,
    const character::CharacterRelationshipsComponent* source_relationships,
    const character::CharacterRelationshipsComponent* target_relationships);

/**
 * Calculate personal influence using character relationships
 * Used by CalculatePersonalInfluenceIntegrated
 */
double CalculatePersonalInfluenceWithCharacters(
    types::EntityID source_ruler,
    types::EntityID target_ruler,
    const character::CharacterRelationshipsComponent* source_relationships,
    const DiplomaticState* diplo_state);

/**
 * Calculate religious influence using faith data
 * Used by CalculateReligiousInfluenceIntegrated
 */
double CalculateReligiousInfluenceWithFaith(
    const religion::CharacterReligionComponent* source_ruler_religion,
    const religion::RealmReligionComponent* source_realm_religion,
    const religion::CharacterReligionComponent* target_ruler_religion,
    const religion::RealmReligionComponent* target_realm_religion,
    const religion::ReligionSystemData* religion_data);

/**
 * Check if realms are neighbors using province adjacency
 * Used by AreRealmsNeighborsIntegrated
 */
bool AreRealmsNeighborsWithProvinces(
    const realm::RealmComponent& realm1,
    const realm::RealmComponent& realm2,
    const province::ProvinceAdjacencyManager* adjacency_manager);

/**
 * Get neighboring realms using province adjacency
 * Used by GetNeighboringRealmsIntegrated
 */
std::vector<types::EntityID> GetNeighboringRealmsWithProvinces(
    types::EntityID realm_id,
    const province::ProvinceAdjacencyManager* adjacency_manager);

/**
 * Calculate border strength between two realms
 */
double CalculateBorderStrength(
    const realm::RealmComponent& realm1,
    const realm::RealmComponent& realm2,
    const province::ProvinceAdjacencyManager* adjacency_manager);

} // namespace diplomacy
} // namespace game
